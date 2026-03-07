// Windows conflict fix - MUST be first
#include "win_fix.h"

#include "gpu_memory_pool.h"

#ifdef USE_CUDA
#include <memory>

// Global instances
std::vector<std::unique_ptr<GpuMemoryPool>> g_gpuPools;
std::vector<std::unique_ptr<CudaStreamManager>> g_streamManagers;

void initGpuPools(int numDevices) {
    g_gpuPools.clear();
    g_streamManagers.clear();
    
    for (int i = 0; i < numDevices; ++i) {
        g_gpuPools.push_back(std::make_unique<GpuMemoryPool>(i));
        g_streamManagers.push_back(std::make_unique<CudaStreamManager>(i, 4));
        
        // Pre-allocate common buffer sizes
        std::vector<size_t> sizes = {
            1024 * 1024,      // 1 MB
            4 * 1024 * 1024,  // 4 MB
            16 * 1024 * 1024, // 16 MB
            64 * 1024 * 1024  // 64 MB
        };
        g_gpuPools[i]->preallocate(sizes);
    }
}

void cleanupGpuPools() {
    g_gpuPools.clear();
    g_streamManagers.clear();
}

#endif // USE_CUDA
