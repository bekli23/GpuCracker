// bsgs_gpu_integration.h - GPU Integration for BSGS Mode
#ifndef BSGS_GPU_INTEGRATION_H
#define BSGS_GPU_INTEGRATION_H

#include "args.h"
#include <string>
#include <iostream>
#include <vector>
#include <cstdint>

// GPU Backend type
enum class GPUBackend {
    NONE,
    CUDA,
    OPENCL
};

// GPU-accelerated BSGS search
class BSGSGPUIntegration {
private:
    GPUBackend backend;
    bool initialized;
    void* cudaAccel;
    void* openclAccel;
    uint64_t babyStepsM;
    int deviceId;
    
public:
    BSGSGPUIntegration();
    ~BSGSGPUIntegration();
    
    bool initialize(const ProgramConfig& cfg, uint64_t m);
    bool uploadBabySteps(const std::vector<std::pair<std::vector<uint8_t>, uint64_t>>& babySteps);
    bool launchGiantSteps(uint64_t startI, uint64_t count);
    bool checkResult(uint64_t& outI, uint64_t& outJ);
    void synchronize();
    bool isActive() const;
    std::string getBackendName() const;
    void cleanup();
    
    static void printGPUInfo();
};

#endif // BSGS_GPU_INTEGRATION_H
