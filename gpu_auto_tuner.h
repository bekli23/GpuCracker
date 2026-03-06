#pragma once

#ifdef USE_CUDA
#include <cuda_runtime.h>

#include <string>
#include <vector>
#include <chrono>
#include <iostream>
#include "logger.h"

// =============================================================
// GPU AUTO-TUNER - Automatically find optimal settings
// =============================================================

struct TuningProfile {
    int blocks;
    int threads;
    int points;
    double throughput; // seeds/second
    double gpuUtilization;
    std::string deviceName;
    
    std::string toString() const {
        return "B:" + std::to_string(blocks) + 
               " T:" + std::to_string(threads) + 
               " P:" + std::to_string(points) +
               " => " + std::to_string((long long)throughput) + " seeds/s";
    }
};

class GpuAutoTuner {
private:
    int deviceId;
    cudaDeviceProp prop;
    
    struct TestConfig {
        int blocks;
        int threads;
        int points;
    };
    
    // Test kernel (dummy computation)
    __global__ void benchmarkKernel(unsigned long long* results, int iterations) {
        int tid = blockIdx.x * blockDim.x + threadIdx.x;
        unsigned long long sum = tid;
        
        for (int i = 0; i < iterations; i++) {
            sum = sum * 6364136223846793005ULL + 1442695040888963407ULL;
        }
        
        if (tid < 1024) {
            results[tid] = sum;
        }
    }
    
    double runBenchmark(const TestConfig& config, int durationMs = 500) {
        const int warmupIterations = 100;
        const int testIterations = 1000;
        
        unsigned long long* d_results;
        cudaMalloc(&d_results, 1024 * sizeof(unsigned long long));
        
        // Warmup
        benchmarkKernel<<<config.blocks, config.threads>>>(d_results, warmupIterations);
        cudaDeviceSynchronize();
        
        // Benchmark
        auto start = std::chrono::high_resolution_clock::now();
        int totalRuns = 0;
        
        while (true) {
            benchmarkKernel<<<config.blocks, config.threads>>>(d_results, testIterations);
            cudaDeviceSynchronize();
            totalRuns++;
            
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            
            if (elapsed >= durationMs) break;
        }
        
        cudaFree(d_results);
        
        // Calculate throughput
        double totalWork = (double)config.blocks * config.threads * testIterations * totalRuns;
        double seconds = durationMs / 1000.0;
        return totalWork / seconds;
    }
    
    std::vector<TestConfig> generateTestConfigs() {
        std::vector<TestConfig> configs;
        
        // Generate configs based on GPU architecture
        int smCount = prop.multiProcessorCount;
        
        // RTX 40 series (sm_89) - Ada Lovelace
        if (prop.major == 8 && prop.minor == 9) {
            configs.push_back({smCount * 4, 256, 100});
            configs.push_back({smCount * 8, 128, 100});
            configs.push_back({smCount * 4, 512, 50});
            configs.push_back({smCount * 2, 512, 100});
            configs.push_back({4096, 512, 64});
            configs.push_back({8192, 256, 64});
        }
        // RTX 30 series (sm_86) - Ampere
        else if (prop.major == 8 && prop.minor == 6) {
            configs.push_back({smCount * 4, 256, 100});
            configs.push_back({smCount * 8, 128, 100});
            configs.push_back({2048, 512, 64});
            configs.push_back({4096, 256, 64});
        }
        // RTX 20 series (sm_75) - Turing
        else if (prop.major == 7 && prop.minor == 5) {
            configs.push_back({smCount * 4, 128, 100});
            configs.push_back({smCount * 8, 64, 100});
            configs.push_back({2048, 256, 64});
            configs.push_back({1024, 512, 64});
        }
        // GTX 10 series (sm_61) - Pascal
        else if (prop.major == 6) {
            configs.push_back({smCount * 2, 128, 100});
            configs.push_back({smCount * 4, 64, 100});
            configs.push_back({1024, 256, 64});
            configs.push_back({512, 512, 64});
        }
        // Default configs
        else {
            configs.push_back({1024, 256, 64});
            configs.push_back({2048, 128, 64});
            configs.push_back({512, 512, 64});
            configs.push_back({1024, 512, 32});
            configs.push_back({4096, 256, 32});
        }
        
        return configs;
    }

public:
    GpuAutoTuner(int devId = 0) : deviceId(devId) {
        cudaSetDevice(deviceId);
        cudaGetDeviceProperties(&prop, deviceId);
    }
    
