// bsgs_cuda.cu - CUDA GPU Acceleration for BSGS
#include "bsgs_cuda.h"

// Structures
struct ECPoint {
    uint32_t x[8];
    uint32_t y[8];
    uint32_t z[8];
    int infinity;
};

struct GPUHashEntry {
    uint32_t hash160[5];
    uint64_t value;
    int valid;
};

// Device functions
__device__ void pointDouble(ECPoint* result, const ECPoint* P) {
    if (P->infinity) {
        result->infinity = 1;
        return;
    }
    result->infinity = 0;
}

__device__ void pointAdd(ECPoint* result, const ECPoint* P, const ECPoint* Q) {
    if (P->infinity) { *result = *Q; return; }
    if (Q->infinity) { *result = *P; return; }
    result->infinity = 0;
}

__device__ void scalarMultiply(ECPoint* result, const ECPoint* base, uint64_t scalar) {
    result->infinity = 1;
    ECPoint temp = *base;
    while (scalar > 0) {
        if (scalar & 1) pointAdd(result, result, &temp);
        pointDouble(&temp, &temp);
        scalar >>= 1;
    }
}

__device__ void pubKeyToHash160(uint32_t* hash160, const uint32_t* x, const uint32_t* y) {
    for (int i = 0; i < 5; i++) hash160[i] = x[i] ^ x[i+3];
}

// Kernel
__global__ void giantStepsKernel(
    GPUHashEntry* table, uint32_t tableSize, ECPoint targetPubKey, ECPoint mG,
    uint64_t startI, uint64_t totalSteps, uint64_t* foundI, uint64_t* foundJ, int* foundFlag
) {
    uint64_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    uint64_t stride = blockDim.x * gridDim.x;
    
    for (uint64_t i = startI + idx; i < startI + totalSteps; i += stride) {
        if (*foundFlag) return;
        
        ECPoint giantPoint;
        scalarMultiply(&giantPoint, &mG, i);
        
        uint32_t hash160[5];
        pubKeyToHash160(hash160, giantPoint.x, giantPoint.y);
        
        uint32_t slot = hash160[0] % tableSize;
        for (uint32_t probe = 0; probe < 100; probe++) {
            uint32_t pos = (slot + probe) % tableSize;
            GPUHashEntry entry = table[pos];
            if (!entry.valid) continue;
            
            bool match = true;
            for (int j = 0; j < 5; j++) {
                if (entry.hash160[j] != hash160[j]) { match = false; break; }
            }
            if (match) {
                *foundI = i; *foundJ = entry.value; *foundFlag = 1; return;
            }
        }
    }
}

// Class implementation
BSGSCudaAccelerator::BSGSCudaAccelerator() {
    d_babyStepsTable = 0; tableSize = 0; d_foundI = 0; d_foundJ = 0; d_foundFlag = 0; initialized = false;
}

BSGSCudaAccelerator::~BSGSCudaAccelerator() { cleanup(); }

bool BSGSCudaAccelerator::initialize(uint64_t m) {
    tableSize = (uint32_t)(m * 2);
    if (tableSize > 100000000) tableSize = 100000000;
    
    size_t tableBytes = tableSize * sizeof(GPUHashEntry);
    
    cudaError_t err = cudaMalloc(&d_babyStepsTable, tableBytes);
    if (err != cudaSuccess) { printf("[CUDA] Failed to allocate table\n"); return false; }
    
    err = cudaMemset(d_babyStepsTable, 0, tableBytes);
    if (err != cudaSuccess) { printf("[CUDA] Failed to init table\n"); return false; }
    
    cudaMalloc(&d_foundI, sizeof(uint64_t));
    cudaMalloc(&d_foundJ, sizeof(uint64_t));
    cudaMalloc(&d_foundFlag, sizeof(int));
    
    printf("[CUDA] Initialized: %u entries (%.2f MB)\n", tableSize, tableBytes / (1024.0 * 1024.0));
    initialized = true;
    return true;
}

bool BSGSCudaAccelerator::launchGiantSteps(uint64_t startI, uint64_t count, int threadsPerBlock, int numBlocks) {
    if (!initialized) return false;
    
    int h_foundFlag = 0;
    cudaMemcpy(d_foundFlag, &h_foundFlag, sizeof(int), cudaMemcpyHostToDevice);
    
    ECPoint targetPubKey; targetPubKey.infinity = 0;
    ECPoint mG; mG.infinity = 0;
    
    giantStepsKernel<<<numBlocks, threadsPerBlock>>>(
        d_babyStepsTable, tableSize, targetPubKey, mG, startI, count, d_foundI, d_foundJ, d_foundFlag
    );
    
    if (cudaGetLastError() != cudaSuccess) return false;
    return true;
}

bool BSGSCudaAccelerator::checkResult(uint64_t* outI, uint64_t* outJ) {
    if (!initialized) return false;
    
    int h_foundFlag = 0;
    cudaMemcpy(&h_foundFlag, d_foundFlag, sizeof(int), cudaMemcpyDeviceToHost);
    if (h_foundFlag) {
        uint64_t h_i = 0, h_j = 0;
        cudaMemcpy(&h_i, d_foundI, sizeof(uint64_t), cudaMemcpyDeviceToHost);
        cudaMemcpy(&h_j, d_foundJ, sizeof(uint64_t), cudaMemcpyDeviceToHost);
        *outI = h_i; *outJ = h_j;
        return true;
    }
    return false;
}

void BSGSCudaAccelerator::synchronize() { cudaDeviceSynchronize(); }

void BSGSCudaAccelerator::cleanup() {
    if (d_babyStepsTable) cudaFree(d_babyStepsTable);
    if (d_foundI) cudaFree(d_foundI);
    if (d_foundJ) cudaFree(d_foundJ);
    if (d_foundFlag) cudaFree(d_foundFlag);
    d_babyStepsTable = 0; d_foundI = 0; d_foundJ = 0; d_foundFlag = 0; initialized = false;
}

void BSGSCudaAccelerator::printGPUInfo() {
    int deviceCount = 0;
    cudaGetDeviceCount(&deviceCount);
    printf("[CUDA] Found %d GPU(s)\n", deviceCount);
    for (int i = 0; i < deviceCount; i++) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, i);
        printf("[CUDA] Device %d: %s (%.2f GB)\n", i, prop.name, prop.totalGlobalMem / (1024.0 * 1024.0 * 1024.0));
    }
}

// C interface
extern "C" {
    BSGSCudaAccelerator* bsgscuda_create() { return new BSGSCudaAccelerator(); }
    void bsgscuda_destroy(BSGSCudaAccelerator* accel) { delete accel; }
    int bsgscuda_initialize(BSGSCudaAccelerator* accel, uint64_t m) { return accel->initialize(m) ? 1 : 0; }
    int bsgscuda_launch(BSGSCudaAccelerator* accel, uint64_t startI, uint64_t count) { return accel->launchGiantSteps(startI, count, 256, 1024) ? 1 : 0; }
    int bsgscuda_check_result(BSGSCudaAccelerator* accel, uint64_t* i, uint64_t* j) { return accel->checkResult(i, j) ? 1 : 0; }
    void bsgscuda_sync(BSGSCudaAccelerator* accel) { accel->synchronize(); }
    void bsgscuda_print_info() { BSGSCudaAccelerator::printGPUInfo(); }
}
