#pragma once
#define _CRT_SECURE_NO_WARNINGS

// Windows conflict fix - MUST be first
#include "win_fix.h"

// Standard library headers
#include <iostream>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <omp.h>
#include <fstream>
#include <sstream>
#include <map>
#include <queue>
#include <random>
#include <cstring>

// Core interface - MUST be included first
#include "gpu_interface.h"

// Dependinte Proiect
#include "args.h"
#include "bloom.h"
// RPC support removed - using only blockchain file reader
#ifdef HAS_CUDA
#include "cuda_provider.h"
#endif
#ifndef NO_OPENCL
#include "opencl_provider.h"
#endif
#include "utils.h"
#ifndef NO_SECP256K1
#include "mnemonic.h"
#include "mode_akm.h"
#endif
#include "akm_profile_manager.h"
#ifndef NO_SECP256K1
#include "check.h"
#endif
#include "scan.h" 
#include "multicoin.h" // [CRITIC] Pentru suportul extins de adrese
#include "coin_database.h" // [NEW] 100+ coin auto-detection support
#ifndef NO_SECP256K1
#include "pubkey.h" // [NEW] Public Key Recovery mode
#include "bsgs.h"   // [NEW] Baby Step Giant Step mode
#include "rho.h"    // [NEW] Pollard's Rho algorithm (memory-efficient)
#endif
#include "block_explorer.h" // [NEW] Mini Block Explorer web interface
#include "blockchain_reader.h" // [NEW] Direct blockchain file reader


#ifndef NO_OPENCL
#include <CL/cl.h>
#endif

#ifndef __CUDACC__
    #include <openssl/sha.h>
    #include <openssl/ripemd.h>
    #include <openssl/hmac.h>
    #include <openssl/evp.h>
    #ifndef NO_SECP256K1
    #include <secp256k1.h>
    #endif
#endif

#ifdef HAS_CUDA
extern "C" {
    void launch_gpu_akm_search(
        unsigned long long startSeed,
        unsigned long long count,
        int blocks,
        int threads,
        int points,
        const void* bloomFilterData,
        size_t bloomFilterSize,
        unsigned long long* outFoundSeeds,
        int* outFoundCount,
        int targetBits,
        bool sequential,
        const void* prefix,
        int prefixLen
    );
    
    void launch_gpu_mnemonic_search(
        unsigned long long start_seed, 
        unsigned long long count, 
        int blocks, 
        int threads, 
        const void* bloom_filter, 
        size_t bloom_size, 
        unsigned long long* out_seeds, 
        int* out_count
    );
}
#endif

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
#include <windows.h>
    // Undefine Windows macros that conflict with C++ code
    #ifdef CRITICAL
        #undef CRITICAL
    #endif
inline void setupConsole() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0; GetConsoleMode(hOut, &dwMode); dwMode |= 0x0004; 
    SetConsoleMode(hOut, dwMode); SetConsoleOutputCP(65001); 
    CONSOLE_CURSOR_INFO ci; GetConsoleCursorInfo(hOut, &ci); ci.bVisible = false; SetConsoleCursorInfo(hOut, &ci);
    
    // Set console mode to disable line wrapping
    SetConsoleMode(hOut, dwMode & ~ENABLE_WRAP_AT_EOL_OUTPUT);
}
inline void restoreConsole() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci; GetConsoleCursorInfo(hOut, &ci); ci.bVisible = true; SetConsoleCursorInfo(hOut, &ci);
}
// Robust clear screen using WinAPI
inline void clearScreenWin32() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
    
    COORD coordScreen = {0, 0};
    DWORD cCharsWritten;
    DWORD dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
    
    // Fill entire screen with spaces
    FillConsoleOutputCharacter(hOut, ' ', dwConSize, coordScreen, &cCharsWritten);
    // Fill with current attributes
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);
    // Move cursor to top-left
    SetConsoleCursorPosition(hOut, coordScreen);
}
// Get console width
inline int getConsoleWidth() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    return 120; // Default fallback
}
#else
#include <unistd.h>
#include <sys/ioctl.h>
inline void setupConsole() {} 
inline void restoreConsole() {}
inline void clearScreenWin32() {}
inline int getConsoleWidth() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        return w.ws_col;
    }
    return 120; // Default fallback
}
#endif

inline void moveTo(int r, int c) { std::cout << "\033[" << r << ";" << c << "H"; }

struct Scheme { std::string name; std::string path; int type; };

struct DisplayState {
    unsigned long long countId = 0;
    std::string mnemonic = "-";
    std::string entropyHex = "-"; 
    std::string hexKey = "-";
    struct AddrInfo { std::string type, path, addr; std::string status; bool isHit; };
    std::vector<AddrInfo> rows;
    int currentBit = 0;
};

// Forward declaration (defined in gpu_interface.h which is included at top)
class IGpuProvider;

struct ActiveGpuContext { IGpuProvider* provider; std::string backend; int deviceId; int globalId; };

#ifndef __CUDACC__
class Runner {
private:
    ProgramConfig cfg;
    BloomFilter bloom;
    // RPC removed - using blockchain file reader only
#ifndef NO_SECP256K1
    MnemonicTool mnemonicTool;
    AkmTool akmTool;
#endif
    XprvGenerator xprvGen; 
    
    std::atomic<bool> running{ true };
    std::atomic<unsigned long long> totalSeedsChecked{ 0 }; 
    std::atomic<unsigned long long> totalAddressesChecked{ 0 }; 
    std::atomic<unsigned long long> realSeedsProcessed{ 0 }; 
    std::atomic<unsigned long long> schematicUICounter{ 0 }; // Contor secvențial pentru UI schematic
    
    std::atomic<int> currentActiveBit{ 0 };

    std::mutex displayMutex, fileMutex;
    DisplayState currentDisplay;
    std::vector<ActiveGpuContext> activeGpus;
    std::vector<unsigned char*> hostBuffers;
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
    
    std::string startHexFullDisplay = ""; 

    int uiBaseLine = 22; int totalCores = 0; int workerCores = 0;
    bool isInputMode = false;
    int loadedPathsCount = 0;

    std::vector<std::string> wordlist;
    std::map<std::string, int> wordMap;
    
    // Cache for balance lookups (address -> balance)
    std::map<std::string, int64_t> balanceCache;
    std::mutex balanceCacheMutex;
    
    std::queue<std::pair<std::string, uint64_t>> pendingBalanceChecks; // address -> seedId
    std::mutex pendingBalanceMutex;
    int entropyBytes = 16;
    
    // AKM Profile Rotation
    AkmProfileRotator* profileRotator = nullptr;
    std::string currentAkmProfileName;
    std::chrono::time_point<std::chrono::steady_clock> profileStartTime;

    unsigned long long convertHexToULL(std::string hexStr) {
        try {
            if (hexStr.size() > 2 && (hexStr.substr(0, 2) == "0x" || hexStr.substr(0, 2) == "0X")) {
                hexStr = hexStr.substr(2);
            }
            if (hexStr.length() > 16) {
                hexStr = hexStr.substr(hexStr.length() - 16); 
            }
            return std::stoull(hexStr, nullptr, 16);
        } catch (...) {
            return 0;
        }
    }
    
    // Format satoshis to BTC string
    std::string FormatBTC(uint64_t satoshis) {
        double btc = satoshis / 100000000.0;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(8) << btc << " BTC";
        return ss.str();
    }

    void expandPathRecursive(const std::vector<std::string>& segments, int index, std::string currentPath, std::vector<std::string>& results) {
        if (index >= (int)segments.size()) {
            results.push_back(currentPath);
            return;
        }
        
        std::string seg = segments[index];
        size_t dash = seg.find('-');
        
        if (dash != std::string::npos) {
            bool hardened = (seg.back() == '\'');
            std::string sStart = seg.substr(0, dash);
            std::string sEnd = seg.substr(dash + 1);
            if (hardened && !sEnd.empty() && sEnd.back() == '\'') sEnd.pop_back();

            int start = 0, end = 0;
            try { 
                start = std::stoi(sStart); 
                end = std::stoi(sEnd); 
            } catch(...) { 
                std::string nextPath = (currentPath.empty() ? "" : currentPath + "/") + seg;
                expandPathRecursive(segments, index + 1, nextPath, results);
                return;
            }

            for (int i = start; i <= end; i++) {
                std::string nextSeg = std::to_string(i) + (hardened ? "'" : "");
                std::string nextPath = (currentPath.empty() ? "m" : currentPath) + "/" + nextSeg;
                if (index == 0 && currentPath.empty()) nextPath = "m/" + nextSeg;
                else if (!currentPath.empty()) nextPath = currentPath + "/" + nextSeg;
                
                expandPathRecursive(segments, index + 1, nextPath, results);
            }
        } else {
            std::string nextPath;
            if (index == 0 && currentPath.empty()) nextPath = seg;
            else nextPath = currentPath + "/" + seg;
            
            expandPathRecursive(segments, index + 1, nextPath, results);
        }
    }

