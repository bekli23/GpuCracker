#pragma once

// =============================================================
// INTEGRATION HEADER - Links all enhanced components
// =============================================================

#include "logger.h"
#include "config_manager.h"
#include "checkpoint_manager.h"
#include "two_stage_filter.h"
#include "web_dashboard.h"
#include "altcoins.h"

#ifdef USE_CUDA
#include "gpu_memory_pool.h"
#include "gpu_auto_tuner.h"
#include "benchmark.h"
#endif

// Global instances
ConfigManager g_config;
CheckpointManager g_checkpoint;
TwoStageFilter g_filter;
std::unique_ptr<WebDashboard> g_dashboard;

#ifdef USE_CUDA
std::vector<std::unique_ptr<GpuMemoryPool>> g_gpuPools;
std::vector<std::unique_ptr<CudaStreamManager>> g_streamManagers;
#endif

// Initialize all subsystems
inline void initializeSystem(int argc, char* argv[]) {
    // 1. Initialize Logger
    LOG_INIT("gpucracker.log", LogLevel::INFO, false, true);
    LOG_INFO("System", "GpuCracker v50.0 Enhanced Starting...");
    
    // 2. Load Configuration
    if (std::ifstream("gpucracker.conf").good()) {
        g_config.load("gpucracker.conf");
        LOG_INFO("System", "Configuration loaded from gpucracker.conf");
    } else {
        LOG_WARN("System", "No configuration file found, using defaults");
    }
    
    // Apply command line overrides
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            g_config.load(argv[++i]);
        }
    }
    
    // 3. Initialize GPU Memory Pools
    #ifdef USE_CUDA
    int deviceCount = 0;
    cudaGetDeviceCount(&deviceCount);
    if (deviceCount > 0) {
        initGpuPools(deviceCount);
        LOG_INFO("System", "GPU Memory Pools initialized for " + std::to_string(deviceCount) + " device(s)");
    }
    #else
    LOG_INFO("System", "CUDA not available, skipping GPU Memory Pools");
    #endif
    
    // 4. Initialize Two-Stage Filter
    bool useSecondStage = g_config.getBool("filter.use_second_stage", true);
    g_filter = TwoStageFilter(useSecondStage);
    
    // 5. Initialize Web Dashboard if enabled
    if (g_config.getBool("dashboard.enabled", false)) {
        int port = g_config.getInt("dashboard.port", 8080);
        initDashboard(port);
        LOG_INFO("System", "Web Dashboard started on port " + std::to_string(port));
    }
    
    LOG_INFO("System", "System initialization complete");
}

// Cleanup all subsystems
inline void cleanupSystem() {
    LOG_INFO("System", "Shutting down...");
    
    stopDashboard();
    #ifdef USE_CUDA
    cleanupGpuPools();
    #endif
    
    LOG_INFO("System", "Cleanup complete");
}

// Enhanced argument parser with config support
inline ProgramConfig parseArgsEnhanced(int argc, char* argv[]) {
    // First parse command line
    ProgramConfig cfg = parseArgs(argc, argv);
    
    // Then apply config file values if not overridden
    if (g_config.isLoaded()) {
        if (cfg.cudaBlocks == 128) // Default value
            cfg.cudaBlocks = g_config.getInt("gpu.cuda_blocks", 128);
        if (cfg.cudaThreads == 128)
            cfg.cudaThreads = g_config.getInt("gpu.cuda_threads", 128);
        if (cfg.pointsPerThread == 1)
            cfg.pointsPerThread = g_config.getInt("gpu.points_per_thread", 1);
        if (cfg.deviceId == -1)
            cfg.deviceId = g_config.getInt("gpu.device_id", -1);
        if (cfg.words == 0)
            cfg.words = g_config.getInt("search.words", 12);
        if (!cfg.infinite)
            cfg.infinite = g_config.getBool("search.infinite", false);
        if (cfg.winFile == "win.txt")
            cfg.winFile = g_config.getString("files.win_file", "win.txt");
        
        // Load bloom files from config if not specified
        if (cfg.bloomFiles.empty()) {
            cfg.bloomFiles = g_config.getList("files.bloom_keys");
        }
    }
    
    // Check for auto-tune flag
    #ifdef USE_CUDA
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--auto-tune") {
            LOG_INFO("Config", "Running auto-tuner...");
            TuningProfile profile = autoTuneGpu(cfg.deviceId >= 0 ? cfg.deviceId : 0, true);
            cfg.cudaBlocks = profile.blocks;
            cfg.cudaThreads = profile.threads;
            cfg.pointsPerThread = profile.points;
            LOG_INFO("Config", "Auto-tune complete: " + profile.toString());
        }
    }
    
    // Check for benchmark flag
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--benchmark") {
            GpuBenchmark::runFullBenchmark(cfg.deviceId >= 0 ? cfg.deviceId : 0);
            exit(0);
        }
    }
    #endif
    
    // Check for test flag
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--test") {
            int failed = runAllTests();
            exit(failed);
        }
    }
    
    return cfg;
}

// Enhanced runner initialization
class EnhancedRunner : public Runner {
public:
    EnhancedRunner(ProgramConfig c) : Runner(c) {
        // Load checkpoint if exists and resume flag is set
        if (!c.startFrom.empty() && g_checkpoint.exists()) {
            SearchState state;
            if (g_checkpoint.load(state)) {
                LOG_INFO("Runner", "Resuming from checkpoint: offset=" + 
                         std::to_string(state.currentOffset));
            }
        }
    }
    
    void updateCheckpoint() {
        if (!g_config.getBool("checkpoint.enabled", true)) return;
        
        SearchState state;
        state.mode = cfg.runMode;
        state.seedsChecked = realSeedsProcessed.load();
        state.currentOffset = totalSeedsChecked.load();
        state.words = cfg.words;
        state.targetBits = cfg.akmBits;
        state.profile = cfg.akmProfile;
        state.running = running;
        
        g_checkpoint.autoSave(state);
    }
    
    void recordHit(const std::string& hitInfo) {
        if (g_dashboard) {
            g_dashboard->getStats().addHit(hitInfo);
        }
    }
    
protected:
    using Runner::cfg;
    using Runner::realSeedsProcessed;
    using Runner::totalSeedsChecked;
    using Runner::running;
};

// Signal handlers for graceful shutdown
#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#endif

inline void setupSignalHandlers() {
    auto signalHandler = [](int sig) {
        LOG_WARN("System", "Received signal " + std::to_string(sig) + ", shutting down gracefully...");
        cleanupSystem();
        exit(0);
    };
    
    #ifndef _WIN32
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    #endif
}
