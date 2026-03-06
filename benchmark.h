#pragma once
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <functional>
#include <map>
#include "logger.h"
#include "gpu_auto_tuner.h"

// =============================================================
// BENCHMARK SUITE - Performance testing and comparison
// =============================================================

struct BenchmarkResult {
    std::string name;
    double durationMs;
    size_t iterations;
    double throughput; // ops/sec
    double avgLatency; // ms
    double minLatency;
    double maxLatency;
    std::vector<double> samples;
    
    std::string toString() const {
        std::stringstream ss;
        ss << name << ": " 
           << std::fixed << std::setprecision(2)
           << throughput << " ops/sec, "
           << "avg=" << avgLatency << "ms, "
           << "min=" << minLatency << "ms, "
           << "max=" << maxLatency << "ms";
        return ss.str();
    }
    
    std::string toJson() const {
        std::stringstream ss;
        ss << "{"
           << "\"name\":\"" << name << "\","
           << "\"duration_ms\":" << durationMs << ","
           << "\"iterations\":" << iterations << ","
           << "\"throughput\":" << throughput << ","
           << "\"avg_latency_ms\":" << avgLatency << ","
           << "\"min_latency_ms\":" << minLatency << ","
           << "\"max_latency_ms\":" << maxLatency
           << "}";
        return ss.str();
    }
};

class BenchmarkSuite {
private:
    std::vector<BenchmarkResult> results;
    std::string suiteName;
    std::chrono::time_point<std::chrono::high_resolution_clock> suiteStart;
    
public:
    explicit BenchmarkSuite(const std::string& name = "GpuCracker Benchmark") 
        : suiteName(name) 
    {
        suiteStart = std::chrono::high_resolution_clock::now();
    }
    
    // Run a benchmark with automatic timing
    template<typename Func>
    BenchmarkResult run(const std::string& name, Func&& func, int warmupRuns = 3, int measurementRuns = 10) {
        LOG_INFO("Benchmark", "Running: " + name);
        
        BenchmarkResult result;
        result.name = name;
        
        // Warmup
        for (int i = 0; i < warmupRuns; i++) {
            func();
        }
        
        // Measurement
        result.minLatency = 1e9;
        result.maxLatency = 0;
        double totalTime = 0;
        
        for (int i = 0; i < measurementRuns; i++) {
            auto start = std::chrono::high_resolution_clock::now();
            
            func();
            
            auto end = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(end - start).count();
            
            result.samples.push_back(ms);
            totalTime += ms;
            result.minLatency = std::min(result.minLatency, ms);
            result.maxLatency = std::max(result.maxLatency, ms);
        }
        
        result.durationMs = totalTime;
        result.iterations = measurementRuns;
        result.avgLatency = totalTime / measurementRuns;
        result.throughput = 1000.0 / result.avgLatency; // ops/sec
        
        results.push_back(result);
        
        LOG_INFO("Benchmark", "  " + result.toString());
        
        return result;
    }
    
    // Run throughput benchmark (measures ops over time)
    template<typename Func>
    BenchmarkResult runThroughput(const std::string& name, Func&& func, int durationMs = 1000) {
        LOG_INFO("Benchmark", "Running throughput test: " + name);
        
        BenchmarkResult result;
        result.name = name;
        
        // Warmup
        for (int i = 0; i < 10; i++) {
            func();
        }
        
        // Measurement
        auto start = std::chrono::high_resolution_clock::now();
        size_t count = 0;
        
        while (true) {
            func();
            count++;
            
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            
            if (elapsed >= durationMs) break;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        double totalMs = std::chrono::duration<double, std::milli>(end - start).count();
        
        result.durationMs = totalMs;
        result.iterations = count;
        result.throughput = (count / totalMs) * 1000.0;
        result.avgLatency = totalMs / count;
        result.minLatency = result.avgLatency;
        result.maxLatency = result.avgLatency;
        
        results.push_back(result);
        
        LOG_INFO("Benchmark", "  " + result.toString());
        
        return result;
    }
    
    // Print summary
    void printSummary() {
        auto suiteEnd = std::chrono::high_resolution_clock::now();
        double suiteDuration = std::chrono::duration<double>(suiteEnd - suiteStart).count();
        
        LOG_INFO("Benchmark", "=== " + suiteName + " Summary ===");
        LOG_INFO("Benchmark", "Total time: " + std::to_string(suiteDuration) + "s");
        LOG_INFO("Benchmark", "Results:");
        
        for (const auto& result : results) {
            LOG_INFO("Benchmark", "  " + result.toString());
        }
    }
    
    // Export to JSON
    void exportJson(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            LOG_ERROR("Benchmark", "Cannot write to: " + filename);
            return;
        }
        
        auto suiteEnd = std::chrono::high_resolution_clock::now();
        double suiteDuration = std::chrono::duration<double>(suiteEnd - suiteStart).count();
        
        file << "{\n";
        file << "  \"suite_name\": \"" << suiteName << "\",\n";
        file << "  \"total_duration_s\": " << suiteDuration << ",\n";
        file << "  \"results\": [\n";
        
        for (size_t i = 0; i < results.size(); ++i) {
            file << "    " << results[i].toJson();
            if (i < results.size() - 1) file << ",";
            file << "\n";
        }
        
        file << "  ]\n";
        file << "}\n";
        
        file.close();
        
        LOG_INFO("Benchmark", "Results exported to: " + filename);
    }
    
    // Export to CSV
    void exportCsv(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            LOG_ERROR("Benchmark", "Cannot write to: " + filename);
            return;
        }
        
        file << "Name,Duration(ms),Iterations,Throughput(ops/sec),Avg Latency(ms),Min Latency(ms),Max Latency(ms)\n";
        
        for (const auto& result : results) {
            file << result.name << ","
                 << result.durationMs << ","
                 << result.iterations << ","
                 << result.throughput << ","
                 << result.avgLatency << ","
                 << result.minLatency << ","
                 << result.maxLatency << "\n";
        }
        
        file.close();
        
        LOG_INFO("Benchmark", "Results exported to: " + filename);
    }
    
    // Compare with previous results
    void compareWith(const std::string& previousResultsJson) {
        // TODO: Implement comparison logic
        LOG_INFO("Benchmark", "Comparison not yet implemented");
    }
    
    const std::vector<BenchmarkResult>& getResults() const {
        return results;
    }
};

// GPU-specific benchmarks
#ifdef USE_CUDA
#include <cuda_runtime.h>

class GpuBenchmark {
public:
    static void runFullBenchmark(int deviceId = 0) {
        LOG_INFO("Benchmark", "=== GPU Full Benchmark ===");
        
        cudaSetDevice(deviceId);
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, deviceId);
        
        LOG_INFO("Benchmark", "Device: " + std::string(prop.name));
        
        BenchmarkSuite suite("GPU Benchmark - " + std::string(prop.name));
        
        // Auto-tune benchmark
        suite.run("auto_tune", [&]() {
            GpuAutoTuner tuner(deviceId);
            tuner.quickTune();
        }, 1, 3);
        
        // Memory allocation benchmark
        suite.runThroughput("memory_alloc", [&]() {
            void* ptr;
            cudaMalloc(&ptr, 1024 * 1024);
            cudaFree(ptr);
        }, 500);
        
        // Kernel launch benchmark
        suite.runThroughput("kernel_launch", [&]() {
            // Dummy kernel launch
            auto dummyKernel = [] __device__ () {};
            dummyKernel<<<1, 1>>>();
            cudaDeviceSynchronize();
        }, 500);
        
        suite.printSummary();
        suite.exportJson("gpu_benchmark.json");
    }
};
#endif