    void loadPathFile() {
        std::vector<PathInfo> finalPaths;
        std::string coinList = cfg.multiCoin; // ex: "btc,ltc,doge"
        std::transform(coinList.begin(), coinList.end(), coinList.begin(), ::tolower);
        
        // Parse requested coins using the coin database
        std::vector<std::string> requestedCoins = CoinDatabase::parseCoinList(cfg.multiCoin);
        
        // Create a map for quick lookup
        std::map<std::string, bool> activeCoins;
        for (const auto& coin : requestedCoins) {
            activeCoins[coin] = true;
        }
        
        // Default to BTC if nothing specified or invalid coins
        if (activeCoins.empty()) {
            activeCoins["btc"] = true;
            requestedCoins.push_back("btc");
        }

        if (!cfg.pathFile.empty()) {
            std::ifstream file(cfg.pathFile);
            if (file.is_open()) {
                // std::cout << "[INFO] Loading paths from file: " << cfg.pathFile << "\n"; // Suppressed to prevent UI ghosting
                std::string line;
                int countLoaded = 0;
                while (std::getline(file, line)) {
                    size_t commentPos = line.find('#');
                    if (commentPos != std::string::npos) line = line.substr(0, commentPos);
                    line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
                    if (line.empty()) continue;

                    std::vector<std::string> segments;
                    std::stringstream ss(line);
                    std::string segment;
                    while (std::getline(ss, segment, '/')) {
                        if(!segment.empty()) segments.push_back(segment);
                    }

                    std::vector<std::string> expanded;
                    expandPathRecursive(segments, 0, "", expanded);

                    for (const auto& p : expanded) {
                        // Auto-detect coin from path using SLIP-44 index
                        std::string label = "CUSTOM";
                        std::string coin = "BTC"; // Default
                        bool detected = false;
                        
                        // Try to detect coin from m/44'/X' pattern
                        for (const auto& coinInfo : CoinDatabase::getAllCoins()) {
                            std::string slip44Path = "m/44'/" + std::to_string(coinInfo.slip44) + "'";
                            if (p.find(slip44Path) == 0) {
                                coin = coinInfo.symbol;
                                label = coinInfo.symbol + "-PATH";
                                detected = true;
                                break;
                            }
                        }
                        
                        // Check Bitcoin-specific paths (BIP49, BIP84)
                        if (!detected) {
                            if (p.find("m/49'/0'") == 0 || p.find("m/84'/0'") == 0) {
                                coin = "btc";
                                label = "BTC-PATH";
                            }
                        }

                        // Add if coin was requested
                        bool add = activeCoins.find(coin) != activeCoins.end();
                        
                        // If generic path, add for all active coins
                        if (label == "CUSTOM" && !detected) {
                            for (const auto& activeCoin : activeCoins) {
                                finalPaths.push_back({"GENERIC-" + activeCoin.first, p, activeCoin.first});
                            }
                            add = false; // Already added
                        }

                        if (add) {
                            finalPaths.push_back({label, p, coin});
                            countLoaded++;
                        }
                    }
                }
                file.close();
                // std::cout << "[INFO] Total Expanded Paths: " << countLoaded << "\n"; // Suppressed
                
                if (countLoaded == 0) {
                    // std::cout << "[WARN] File was empty. Loading EXTENDED defaults.\n"; // Suppressed
                    goto load_defaults;
                }

            } else {
                // std::cout << "[WARN] Cannot open path file. Using EXTENDED defaults.\n"; // Suppressed
                goto load_defaults;
            }
        }
        else {
        load_defaults:
            // Generate paths for all active coins using the database
            for (const auto& coinSymbol : activeCoins) {
                CoinInfo coin = CoinDatabase::getCoinBySymbol(coinSymbol.first);
                std::string upperCoin = coin.symbol;
                std::transform(upperCoin.begin(), upperCoin.end(), upperCoin.begin(), ::toupper);
                
                if (coin.isEthereumBased) {
                    // Ethereum-based coins (ETH, ETC, BNB, etc.)
                    finalPaths.push_back({upperCoin + "-BIP44", "m/44'/" + std::to_string(coin.slip44) + "'/0'/0/0", coin.symbol});
                    finalPaths.push_back({upperCoin + "-BIP32", "m/0", coin.symbol});
                } else if (coin.symbol == "btc") {
                    // Bitcoin with all derivation types (BIP44, BIP49, BIP84/BIP141)
                    finalPaths.push_back({"BIP32", "m/0", "btc"});
                    finalPaths.push_back({"BIP32", "m/0/0", "btc"});
                    finalPaths.push_back({"BIP44", "m/44'/0'/0'/0/0", "btc"});  // Legacy
                    finalPaths.push_back({"BIP49", "m/49'/0'/0'/0/0", "btc"});  // SegWit P2SH
                    finalPaths.push_back({"BIP84", "m/84'/0'/0'/0/0", "btc"});  // Native SegWit (BIP141)
                } else {
                    // Bitcoin-derived coins (LTC, DOGE, DASH, etc.)
                    std::string path44 = "m/44'/" + std::to_string(coin.slip44) + "'/0'/0/0";
                    finalPaths.push_back({upperCoin + "-BIP44", path44, coin.symbol});
                    
                    // Add Segwit variants if coin supports it (BIP49 = P2SH-SegWit, BIP84/BIP141 = Native SegWit)
                    if (coin.hasSegwit && coin.slip44 != 0) { // Don't duplicate BTC
                        std::string path49 = "m/49'/" + std::to_string(coin.slip44) + "'/0'/0/0";
                        std::string path84 = "m/84'/" + std::to_string(coin.slip44) + "'/0'/0/0";
                        finalPaths.push_back({upperCoin + "-BIP49", path49, coin.symbol});
                        finalPaths.push_back({upperCoin + "-BIP84/BIP141", path84, coin.symbol});
                    }
                }
            }
        }
#ifndef NO_SECP256K1
        mnemonicTool.setPaths(finalPaths);
#endif
        loadedPathsCount = (int)finalPaths.size();
        
        // Display active coins
        // std::cout << "[INFO] Active coins (" << requestedCoins.size() << "): "; // Suppressed
        for (size_t i = 0; i < requestedCoins.size(); i++) {
            // if (i > 0) std::cout << ", ";
            // std::cout << requestedCoins[i]; // Suppressed
        }
        // std::cout << " (" << loadedPathsCount << " paths)\n"; // Suppressed
    }

    void loadWords() {
        if (cfg.runMode != "mnemonic") return; 
        std::string target = cfg.language;
        if (target.find(".txt") == std::string::npos) target += ".txt";
        std::vector<std::string> paths = { "bip39/" + target, "bip39\\" + target, target, "bip39/english.txt" };
        std::ifstream file;
        for (const auto& p : paths) { file.open(p); if (file.is_open()) break; }
        if (!file.is_open()) {
            std::cout << "\n\033[1;31m[CRITICAL] Wordlist not found!\033[0m\n";
            return; 
        }
        wordlist.clear(); wordMap.clear();
        std::string line;
        while (std::getline(file, line)) {
            line.erase(std::remove_if(line.begin(), line.end(), [](unsigned char c){ return std::isspace(c); }), line.end());
            if (!line.empty()) { 
                wordlist.push_back(line); 
                wordMap[line] = (int)wordlist.size() - 1; 
            }
        }
        file.close();
        std::cout << "[INFO] Loaded " << wordlist.size() << " words into Runner\n";
    }

#ifdef HAS_CUDA
    void uploadWordsToGPU() {
        if (wordlist.empty()) return;
        std::string flatWL = "";
        for (const auto& w : wordlist) flatWL += w + " ";
        launch_gpu_init(flatWL.c_str(), (int)flatWL.length(), (int)wordlist.size());
        // std::cout << "[GPU] Wordlist uploaded (" << wordlist.size() << " words).\n"; // Suppressed
    }
#else
    void uploadWordsToGPU() {
        // No CUDA support - function stub
    }
#endif

