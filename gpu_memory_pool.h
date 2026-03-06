#pragma once

#ifdef USE_CUDA

#include <cuda_runtime.h>
#include <vector>
#include <memory>
#include <mutex>
#include <iostream>
#include <sstream>
#include "logger.h"

// =============================================================
// GPU MEMORY POOL - Efficient memory management for CUDA
// Prevents allocation/deallocation overhead
// =============================================================

template<typename T>
class GpuBuffer {
private:
    T* d_ptr = nullptr;
    size_t size = 0;
    bool owned = true;
    
public:
    GpuBuffer() {}
    
    explicit GpuBuffer(size_t count) {
        allocate(count);
    }
    
    ~GpuBuffer() {
        if (owned && d_ptr) {
            cudaFree(d_ptr);
        }
    }
    
    // Disable copy
    GpuBuffer(const GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;
    
    // Enable move
    GpuBuffer(GpuBuffer&& other) noexcept 
        : d_ptr(other.d_ptr), size(other.size), owned(other.owned) {
        other.d_ptr = nullptr;
        other.size = 0;
    }
    
    GpuBuffer& operator=(GpuBuffer&& other) noexcept {
        if (this != &other) {
            if (owned && d_ptr) cudaFree(d_ptr);
            d_ptr = other.d_ptr;
            size = other.size;
            owned = other.owned;
            other.d_ptr = nullptr;
            other.size = 0;
        }
        return *this;
    }
    
    bool allocate(size_t count) {
        if (d_ptr) cudaFree(d_ptr);
        size = count;
        cudaError_t err = cudaMalloc(&d_ptr, count * sizeof(T));
        if (err != cudaSuccess) {
            LOG_ERROR("GpuBuffer", "Failed to allocate GPU memory: " + std::string(cudaGetErrorString(err)));
            d_ptr = nullptr;
            size = 0;
            return false;
        }
        return true;
    }
    
    void zero() {
        if (d_ptr && size > 0) {
            cudaMemset(d_ptr, 0, size * sizeof(T));
        }
    }
    
    T* get() const { return d_ptr; }
    size_t getSize() const { return size; }
    size_t getBytes() const { return size * sizeof(T); }
    
    bool copyFromHost(const T* hostPtr, size_t count) {
        if (!d_ptr || count > size) return false;
        cudaError_t err = cudaMemcpy(d_ptr, hostPtr, count * sizeof(T), cudaMemcpyHostToDevice);
        return err == cudaSuccess;
    }
    
    bool copyToHost(T* hostPtr, size_t count) const {
        if (!d_ptr || count > size) return false;
        cudaError_t err = cudaMemcpy(hostPtr, d_ptr, count * sizeof(T), cudaMemcpyDeviceToHost);
        return err == cudaSuccess;
    }
};

class GpuMemoryPool {
private:
    struct PoolBuffer {
        void* ptr;
        size_t size;
        bool inUse;
        cudaStream_t stream;
    };
    
    std::vector<PoolBuffer> buffers;
    std::mutex poolMutex;
    size_t totalAllocated = 0;
    size_t totalUsed = 0;
    int deviceId = 0;
    
public:
    GpuMemoryPool(int devId = 0) : deviceId(devId) {
        cudaSetDevice(deviceId);
    }
    
    ~GpuMemoryPool() {
        cleanup();
    }
    
    // Pre-allocate buffers of different sizes
    void preallocate(const std::vector<size_t>& sizes) {
        std::lock_guard<std::mutex> lock(poolMutex);
        
        for (size_t sz : sizes) {
            void* ptr = nullptr;
            cudaError_t err = cudaMalloc(&ptr, sz);
            if (err == cudaSuccess) {
                PoolBuffer buf;
                buf.ptr = ptr;
                buf.size = sz;
                buf.inUse = false;
                buf.stream = 0;
                buffers.push_back(buf);
                totalAllocated += sz;
            }
        }
        
        LOG_INFO("GpuPool", "Pre-allocated " + std::to_string(buffers.size()) + 
                 " buffers, total: " + std::to_string(totalAllocated / 1024 / 1024) + " MB");
    }
    
