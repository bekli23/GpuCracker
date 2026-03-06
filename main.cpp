#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fstream>

// Includem runner-ul care conține logica CPU
#include "runner.h"
#include "config_manager.h"

// =============================================================
// MAIN ENTRY POINT (CPU ONLY)
// =============================================================
int main(int argc, char* argv[]) {
    // 0. Load configuration from file if exists
    ConfigManager config;
    if (std::ifstream("gpucracker.conf").good()) {
        config.load("gpucracker.conf");
    }
    
    // 1. Parsare Argumente
    ProgramConfig cfg = parseArgs(argc, argv);
    
    // 2. Apply config file settings (if not overridden by command line)
    if (cfg.cudaBlocks == 128) // default
        cfg.cudaBlocks = config.getInt("gpu.cuda_blocks", 128);
    if (cfg.cudaThreads == 128)
        cfg.cudaThreads = config.getInt("gpu.cuda_threads", 128);
    if (cfg.pointsPerThread == 1)
        cfg.pointsPerThread = config.getInt("gpu.points_per_thread", 1);
    if (cfg.deviceId == -1)
        cfg.deviceId = config.getInt("gpu.device_id", -1);
    if (cfg.cpuCores == 0)
        cfg.cpuCores = config.getInt("performance.cpu_cores", 0);
    
    // 2a. Apply pubkey mode config settings
    if (cfg.pubAddress.empty())
        cfg.pubAddress = config.getString("pubkey.target_address", "");
    if (cfg.pubBitStart == 0)
        cfg.pubBitStart = config.getInt("pubkey.bit_start", 0);
    if (cfg.pubBitEnd == 0)
        cfg.pubBitEnd = config.getInt("pubkey.bit_end", 66);
    if (cfg.pubKeyType == "auto")
        cfg.pubKeyType = config.getString("pubkey.address_type", "auto");

    // 2. Afișare Help
    if (cfg.help) {
        printHelp();
        return 0;
    }

    // 3. Inițializare Random
    srand((unsigned int)time(NULL));

    // 4. Pornire Runner
    // Acum compilatorul C++ vede clasa Runner corect
    Runner runner(cfg);
    runner.start();

    return 0;
}