    void calculateMnemonicParams() {
        entropyBytes = (cfg.words * 11 - cfg.words / 3) / 8;
    }

    bool isAddressTypeMatch(const std::string& typeStr, const std::string& pathStr, const std::string& filter) {
    if (filter.empty() || filter == "ALL") return true;

    std::string t = typeStr;
    std::string p = pathStr;
    std::transform(t.begin(), t.end(), t.begin(), ::toupper);
    std::transform(p.begin(), p.end(), p.begin(), ::toupper);

    std::stringstream ss(filter);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item.erase(0, item.find_first_not_of(" \t\r\n"));
        item.erase(item.find_last_not_of(" \t\r\n") + 1);
        if (item.empty()) continue;

        std::string f = item;
        std::transform(f.begin(), f.end(), f.begin(), ::toupper);

        if (f == "LEGACY" || f == "P2PKH") {
            if (t.find("LEGACY") != std::string::npos || t.find("P2PKH") != std::string::npos)
                return true;
        }
        else if (f == "P2SH") {
            if (t.find("P2SH") != std::string::npos) return true;
        }
        else if (f == "BECH32") {
            if (t.find("BECH32") != std::string::npos) return true;
        }
        else if (f == "SEGWIT") {
            if (t.find("P2SH") != std::string::npos || t.find("BECH32") != std::string::npos || t.find("SEGWIT") != std::string::npos)
                return true;
        }
        else if (f == "ETH") {
            if (t.find("ETH") != std::string::npos) return true;
        }
        else if (f == "COMP") {
            // Comprimat: orice tip care NU conține "UNCOMP"
            if (t.find("UNCOMP") == std::string::npos) return true;
        }
        else if (f == "UNCOMP") {
            if (t.find("UNCOMP") != std::string::npos) return true;
        }
        else {
            // Căutare generală în tip sau cale
            if (t.find(f) != std::string::npos || p.find(f) != std::string::npos)
                return true;
        }
    }
    return false;
}
    std::string formatUnits(double num, const std::string& unit) {
        const char* suffixes[] = { "", " K", " M", " B", " T", " Q" };
        int i = 0; while (num >= 1000 && i < 5) { num /= 1000; i++; }
        std::stringstream ss; ss << std::fixed << std::setprecision(2) << num << suffixes[i] << " " << unit;
        return ss.str();
    }

    void logDetailedHit(const std::string& mode, const std::string& info, const std::string& secret, const std::string& addr, const std::string& pk, const std::string& path) {
        std::lock_guard<std::mutex> lock(fileMutex);
        std::string fname = cfg.winFile.empty() ? "win.txt" : cfg.winFile;
        std::ofstream file(fname, std::ios::out | std::ios::app);
        if (file.is_open()) {
            file << "\n======================================================\n";
            file << "HIT FOUND (" << mode << ") | " << __DATE__ << " " << __TIME__ << "\n";
            file << "------------------------------------------------------\n";
            file << "Seed/Entropy: " << info << "\n";
            file << "Secret:       " << secret << "\n";
            file << "Address:      " << addr << "\n";
            file << "Path:         " << path << "\n";
            file << "PrivKey:      " << pk << "\n";
            file << "======================================================\n\n";
            file.flush();
            file.close();
        }
    }

    void detectHardware() {
        int gIdx = 0; bool useAll = (cfg.deviceId == -1);
        bool wantCuda = (cfg.gpuType == "cuda" || cfg.gpuType == "auto");
        bool wantOpenCL = (cfg.gpuType == "opencl" || cfg.gpuType == "auto");
        bool wantVulkan = (cfg.gpuType == "vulkan" || cfg.gpuType == "auto");
        
#ifdef HAS_CUDA
        if (wantCuda) {
            int cudaCount = 0;
            cudaError_t err = cudaGetDeviceCount(&cudaCount);
            if (err != cudaSuccess) {
                std::cerr << "[CUDA] Error getting device count: " << cudaGetErrorString(err) << "\n";
            } else if (cudaCount == 0) {
                std::cerr << "[CUDA] No CUDA devices found\n";
            } else {
                std::cout << "[CUDA] Found " << cudaCount << " device(s)\n";
                for (int i = 0; i < cudaCount; i++) {
                    if (useAll || cfg.deviceId == gIdx) {
                        try {
                            auto* p = new CudaProvider(i, cfg.cudaBlocks, cfg.cudaThreads, cfg.pointsPerThread, true);
                            p->init(); 
                            activeGpus.push_back({ p, "CUDA", i, gIdx });
                            std::cout << "[CUDA] Initialized device " << i << ": " << p->getName() << "\n";
                        } catch (const std::exception& e) {
                            std::cerr << "[CUDA] Failed to initialize device " << i << ": " << e.what() << "\n";
                        } catch (...) {
                            std::cerr << "[CUDA] Failed to initialize device " << i << ": unknown error\n";
                        }
                    }
                    gIdx++;
                }
            }
        }
#else
        if (wantCuda) {
            std::cerr << "[CUDA] Not compiled with CUDA support (HAS_CUDA not defined)\n";
        }
#endif
        
        // TODO: Add OpenCL and Vulkan detection here
        
        if (activeGpus.empty()) {
            std::cerr << "[Hardware] No GPU backends initialized (requested: " << cfg.gpuType << ")\n";
        } else {
            std::cout << "[Hardware] Total GPUs initialized: " << activeGpus.size() << "\n";
        }
        
        for(auto& gpu : activeGpus) hostBuffers.push_back(new unsigned char[(size_t)gpu.provider->getBatchSize() * 32]);
    }

