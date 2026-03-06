// bsgs_cuda.h - CUDA GPU Acceleration Header
#ifndef BSGS_CUDA_H
#define BSGS_CUDA_H

#include <cuda_runtime.h>
#include <stdio.h>
#include <stdint.h>

// CUDA BSGS Accelerator class
class BSGSCudaAccelerator {
private:
    struct GPUHashEntry* d_babyStepsTable;
    uint32_t tableSize;
    uint64_t* d_foundI;
    uint64_t* d_foundJ;
    int* d_foundFlag;
    bool initialized;
    
public:
    BSGSCudaAccelerator();
    ~BSGSCudaAccelerator();
    
    bool initialize(uint64_t m);
    bool launchGiantSteps(uint64_t startI, uint64_t count, int threadsPerBlock, int numBlocks);
    bool checkResult(uint64_t* outI, uint64_t* outJ);
    void synchronize();
    void cleanup();
    
    static void printGPUInfo();
};

// C interface
extern "C" {
    BSGSCudaAccelerator* bsgscuda_create();
    void bsgscuda_destroy(BSGSCudaAccelerator* accel);
    int bsgscuda_initialize(BSGSCudaAccelerator* accel, uint64_t m);
    int bsgscuda_launch(BSGSCudaAccelerator* accel, uint64_t startI, uint64_t count);
    int bsgscuda_check_result(BSGSCudaAccelerator* accel, uint64_t* i, uint64_t* j);
    void bsgscuda_sync(BSGSCudaAccelerator* accel);
    void bsgscuda_print_info();
}

#endif // BSGS_CUDA_H