    TuningProfile tune(int testDurationMs = 500) {
        LOG_INFO("AutoTuner", "Starting auto-tuning for: " + std::string(prop.name));
        LOG_INFO("AutoTuner", "SM Count: " + std::to_string(prop.multiProcessorCount) + 
                 " | Compute: " + std::to_string(prop.major) + "." + std::to_string(prop.minor));
        
        auto configs = generateTestConfigs();
        
        TuningProfile best;
        best.throughput = 0;
        best.deviceName = prop.name;
        
        for (const auto& cfg : configs) {
            LOG_INFO("AutoTuner", "Testing config: B=" + std::to_string(cfg.blocks) + 
                     " T=" + std::to_string(cfg.threads) + " P=" + std::to_string(cfg.points));
            
            double throughput = runBenchmark(cfg, testDurationMs);
            
            LOG_INFO("AutoTuner", "  -> Throughput: " + std::to_string((long long)throughput));
            
            if (throughput > best.throughput) {
                best.blocks = cfg.blocks;
                best.threads = cfg.threads;
                best.points = cfg.points;
                best.throughput = throughput;
            }
        }
        
        // Estimate GPU utilization (simplified)
        best.gpuUtilization = std::min(100.0, (best.throughput / 1000000000.0) * 100.0);
        
        LOG_INFO("AutoTuner", "Best config: " + best.toString());
        
        return best;
    }
    
    TuningProfile quickTune() {
        return tune(200); // Quick 200ms test
    }
    
    TuningProfile fullTune() {
        return tune(1000); // Full 1s test
    }
    
    // Recommendations based on GPU model
    TuningProfile getRecommendedConfig() {
        TuningProfile rec;
        rec.deviceName = prop.name;
        
        // Detect GPU model from name
        std::string name = prop.name;
        
        if (name.find("4090") != std::string::npos) {
            rec.blocks = 8192;
            rec.threads = 512;
            rec.points = 64;
        } else if (name.find("4080") != std::string::npos) {
            rec.blocks = 6144;
            rec.threads = 512;
            rec.points = 64;
        } else if (name.find("4070") != std::string::npos) {
            rec.blocks = 4096;
            rec.threads = 512;
            rec.points = 64;
        } else if (name.find("3090") != std::string::npos || name.find("3090 Ti") != std::string::npos) {
            rec.blocks = 8192;
            rec.threads = 256;
            rec.points = 64;
        } else if (name.find("3080") != std::string::npos) {
            rec.blocks = 6144;
            rec.threads = 256;
            rec.points = 64;
        } else if (name.find("3070") != std::string::npos) {
            rec.blocks = 4096;
            rec.threads = 256;
            rec.points = 64;
        } else if (name.find("2080") != std::string::npos || name.find("2070") != std::string::npos) {
            rec.blocks = 2048;
            rec.threads = 512;
            rec.points = 64;
        } else if (name.find("1080") != std::string::npos || name.find("1070") != std::string::npos) {
            rec.blocks = 2048;
            rec.threads = 256;
            rec.points = 64;
        } else if (name.find("1060") != std::string::npos) {
            rec.blocks = 2048;
            rec.threads = 512;
            rec.points = 128;
        } else {
            // Generic fallback
            rec.blocks = prop.multiProcessorCount * 4;
            rec.threads = 256;
            rec.points = 64;
        }
        
        rec.throughput = 0; // Unknown until tested
        rec.gpuUtilization = 0;
        
        LOG_INFO("AutoTuner", "Recommended config for " + name + ": " + rec.toString());
        
        return rec;
    }
    
    void printGpuInfo() {
        LOG_INFO("AutoTuner", "=== GPU Information ===");
        LOG_INFO("AutoTuner", "Name: " + std::string(prop.name));
        LOG_INFO("AutoTuner", "SM Count: " + std::to_string(prop.multiProcessorCount));
        LOG_INFO("AutoTuner", "Compute Capability: " + std::to_string(prop.major) + "." + std::to_string(prop.minor));
        LOG_INFO("AutoTuner", "Max Threads per Block: " + std::to_string(prop.maxThreadsPerBlock));
        LOG_INFO("AutoTuner", "Shared Memory per Block: " + std::to_string(prop.sharedMemPerBlock / 1024) + " KB");
        LOG_INFO("AutoTuner", "Total Global Memory: " + std::to_string(prop.totalGlobalMem / 1024 / 1024) + " MB");
        LOG_INFO("AutoTuner", "Memory Clock Rate: " + std::to_string(prop.memoryClockRate / 1000) + " MHz");
        LOG_INFO("AutoTuner", "Memory Bus Width: " + std::to_string(prop.memoryBusWidth) + " bits");
    }
};

// Convenience function
inline TuningProfile autoTuneGpu(int deviceId = 0, bool quick = false) {
    GpuAutoTuner tuner(deviceId);
    tuner.printGpuInfo();
    return quick ? tuner.quickTune() : tuner.fullTune();
}

#endif // USE_CUDA