public:
    Runner(ProgramConfig c) : cfg(c) {
        totalCores = std::thread::hardware_concurrency();
        workerCores = (cfg.cpuCores > 0) ? cfg.cpuCores : totalCores;
        setupConsole();
        
        if (!cfg.startFrom.empty()) {
            std::string s = cfg.startFrom;
            if (s.length() >= 2 && (s.substr(0,2) == "0x" || s.substr(0,2) == "0X")) s = s.substr(2);
            s.erase(0, s.find_first_not_of('0'));
            if (s.empty()) s = "0";
            startHexFullDisplay = "0x" + s;

            if (s.length() > 16) {
                std::string lowPart = s.substr(s.length() - 16);
                std::string highPart = s.substr(0, s.length() - 16);
                try {
                    totalSeedsChecked = std::stoull(lowPart, nullptr, 16);
                } catch(...) { totalSeedsChecked = 0; }

                if (cfg.runMode == "akm") {
#ifndef NO_SECP256K1
                    akmTool.setHighHexPrefix(highPart);
#endif
                    // std::cout << "[INFO] Huge Start Detected. Low: " << std::hex << totalSeedsChecked 
                    //           << " High: " << highPart << std::dec << "\n"; // Suppressed
                }
            } else {
                try {
                    totalSeedsChecked = std::stoull(s, nullptr, 16);
                } catch(...) { totalSeedsChecked = 0; }
            }
            // std::cout << "[INFO] Resuming from offset: " << startHexFullDisplay << "\n"; // Suppressed
        }

        loadPathFile();
        if (cfg.runMode == "akm") {
            if (cfg.akmListProfiles) { 
                AkmProfileRotator rotator(0); 
                rotator.listProfiles(); 
                exit(0); 
            }
            
            // Initialize profile rotator with rotation minutes (0 = no rotation)
            profileRotator = new AkmProfileRotator(cfg.akmProfileRotationSeconds, "akm_profile");
            
            if (cfg.akmRandomProfile) {
                // Random profile mode (with or without rotation)
                if (profileRotator->initRandom()) {
                    currentAkmProfileName = profileRotator->getCurrentName();
                    cfg.akmProfile = profileRotator->getCurrentPath();
                    // std::cout << "[AKM] Random profile selected: " << currentAkmProfileName << "\n"; // Suppressed
                    if (cfg.akmProfileRotationSeconds > 0) {
                        // std::cout << "[AKM] Profile rotation enabled: " << cfg.akmProfileRotationSeconds 
                        //           << " seconds per profile\n"; // Suppressed
                    }
                } else {
                    std::cerr << "[ERROR] No profiles found in akm_profile folder!\n";
                    exit(1);
                }
            } else {
                // Specific profile mode
                // First check if it's a profile name from akm_profile folder
                std::string resolvedPath = profileRotator->resolveProfilePath(cfg.akmProfile);
                if (!resolvedPath.empty()) {
                    cfg.akmProfile = resolvedPath;
                    currentAkmProfileName = profileRotator->getCurrentName();
                    // std::cout << "[AKM] Using profile: " << currentAkmProfileName << "\n"; // Suppressed
                } else {
                    // Try direct init
                    currentAkmProfileName = cfg.akmProfile;
                    // std::cout << "[AKM] Using profile path: " << cfg.akmProfile << "\n"; // Suppressed
                }
            }
            
            profileStartTime = std::chrono::steady_clock::now();
#ifndef NO_SECP256K1
            akmTool.init(cfg.akmProfile, "akm/wordlist_512_ascii.txt");
            
            // Set active coins for multi-coin AKM support
            std::vector<std::string> akmCoins = CoinDatabase::parseCoinList(cfg.multiCoin);
            if (akmCoins.empty()) akmCoins.push_back("btc");
            akmTool.setActiveCoins(akmCoins);
#endif
            // std::cout << "[INFO] AKM coins: " << akmCoins.size() << " (" << cfg.multiCoin << ")\n"; // Suppressed
        } else if (cfg.runMode != "brainwallet" && cfg.brainwalletMode.empty()) {
            // Don't load BIP39 wordlist for brainwallet mode (it doesn't need it)
            calculateMnemonicParams();
            loadWords();
#ifndef NO_SECP256K1
            bool wlLoaded = mnemonicTool.loadWordlist(cfg.language);
#else
            bool wlLoaded = false;
#endif
            // std::cout << "[INFO] Mnemonic wordlist loaded: " << (wlLoaded ? "YES" : "NO") 
            //           << " (lang: " << cfg.language << ")\n"; // Suppressed
            if (!wordlist.empty()) uploadWordsToGPU();
        }
        
        if (!cfg.bloomFiles.empty()) bloom.load(cfg.bloomFiles);
        
        // Check that at least one checking method is configured
        if (cfg.bloomFiles.empty() && cfg.blockchainDir.empty()) {
            std::cerr << "[WARN] No address checking method configured!\n";
            std::cerr << "       Use --bloom-keys FILE.blf OR --blockchain-dir PATH\n";
            std::cerr << "       Continuing without address verification...\n";
        }
        
        detectHardware();
    }

    ~Runner() { 
        restoreConsole(); 
        for (auto& g : activeGpus) if(g.provider) delete g.provider; 
        for (auto* b : hostBuffers) delete[] b; 
        if (profileRotator) delete profileRotator;
    }

    void drawInterface() { std::cout << "\033[2J\033[H"; }
    
    // Check and handle AKM profile rotation
    bool checkProfileRotation() {
        if (!profileRotator || cfg.akmProfileRotationSeconds <= 0) {
            return false;  // No rotation configured
        }
        
        if (!profileRotator->shouldRotate()) {
            return false;  // Not time to rotate yet
        }
        
        // Time to rotate!
        std::cout << "\n[AKM] Rotating profile...\n";
        std::string oldProfile = currentAkmProfileName;
        
        if (profileRotator->rotate()) {
            // Update config and reinitialize AKM tool
            currentAkmProfileName = profileRotator->getCurrentName();
            cfg.akmProfile = profileRotator->getCurrentPath();
            
            // Reinitialize AKM tool with new profile
#ifndef NO_SECP256K1
            akmTool.init(cfg.akmProfile, "akm/wordlist_512_ascii.txt");
#endif
            
            // Reset counters for new profile
            profileStartTime = std::chrono::steady_clock::now();
            
            std::cout << "[AKM] Switched from '" << oldProfile << "' to '" << currentAkmProfileName << "'\n";
            std::cout << "[AKM] Next rotation in " << cfg.akmProfileRotationSeconds << " seconds\n\n";
            return true;
        } else {
            std::cerr << "[WARN] Profile rotation failed!\n";
            return false;
        }
    }

    // Queue address for balance check
    void queueAddressForBalanceCheck(const std::string& address, uint64_t seedId) {
        // Skip if no checking method available
        if (cfg.blockchainDir.empty()) return;
        
        // Check if already in cache
        {
            std::lock_guard<std::mutex> lock(balanceCacheMutex);
            if (balanceCache.find(address) != balanceCache.end()) {
                return; // Already checked
            }
        }
        
        std::lock_guard<std::mutex> lock(pendingBalanceMutex);
        pendingBalanceChecks.push({address, seedId});
    }
    
    // Check address using blockchain reader
    bool checkAddressInBlockchain(const std::string& address) {
        if (cfg.blockchainDir.empty()) return false;
        
        auto* reader = BlockchainReader::GetBlockchainReader();
        if (!reader) return false;
        
        return reader->HasAddress(address);
    }
    
    // ============================================================================
    // BLOCKCHAIN CHECKER - Check addresses against local blockchain index
    // ============================================================================
    struct BlockchainCheckItem {
        std::string address;
        uint64_t seedId;
        std::string mnemonic;
        std::string privKey;
        std::string entropy;
        std::string path;
        std::string addrType;
    };
    
    std::thread blockchainCheckThread;
    std::atomic<bool> blockchainCheckRunning{false};
    std::queue<BlockchainCheckItem> pendingBlockchainChecks;
    std::mutex blockchainCheckMutex;
    std::set<std::string> blockchainHitCache;
    std::mutex blockchainHitCacheMutex;
    
    void blockchainCheckerWorker() {
        std::cout << "[Blockchain] Address checker started\n";
        blockchainCheckRunning = true;
        
        while (blockchainCheckRunning && running) {
            std::vector<BlockchainCheckItem> toCheck;
            
            // Get pending addresses
            {
                std::lock_guard<std::mutex> lock(blockchainCheckMutex);
                while (!pendingBlockchainChecks.empty() && toCheck.size() < 100) {
                    toCheck.push_back(pendingBlockchainChecks.front());
                    pendingBlockchainChecks.pop();
                }
            }
            
            if (toCheck.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            
            // Check each address
            for (const auto& item : toCheck) {
                if (!blockchainCheckRunning || !running) break;
                
                // Check if already in cache
                {
                    std::lock_guard<std::mutex> lock(blockchainHitCacheMutex);
                    if (blockchainHitCache.find(item.address) != blockchainHitCache.end()) {
                        continue;
                    }
                }
                
                // Check against blockchain
                if (checkAddressInBlockchain(item.address)) {
                    uint64_t height = 0;
                    uint64_t balanceSat = 0;
                    auto* reader = BlockchainReader::GetBlockchainReader();
                    if (reader) {
                        height = reader->GetAddressHeight(item.address);
                        auto bal = reader->GetAddressBalance(item.address);
                        balanceSat = bal.balanceSatoshis;
                    }
                    
                    // Always show hit
                    std::cout << "\n";
                    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
                    std::cout << "║  🎯 BLOCKCHAIN HIT! ADDRESS FOUND IN BLOCKCHAIN! 🎯          ║\n";
                    std::cout << "╠═══════════════════════════════════════════════════════════════╣\n";
                    std::cout << "║  Address: " << std::left << std::setw(49) << item.address << " ║\n";
                    std::cout << "║  Block Height: " << std::setw(43) << height << " ║\n";
                    std::cout << "║  Balance: " << std::setw(49) << FormatBTC(balanceSat) << " ║\n";
                    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
                    std::cout << "\n";
                    
                    // Save to blockchain hits file
                    std::ofstream hitFile("blockchain_hits.txt", std::ios::app);
                    if (hitFile.is_open()) {
                        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                        hitFile << "[BLOCKCHAIN HIT] " << item.address 
                                << " Height: " << height 
                                << " Balance: " << FormatBTC(balanceSat)
                                << " Time: " << now << "\n";
                    }
                    
                    // Save to WIN file if balance > 0.00000001 BTC (1 satoshi)
                    if (balanceSat > 1) {
                        std::ofstream winFile(cfg.winFile.empty() ? "win.txt" : cfg.winFile, std::ios::app);
                        if (winFile.is_open()) {
                            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                            winFile << "========================================\n";
                            winFile << "[WIN] BALANCE FOUND IN BLOCKCHAIN!\n";
                            winFile << "========================================\n";
                            winFile << "Timestamp: " << now << "\n";
                            winFile << "Seed ID: " << item.seedId << "\n";
                            winFile << "\n--- ADDRESS INFO ---\n";
                            winFile << "Address: " << item.address << "\n";
                            winFile << "Address Type: " << (item.addrType.empty() ? "Unknown" : item.addrType) << "\n";
                            winFile << "Derivation Path: " << (item.path.empty() ? "Unknown" : item.path) << "\n";
                            winFile << "Balance: " << FormatBTC(balanceSat) << " (" << balanceSat << " satoshis)\n";
                            winFile << "Block Height: " << height << "\n";
                            winFile << "\n--- SEED/KEY INFO ---\n";
                            winFile << "Mnemonic (BIP39): " << (item.mnemonic.empty() ? "N/A" : item.mnemonic) << "\n";
                            winFile << "Private Key (WIF): " << (item.privKey.empty() ? "N/A" : item.privKey) << "\n";
                            winFile << "Entropy: " << (item.entropy.empty() ? "N/A" : item.entropy) << "\n";
                            winFile << "========================================\n\n";
                            
                            std::cout << "\033[1;32m[WIN] Balance saved to win file: " << FormatBTC(balanceSat) << "\033[0m\n\n";
                        }
                    }
                    
                    // Add to cache
                    std::lock_guard<std::mutex> lock(blockchainHitCacheMutex);
                    blockchainHitCache.insert(item.address);
                }
            }
        }
        
        std::cout << "[Blockchain] Address checker stopped\n";
    }
    
    void startBlockchainChecker() {
        if (!cfg.blockchainDir.empty() && !blockchainCheckRunning) {
            blockchainCheckThread = std::thread(&Runner::blockchainCheckerWorker, this);
        }
    }
    
    void stopBlockchainChecker() {
        blockchainCheckRunning = false;
        if (blockchainCheckThread.joinable()) {
            blockchainCheckThread.join();
        }
    }
    
    void queueAddressForBlockchainCheck(const std::string& address, uint64_t seedId,
                                        const std::string& mnemonic = "",
                                        const std::string& privKey = "",
                                        const std::string& entropy = "",
                                        const std::string& path = "",
                                        const std::string& addrType = "") {
        if (cfg.blockchainDir.empty()) return;
        
        // Check if already in cache
        {
            std::lock_guard<std::mutex> lock(blockchainHitCacheMutex);
            if (blockchainHitCache.find(address) != blockchainHitCache.end()) {
                return;
            }
        }
        
        std::lock_guard<std::mutex> lock(blockchainCheckMutex);
        if (pendingBlockchainChecks.size() < 10000) { // Limit queue size
            pendingBlockchainChecks.push({address, seedId, mnemonic, privKey, entropy, path, addrType});
        }
    }

    void updateStats() {
        DisplayState s; { std::lock_guard<std::mutex> lock(displayMutex); s = currentDisplay; }
        double secs = std::max(0.001, std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - startTime).count());
        static int uiFrame = 0; uiFrame++;
        
        // Get console width to prevent text wrapping issues
        int consoleWidth = getConsoleWidth();
        int maxLineLen = std::max(80, consoleWidth - 2);

        // Use ANSI escape to clear screen once at start, then just move cursor
        static bool firstRun = true;
        if (firstRun) {
            std::cout << "\033[2J\033[H";
            firstRun = false;
        } else {
            std::cout << "\033[H";
        }
        std::cout << "=== GpuCracker v50.0 (BSGS + Rho + Hybrid ECDLP) ===\033[K\n";
        std::cout << "Class C: 100M h/s | Class D: 1B+ h/s\033[K\n"; 
        
        for (const auto& gpu : activeGpus) {
            std::string memInfo = "Shared/Global";
#ifdef HAS_CUDA
            if (gpu.backend == "CUDA") {
                size_t f = 0, t = 0; cudaSetDevice(gpu.deviceId); 
                if (cudaMemGetInfo(&f, &t) == cudaSuccess) {
                    std::stringstream ss; ss << (t-f)/(1024*1024) << "/" << t/(1024*1024) << " MB";
                    memInfo = ss.str();
                }
            }
#endif
            std::string gpuName = gpu.provider->getName();
            std::cout << "GPU " << gpu.globalId << ": \033[1;33m" << gpuName << "\033[0m (" << gpu.backend << ")\033[K\n";
            std::cout << "Conf:    \033[1;36m" << gpu.provider->getConfig() << "\033[0m | VRAM: " << memInfo << "\033[K\n";
            
            std::string recommendation = "";
            bool isLowSettings = (cfg.cudaBlocks < 1024 || cfg.cudaThreads < 512);

            if (gpuName.find("1060") != std::string::npos) {
                 if (cfg.cudaBlocks < 2048) {
                      recommendation = " [REC: GTX 1060 likes High VRAM. Try: --blocks 2048 --threads 2048 --points 128 for >14 T/s]";
                 }
            }
            else if (gpuName.find("RTX") != std::string::npos || gpuName.find("3060") != std::string::npos || gpuName.find("4090") != std::string::npos) {
                if (cfg.cudaBlocks < 4096) {
                    recommendation = " [REC: RTX Card detected. Try --blocks 4096 --threads 512 for speed boost]";
                }
            }
            else if (isLowSettings) {
                 recommendation = " [REC: High Load (100%) != Max Speed. Increase VRAM usage (--blocks 2048 --threads 1024) for optimal throughput]";
            }

            if (!recommendation.empty()) {
                std::cout << "\033[1;32m" << recommendation << "\033[0m\033[K\n";
            }
        }

        std::cout << "CPU:    Using " << workerCores << "/" << totalCores << " Cores\033[K\n";
        std::cout << "Filter: " << (cfg.setAddress.empty() ? "ALL" : cfg.setAddress) << "\033[K\n";
        
        // Display checking method
        if (!cfg.bloomFiles.empty()) {
            size_t totalBloomSize = 0;
            for(size_t i=0; i<bloom.getLayerCount(); i++) totalBloomSize += bloom.getLayerSize(i);
            std::cout << "Bloom Info: " << bloom.getLayerCount() << " File(s) | Total Size: " 
                      << (totalBloomSize / 1024 / 1024) << " MB\033[K\n";
        } else if (!cfg.blockchainDir.empty()) {
            std::cout << "Blockchain Reader: " << cfg.blockchainDir << "\033[K\n";
        } else {
            std::cout << "Check Mode: NO VERIFICATION (bloom or blockchain required)\033[K\n";
        }

        std::cout << "Paths:  Loaded " << loadedPathsCount << " variations (" << cfg.multiCoin << ")\033[K\n";
        std::cout << "\033[1;37mClasses of Attack Strategy:\033[0m\033[K\n";
        std::cout << "Class A: 10K h/s | \033[1;32mClass B: 1M h/s (GPU Experimental)\033[0m\033[K\n";
        std::cout << "Class C: 100M h/s | Class D: 1B+ h/s\033[K\n";
        
        std::string entDisplay = "Standard Random";
        
        if (cfg.runMode == "xprv-mode") {
             entDisplay = "XPRV SCANNER (Auto: " + cfg.entropyMode + ")";
        }
        else if (!cfg.xprv.empty()) {
            entDisplay = "Extended Private Key (XPRV Derivation - Single)";
        }
        else if (cfg.runMode == "akm") {
             // [MODIFICAT] Afișăm profilul curent cu info despre rotație
             std::string currentProfile = currentAkmProfileName.empty() ? 
                                          (cfg.akmProfile.empty() ? "Default" : cfg.akmProfile) : 
                                          currentAkmProfileName;
             
             // Add rotation info if enabled
             std::string rotationInfo = "";
             if (profileRotator && cfg.akmProfileRotationSeconds > 0) {
                 int remaining = profileRotator->getTimeUntilRotation();
                 rotationInfo = " [Rotate in " + std::to_string(remaining) + "s]";
             }

             if (cfg.akmBits.size() > 1) {
                 int liveBit = currentActiveBit.load();
                 entDisplay = "AKM [" + currentProfile + "]" + rotationInfo + " [POOL: " + std::to_string(cfg.akmBits.size()) + "] Bit: " + std::to_string(liveBit);
             } else {
                 entDisplay = "AKM [" + currentProfile + "]" + rotationInfo + " (Target: " + std::to_string((cfg.akmBits.empty() ? 0 : cfg.akmBits[0])) + "b)";
             }
        }
        else if (!cfg.entropyStr.empty()) {
            if (cfg.entropyStr == "random") entDisplay = "Generator: RANDOM (" + cfg.entropyMode + ")";
            else if (cfg.entropyStr == "schematic") entDisplay = "Generator: SCHEMATIC Patterns";
            else if (cfg.entropyStr == "brainwallet") entDisplay = "Generator: BRAINWALLET (Sequential)";
            else entDisplay = "PRESET VALUE (Checking single entropy)";
        }
        else if (cfg.mnemonicOrder == "schematic") {
            entDisplay = "Generator: SCHEMATIC Patterns (via Order)";
        }

        std::cout << "Entropy: \033[1;36m" << entDisplay << "\033[0m\033[K\n\033[K\n";

        if (cfg.count > 0) std::cout << "Target: Stop after " << cfg.count << " seeds.\033[K\n";
        else std::cout << "\033[K\n";

        std::cout << "Seed # " << s.countId << "\033[K\n";
        
        // Truncate long strings to fit console width
        std::string entropyDisplay = s.entropyHex;
        std::string mnemonicDisplay = s.mnemonic;
        std::string hexKeyDisplay = s.hexKey;
        if ((int)entropyDisplay.length() > maxLineLen - 20) entropyDisplay = entropyDisplay.substr(0, maxLineLen - 23) + "...";
        if ((int)mnemonicDisplay.length() > maxLineLen - 15) mnemonicDisplay = mnemonicDisplay.substr(0, maxLineLen - 18) + "...";
        if ((int)hexKeyDisplay.length() > maxLineLen - 15) hexKeyDisplay = hexKeyDisplay.substr(0, maxLineLen - 18) + "...";
        
        std::cout << "Filtered Entropy: \033[1;35m" << entropyDisplay << "\033[0m\033[K\n";
        std::cout << "Phrase/Key: " << mnemonicDisplay << "\033[K\n";
        std::cout << "PrivKey:    " << hexKeyDisplay << "\033[K\n\033[K\n";
        
        std::cout << "TYPE                PATH                           ADDRESS                                        STATUS\033[K\n";
        std::cout << "---------------------------------------------------------------------------------------------\033[K\n";

        std::map<std::string, std::vector<DisplayState::AddrInfo>> grouped;
        for (const auto& row : s.rows) grouped[row.type].push_back(row);
        
        for (const auto& kv : grouped) {
            const std::vector<DisplayState::AddrInfo>& items = kv.second;
            if (items.empty()) continue;
            int idx = uiFrame % items.size();
            const auto& currentItem = items[idx];
            
            // Build line with fixed column widths for proper alignment
            // TYPE column: 20 chars
            std::string line = currentItem.type;
            if (line.length() > 19) line = line.substr(0, 19);
            line += std::string(20 - line.length(), ' ');
            
            // PATH column: 31 chars (20+31=51)
            std::string pathPart = currentItem.path;
            if (pathPart.length() > 30) pathPart = pathPart.substr(0, 30);
            line += pathPart + std::string(31 - pathPart.length(), ' ');
            
            // ADDRESS column: 47 chars (51+47=98) - STATUS starts at column 98
            std::string addrPart = currentItem.addr;
            if (addrPart.length() > 46) addrPart = addrPart.substr(0, 46);
            line += addrPart + std::string(47 - addrPart.length(), ' ');
            
            std::cout << line;
            
            // Check blockchain balance if enabled
            if (!cfg.blockchainDir.empty()) {
                auto* reader = BlockchainReader::GetBlockchainReader();
                if (reader) {
                    auto bal = reader->GetAddressBalance(currentItem.addr);
                    if (bal.balanceSatoshis > 0) {
                        // Show balance in green if > 0
                        std::cout << "\033[1;32m" << FormatBTC(bal.balanceSatoshis) << "\033[0m\033[K";
                    } else {
                        std::cout << "\033[90m" << FormatBTC(0) << "\033[0m\033[K";
                    }
                } else {
                    std::cout << "-\033[K";
                }
            } else {
                // Show HIT or - (bloom filter mode)
                if (currentItem.isHit) std::cout << "\033[1;32mHIT\033[0m\033[K"; else std::cout << "-\033[K";
            }
            std::cout << "\n"; 
        }
        for(int i = (int)grouped.size(); i < 20; i++) std::cout << "\033[K\n"; 
        std::cout << "--------------------------------------------------------------------------------------\033[K\n";

        std::cout << "Total: " << formatUnits((double)totalAddressesChecked.load(), "addr") << " | Speed: " << formatUnits((double)totalAddressesChecked.load() / secs, "addr/s") << "   \033[K\n";
    }

    void workerClassBGPU(int gpuIdx) {
        bool hasEntropyStr = !cfg.entropyStr.empty();
        
        if (cfg.runMode == "xprv-mode") {
#ifndef NO_SECP256K1
             std::mt19937_64 rng(std::random_device{}() + gpuIdx);
             
             while(running) {
                 if (cfg.count > 0 && realSeedsProcessed.load() >= cfg.count) { running = false; break; }
                 
                 unsigned long long currentId;
                 if (cfg.entropyMode == "schematic") currentId = totalSeedsChecked.fetch_add(1);
                 else currentId = rng(); 

                 std::string generatedXprv = xprvGen.generate(currentId, cfg.entropyMode);
                 MnemonicResult res = mnemonicTool.processSeed(0, "random", cfg.words, bloom, cfg.setAddress, "xprv", generatedXprv);
                 
                 bool anyHit = false;
                 for(auto& r : res.rows) {
                     if (isAddressTypeMatch(r.type, r.path, cfg.setAddress)) {
                         if(r.isHit) {
                             logDetailedHit("XPRV_SCAN_HIT", std::to_string(currentId), generatedXprv, r.addr, "DERIVED", r.path);
                             anyHit = true;
                         }
                     }
                 }

                 if (((gpuIdx == 0 && !activeGpus.empty()) || (gpuIdx == -1)) && (currentId % 50 == 0)) {
                    std::lock_guard<std::mutex> lock(displayMutex); 
                    currentDisplay.countId = currentId; 
                    currentDisplay.mnemonic = generatedXprv; 
                    currentDisplay.entropyHex = "XPRV-" + cfg.entropyMode; 
                    currentDisplay.hexKey = "AUTO-GEN"; 
                    currentDisplay.rows.clear();
                    for(auto& r : res.rows) {
                        if (isAddressTypeMatch(r.type, r.path, cfg.setAddress)) {
                             currentDisplay.rows.push_back({r.type, r.path, r.addr, (r.isHit ? "HIT" : "-"), r.isHit});
                             queueAddressForBalanceCheck(r.addr, currentId);
                             queueAddressForBlockchainCheck(r.addr, currentId, generatedXprv, "", "", r.path, r.type);
                        }
                    }
                 } else if ((gpuIdx == 0 && !activeGpus.empty()) || (gpuIdx == -1)) {
                     // Still queue addresses even if not updating display
                     for(auto& r : res.rows) {
                         queueAddressForBalanceCheck(r.addr, currentId);
                         queueAddressForBlockchainCheck(r.addr, currentId, generatedXprv, "", "", r.path, r.type);
                     }
                 }

                 realSeedsProcessed.fetch_add(1);
                 totalAddressesChecked.fetch_add(res.rows.size());
             }
             return;
#endif
        }

        bool isKeyword = (cfg.entropyStr == "random" || cfg.entropyStr == "schematic" || 
                          cfg.entropyStr == "bip32" || cfg.entropyStr == "raw" ||
                          cfg.entropyStr == "brainwallet" || cfg.entropyStr == "password" ||
                          cfg.entropyStr == "alpha" || cfg.entropyStr == "num");
        
        bool isXprv = !cfg.xprv.empty();
        bool isPresetValue = (hasEntropyStr && !isKeyword) || isXprv;
        
        if (isPresetValue) {
            if ((gpuIdx == 0 && !activeGpus.empty()) || (gpuIdx == -1)) { 
#ifndef NO_SECP256K1
                std::string phrase;
                if (isXprv) {
                    phrase = cfg.xprv;
                } else {
                    phrase = mnemonicTool.phraseFromEntropyString(cfg.entropyStr, cfg.entropyMode);
                }

                if (phrase.substr(0, 5) == "ERROR") { running = false; return; }
                
                MnemonicResult res = mnemonicTool.processSeed(0, "random", cfg.words, bloom, cfg.setAddress, "default", phrase);
                std::vector<DisplayState::AddrInfo> fRows;
                for(auto& r : res.rows) {
                    if (isAddressTypeMatch(r.type, r.path, cfg.setAddress)) {
                        fRows.push_back({r.type, r.path, r.addr, (r.isHit ? "HIT" : "-"), r.isHit});
                        if(r.isHit) logDetailedHit("PRESET_HIT", cfg.entropyStr, res.mnemonic, r.addr, "DERIVED", r.path);
                    }
                }
                { 
                    std::lock_guard<std::mutex> lock(displayMutex); 
                    currentDisplay.countId = 1; 
                    currentDisplay.mnemonic = res.mnemonic; 
                    currentDisplay.entropyHex = isXprv ? "XPRV" : cfg.entropyStr; 
                    currentDisplay.hexKey = "PRESET"; 
                    currentDisplay.rows = fRows; 
                }
#else
                std::cerr << "[ERROR] Preset value mode requires SECP256K1 support\n";
#endif
                running = false; 
            }
            return;
        }

        // Handle CPU-only mode (gpuIdx = -1)
        unsigned long long bSz = 1000; // Default batch size for CPU
        
        if (gpuIdx >= 0 && gpuIdx < (int)activeGpus.size()) {
            ActiveGpuContext& gpu = activeGpus[gpuIdx];
#ifdef HAS_CUDA
            if (gpu.backend == "CUDA") cudaSetDevice(gpu.deviceId);
#endif
            bSz = gpu.provider->getBatchSize();
        }
        unsigned long long* fSeeds = new unsigned long long[1024]; int fCnt = 0;
        std::mt19937_64 rng(std::random_device{}() + gpuIdx);
        
        std::string genMode = "default";
        if (isKeyword) {
            if (cfg.entropyStr == "random") genMode = cfg.entropyMode;
            else genMode = cfg.entropyStr; 
        }

        std::vector<int> targetBits = cfg.akmBits;
        if (targetBits.empty()) targetBits.push_back(0); 
        int akmLen = (!cfg.akmLengths.empty()) ? cfg.akmLengths[0] : 10;
        bool isAkmSchematic = (cfg.runMode == "akm" && cfg.akmGenMode == "schematic");
        bool forceSequential = !cfg.startFrom.empty() || isAkmSchematic;

        while (running) {
            if (cfg.count > 0 && realSeedsProcessed.load() >= cfg.count) { running = false; break; }
            unsigned long long base = 0;
            
            if (cfg.mnemonicOrder == "sequential" || cfg.mnemonicOrder == "schematic" || forceSequential) {
                unsigned long long current = totalSeedsChecked.load();
                if (!forceSequential && cfg.count > 0 && current >= cfg.count) { running = false; break; }
                base = totalSeedsChecked.fetch_add(bSz);
            } else { 
                base = rng(); 
            }

            int currentTargetBit = 0;
            if (targetBits.size() > 1) {
                int rIdx = rng() % targetBits.size();
                currentTargetBit = targetBits[rIdx];
            } else {
                currentTargetBit = targetBits[0];
            }

            // Update UI from first GPU or first CPU worker
            if ((gpuIdx == 0 && !activeGpus.empty()) || (gpuIdx == -1)) {
                currentActiveBit.store(currentTargetBit);
            }

#ifdef HAS_CUDA
            // Skip GPU search for CPU workers (gpuIdx = -1)
            if (gpuIdx >= 0 && bloom.isLoaded() && activeGpus[gpuIdx].backend == "CUDA") {
                for (size_t k = 0; k < bloom.getLayerCount(); k++) {
                    const uint8_t* blfData = bloom.getLayerData(k);
                    size_t blfSize = bloom.getLayerSize(k);

                    if (cfg.runMode == "akm") {
#ifndef NO_SECP256K1
						bool sequential = (cfg.akmGenMode == "schematic" || !cfg.startFrom.empty());
                        const std::vector<uint8_t>& highBytes = akmTool.getHighBytes();
                        const void* prefixPtr = highBytes.empty() ? nullptr : highBytes.data();
                        int prefixLen = (int)highBytes.size();
						
                         launch_gpu_akm_search(base, bSz, cfg.cudaBlocks, cfg.cudaThreads, cfg.pointsPerThread, 
                                               blfData, blfSize, 
                                               fSeeds, &fCnt, currentTargetBit,
											   sequential, prefixPtr, prefixLen);
                         
                         if (fCnt > 0) {
                            std::string akmOrder = isAkmSchematic ? "schematic" : cfg.mnemonicOrder;
                            for (int i = 0; i < fCnt; i++) {
                                 AkmResult res = akmTool.processAkmSeed(fSeeds[i], akmOrder, currentTargetBit, akmLen, bloom);
                                 for (auto& r : res.rows) {
                                     if (isAddressTypeMatch(r.type, r.path, cfg.setAddress)) {
                                         if (r.isHit) logDetailedHit("AKM_HIT_BIT_" + std::to_string(currentTargetBit), std::to_string(fSeeds[i]), res.phrase, r.addr, res.hexKey, "AKM_Derivation");
                                     }
                                 }
                            }
                            fCnt = 0;
                         }
#endif
                    } 
                    else {
#ifndef NO_SECP256K1
                         launch_gpu_mnemonic_search(base, bSz, cfg.cudaBlocks, cfg.cudaThreads, 
                                                    blfData, blfSize, 
                                                    fSeeds, &fCnt);
                         if (fCnt > 0) {
                            for (int i = 0; i < fCnt; i++) {
                                 MnemonicResult res = mnemonicTool.processSeed(fSeeds[i], cfg.mnemonicOrder, cfg.words, bloom, cfg.setAddress, genMode);
                                 for (auto& r : res.rows) {
                                     if (isAddressTypeMatch(r.type, r.path, cfg.setAddress)) {
                                         if (r.isHit) logDetailedHit("MNEM_GPU_HIT", std::to_string(fSeeds[i]), res.mnemonic, r.addr, "BIP39", r.path);
                                     }
                                 }
                            }
                            fCnt = 0;
                         }
#endif
                    }
                }
            }
#endif

            // Update UI from first GPU or first CPU worker
            if ((gpuIdx == 0 && !activeGpus.empty()) || (gpuIdx == -1)) {
                unsigned long long visualSeed = base;
                if (cfg.mnemonicOrder == "schematic") {
                    // Pentru modul schematic, folosim contor secvențial dedicat pentru UI
                    visualSeed = schematicUICounter.fetch_add(1);
                } else if (cfg.mnemonicOrder == "random" && !forceSequential) {
                    visualSeed += (rng() % bSz);
                }
                
#ifndef NO_SECP256K1
                if (cfg.runMode == "akm") {
                    std::string akmOrder = isAkmSchematic ? "schematic" : cfg.mnemonicOrder;
                    AkmResult res = akmTool.processAkmSeed(visualSeed, akmOrder, currentTargetBit, akmLen, bloom);
                    { 
                        std::lock_guard<std::mutex> lock(displayMutex); 
                        currentDisplay.countId = visualSeed; 
                        currentDisplay.mnemonic = res.phrase;
                        currentDisplay.entropyHex = "AKM (Bit " + std::to_string(currentTargetBit) + ")";
                        currentDisplay.hexKey = res.hexKey;
                        currentDisplay.rows.clear();
                        for(auto& r : res.rows) {
                            if (isAddressTypeMatch(r.type, r.path, cfg.setAddress)) {
                                currentDisplay.rows.push_back({r.type, r.path, r.addr, (r.isHit ? "HIT" : "-"), r.isHit});
                                queueAddressForBalanceCheck(r.addr, visualSeed);
                                queueAddressForBlockchainCheck(r.addr, visualSeed, res.phrase, res.hexKey, "", r.path, r.type);
                                if (r.isHit) logDetailedHit("AKM_UI_HIT", std::to_string(visualSeed), res.phrase, r.addr, res.hexKey, "AKM_Display");
                            }
                        }
                    }
                } else {
                    MnemonicResult res = mnemonicTool.processSeed(visualSeed, cfg.mnemonicOrder, cfg.words, bloom, cfg.setAddress, genMode);
                    { 
                        std::lock_guard<std::mutex> lock(displayMutex); 
                        currentDisplay.countId = visualSeed; 
                        currentDisplay.mnemonic = res.mnemonic;
                        currentDisplay.entropyHex = res.entropyHex; 
                        currentDisplay.hexKey = (genMode == "default") ? "BIP39" : genMode;
                        currentDisplay.rows.clear();
                        for(auto& r : res.rows) {
                            if (isAddressTypeMatch(r.type, r.path, cfg.setAddress)) {
                                currentDisplay.rows.push_back({r.type, r.path, r.addr, (r.isHit ? "HIT" : "-"), r.isHit});
                                queueAddressForBalanceCheck(r.addr, visualSeed);
                                queueAddressForBlockchainCheck(r.addr, visualSeed, res.mnemonic, "", res.entropyHex, r.path, r.type);
                                if (r.isHit) logDetailedHit("MNEM_UI_HIT", std::to_string(visualSeed), res.mnemonic, r.addr, "BIP39", r.path);
                            }
                        }
                    }
                }
#endif
            }
            
            realSeedsProcessed.fetch_add(bSz);
            totalAddressesChecked.fetch_add(bSz * 4);
        }
        delete[] fSeeds;
    }

    // ============================================================================
    // PUBKEY MODE - Public Key Recovery / Private Key Search
    // ============================================================================
