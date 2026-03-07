#pragma once

// Windows conflict fix - MUST be first
#include "win_fix.h"

#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <functional>
#include <map>
#include <thread>
#include <atomic>
#include "logger.h"

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
        LOG_INFO("Benchmark", "Result: " + result.toString());
        
        return result;
    }
    
    // Run throughput benchmark (measures ops over time)
    template<typename Func>
    BenchmarkResult runThroughput(const std::string& name, Func&& func, int durationMs = 1000) {
        LOG_INFO("Benchmark", "Running throughput: " + name + " (" + std::to_string(durationMs) + "ms)");
        
        BenchmarkResult result;
        result.name = name;
        result.minLatency = 1e9;
        result.maxLatency = 0;
        
        // Warmup
        for (int i = 0; i < 10; i++) {
            func();
        }
        
        // Measurement
        size_t iterations = 0;
        auto start = std::chrono::high_resolution_clock::now();
        auto end = start + std::chrono::milliseconds(durationMs);
        
        while (std::chrono::high_resolution_clock::now() < end) {
            auto iterStart = std::chrono::high_resolution_clock::now();
            func();
            auto iterEnd = std::chrono::high_resolution_clock::now();
            
            double ms = std::chrono::duration<double, std::milli>(iterEnd - iterStart).count();
            result.samples.push_back(ms);
            result.minLatency = std::min(result.minLatency, ms);
            result.maxLatency = std::max(result.maxLatency, ms);
            iterations++;
        }
        
        auto actualEnd = std::chrono::high_resolution_clock::now();
        double totalMs = std::chrono::duration<double, std::milli>(actualEnd - start).count();
        
        result.durationMs = totalMs;
        result.iterations = iterations;
        result.avgLatency = totalMs / iterations;
        result.throughput = (iterations * 1000.0) / totalMs;
        
        results.push_back(result);
        LOG_INFO("Benchmark", "Result: " + result.toString());
        
        return result;
    }
    
    // Run parallel benchmark with multiple threads
    template<typename Func>
    BenchmarkResult runParallel(const std::string& name, Func&& func, int numThreads, int durationMs = 1000) {
        LOG_INFO("Benchmark", "Running parallel: " + name + " with " + std::to_string(numThreads) + " threads");
        
        BenchmarkResult result;
        result.name = name + "_" + std::to_string(numThreads) + "t";
        
        std::atomic<size_t> totalIterations{0};
        std::vector<std::thread> threads;
        
        auto start = std::chrono::high_resolution_clock::now();
        auto end = start + std::chrono::milliseconds(durationMs);
        
        for (int t = 0; t < numThreads; t++) {
            threads.emplace_back([&]() {
                size_t localCount = 0;
                while (std::chrono::high_resolution_clock::now() < end) {
                    func();
                    localCount++;
                }
                totalIterations += localCount;
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        auto actualEnd = std::chrono::high_resolution_clock::now();
        double totalMs = std::chrono::duration<double, std::milli>(actualEnd - start).count();
        
        result.durationMs = totalMs;
        result.iterations = totalIterations.load();
        result.avgLatency = totalMs / result.iterations;
        result.throughput = (result.iterations * 1000.0) / totalMs;
        result.minLatency = 0;
        result.maxLatency = 0;
        
        results.push_back(result);
        LOG_INFO("Benchmark", "Result: " + result.toString());
        
        return result;
    }
    
    void printSummary() const {
        auto suiteEnd = std::chrono::high_resolution_clock::now();
        double suiteDuration = std::chrono::duration<double>(suiteEnd - suiteStart).count();
        
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║             " << suiteName << " Summary              ║\n";
        std::cout << "╠════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Total Duration: " << std::fixed << std::setprecision(2) << suiteDuration << "s" 
                  << std::string(42 - std::to_string((int)suiteDuration).length(), ' ') << "║\n";
        std::cout << "╠════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Benchmark Results:                                         ║\n";
        
        for (const auto& r : results) {
            std::string line = "║  " + r.name + ": " + std::to_string((int)r.throughput) + " ops/sec";
            line += std::string(58 - line.length(), ' ') + "║\n";
            std::cout << line;
        }
        
        std::cout << "╚════════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
    }
    
    void exportJson(const std::string& filename) const {
        std::ofstream file(filename);
        file << "{\n";
        file << "  \"suite\":\"" << suiteName << "\",\n";
        file << "  \"results\":[\n";
        
        for (size_t i = 0; i < results.size(); i++) {
            file << "    " << results[i].toJson();
            if (i < results.size() - 1) file << ",";
            file << "\n";
        }
        
        file << "  ]\n";
        file << "}\n";
        file.close();
        
        LOG_INFO("Benchmark", "Results exported to: " + filename);
    }
    
    const std::vector<BenchmarkResult>& getResults() const { return results; }
};

// =============================================================
// COMPREHENSIVE BENCHMARK RUNNER
// =============================================================

class GpuCrackerBenchmark {
public:
    static void runFullBenchmark(int deviceId = -1) {
        LOG_INFO("Benchmark", "=== GpuCracker v50.0 Full Benchmark ===");
        
        BenchmarkSuite suite("GpuCracker v50.0 Full Benchmark");
        
        // Get system info
        int numThreads = std::thread::hardware_concurrency();
        LOG_INFO("Benchmark", "CPU Threads: " + std::to_string(numThreads));
        
        // ========================================
        // CPU BENCHMARKS
        // ========================================
        
        // SHA256 benchmark
        suite.runThroughput("cpu_sha256", []() {
            // Simulate SHA256 workload
            volatile uint64_t sum = 0;
            for (int i = 0; i < 1000; i++) {
                sum += i * 2654435761u;
            }
        }, 1000);
        
        // PBKDF2-like benchmark
        suite.runThroughput("cpu_pbkdf2", []() {
            volatile uint64_t result = 0;
            for (int i = 0; i < 2048; i++) {
                result = result * 6364136223846793005ull + 1442695040888963407ull;
            }
        }, 1000);
        
        // secp256k1 point operations (simulated)
        suite.runThroughput("cpu_secp256k1", []() {
            volatile uint64_t x = 0x79BE667EF9DCBBAC;
            volatile uint64_t y = 0x483ADA7726A3C465;
            for (int i = 0; i < 100; i++) {
                x = (x * y + 0xFFFF) % 0xFFFFFFFFFFFFFFFF;
                y = (y * x + 0xAAAA) % 0xFFFFFFFFFFFFFFFF;
            }
        }, 1000);
        
        // ========================================
        // PARALLEL CPU BENCHMARKS
        // ========================================
        
        if (numThreads > 1) {
            suite.runParallel("cpu_parallel", []() {
                volatile uint64_t sum = 0;
                for (int i = 0; i < 1000; i++) {
                    sum += i * 2654435761u;
                }
            }, numThreads, 2000);
        }
        
        // ========================================
        // MODE-SPECIFIC BENCHMARKS
        // ========================================
        
        // Mnemonic generation benchmark
        suite.runThroughput("mode_mnemonic", []() {
            // Simulate mnemonic generation
            volatile int checksum = 0;
            for (int i = 0; i < 12; i++) {
                checksum = (checksum * 31 + i) % 2048;
            }
        }, 1000);
        
        // AKM generation benchmark
        suite.runThroughput("mode_akm", []() {
            // Simulate AKM key generation
            volatile uint64_t key = 0;
            for (int i = 0; i < 32; i++) {
                key = (key << 8) | (i * 17);
            }
        }, 1000);
        
        // BSGS baby steps benchmark
        suite.runThroughput("mode_bsgs_babystep", []() {
            // Simulate baby step calculation
            volatile uint64_t point = 0x79BE667EF9DCBBAC;
            for (int i = 0; i < 256; i++) {
                point = (point * 6364136223846793005ull + i) % 0xFFFFFFFFFFFFFFFF;
            }
        }, 1000);
        
        // Rho step benchmark
        suite.runThroughput("mode_rho_step", []() {
            // Simulate Rho iteration
            volatile uint64_t x = 0;
            volatile uint64_t c = 12345;
            for (int i = 0; i < 128; i++) {
                x = (x * x + c) % 0xFFFFFFFFFFFFFFFF;
            }
        }, 1000);
        
        // ========================================
        // MEMORY BENCHMARKS
        // ========================================
        
        suite.run("memory_alloc_1MB", []() {
            std::vector<uint8_t> buffer(1024 * 1024);
            for (size_t i = 0; i < buffer.size(); i += 4096) {
                buffer[i] = i & 0xFF;
            }
        }, 1, 10);
        
        // ========================================
        // GPU BENCHMARKS (if available)
        // ========================================
        
        #ifdef USE_CUDA
        if (deviceId >= 0) {
            runGpuBenchmarks(suite, deviceId);
        }
        #endif
        
        // ========================================
        // SUMMARY
        // ========================================
        
        suite.printSummary();
        suite.exportJson("gpucracker_benchmark.json");
        
        // Export detailed report
        exportDetailedReport(suite);
    }
    
    static void runModeBenchmark(const std::string& mode, int durationSec = 10) {
        LOG_INFO("Benchmark", "=== Mode-Specific Benchmark: " + mode + " ===");
        
        BenchmarkSuite suite("Mode Benchmark: " + mode);
        int durationMs = durationSec * 1000;
        
        if (mode == "mnemonic" || mode == "all") {
            suite.runThroughput("mnemonic_generation", []() {
                volatile int checksum = 0;
                for (int i = 0; i < 12; i++) {
                    checksum = (checksum * 31 + i) % 2048;
                }
            }, durationMs);
        }
        
        if (mode == "akm" || mode == "all") {
            suite.runThroughput("akm_key_gen", []() {
                volatile uint64_t key = 0;
                for (int i = 0; i < 32; i++) {
                    key = (key << 8) | (i * 17);
                }
            }, durationMs);
        }
        
        if (mode == "bsgs" || mode == "all") {
            suite.runThroughput("bsgs_baby_step", []() {
                volatile uint64_t point = 0x79BE667EF9DCBBAC;
                for (int i = 0; i < 256; i++) {
                    point = (point * 6364136223846793005ull + i) % 0xFFFFFFFFFFFFFFFF;
                }
            }, durationMs);
            
            suite.runThroughput("bsgs_giant_step", []() {
                volatile uint64_t point = 0x79BE667EF9DCBBAC;
                for (int i = 0; i < 512; i++) {
                    point = (point * 6364136223846793005ull + i) % 0xFFFFFFFFFFFFFFFF;
                }
            }, durationMs);
        }
        
        if (mode == "rho" || mode == "all") {
            suite.runThroughput("rho_iteration", []() {
                volatile uint64_t x = 0;
                volatile uint64_t c = 12345;
                for (int i = 0; i < 128; i++) {
                    x = (x * x + c) % 0xFFFFFFFFFFFFFFFF;
                }
            }, durationMs);
        }
        
        suite.printSummary();
    }
    
private:
    #ifdef USE_CUDA
    static void runGpuBenchmarks(BenchmarkSuite& suite, int deviceId) {
        LOG_INFO("Benchmark", "Running GPU benchmarks...");
        
        // These would need actual CUDA implementation
        // For now, just log that GPU is available
        LOG_INFO("Benchmark", "GPU device ID: " + std::to_string(deviceId));
        
        // Memory allocation benchmark
        suite.runThroughput("gpu_memory_alloc", []() {
            // Simulated GPU memory operation
            volatile uint64_t dummy = 0;
            for (int i = 0; i < 1000; i++) {
                dummy += i;
            }
        }, 1000);
        
        // Kernel launch benchmark
        suite.runThroughput("gpu_kernel_launch", []() {
            // Simulated kernel launch
            volatile uint64_t dummy = 0;
            for (int i = 0; i < 100; i++) {
                dummy += i * 2;
            }
        }, 1000);
    }
    #endif
    
    static void exportDetailedReport(const BenchmarkSuite& suite) {
        std::ofstream file("gpucracker_benchmark_report.txt");
        
        file << "╔════════════════════════════════════════════════════════════╗\n";
        file << "║         GpuCracker v50.0 Benchmark Report                 ║\n";
        file << "╚════════════════════════════════════════════════════════════╝\n\n";
        
        file << "System Information:\n";
        file << "  CPU Threads: " << std::thread::hardware_concurrency() << "\n";
        file << "  Date: " << __DATE__ << " " << __TIME__ << "\n\n";
        
        file << "Detailed Results:\n";
        file << "═══════════════════════════════════════════════════════════════\n\n";
        
        for (const auto& r : suite.getResults()) {
            file << "Test: " << r.name << "\n";
            file << "  Throughput: " << std::fixed << std::setprecision(2) << r.throughput << " ops/sec\n";
            file << "  Avg Latency: " << r.avgLatency << " ms\n";
            file << "  Min Latency: " << r.minLatency << " ms\n";
            file << "  Max Latency: " << r.maxLatency << " ms\n";
            file << "  Iterations: " << r.iterations << "\n";
            file << "  Total Duration: " << r.durationMs << " ms\n\n";
        }
        
        file << "═══════════════════════════════════════════════════════════════\n";
        file << "Generated by GpuCracker v50.0 Benchmark Suite\n";
        file.close();
        
        LOG_INFO("Benchmark", "Detailed report saved to: gpucracker_benchmark_report.txt");
    }
};
