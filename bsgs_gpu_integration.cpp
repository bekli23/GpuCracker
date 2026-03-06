// bsgs_gpu_integration.cpp - GPU Integration Implementation

#include "bsgs_gpu_integration.h"

// Try to include CUDA header
#ifdef USE_CUDA
#include "bsgs_cuda.h"
#endif

// Try to include OpenCL header  
#ifdef USE_OPENCL
#include "bsgs_opencl.h"
#endif

BSGSGPUIntegration::BSGSGPUIntegration() {
    backend = GPUBackend::NONE;
    initialized = false;
    cudaAccel = 0;
    openclAccel = 0;
    babyStepsM = 0;
    deviceId = 0;
}

BSGSGPUIntegration::~BSGSGPUIntegration() {
    cleanup();
}

bool BSGSGPUIntegration::initialize(const ProgramConfig& cfg, uint64_t m) {
    babyStepsM = m;
    deviceId = cfg.deviceId;
    
    std::cout << "\n[BSGS-GPU] Initializing GPU acceleration...\n";
    
#ifdef USE_CUDA
    // Try CUDA first
    cudaAccel = (void*)new BSGSCudaAccelerator();
    if (((BSGSCudaAccelerator*)cudaAccel)->initialize(m)) {
        backend = GPUBackend::CUDA;
        initialized = true;
        std::cout << "[BSGS-GPU] CUDA backend initialized\n";
        return true;
    }
    delete (BSGSCudaAccelerator*)cudaAccel;
    cudaAccel = 0;
#endif
    
#ifdef USE_OPENCL
    // Try OpenCL
    openclAccel = (void*)new BSGSOpenCLAccelerator();
    if (((BSGSOpenCLAccelerator*)openclAccel)->initialize(m)) {
        backend = GPUBackend::OPENCL;
        initialized = true;
        std::cout << "[BSGS-GPU] OpenCL backend initialized\n";
        return true;
    }
    delete (BSGSOpenCLAccelerator*)openclAccel;
    openclAccel = 0;
#endif
    
    std::cout << "[BSGS-GPU] No GPU backend available, using CPU\n";
    return false;
}

bool BSGSGPUIntegration::uploadBabySteps(const std::vector<std::pair<std::vector<uint8_t>, uint64_t>>& babySteps) {
    if (!initialized) return false;
    
    std::cout << "[BSGS-GPU] Uploading " << babySteps.size() << " baby steps to GPU...\n";
    
    // For now, baby steps are computed on GPU directly in the kernel
    // In a full implementation, we would copy the hash table to GPU memory
    // This is a placeholder that returns success
    
    return true;
}

bool BSGSGPUIntegration::launchGiantSteps(uint64_t startI, uint64_t count) {
    if (!initialized) return false;
    
    if (backend == GPUBackend::CUDA && cudaAccel) {
#ifdef USE_CUDA
        return ((BSGSCudaAccelerator*)cudaAccel)->launchGiantSteps(startI, count, 256, 1024);
#endif
    }
    else if (backend == GPUBackend::OPENCL && openclAccel) {
#ifdef USE_OPENCL
        return ((BSGSOpenCLAccelerator*)openclAccel)->launchGiantSteps(startI, count, 256);
#endif
    }
    
    return false;
}

bool BSGSGPUIntegration::checkResult(uint64_t& outI, uint64_t& outJ) {
    if (!initialized) return false;
    
    if (backend == GPUBackend::CUDA && cudaAccel) {
#ifdef USE_CUDA
        return ((BSGSCudaAccelerator*)cudaAccel)->checkResult(&outI, &outJ);
#endif
    }
    else if (backend == GPUBackend::OPENCL && openclAccel) {
#ifdef USE_OPENCL
        return ((BSGSOpenCLAccelerator*)openclAccel)->checkResult(outI, outJ);
#endif
    }
    
    return false;
}

void BSGSGPUIntegration::synchronize() {
    if (!initialized) return;
    
    if (backend == GPUBackend::CUDA && cudaAccel) {
#ifdef USE_CUDA
        ((BSGSCudaAccelerator*)cudaAccel)->synchronize();
#endif
    }
    else if (backend == GPUBackend::OPENCL && openclAccel) {
#ifdef USE_OPENCL
        ((BSGSOpenCLAccelerator*)openclAccel)->synchronize();
#endif
    }
}

bool BSGSGPUIntegration::isActive() const {
    return initialized;
}

std::string BSGSGPUIntegration::getBackendName() const {
    switch (backend) {
        case GPUBackend::CUDA: return "CUDA";
        case GPUBackend::OPENCL: return "OpenCL";
        default: return "CPU";
    }
}

void BSGSGPUIntegration::cleanup() {
#ifdef USE_CUDA
    if (cudaAccel) {
        delete (BSGSCudaAccelerator*)cudaAccel;
        cudaAccel = 0;
    }
#endif
#ifdef USE_OPENCL
    if (openclAccel) {
        delete (BSGSOpenCLAccelerator*)openclAccel;
        openclAccel = 0;
    }
#endif
    
    initialized = false;
    backend = GPUBackend::NONE;
}

void BSGSGPUIntegration::printGPUInfo() {
    std::cout << "\n[BSGS-GPU] Available GPU Acceleration:\n";
#ifdef USE_CUDA
    std::cout << "[BSGS-GPU] CUDA: ENABLED\n";
#else
    std::cout << "[BSGS-GPU] CUDA: DISABLED\n";
#endif
#ifdef USE_OPENCL
    std::cout << "[BSGS-GPU] OpenCL: ENABLED\n";
#else
    std::cout << "[BSGS-GPU] OpenCL: DISABLED\n";
#endif
}