#ifndef NO_SECP256K1
    void runPubkeyMode() {
        PubkeyMode pubkeyMode(cfg, running);
        pubkeyMode.run();
    }
    
    // ============================================================================
    // BSGS MODE - Baby Step Giant Step for Secp256k1
    // ============================================================================
    void runBSGSMode() {
        BSGSMode bsgsMode(cfg, running);
        bsgsMode.run();
    }
#else
    void runPubkeyMode() {
        std::cerr << "[PUBKEY] ERROR: Secp256k1 support required for pubkey mode" << std::endl;
    }
    void runBSGSMode() {
        std::cerr << "[BSGS] ERROR: Secp256k1 support required for BSGS mode" << std::endl;
    }
#endif

    void start() {
#ifndef NO_SECP256K1
        if (cfg.runMode == "check") {
            CheckMode checker(mnemonicTool, bloom, cfg, running, nullptr);
            checker.run();
            return;
        }
#else
        if (cfg.runMode == "check") {
            std::cerr << "[CHECK] ERROR: Check mode requires secp256k1 support" << std::endl;
            return;
        }
#endif
        
        // Brainwallet mode: --mode brainwallet OR --brainwallet <mode>
        if (cfg.runMode == "brainwallet" || !cfg.brainwalletMode.empty()) {
#ifndef NO_SECP256K1
            // Set default brainwallet mode to random if not specified
            if (cfg.brainwalletMode.empty()) {
                cfg.brainwalletMode = "random";
            }
            CheckMode checker(mnemonicTool, bloom, cfg, running, nullptr);
            checker.run();
            return;
#else
            std::cerr << "[BRAINWALLET] ERROR: Brainwallet mode requires secp256k1 support" << std::endl;
            return;
#endif
        }
        
        // Pubkey mode: --mode pubkey
        if (cfg.runMode == "pubkey") {
            runPubkeyMode();
            return;
        }
        
        // BSGS mode: --mode bsgs
        if (cfg.runMode == "bsgs") {
            runBSGSMode();
            return;
        }
        
        // Pollard's Rho mode: --mode rho (memory-efficient for large ranges)
        #ifndef NO_SECP256K1
        if (cfg.runMode == "rho") {
            std::cout << "[MODE] Pollard's Rho algorithm (memory-efficient discrete log solver)\n";
            PollardsRho rhoMode(cfg, running);
            rhoMode.run();
            return;
        }
        
        // Hybrid mode: --mode hybrid (auto-select BSGS or Rho based on range/memory)
        if (cfg.runMode == "hybrid") {
            std::cout << "[MODE] Hybrid BSGS/Rho mode (auto-selecting best algorithm)\n";
            HybridBSGSRho hybridMode(cfg, running);
            hybridMode.run();
            return;
        }
        #else
        if (cfg.runMode == "rho" || cfg.runMode == "hybrid") {
            std::cerr << "[ERROR] Pollard's Rho and Hybrid modes require secp256k1 library.\n";
            std::cerr << "        Install with: sudo apt-get install libsecp256k1-dev\n";
            return;
        }
        #endif

        startTime = std::chrono::high_resolution_clock::now(); drawInterface();
        
        // RPC mode removed - using blockchain file reader only
        
        // Initialize blockchain reader if directory specified
        if (!cfg.blockchainDir.empty()) {
            std::cout << "[Blockchain] Initializing blockchain reader from: " << cfg.blockchainDir << "\n";
            if (!cfg.indexDir.empty()) {
                std::cout << "[Blockchain] Index directory: " << cfg.indexDir << "\n";
            }
            BlockchainReader::InitializeBlockchainReader(cfg.blockchainDir, cfg.indexDir);
            // Start indexing in background thread
            std::thread([](const std::string& dir) {
                auto* reader = BlockchainReader::GetBlockchainReader();
                if (reader) {
                    reader->StartIndexing();
                }
            }, cfg.blockchainDir).detach();
            // Start blockchain address checker
            startBlockchainChecker();
        }
        
        // Start mini block explorer if enabled
        if (cfg.enableExplorer) {
            BlockExplorer::startBlockExplorer(cfg);
        }
        
        // Wait for blockchain indexing to reach ~80% before starting generation
        if (!cfg.blockchainDir.empty()) {
            std::cout << "\n[Runner] Waiting for blockchain indexing to reach 80% before starting generation...\n";
            bool ready = BlockchainReader::WaitForBlockchainIndexing(0.8, 300); // Wait up to 5 minutes
            if (!ready) {
                std::cout << "[Runner] Warning: Blockchain indexing not at 80% yet, starting anyway...\n";
            }
        }
        
        std::vector<std::thread> threads;
        
        // Spawn GPU worker threads
        for (int i = 0; i < (int)activeGpus.size(); i++) {
            threads.emplace_back(&Runner::workerClassBGPU, this, i);
        }
        
        // If no GPUs available, spawn CPU worker threads
        if (activeGpus.empty()) {
            int cpuWorkers = cfg.cpuCores > 0 ? cfg.cpuCores : std::thread::hardware_concurrency();
            if (cpuWorkers < 1) cpuWorkers = 1;
            std::cout << "[Runner] No GPUs detected. Using " << cpuWorkers << " CPU worker threads.\n";
            for (int i = 0; i < cpuWorkers; i++) {
                threads.emplace_back(&Runner::workerClassBGPU, this, -1); // -1 indicates CPU worker
            }
        }
        int rotationCheckCounter = 0;
        int frameCounter = 0;
        while (running) { 
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); 
            
            // Update UI every 5th frame (~4 times per second) to reduce flickering
            if (++frameCounter % 5 == 0) {
                updateStats();
            }
            
            // Check for profile rotation every ~1 second (20 iterations * 50ms)
            if (cfg.runMode == "akm" && profileRotator && (++rotationCheckCounter % 20) == 0) {
                checkProfileRotation();
            }
            
            if (cfg.count > 0 && realSeedsProcessed.load() >= cfg.count) running = false;
        }
        for (auto& th : threads) if (th.joinable()) th.join();
        // RPC cleanup removed
        stopBlockchainChecker();
        BlockExplorer::stopBlockExplorer();
        updateStats(); 
        restoreConsole();
        std::cout << "\n\n[INFO] Job Finished. Total seeds checked: " << realSeedsProcessed.load() << "\n";
    }
};
#endif