    // Acquire a buffer of at least 'size' bytes
    void* acquire(size_t size, cudaStream_t stream = 0) {
        std::lock_guard<std::mutex> lock(poolMutex);
        
        // Find available buffer
        for (auto& buf : buffers) {
            if (!buf.inUse && buf.size >= size) {
                buf.inUse = true;
                buf.stream = stream;
                totalUsed += buf.size;
                return buf.ptr;
            }
        }
        
        // No suitable buffer, allocate new one
        void* ptr = nullptr;
        cudaError_t err = cudaMalloc(&ptr, size);
        if (err != cudaSuccess) {
            LOG_ERROR("GpuPool", "Failed to allocate " + std::to_string(size) + " bytes");
            return nullptr;
        }
        
        PoolBuffer buf;
        buf.ptr = ptr;
        buf.size = size;
        buf.inUse = true;
        buf.stream = stream;
        buffers.push_back(buf);
        totalAllocated += size;
        totalUsed += size;
        
        return ptr;
    }
    
    // Release a buffer back to pool
    void release(void* ptr) {
        if (!ptr) return;
        
        std::lock_guard<std::mutex> lock(poolMutex);
        
        for (auto& buf : buffers) {
            if (buf.ptr == ptr && buf.inUse) {
                buf.inUse = false;
                buf.stream = 0;
                totalUsed -= buf.size;
                return;
            }
        }
    }
    
    // Cleanup all buffers
    void cleanup() {
        std::lock_guard<std::mutex> lock(poolMutex);
        
        for (auto& buf : buffers) {
            if (buf.ptr) {
                cudaFree(buf.ptr);
            }
        }
        buffers.clear();
        totalAllocated = 0;
        totalUsed = 0;
    }
    
    // Get statistics
    std::string getStats() {
        std::lock_guard<std::mutex> lock(poolMutex);
        std::stringstream ss;
        ss << "GPU Pool [Device " << deviceId << "]: ";
        ss << buffers.size() << " buffers, ";
        ss << (totalAllocated / 1024 / 1024) << " MB total, ";
        ss << (totalUsed / 1024 / 1024) << " MB in use";
        return ss.str();
    }
    
    size_t getTotalAllocated() const { return totalAllocated; }
    size_t getTotalUsed() const { return totalUsed; }
};

// =============================================================
// CUDA STREAM MANAGER - Multiple streams for async operations
// =============================================================

class CudaStreamManager {
private:
    std::vector<cudaStream_t> streams;
    std::vector<bool> inUse;
    int deviceId;
    std::mutex streamMutex;
    
public:
    CudaStreamManager(int devId = 0, int numStreams = 4) : deviceId(devId) {
        cudaSetDevice(deviceId);
        streams.resize(numStreams);
        inUse.resize(numStreams, false);
        
        for (int i = 0; i < numStreams; ++i) {
            cudaStreamCreate(&streams[i]);
        }
        
        LOG_INFO("CudaStreams", "Created " + std::to_string(numStreams) + " streams on device " + std::to_string(deviceId));
    }
    
    ~CudaStreamManager() {
        for (auto& stream : streams) {
            cudaStreamDestroy(stream);
        }
    }
    
    // Get next available stream
    cudaStream_t acquireStream() {
        std::lock_guard<std::mutex> lock(streamMutex);
        
        for (size_t i = 0; i < streams.size(); ++i) {
            if (!inUse[i]) {
                inUse[i] = true;
                return streams[i];
            }
        }
        
        // All streams in use, create new one
        cudaStream_t newStream;
        cudaStreamCreate(&newStream);
        streams.push_back(newStream);
        inUse.push_back(true);
        
        return newStream;
    }
    
    void releaseStream(cudaStream_t stream) {
        std::lock_guard<std::mutex> lock(streamMutex);
        
        for (size_t i = 0; i < streams.size(); ++i) {
            if (streams[i] == stream) {
                inUse[i] = false;
                return;
            }
        }
    }
    
    void synchronizeAll() {
        for (auto& stream : streams) {
            cudaStreamSynchronize(stream);
        }
    }
    
    size_t getStreamCount() const {
        return streams.size();
    }
};

// Global instances (one per device)
extern std::vector<std::unique_ptr<GpuMemoryPool>> g_gpuPools;
extern std::vector<std::unique_ptr<CudaStreamManager>> g_streamManagers;

void initGpuPools(int numDevices);
void cleanupGpuPools();

#endif // USE_CUDA
