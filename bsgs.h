#pragma once

// Windows conflict fix - MUST be first
#include "win_fix.h"

// ============================================================================
// BSGS MODE - Baby Step Giant Step for Secp256k1
// ============================================================================
// Algoritm eficient pentru rezolvarea problemei logaritmului discret
// Folosit pentru a recupera cheia privată din cheia publică
// 
// Formula: Q = k * G, unde Q = public key, k = private key, G = generator
// k = m * i + j, unde m = sqrt(n)
// Baby steps: j * G pentru j = 0, 1, ..., m-1
// Giant steps: Q - i * m * G pentru i = 0, 1, ..., m-1
// Când găsim match: j * G = Q - i * m * G => k = m * i + j
//
// Exemplu: --mode bsgs --bsgs-pub 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <atomic>
#include <cstring>
#include <thread>
#include <unordered_map>
#include <memory>
#include <random>
#include <algorithm>
#include <cctype>

#include <boost/multiprecision/cpp_int.hpp>

#include <openssl/sha.h>
#include <openssl/ripemd.h>

// For memory detection
#ifdef __linux__
    #include <sys/sysinfo.h>
#elif __APPLE__
    #include <sys/types.h>
    #include <sys/sysctl.h>
#elif _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    // Undefine Windows macros that conflict with C++ code
    #ifdef CRITICAL
        #undef CRITICAL
    #endif
#endif
#ifndef NO_SECP256K1
#include <secp256k1.h>
#endif

// BSGS Bloom Filter Address Checking
#include "bsgs_bloom_x.h"

// GPU Acceleration
#include "bsgs_gpu_integration.h"

namespace mp = boost::multiprecision;
using uint256_t = mp::number<mp::cpp_int_backend<256, 256, mp::unsigned_magnitude, mp::unchecked, void>>;

#include "utils.h"
#include "bloom.h"
#include "args.h"
#include "bloom_filter.h"

// ============================================================================
// Secp256k1 Point Structure
// ============================================================================
struct ECPoint {
    uint8_t x[32];
    uint8_t y[32];
    bool infinity;
    
    ECPoint() : infinity(false) {
        memset(x, 0, 32);
        memset(y, 0, 32);
    }
    
    bool operator==(const ECPoint& other) const {
        return memcmp(x, other.x, 32) == 0 && 
               memcmp(y, other.y, 32) == 0;
    }
};

// ============================================================================
// Hash function for ECPoint (for unordered_map)
// ============================================================================
struct ECPointHash {
    size_t operator()(const ECPoint& p) const {
        // Use first 8 bytes of x as hash
        size_t hash = 0;
        for (int i = 0; i < 8; i++) {
            hash = (hash << 8) | p.x[i];
        }
        return hash;
    }
};

// ============================================================================
// Memory Detection Helper
// ============================================================================
inline uint64_t getAvailableMemoryMB() {
    uint64_t availableMB = 0;
    
#ifdef __linux__
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        // freeram is in bytes, convert to MB
        availableMB = info.freeram / (1024 * 1024);
    }
#elif __APPLE__
    vm_size_t page_size;
    mach_port_t mach_port = mach_host_self();
    vm_statistics64_t vm_stats;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    if (host_page_size(mach_port, &page_size) == KERN_SUCCESS &&
        host_statistics64(mach_port, HOST_VM_INFO64, (host_info64_t)&vm_stats, &count) == KERN_SUCCESS) {
        availableMB = (vm_stats.free_count * page_size) / (1024 * 1024);
    }
#elif _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        availableMB = status.ullAvailPhys / (1024 * 1024);
    }
#endif
    
    return availableMB;
}

inline uint64_t getTotalMemoryMB() {
    uint64_t totalMB = 0;
    
#ifdef __linux__
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        totalMB = info.totalram / (1024 * 1024);
    }
#elif __APPLE__
    int mib[2];
    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    int64_t size = 0;
    size_t len = sizeof(size);
    if (sysctl(mib, 2, &size, &len, NULL, 0) == 0) {
        totalMB = size / (1024 * 1024);
    }
#elif _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        totalMB = status.ullTotalPhys / (1024 * 1024);
    }
#endif
    
    return totalMB;
}

// ============================================================================
// BSGS Algorithm Implementation
// ============================================================================
class BSGSMode {
private:
    ProgramConfig cfg;
    std::atomic<bool>& running;
    secp256k1_context* ctx;
    
    // Baby steps table: serialized point (65 bytes) -> scalar j
    // We use string as key since secp256k1_pubkey doesn't have std::hash
    std::unordered_map<std::string, uint64_t> babySteps;
    
    // Optional Bloom filter for memory-efficient lookup
    std::unique_ptr<BSGSBloomFilter> bloomFilter;
    
    // Generator point G
    secp256k1_pubkey G;
    
    // Target public key Q (single target mode)
    secp256k1_pubkey targetPub;
    ECPoint targetPoint;
    bool targetLoaded;
    
    // Multi-target mode: multiple public keys to search
    struct TargetInfo {
        secp256k1_pubkey pubKey;
        std::string pubKeyHex;
        bool found;
        uint256_t foundKey;
    };
    std::vector<TargetInfo> multiTargets;
    bool multiTargetMode;
    
    // Bloom filter address checking (optional)
    BSGSBloomIntegration bloomIntegration;
    
    // GPU acceleration (optional)
    BSGSGPUIntegration gpuIntegration;
    
    // Statistics tracking
    struct BSGSStats {
        std::atomic<uint64_t> giantStepsDone{0};
        std::atomic<uint64_t> babyStepsHits{0};
        std::atomic<uint64_t> bloomFilterHits{0};
        std::atomic<uint64_t> falsePositives{0};
        std::chrono::steady_clock::time_point startTime;
        
        void reset() {
            giantStepsDone = 0;
            babyStepsHits = 0;
            bloomFilterHits = 0;
            falsePositives = 0;
            startTime = std::chrono::steady_clock::now();
        }
        
        double getElapsedSeconds() const {
            return std::chrono::duration<double>(
                std::chrono::steady_clock::now() - startTime).count();
        }
        
        double getGiantStepsPerSecond() const {
            double elapsed = getElapsedSeconds();
            return elapsed > 0 ? giantStepsDone.load() / elapsed : 0;
        }
    } stats;
    
public:
    BSGSMode(const ProgramConfig& config, std::atomic<bool>& runFlag) 
        : cfg(config), running(runFlag), targetLoaded(false), multiTargetMode(false) {
        ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
        
        // Initialize generator point G
        unsigned char genPriv[32] = {0};
        genPriv[31] = 1; // Private key = 1
        secp256k1_ec_pubkey_create(ctx, &G, genPriv);
        
        // Initialize Bloom filter address checking if files provided
        if (!cfg.bsgsBloomKeysFiles.empty()) {
            bloomIntegration.initialize(cfg.bsgsBloomKeysFiles);
        }
    }
    
    ~BSGSMode() {
        secp256k1_context_destroy(ctx);
    }
    
    // Parse public key from hex string
    bool parsePublicKey(const std::string& pubHex) {
        std::vector<uint8_t> bytes;
        
        // Check if it's compressed (33 bytes) or uncompressed (65 bytes)
        if (pubHex.length() == 66) {
            // Compressed format: 02 or 03 + 32 bytes X
            for (size_t i = 0; i < 66; i += 2) {
                std::string byteString = pubHex.substr(i, 2);
                bytes.push_back((uint8_t)strtol(byteString.c_str(), NULL, 16));
            }
        } else if (pubHex.length() == 130) {
            // Uncompressed format: 04 + 32 bytes X + 32 bytes Y
            for (size_t i = 0; i < 130; i += 2) {
                std::string byteString = pubHex.substr(i, 2);
                bytes.push_back((uint8_t)strtol(byteString.c_str(), NULL, 16));
            }
        } else {
            std::cerr << "[BSGS ERROR] Invalid public key length: " << pubHex.length() << "\n";
            std::cerr << "  Expected 66 chars (compressed) or 130 chars (uncompressed)\n";
            return false;
        }
        
        // Parse with secp256k1
        if (!secp256k1_ec_pubkey_parse(ctx, &targetPub, bytes.data(), bytes.size())) {
            std::cerr << "[BSGS ERROR] Failed to parse public key\n";
            return false;
        }
        
        // Store target point
        memset(&targetPoint, 0, sizeof(targetPoint));
        
        // Serialize to get coordinates
        size_t len = 65;
        unsigned char serialized[65];
        secp256k1_ec_pubkey_serialize(ctx, serialized, &len, &targetPub, SECP256K1_EC_UNCOMPRESSED);
        
        // serialized[0] = 0x04, then 32 bytes X, then 32 bytes Y
        memcpy(targetPoint.x, serialized + 1, 32);
        memcpy(targetPoint.y, serialized + 33, 32);
        targetPoint.infinity = false;
        
        targetLoaded = true;
        return true;
    }
    
    // Load multiple targets from file (one per line)
    bool loadTargetsFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "[BSGS ERROR] Cannot open targets file: " << filename << "\n";
            return false;
        }
        
        multiTargets.clear();
        std::string line;
        int lineNum = 0;
        
        std::cout << "[BSGS] Loading targets from " << filename << "...\n";
        
        while (std::getline(file, line) && running) {
            lineNum++;
            
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;
            
            // Remove whitespace
            line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
            
            if (line.length() != 66 && line.length() != 130) {
                std::cerr << "[BSGS WARNING] Invalid public key on line " << lineNum 
                          << ": " << line.substr(0, 20) << "... (expected 66 or 130 hex chars)\n";
                continue;
            }
            
            // Parse the public key
            std::vector<uint8_t> bytes;
            for (size_t i = 0; i < line.length(); i += 2) {
                std::string byteString = line.substr(i, 2);
                bytes.push_back((uint8_t)strtol(byteString.c_str(), NULL, 16));
            }
            
            secp256k1_pubkey pub;
            if (!secp256k1_ec_pubkey_parse(ctx, &pub, bytes.data(), bytes.size())) {
                std::cerr << "[BSGS WARNING] Failed to parse public key on line " << lineNum << "\n";
                continue;
            }
            
            TargetInfo info;
            info.pubKey = pub;
            info.pubKeyHex = line;
            info.found = false;
            multiTargets.push_back(info);
        }
        
        file.close();
        
        if (multiTargets.empty()) {
            std::cerr << "[BSGS ERROR] No valid public keys found in " << filename << "\n";
            return false;
        }
        
        multiTargetMode = true;
        targetLoaded = true;
        
        std::cout << "[BSGS] Loaded " << multiTargets.size() << " target(s) from file\n";
        std::cout << "[BSGS] Baby steps will be computed once and reused for all targets\n";
        std::cout << "[BSGS] Linear speedup: " << multiTargets.size() << " targets = ~same time as 1 target\n\n";
        
        return true;
    }
    
    // Save found keys for all targets
    void saveMultiTargetResults() {
        std::ofstream outFile("bsgs_multi_target_results.txt", std::ios::app);
        if (!outFile.is_open()) {
            std::cerr << "[BSGS WARNING] Cannot save results to file\n";
            return;
        }
        
        outFile << "=== BSGS Multi-Target Search Results ===\n";
        outFile << "Timestamp: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << "\n";
        outFile << "Total targets: " << multiTargets.size() << "\n\n";
        
        int foundCount = 0;
        for (size_t i = 0; i < multiTargets.size(); i++) {
            outFile << "Target " << (i + 1) << ":\n";
            outFile << "  Public Key: " << multiTargets[i].pubKeyHex << "\n";
            if (multiTargets[i].found) {
                outFile << "  Status: FOUND\n";
                outFile << "  Private Key: " << multiTargets[i].foundKey << "\n";
                foundCount++;
            } else {
                outFile << "  Status: NOT FOUND\n";
            }
            outFile << "\n";
        }
        
        outFile << "Summary: " << foundCount << "/" << multiTargets.size() << " keys found\n";
        outFile << "========================================\n\n";
        
        std::cout << "\n[BSGS] Results saved to bsgs_multi_target_results.txt\n";
        std::cout << "[BSGS] Found: " << foundCount << "/" << multiTargets.size() << " keys\n";
    }
    
    // Serialize pubkey to string for use as hash key
    std::string serializePubkey(const secp256k1_pubkey& pub) {
        size_t len = 65;
        unsigned char serialized[65];
        secp256k1_ec_pubkey_serialize(ctx, serialized, &len, &pub, SECP256K1_EC_UNCOMPRESSED);
        return std::string((char*)serialized, 65);
    }
    
    // Generate filename for baby steps cache
    std::string getBabyStepsFilename(uint64_t m) {
        // If user specified a custom cache file, use it
        if (!cfg.bsgsCacheFile.empty()) {
            return cfg.bsgsCacheFile;
        }
        return "bsgs_baby_steps_m" + std::to_string(m) + ".cache";
    }
    
    // Get m value - use user-specified if available, otherwise calculate from memory
    uint64_t getBabyStepsM() {
        if (cfg.bsgsM > 0) {
            std::cout << "[BSGS] Using user-specified m = " << cfg.bsgsM << "\n";
            return cfg.bsgsM;
        }
        
        // Calculate based on available memory
        uint64_t availableMemMB = getAvailableMemoryMB();
        if (availableMemMB == 0) availableMemMB = 4096;
        
        uint64_t usableMemMB = availableMemMB * 0.5;
        uint64_t m = (usableMemMB * 1024 * 1024) / 80;
        
        // Cap at 2^32
        if (m > 0x100000000ULL) m = 0x100000000ULL;
        
        return m;
    }
    
    // Save baby steps to disk
    bool saveBabySteps(uint64_t m) {
        std::string filename = getBabyStepsFilename(m);
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[BSGS WARNING] Failed to open " << filename << " for writing\n";
            return false;
        }
        
        // Write header: magic (4 bytes) + version (4 bytes) + m (8 bytes) + count (8 bytes)
        const uint32_t magic = 0x42534753; // "BSGS"
        const uint32_t version = 1;
        uint64_t count = babySteps.size();
        
        file.write((char*)&magic, sizeof(magic));
        file.write((char*)&version, sizeof(version));
        file.write((char*)&m, sizeof(m));
        file.write((char*)&count, sizeof(count));
        
        // Write each entry: 65 bytes key + 8 bytes value
        uint64_t written = 0;
        for (const auto& entry : babySteps) {
            file.write(entry.first.c_str(), 65); // Public key (65 bytes)
            file.write((char*)&entry.second, sizeof(uint64_t)); // j value
            written++;
            
            // Progress every 1M entries
            if (written % 1000000 == 0) {
                std::cout << "\r[BSGS] Saving baby steps: " << written << "/" << count 
                          << std::flush;
            }
        }
        
        std::cout << "\r[BSGS] Baby steps saved to " << filename 
                  << " (" << written << " entries)\n";
        return true;
    }
    
    // Load baby steps from disk
    bool loadBabySteps(uint64_t m) {
        std::string filename = getBabyStepsFilename(m);
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return false; // File doesn't exist
        }
        
        // Read header
        uint32_t magic, version;
        uint64_t fileM, count;
        
        file.read((char*)&magic, sizeof(magic));
        file.read((char*)&version, sizeof(version));
        file.read((char*)&fileM, sizeof(fileM));
        file.read((char*)&count, sizeof(count));
        
        // Verify header
        if (magic != 0x42534753 || version != 1 || fileM != m) {
            std::cerr << "[BSGS WARNING] Invalid baby steps cache file: " << filename << "\n";
            return false;
        }
        
        // Check if bloom filter should be used
        if (cfg.bsgsUseBloom) {
            std::cout << "[BSGS] Bloom filter enabled for memory-efficient lookup\n";
        }
        
        std::cout << "[BSGS] Loading baby steps from " << filename 
                  << " (" << count << " entries)...\n";
        
        babySteps.clear();
        babySteps.reserve(count * 1.1); // Some extra space
        
        // Read entries
        char keyBuf[65];
        uint64_t value;
        uint64_t loaded = 0;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (uint64_t i = 0; i < count && file.good(); i++) {
            file.read(keyBuf, 65);
            file.read((char*)&value, sizeof(value));
            
            babySteps[std::string(keyBuf, 65)] = value;
            loaded++;
            
            // Progress every 1M entries
            if (loaded % 1000000 == 0) {
                std::cout << "\r[BSGS] Loading baby steps: " << loaded << "/" << count 
                          << std::flush;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(end - start).count();
        
        std::cout << "\r[BSGS] Baby steps loaded: " << loaded << " entries in " 
                  << std::fixed << std::setprecision(2) << elapsed << "s\n";
        
        return loaded == count;
    }
    
    // ============================================================================
    // NEW ADVANCED FEATURES
    // ============================================================================
    
    // Calculate cluster node range for distributed computing
    // Node 0 handles [0, m/M), Node 1 handles [m/M, 2m/M), etc.
    std::pair<uint64_t, uint64_t> getClusterRange(uint64_t m) {
        int nodeId = cfg.bsgsNodeId;
        int totalNodes = cfg.bsgsTotalNodes;
        
        if (totalNodes <= 1) {
            return {0, m}; // No clustering, use full range
        }
        
        if (nodeId < 0 || nodeId >= totalNodes) {
            std::cerr << "[BSGS WARNING] Invalid node ID " << nodeId 
                      << ", using node 0\n";
            nodeId = 0;
        }
        
        uint64_t rangeSize = m / totalNodes;
        uint64_t startI = rangeSize * nodeId;
        uint64_t endI = (nodeId == totalNodes - 1) ? m : rangeSize * (nodeId + 1);
        
        std::cout << "[BSGS CLUSTER] Node " << nodeId << " of " << totalNodes << "\n";
        std::cout << "[BSGS CLUSTER] Handling giant steps range: [" 
                  << startI << ", " << endI << ")\n";
        std::cout << "[BSGS CLUSTER] Total work per node: " << (endI - startI) 
                  << " giant steps\n\n";
        
        return {startI, endI};
    }
    
    // Initialize Bloom filter for memory-efficient lookup
    void initBloomFilter(uint64_t m) {
        if (!cfg.bsgsUseBloom) {
            return;
        }
        
        // Bloom filter uses ~10x less memory than hash table
        // With 1% false positive rate
        size_t expectedElements = m;
        bloomFilter.reset(new BSGSBloomFilter(expectedElements, 0.01));
        
        std::cout << "[BSGS BLOOM] Initializing Bloom filter for " << m 
                  << " elements\n";
        
        // Add all baby steps to bloom filter
        for (const auto& entry : babySteps) {
            bloomFilter->add(entry.first);
        }
        
        size_t bloomMem = bloomFilter->getMemoryUsage();
        size_t hashMem = babySteps.size() * 80; // Approximate
        
        std::cout << "[BSGS BLOOM] Bloom filter memory: " << (bloomMem / (1024*1024)) 
                  << " MB (vs " << (hashMem / (1024*1024)) << " MB for hash table)\n";
        std::cout << "[BSGS BLOOM] False positive rate: " 
                  << std::fixed << std::setprecision(4) 
                  << (bloomFilter->getFalsePositiveRate() * 100) << "%\n\n";
    }
    
    // Check if point might be in baby steps (using bloom filter if enabled)
    bool possiblyInBabySteps(const std::string& key) {
        if (!cfg.bsgsUseBloom || !bloomFilter) {
            return true; // No bloom filter, check hash table directly
        }
        
        // First check bloom filter
        bool possiblyInBloom = bloomFilter->possiblyContains(key);
        if (!possiblyInBloom) {
            return false; // Definitely not in set
        }
        
        // Might be in set (or false positive) - check hash table
        return true;
    }
    
    // Print detailed statistics
    void printStats(uint64_t totalGiantSteps, double elapsedSeconds) {
        double rate = elapsedSeconds > 0 ? totalGiantSteps / elapsedSeconds : 0;
        double progress = 100.0 * totalGiantSteps / (stats.giantStepsDone.load() + 1);
        
        // Estimate probability of finding key
        // P = coverage * (segment_size / total_range)
        double coverage = (double)totalGiantSteps / (double)(stats.giantStepsDone.load() + 1);
        
        std::cout << "\n[BSGS STATS] ========== Progress Report ==========\n";
        std::cout << "[BSGS STATS] Giant steps completed: " << totalGiantSteps << "\n";
        std::cout << "[BSGS STATS] Rate: " << std::fixed << std::setprecision(1) 
                  << rate << " steps/sec\n";
        std::cout << "[BSGS STATS] Elapsed: " << (int)(elapsedSeconds / 3600) << "h "
                  << ((int)(elapsedSeconds) % 3600) / 60 << "m "
                  << (int)(elapsedSeconds) % 60 << "s\n";
        
        if (cfg.bsgsUseBloom) {
            std::cout << "[BSGS STATS] Bloom filter hits: " << stats.bloomFilterHits.load() << "\n";
            std::cout << "[BSGS STATS] False positives: " << stats.falsePositives.load() << "\n";
        }
        
        std::cout << "[BSGS STATS] Baby steps hits: " << stats.babyStepsHits.load() << "\n";
        std::cout << "[BSGS STATS] =====================================\n\n";
    }
    
    // Check if this is a distinguished point (for memory-efficient giant steps)
    bool isDistinguishedPoint(const secp256k1_pubkey& p, int bits) {
        if (bits <= 0) return true; // All points are distinguished if bits=0
        
        size_t len = 33;
        unsigned char serialized[33];
        secp256k1_ec_pubkey_serialize(ctx, serialized, &len, &p, SECP256K1_EC_COMPRESSED);
        
        // Check if first 'bits' bits are zero
        int fullBytes = bits / 8;
        int remainingBits = bits % 8;
        
        for (int i = 0; i < fullBytes; i++) {
            if (serialized[i + 1] != 0) return false; // Skip first byte (compression indicator)
        }
        
        if (remainingBits > 0) {
            unsigned char mask = (0xFF << (8 - remainingBits)) & 0xFF;
            if ((serialized[fullBytes + 1] & mask) != 0) return false;
        }
        
        return true;
    }
    
    // ============================================================================
    // END NEW ADVANCED FEATURES
    // ============================================================================
    
    // Compute baby steps: j * G for j = 0, 1, ..., m-1
    void computeBabySteps(uint64_t m) {
        // First try to load from cache
        if (loadBabySteps(m)) {
            std::cout << "[BSGS] Using cached baby steps from disk.\n";
            return;
        }
        
        std::cout << "[BSGS] Computing baby steps (m = " << m << ")...\n";
        std::cout << "[BSGS] This may take a while. Baby steps will be saved to disk for future use.\n";
        
        babySteps.clear();
        babySteps.reserve(m * 2); // Preallocate for performance
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (uint64_t j = 0; j < m && running; j++) {
            secp256k1_pubkey pub;
            
            if (j == 0) {
                // j=0 is the point at infinity (identity element)
                // Serialize as all zeros with prefix
                unsigned char infSerialized[65] = {0};
                infSerialized[0] = 0x04; // Uncompressed prefix
                // X=0, Y=0 represents point at infinity
                std::string key((char*)infSerialized, 65);
                babySteps[key] = 0;
                continue;
            }
            
            // Compute j*G directly using secp256k1
            unsigned char priv[32] = {0};
            
            // Convert j to 32-byte big-endian format
            uint64_t temp = j;
            for (int i = 31; i >= 24; i--) {
                priv[i] = temp & 0xFF;
                temp >>= 8;
            }
            
            // Compute public key for this private key
            if (!secp256k1_ec_pubkey_create(ctx, &pub, priv)) {
                std::cerr << "\n[BSGS ERROR] Failed to create pubkey for j=" << j << "\n";
                continue;
            }
            
            // Store serialized point -> j mapping
            std::string key = serializePubkey(pub);
            babySteps[key] = j;
            
            // Progress update every 0.1% for better visibility
            uint64_t updateInterval = m / 1000 + 1; // 0.1% intervals
            if (j % updateInterval == 0) {
                double progress = (100.0 * j) / m;
                auto now = std::chrono::high_resolution_clock::now();
                double elapsed = std::chrono::duration<double>(now - start).count();
                double eta = (elapsed / (j + 1)) * (m - j);
                
                // Format ETA
                std::string etaStr;
                if (eta > 86400) {
                    etaStr = std::to_string((int)(eta / 86400)) + "d " + 
                             std::to_string(((int)eta % 86400) / 3600) + "h";
                } else if (eta > 3600) {
                    etaStr = std::to_string((int)(eta / 3600)) + "h " +
                             std::to_string(((int)eta % 3600) / 60) + "m";
                } else {
                    etaStr = std::to_string((int)eta / 60) + "m " +
                             std::to_string((int)eta % 60) + "s";
                }
                
                // Memory usage estimate
                size_t memUsed = babySteps.size() * 80; // Approximate bytes per entry
                std::string memStr;
                if (memUsed > 1024 * 1024 * 1024) {
                    memStr = std::to_string(memUsed / (1024 * 1024 * 1024)) + " GB";
                } else if (memUsed > 1024 * 1024) {
                    memStr = std::to_string(memUsed / (1024 * 1024)) + " MB";
                } else {
                    memStr = std::to_string(memUsed / 1024) + " KB";
                }
                
                std::cout << "\r[BSGS] Baby steps: " << std::fixed << std::setprecision(2) 
                          << progress << "% | " << j << "/" << m 
                          << " | Mem: " << memStr
                          << " | ETA: " << etaStr
                          << "          " << std::flush; // Extra spaces to clear previous
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(end - start).count();
        
        // Final memory usage
        size_t memUsed = babySteps.size() * 80;
        std::string memStr;
        if (memUsed > 1024 * 1024 * 1024) {
            memStr = std::to_string(memUsed / (1024 * 1024 * 1024)) + "." + 
                     std::to_string((memUsed % (1024 * 1024 * 1024)) / (1024 * 1024 * 100)) + " GB";
        } else {
            memStr = std::to_string(memUsed / (1024 * 1024)) + "." +
                     std::to_string((memUsed % (1024 * 1024)) / (1024 * 100)) + " MB";
        }
        
        std::cout << "\r[BSGS] Baby steps completed: " << babySteps.size() << " points | "
                  << memStr << " | " 
                  << std::fixed << std::setprecision(2) << elapsed << "s (" 
                  << (babySteps.size() / elapsed) << " pts/s)                    \n";
        
        // Save to disk for future use
        saveBabySteps(m);
    }
    
    // Compute k*G for a given scalar k (uint64_t version)
    bool computePubKey(uint64_t k, secp256k1_pubkey& outPub) {
        unsigned char priv[32] = {0};
        uint64_t temp = k;
        for (int i = 31; i >= 24; i--) {
            priv[i] = temp & 0xFF;
            temp >>= 8;
        }
        return secp256k1_ec_pubkey_create(ctx, &outPub, priv);
    }
    
    // Compute k*G for a given scalar k (uint256_t version for large values)
    bool computePubKey(const uint256_t& k, secp256k1_pubkey& outPub) {
        unsigned char priv[32] = {0};
        uint256_t temp = k;
        for (int i = 31; i >= 0 && temp > 0; i--) {
            priv[i] = (unsigned char)(temp & 0xFF);
            temp >>= 8;
        }
        return secp256k1_ec_pubkey_create(ctx, &outPub, priv);
    }
    
    // Negate a point (P -> -P by negating Y coordinate)
    bool negatePoint(const secp256k1_pubkey& in, secp256k1_pubkey& out) {
        // Serialize to get coordinates
        size_t len = 65;
        unsigned char serialized[65];
        secp256k1_ec_pubkey_serialize(ctx, serialized, &len, &in, SECP256K1_EC_UNCOMPRESSED);
        
        // serialized[0] = 0x04, X = [1:33), Y = [33:65)
        // To negate: Y' = p - Y where p is the field prime
        // For secp256k1, p = FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F
        // We can just negate modulo p
        
        unsigned char negSerialized[65];
        negSerialized[0] = 0x04;
        memcpy(negSerialized + 1, serialized + 1, 32); // Copy X
        
        // Negate Y: compute p - Y
        // p = 2^256 - 2^32 - 2^9 - 2^8 - 2^7 - 2^6 - 2^4 - 1
        // = FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F
        const unsigned char p[32] = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x2F
        };
        
        // Compute p - Y
        int borrow = 0;
        for (int i = 31; i >= 0; i--) {
            int diff = p[i] - serialized[33 + i] - borrow;
            if (diff < 0) {
                diff += 256;
                borrow = 1;
            } else {
                borrow = 0;
            }
            negSerialized[33 + i] = (unsigned char)diff;
        }
        
        return secp256k1_ec_pubkey_parse(ctx, &out, negSerialized, 65);
    }
    
    // Search using proper BSGS algorithm
    bool search(uint64_t maxKey, uint64_t startOffset = 0) {
        if (!targetLoaded) {
            std::cerr << "[BSGS ERROR] Target public key not loaded\n";
            return false;
        }
        
        // Calculate m = sqrt(maxKey)
        uint64_t m = (uint64_t)std::sqrt((double)maxKey) + 1;
        
        std::cout << "\n=== BSGS MODE (Baby Step Giant Step) ===\n";
        if (startOffset > 0) {
            std::cout << "Search range: " << startOffset << " to " << (startOffset + maxKey) 
                      << " (2^" << (int)std::log2(maxKey) << " + offset)\n";
        } else {
            std::cout << "Search range: 0 to " << maxKey << " (2^" << (int)std::log2(maxKey) << ")\n";
        }
        uint64_t memRequiredMB = (m * 80) / (1024 * 1024);
        
        std::cout << "Baby steps (m): " << m << "\n";
        std::cout << "Giant steps: " << m << "\n";
        std::cout << "Total complexity: O(" << m << ")\n";
        std::cout << "Memory required: ~" << memRequiredMB << " MB (" 
                  << std::fixed << std::setprecision(2) << (memRequiredMB / 1024.0) << " GB)\n\n";
        
        // Check available system memory
        uint64_t totalMemMB = getTotalMemoryMB();
        uint64_t availMemMB = getAvailableMemoryMB();
        
        if (totalMemMB > 0) {
            std::cout << "[BSGS] System memory: " << totalMemMB << " MB total, " 
                      << availMemMB << " MB available\n";
            
            // Need at least required memory + 20% buffer for OS and overhead
            uint64_t memNeededWithBuffer = memRequiredMB * 1.2;
            
            if (memNeededWithBuffer > totalMemMB) {
                std::cerr << "\n[BSGS ERROR] INSUFFICIENT MEMORY!\n";
                std::cerr << "  Required: " << memNeededWithBuffer << " MB (with 20% buffer)\n";
                std::cerr << "  Available: " << totalMemMB << " MB total\n\n";
                std::cerr << "  Options:\n";
                std::cerr << "  1. Reduce search range (e.g., --bsgs-range 48 for 2^48)\n";
                std::cerr << "  2. Use Pollard's Rho algorithm (better for large ranges)\n";
                std::cerr << "  3. Add more RAM or use swap (will be very slow)\n\n";
                return false;
            } else if (memNeededWithBuffer > availMemMB) {
                std::cerr << "[BSGS WARNING] Available memory (" << availMemMB 
                          << " MB) may be insufficient.\n";
                std::cerr << "  Required: " << memNeededWithBuffer << " MB\n";
                std::cerr << "  Close other applications or reduce range.\n\n";
            } else {
                std::cout << "[BSGS] Memory check: OK (sufficient memory available)\n\n";
            }
        } else {
            std::cerr << "[BSGS WARNING] Could not detect system memory.\n";
            std::cerr << "  Ensure you have at least " << memRequiredMB << " MB free.\n\n";
        }
        
        // Additional warning for very large m
        if (m > 50000000) { // 50 million
            std::cerr << "[BSGS WARNING] m is very large (" << m << "). This will use a lot of memory.\n";
        }
        
        // Step 1: Compute baby steps j*G for j in [0, m)
        computeBabySteps(m);
        
        if (!running) {
            std::cout << "\n[BSGS] Search cancelled.\n";
            return false;
        }
        
        // Step 2: Giant steps
        // We want to find k such that k*G = Q
        // Write k = i*m + j, where j < m
        // Then: k*G = i*m*G + j*G = Q
        // So: j*G = Q - i*m*G
        // 
        // For each i in [0, m), we compute Q - i*m*G and check if it's in baby steps
        // If Q - i*m*G = j*G, then k = i*m + j
        
        std::cout << "[BSGS] Starting giant steps search...\n";
        std::cout << "[BSGS] Press Ctrl+C to stop\n\n";
        
        auto start = std::chrono::high_resolution_clock::now();
        auto lastProgress = start;
        
        // Print initial status
        std::cout << "[BSGS] Giant steps: 0.0000% | 0/" << m << " | 0.0 steps/s | ETA: ..." << std::flush;
        
        std::atomic<bool> found(false);
        std::atomic<uint64_t> foundKey(0);
        std::atomic<uint64_t> stepsDone(0);
        
        int numThreads = cfg.bsgsThreads;
        if (numThreads <= 0) {
            numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0) numThreads = 4;
        }
        
        std::cout << "[BSGS] Using " << numThreads << " threads\n\n";
        
        // Precompute m*G for giant step increment
        secp256k1_pubkey mG;
        if (!computePubKey(m, mG)) {
            std::cerr << "[BSGS ERROR] Failed to compute m*G\n";
            return false;
        }
        
        // Split work among threads - each thread handles different i values
        std::vector<std::thread> threads;
        uint64_t chunkSize = m / numThreads + 1;
        
        for (int t = 0; t < numThreads; t++) {
            uint64_t startI = t * chunkSize;
            uint64_t endI = std::min((t + 1) * chunkSize, m);
            
            threads.emplace_back([&, startI, endI, startOffset, t]() {
                secp256k1_context* localCtx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
                
                // Compute starting point for this thread: i*m*G
                uint64_t startK = startI * m;
                secp256k1_pubkey giantPoint;
                if (!computePubKey(startK, giantPoint)) {
                    secp256k1_context_destroy(localCtx);
                    return;
                }
                
                for (uint64_t i = startI; i < endI && running && !found; i++) {
                    // Compute Q - giantPoint = Q + (-giantPoint)
                    secp256k1_pubkey negGiant;
                    if (!negatePoint(giantPoint, negGiant)) continue;
                    
                    // Add Q + (-giantPoint)
                    const secp256k1_pubkey* pts[2] = { &targetPub, &negGiant };
                    secp256k1_pubkey candidate;
                    if (!secp256k1_ec_pubkey_combine(localCtx, &candidate, pts, 2)) {
                        // Move to next giant step
                        if (i < endI - 1) {
                            const secp256k1_pubkey* addPts[2] = { &giantPoint, &mG };
                            secp256k1_pubkey nextGiant;
                            if (secp256k1_ec_pubkey_combine(localCtx, &nextGiant, addPts, 2)) {
                                giantPoint = nextGiant;
                            }
                        }
                        continue;
                    }
                    
                    // Check if candidate is in baby steps table
                    std::string candidateKey = serializePubkey(candidate);
                    auto it = babySteps.find(candidateKey);
                    if (it != babySteps.end()) {
                        uint64_t j = it->second;
                        uint64_t k = i * m + j + startOffset;  // Add startOffset
                        if (k < maxKey + startOffset) {
                            found = true;
                            foundKey = k;
                        }
                    }
                    
                    // Move to next giant step: giantPoint += m*G
                    if (i < endI - 1) {
                        const secp256k1_pubkey* addPts[2] = { &giantPoint, &mG };
                        secp256k1_pubkey nextGiant;
                        if (secp256k1_ec_pubkey_combine(localCtx, &nextGiant, addPts, 2)) {
                            giantPoint = nextGiant;
                        }
                    }
                    
                    stepsDone++;
                    
                    // Progress update - show progress from any thread periodically
                    // Update every 1% of total work or every 100ms
                    uint64_t totalSteps = stepsDone.load();
                    if (totalSteps % (m / 100 + 1) == 0 || (i % 100000 == 0 && t == 0)) {
                        auto now = std::chrono::high_resolution_clock::now();
                        double elapsed = std::chrono::duration<double>(now - start).count();
                        double progress = (100.0 * totalSteps) / m;
                        double rate = totalSteps / elapsed;
                        double eta = (elapsed / totalSteps) * (m - totalSteps);
                        
                        // Format ETA nicely
                        std::string etaStr;
                        if (eta > 86400) {
                            etaStr = std::to_string((int)(eta / 86400)) + "d " + 
                                     std::to_string(((int)eta % 86400) / 3600) + "h";
                        } else if (eta > 3600) {
                            etaStr = std::to_string((int)(eta / 3600)) + "h " +
                                     std::to_string(((int)eta % 3600) / 60) + "m";
                        } else {
                            etaStr = std::to_string((int)eta / 60) + "m " +
                                     std::to_string((int)eta % 60) + "s";
                        }
                        
                        std::cout << "\r[BSGS] Giant steps: " << std::fixed << std::setprecision(4) 
                                  << progress << "% | " << totalSteps << "/" << m 
                                  << " | " << std::setprecision(1) << rate << " steps/s"
                                  << " | ETA: " << etaStr << std::flush;
                    }
                }
                
                secp256k1_context_destroy(localCtx);
            });
        }
        
        // Wait for all threads
        for (auto& th : threads) {
            th.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(end - start).count();
        
        std::cout << "\n"; // End progress line
        
        if (found) {
            uint64_t k = foundKey.load();
            
            std::cout << "\n" << std::string(50, '=') << "\n";
            std::cout << "[BSGS] PRIVATE KEY FOUND!\n";
            std::cout << "Key: " << k << " (0x" << std::hex << k << std::dec << ")\n";
            std::cout << "Time: " << std::fixed << std::setprecision(2) << elapsed << " seconds\n";
            std::cout << std::string(50, '=') << "\n";
            
            // Save to file
            std::ofstream outFile("bsgs_found.txt", std::ios::app);
            if (outFile.is_open()) {
                outFile << "=== BSGS PRIVATE KEY FOUND ===\n";
                outFile << "Public Key: " << cfg.bsgsPub << "\n";
                outFile << "Private Key (decimal): " << k << "\n";
                outFile << "Private Key (hex): " << std::hex << k << std::dec << "\n";
                outFile << "Time: " << elapsed << " seconds\n";
                outFile << "==============================\n\n";
            }
            
            return true;
        }
        
        std::cout << "\n\n[BSGS] Key not found in range 0 to " << maxKey << "\n";
        std::cout << "Searched: " << m << " giant steps with " << babySteps.size() << " baby steps\n";
        std::cout << "Total time: " << std::fixed << std::setprecision(2) << elapsed << " seconds\n";
        std::cout << "Average rate: " << (stepsDone.load() / elapsed) << " steps/second\n";
        
        // Save checkpoint for resuming
        std::cout << "\n[BSGS] To continue search from where we left off:\n";
        std::cout << "  --bsgs-start " << maxKey << " --bsgs-range " << cfg.bsgsRange << "\n";
        return false;
    }
    
    // Multi-target search - checks all targets with same baby steps
    // Overload for uint64_t (ranges <= 64 bits)
    bool searchMultiTarget(uint64_t maxKey, uint64_t startOffset = 0) {
        return searchMultiTargetLarge(uint256_t(maxKey), uint256_t(startOffset));
    }
    
    // Multi-target search for large ranges (uint256_t)
    bool searchMultiTargetLarge(const uint256_t& maxKey, const uint256_t& startOffset) {
        if (multiTargets.empty()) {
            std::cerr << "[BSGS ERROR] No targets loaded\n";
            return false;
        }
        
        // Get m from config or calculate based on memory
        uint64_t m = getBabyStepsM();
        
        std::cout << "\n=== BSGS MULTI-TARGET MODE ===\n";
        std::cout << "Number of targets: " << multiTargets.size() << "\n";
        std::cout << "Search range: 0 to " << maxKey << "\n";
        std::cout << "Baby steps (m): " << m << "\n";
        std::cout << "Giant steps: " << m << "\n";
        std::cout << "Complexity per target: O(" << m << ")\n";
        std::cout << "Total work: O(" << m << ") - baby steps reused for all targets!\n\n";
        
        uint64_t memRequiredMB = (m * 80) / (1024 * 1024);
        std::cout << "Memory required: ~" << memRequiredMB << " MB\n\n";
        
        // Step 1: Compute baby steps once
        computeBabySteps(m);
        
        if (!running) {
            std::cout << "\n[BSGS] Search cancelled.\n";
            return false;
        }
        
        // Step 2: Giant steps - check against all targets
        std::cout << "[BSGS] Starting giant steps for " << multiTargets.size() << " targets...\n";
        std::cout << "[BSGS] Press Ctrl+C to stop\n";
        std::cout << "[BSGS] Progress will be reported every 1000 steps\n\n";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::atomic<uint64_t> stepsDone(0);
        std::atomic<int> foundCount(0);
        std::atomic<uint64_t> lastReportSteps(0);
        
        int numThreads = cfg.bsgsThreads;
        if (numThreads <= 0) {
            numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0) numThreads = 4;
        }
        
        // Precompute m*G for giant step increment
        secp256k1_pubkey mG;
        if (!computePubKey(m, mG)) {
            std::cerr << "[BSGS ERROR] Failed to compute m*G\n";
            return false;
        }
        
        std::vector<std::thread> threads;
        uint64_t chunkSize = m / numThreads + 1;
        
        for (int t = 0; t < numThreads; t++) {
            uint64_t startI = t * chunkSize;
            uint64_t endI = std::min((t + 1) * chunkSize, m);
            
            threads.emplace_back([&, startI, endI, startOffset]() {
                secp256k1_context* localCtx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
                
                uint64_t startK = startI * m;
                secp256k1_pubkey giantPoint;
                if (!computePubKey(startK, giantPoint)) {
                    secp256k1_context_destroy(localCtx);
                    return;
                }
                
                for (uint64_t i = startI; i < endI && running; i++) {
                    // For each target, check if Q_target - giantPoint is in baby steps
                    for (size_t targetIdx = 0; targetIdx < multiTargets.size(); targetIdx++) {
                        if (multiTargets[targetIdx].found) continue; // Skip already found
                        
                        // Compute Q - giantPoint
                        secp256k1_pubkey negGiant;
                        if (!negatePoint(giantPoint, negGiant)) continue;
                        
                        const secp256k1_pubkey* pts[2] = { &multiTargets[targetIdx].pubKey, &negGiant };
                        secp256k1_pubkey candidate;
                        if (!secp256k1_ec_pubkey_combine(localCtx, &candidate, pts, 2)) {
                            continue;
                        }
                        
                        // Check baby steps
                        std::string candidateKey = serializePubkey(candidate);
                        auto it = babySteps.find(candidateKey);
                        if (it != babySteps.end()) {
                            uint64_t j = it->second;
                            uint256_t k = uint256_t(i) * uint256_t(m) + uint256_t(j) + startOffset;
                            
                            multiTargets[targetIdx].found = true;
                            multiTargets[targetIdx].foundKey = k;
                            foundCount++;
                            
                            std::cout << "\n[BSGS] TARGET " << (targetIdx + 1) << " FOUND!\n";
                            std::cout << "Public Key: " << multiTargets[targetIdx].pubKeyHex.substr(0, 30) << "...\n";
                            std::cout << "Private Key: " << k << "\n\n";
                            
                            // Also check against bloom filter addresses if enabled
                            if (bloomIntegration.isActive()) {
                                secp256k1_pubkey foundPubKey;
                                if (computePubKey(k, foundPubKey)) {
                                    bloomIntegration.checkAndReport(foundPubKey, k);
                                }
                            }
                        }
                    }
                    
                    // Move to next giant step
                    if (i < endI - 1) {
                        const secp256k1_pubkey* addPts[2] = { &giantPoint, &mG };
                        secp256k1_pubkey nextGiant;
                        if (secp256k1_ec_pubkey_combine(localCtx, &nextGiant, addPts, 2)) {
                            giantPoint = nextGiant;
                        }
                    }
                    
                    stepsDone++;
                    
                    // Progress reporting - every 1000 steps OR every 5 seconds
                    if (t == 0) {
                        uint64_t currentSteps = stepsDone.load();
                        uint64_t lastReport = lastReportSteps.load();
                        
                        if ((currentSteps - lastReport) >= 1000) {
                            auto now = std::chrono::high_resolution_clock::now();
                            double elapsed = std::chrono::duration<double>(now - start).count();
                            uint64_t currentFound = foundCount.load();
                            
                            if (currentSteps > 0 && elapsed > 0) {
                                double progress = (100.0 * currentSteps) / m;
                                double avgRate = currentSteps / elapsed;
                                double eta = (m - currentSteps) / avgRate;
                                
                                // Format ETA
                                std::string etaStr;
                                if (eta > 86400) {
                                    etaStr = std::to_string((int)(eta / 86400)) + "d " + 
                                            std::to_string(((int)(eta) % 86400) / 3600) + "h";
                                } else if (eta > 3600) {
                                    etaStr = std::to_string((int)(eta / 3600)) + "h " +
                                            std::to_string(((int)(eta) % 3600) / 60) + "m";
                                } else if (eta > 60) {
                                    etaStr = std::to_string((int)(eta / 60)) + "m " +
                                            std::to_string((int)eta % 60) + "s";
                                } else {
                                    etaStr = std::to_string((int)eta) + "s";
                                }
                                
                                // Format hash rate with appropriate unit
                                std::string rateStr;
                                if (avgRate >= 1e21) {
                                    rateStr = std::to_string((int)(avgRate / 1e21)) + " ZH/s";
                                } else if (avgRate >= 1e18) {
                                    rateStr = std::to_string((int)(avgRate / 1e18)) + " EH/s";
                                } else if (avgRate >= 1e15) {
                                    rateStr = std::to_string((int)(avgRate / 1e15)) + " PH/s";
                                } else if (avgRate >= 1e12) {
                                    rateStr = std::to_string((int)(avgRate / 1e12)) + " TH/s";
                                } else if (avgRate >= 1e9) {
                                    rateStr = std::to_string((int)(avgRate / 1e9)) + " GH/s";
                                } else if (avgRate >= 1e6) {
                                    rateStr = std::to_string((int)(avgRate / 1e6)) + " MH/s";
                                } else if (avgRate >= 1e3) {
                                    rateStr = std::to_string((int)(avgRate / 1e3)) + " kH/s";
                                } else {
                                    rateStr = std::to_string((int)avgRate) + " H/s";
                                }
                                
                                // Total keys checked = giant steps * number of targets
                                uint64_t totalKeysChecked = currentSteps * multiTargets.size();
                                double keyRate = totalKeysChecked / elapsed;
                                
                                // Format key rate with appropriate unit
                                std::string keyRateStr;
                                if (keyRate >= 1e21) {
                                    keyRateStr = std::to_string((int)(keyRate / 1e21)) + " ZH/s";
                                } else if (keyRate >= 1e18) {
                                    keyRateStr = std::to_string((int)(keyRate / 1e18)) + " EH/s";
                                } else if (keyRate >= 1e15) {
                                    keyRateStr = std::to_string((int)(keyRate / 1e15)) + " PH/s";
                                } else if (keyRate >= 1e12) {
                                    keyRateStr = std::to_string((int)(keyRate / 1e12)) + " TH/s";
                                } else if (keyRate >= 1e9) {
                                    keyRateStr = std::to_string((int)(keyRate / 1e9)) + " GH/s";
                                } else if (keyRate >= 1e6) {
                                    keyRateStr = std::to_string((int)(keyRate / 1e6)) + " MH/s";
                                } else if (keyRate >= 1e3) {
                                    keyRateStr = std::to_string((int)(keyRate / 1e3)) + " kH/s";
                                } else {
                                    keyRateStr = std::to_string((int)keyRate) + " H/s";
                                }
                                
                                std::cout << "[PROGRESS] " << std::fixed << std::setprecision(2) << progress << "%"
                                          << " | Giant Steps: " << currentSteps << "/" << m
                                          << " | Total Keys: " << totalKeysChecked
                                          << " | Speed: " << keyRateStr
                                          << " | Found: " << currentFound << "/" << multiTargets.size()
                                          << " | ETA: " << etaStr << "\n" << std::flush;
                                
                                lastReportSteps.store(currentSteps);
                            }
                        }
                    }
                }
                
                secp256k1_context_destroy(localCtx);
            });
        }
        
        for (auto& th : threads) {
            th.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(end - start).count();
        
        std::cout << "\n[BSGS] Multi-target search completed\n";
        std::cout << "Found: " << foundCount.load() << "/" << multiTargets.size() << " keys\n";
        std::cout << "Time: " << std::fixed << std::setprecision(2) << elapsed << " seconds\n";
        std::cout << "Speedup: " << multiTargets.size() << " targets in ~same time as 1 target!\n\n";
        
        return foundCount.load() > 0;
    }
    
    // Convert uint256_t to 32-byte big-endian array for secp256k1
    bool uint256ToBytes(const uint256_t& value, unsigned char* out) {
        memset(out, 0, 32);
        uint256_t temp = value;
        for (int i = 31; i >= 0 && temp > 0; i--) {
            out[i] = (unsigned char)(temp & 0xFF);
            temp >>= 8;
        }
        return true;
    }
    
    // Compute baby steps for large ranges using uint256_t
    void computeBabyStepsLarge(const uint256_t& m, uint64_t m64) {
        std::cout << "[BSGS] Computing baby steps (m = " << m64 << ")...\n";
        
        babySteps.clear();
        babySteps.reserve(m64 * 2);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (uint64_t j = 0; j < m64 && running; j++) {
            secp256k1_pubkey pub;
            
            if (j == 0) {
                unsigned char infSerialized[65] = {0};
                infSerialized[0] = 0x04;
                std::string key((char*)infSerialized, 65);
                babySteps[key] = 0;
                continue;
            }
            
            unsigned char priv[32] = {0};
            uint64_t temp = j;
            for (int i = 31; i >= 24; i--) {
                priv[i] = temp & 0xFF;
                temp >>= 8;
            }
            
            if (!secp256k1_ec_pubkey_create(ctx, &pub, priv)) {
                std::cerr << "\n[BSGS ERROR] Failed to create pubkey for j=" << j << "\n";
                continue;
            }
            
            std::string key = serializePubkey(pub);
            babySteps[key] = j;
            
            uint64_t updateInterval = m64 / 1000 + 1;
            if (j % updateInterval == 0) {
                double progress = (100.0 * j) / m64;
                auto now = std::chrono::high_resolution_clock::now();
                double elapsed = std::chrono::duration<double>(now - start).count();
                double eta = (elapsed / (j + 1)) * (m64 - j);
                
                std::string etaStr;
                if (eta > 86400) {
                    etaStr = std::to_string((int)(eta / 86400)) + "d " + 
                             std::to_string(((int)eta % 86400) / 3600) + "h";
                } else if (eta > 3600) {
                    etaStr = std::to_string((int)(eta / 3600)) + "h " +
                             std::to_string(((int)eta % 3600) / 60) + "m";
                } else {
                    etaStr = std::to_string((int)eta / 60) + "m " +
                             std::to_string((int)eta % 60) + "s";
                }
                
                size_t memUsed = babySteps.size() * 80;
                std::string memStr;
                if (memUsed > 1024 * 1024 * 1024) {
                    memStr = std::to_string(memUsed / (1024 * 1024 * 1024)) + " GB";
                } else if (memUsed > 1024 * 1024) {
                    memStr = std::to_string(memUsed / (1024 * 1024)) + " MB";
                } else {
                    memStr = std::to_string(memUsed / 1024) + " KB";
                }
                
                std::cout << "\r[BSGS] Baby steps: " << std::fixed << std::setprecision(2) 
                          << progress << "% | " << j << "/" << m64 
                          << " | Mem: " << memStr
                          << " | ETA: " << etaStr
                          << "          " << std::flush;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(end - start).count();
        
        size_t memUsed = babySteps.size() * 80;
        std::string memStr;
        if (memUsed > 1024 * 1024 * 1024) {
            memStr = std::to_string(memUsed / (1024 * 1024 * 1024)) + "." + 
                     std::to_string((memUsed % (1024 * 1024 * 1024)) / (1024 * 1024 * 100)) + " GB";
        } else {
            memStr = std::to_string(memUsed / (1024 * 1024)) + "." +
                     std::to_string((memUsed % (1024 * 1024)) / (1024 * 100)) + " MB";
        }
        
        std::cout << "\r[BSGS] Baby steps completed: " << babySteps.size() << " points | "
                  << memStr << " | " 
                  << std::fixed << std::setprecision(2) << elapsed << "s (" 
                  << (babySteps.size() / elapsed) << " pts/s)                    \n";
    }
    
    // Search for large ranges (> 64 bits) - segmented approach
    bool searchLargeRange(const uint256_t& maxKey, const uint256_t& startOffset) {
        if (!targetLoaded) {
            std::cerr << "[BSGS ERROR] Target public key not loaded\n";
            return false;
        }
        
        // For large ranges, we use a segmented approach
        // We can't store 2^67.5 baby steps, so we search in chunks
        
        std::cout << "\n=== BSGS MODE (Large Range) ===\n";
        std::cout << "Search range: 2^" << cfg.bsgsRange << "\n";
        if (startOffset > 0) {
            std::cout << "Starting from offset: " << startOffset << "\n";
        }
        
        // Get m value (user-specified or calculated from memory)
        uint64_t m = getBabyStepsM();
        
        // Calculate segment size
        uint256_t segmentSize;
        if (cfg.bsgsSegmentBits > 0) {
            segmentSize = uint256_t(1) << cfg.bsgsSegmentBits;
            std::cout << "Baby steps (m): " << m << " (~" << (m * 80 / (1024*1024)) << " MB)\n";
            std::cout << "[BSGS WARNING] Large range search uses segmented BSGS.\n";
            std::cout << "               Each segment covers 2^" << cfg.bsgsSegmentBits << " keys (user-specified).\n\n";
        } else {
            segmentSize = uint256_t(m) * uint256_t(m);
            std::cout << "Baby steps (m): " << m << " (~" << (m * 80 / (1024*1024)) << " MB)\n";
            std::cout << "[BSGS WARNING] Large range search uses segmented BSGS.\n";
            std::cout << "               Each segment covers 2^" << (int)(std::log2(m) * 2) << " keys.\n\n";
        }
        
        // Calculate number of segments needed
        uint256_t totalRange = maxKey;
        uint256_t numSegments = (totalRange + segmentSize - 1) / segmentSize;
        
        std::cout << "Total segments to search: " << numSegments << "\n";
        std::cout << "Keys per segment: " << segmentSize << "\n\n";
        
        // Search each segment
        uint256_t currentOffset = startOffset;
        uint64_t segmentNum = 0;
        
        while (currentOffset < maxKey && running) {
            segmentNum++;
            std::cout << "\n[BSGS] Searching segment " << segmentNum << "/" << numSegments << "\n";
            std::cout << "       Range: " << currentOffset << " to " << (currentOffset + segmentSize) << "\n";
            
            // Compute baby steps for this segment
            computeBabySteps(m);
            
            if (!running) break;
            
            // Search this segment
            bool found = searchSegment(m, currentOffset);
            
            if (found) return true;
            
            currentOffset += segmentSize;
            
            // Clear baby steps for next segment
            babySteps.clear();
        }
        
        std::cout << "\n\n[BSGS] Key not found in range 0 to " << maxKey << "\n";
        std::cout << "Searched " << segmentNum << " segments.\n";
        
        return false;
    }
    
    // Generate a random uint256_t in range [0, max)
    uint256_t randomUint256(const uint256_t& max) {
        // Use multiple 64-bit random values to construct a uint256_t
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        static std::uniform_int_distribution<uint64_t> dis(0, std::numeric_limits<uint64_t>::max());
        
        // Generate 4 random 64-bit values
        uint256_t result = 0;
        for (int i = 0; i < 4; i++) {
            result <<= 64;
            result |= uint256_t(dis(gen));
        }
        
        // Modulo to ensure it's within range
        if (max > 0) {
            result = result % max;
        }
        
        return result;
    }
    
    // Search a single segment at a specific random offset
    bool searchSegment(uint64_t m, const uint256_t& segmentOffset) {
        auto start = std::chrono::high_resolution_clock::now();
        
        std::atomic<bool> found(false);
        uint256_t foundKey = 0;  // Non-atomic, protected by found flag
        std::atomic<uint64_t> stepsDone(0);
        
        int numThreads = cfg.bsgsThreads;
        if (numThreads <= 0) {
            numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0) numThreads = 4;
        }
        
        // Precompute m*G
        secp256k1_pubkey mG;
        if (!computePubKey(m, mG)) {
            std::cerr << "[BSGS ERROR] Failed to compute m*G\n";
            return false;
        }
        
        std::vector<std::thread> threads;
        uint64_t chunkSize = m / numThreads + 1;
        
        for (int t = 0; t < numThreads; t++) {
            uint64_t startI = t * chunkSize;
            uint64_t endI = std::min((t + 1) * chunkSize, m);
            
            threads.emplace_back([&, startI, endI]() {
                secp256k1_context* localCtx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
                
                uint64_t startK = startI * m;
                secp256k1_pubkey giantPoint;
                if (!computePubKey(startK, giantPoint)) {
                    secp256k1_context_destroy(localCtx);
                    return;
                }
                
                for (uint64_t i = startI; i < endI && running && !found; i++) {
                    secp256k1_pubkey negGiant;
                    if (!negatePoint(giantPoint, negGiant)) continue;
                    
                    const secp256k1_pubkey* pts[2] = { &targetPub, &negGiant };
                    secp256k1_pubkey candidate;
                    if (!secp256k1_ec_pubkey_combine(localCtx, &candidate, pts, 2)) {
                        if (i < endI - 1) {
                            const secp256k1_pubkey* addPts[2] = { &giantPoint, &mG };
                            secp256k1_pubkey nextGiant;
                            if (secp256k1_ec_pubkey_combine(localCtx, &nextGiant, addPts, 2)) {
                                giantPoint = nextGiant;
                            }
                        }
                        continue;
                    }
                    
                    std::string candidateKey = serializePubkey(candidate);
                    auto it = babySteps.find(candidateKey);
                    if (it != babySteps.end()) {
                        uint64_t j = it->second;
                        uint256_t k = uint256_t(i) * uint256_t(m) + uint256_t(j) + segmentOffset;
                        foundKey = k;
                        found.store(true, std::memory_order_release);
                    }
                    
                    if (i < endI - 1) {
                        const secp256k1_pubkey* addPts[2] = { &giantPoint, &mG };
                        secp256k1_pubkey nextGiant;
                        if (secp256k1_ec_pubkey_combine(localCtx, &nextGiant, addPts, 2)) {
                            giantPoint = nextGiant;
                        }
                    }
                    
                    stepsDone++;
                }
                
                secp256k1_context_destroy(localCtx);
            });
        }
        
        for (auto& th : threads) {
            th.join();
        }
        
        if (found.load()) {
            uint256_t k = foundKey;
            std::cout << "\n" << std::string(50, '=') << "\n";
            std::cout << "[BSGS] PRIVATE KEY FOUND!\n";
            std::cout << "Key: " << k << "\n";
            std::cout << std::string(50, '=') << "\n";
            
            std::ofstream outFile("bsgs_found.txt", std::ios::app);
            if (outFile.is_open()) {
                outFile << "=== BSGS PRIVATE KEY FOUND ===\n";
                outFile << "Public Key: " << cfg.bsgsPub << "\n";
                outFile << "Private Key: " << k << "\n";
                outFile << "==============================\n\n";
            }
            
            return true;
        }
        
        return false;
    }
    
    // Search using BSGS with random segment selection across the entire range
    // This uses proper Baby Steps + Giant Steps but selects random segments in the full range
    bool searchRandomRange(const uint256_t& maxKey, const uint256_t& startOffset) {
        if (!targetLoaded) {
            std::cerr << "[BSGS ERROR] Target public key not loaded\n";
            return false;
        }
        
        std::cout << "\n=== BSGS MODE (Random Segments with Baby Step Giant Step) ===\n";
        std::cout << "Search range: 2^" << cfg.bsgsRange << " = " << maxKey << "\n";
        std::cout << "Mode: RANDOM BSGS - Using Baby Steps + Giant Steps in random segments\n";
        std::cout << "      This covers the full 2^" << cfg.bsgsRange << " range using BSGS algorithm.\n\n";
        
        if (startOffset > 0) {
            std::cout << "Minimum offset: " << startOffset << "\n";
        }
        
        // Get m value (user-specified or calculated from memory)
        uint64_t m = getBabyStepsM();
        
        // Calculate segment size
        uint256_t segmentSize;
        if (cfg.bsgsSegmentBits > 0) {
            // User-specified segment size: 2^bsgsSegmentBits
            segmentSize = uint256_t(1) << cfg.bsgsSegmentBits;
            std::cout << "Baby steps (m): " << m << " (~" << (m * 80 / (1024*1024)) << " MB)\n";
            std::cout << "BSGS segment size: 2^" << cfg.bsgsSegmentBits << " = " << segmentSize << " keys per segment\n";
            std::cout << "Total searchable range: 2^" << cfg.bsgsRange << "\n";
            std::cout << "[BSGS] Using user-specified segment size (larger = fewer segments)\n\n";
        } else {
            // Default: m*m (optimal for BSGS algorithm)
            segmentSize = uint256_t(m) * uint256_t(m);
            std::cout << "Baby steps (m): " << m << " (~" << (m * 80 / (1024*1024)) << " MB)\n";
            std::cout << "BSGS segment size: " << segmentSize << " (2^" << (int)(std::log2((double)m) * 2) << " keys per segment)\n";
            std::cout << "Total searchable range: 2^" << cfg.bsgsRange << "\n\n";
        }
        
        uint64_t segmentsSearched = 0;
        uint64_t maxAttempts = 1000000; // Safety limit
        uint64_t totalKeysChecked = 0;
        
        auto globalStart = std::chrono::high_resolution_clock::now();
        
        // Precompute baby steps once - they are the same for all segments!
        std::cout << "[BSGS] Precomputing baby steps (used for all segments)...\n";
        computeBabySteps(m);
        
        if (!running) {
            std::cout << "\n[BSGS] Search cancelled.\n";
            return false;
        }
        
        // Initialize GPU acceleration if requested
        if (cfg.bsgsUseGPU) {
            std::cout << "[BSGS-GPU] Initializing GPU acceleration...\n";
            if (gpuIntegration.initialize(cfg, m)) {
                std::cout << "[BSGS-GPU] Using " << gpuIntegration.getBackendName() << " backend\n";
                // Upload baby steps to GPU
                std::vector<std::pair<std::vector<uint8_t>, uint64_t>> babyStepsVec;
                for (const auto& entry : babySteps) {
                    std::vector<uint8_t> key(entry.first.begin(), entry.first.end());
                    babyStepsVec.push_back({key, entry.second});
                }
                gpuIntegration.uploadBabySteps(babyStepsVec);
            } else {
                std::cout << "[BSGS-GPU] GPU initialization failed, falling back to CPU\n";
            }
        }
        
        std::cout << "[BSGS] Baby steps ready. Starting random segment search...\n";
        std::cout << "[BSGS] Press Ctrl+C to stop\n\n";
        
        while (running && segmentsSearched < maxAttempts) {
            // Generate a random segment offset within the range
            // Ensure we don't exceed maxKey - segmentSize
            uint256_t randomOffset;
            if (maxKey > segmentSize) {
                randomOffset = randomUint256(maxKey - segmentSize);
            } else {
                randomOffset = 0;
            }
            
            // Ensure we don't go below startOffset
            if (randomOffset < startOffset) {
                randomOffset = startOffset;
            }
            
            // Round down to segment boundary (m*m alignment)
            uint256_t segmentIndex = randomOffset / segmentSize;
            uint256_t segmentOffset = segmentIndex * segmentSize;
            
            // Ensure segment is within bounds
            if (segmentOffset >= maxKey) {
                segmentOffset = maxKey - segmentSize;
            }
            if (segmentOffset < startOffset) {
                segmentOffset = startOffset;
            }
            
            segmentsSearched++;
            
            std::cout << "\n[BSGS] Random BSGS segment #" << segmentsSearched << "\n";
            std::cout << "       Segment base: " << segmentOffset << "\n";
            std::cout << "       Segment range: " << segmentOffset << " to " << (segmentOffset + segmentSize) << "\n";
            std::cout << "       Searching with Baby Steps (m=" << m << ") + Giant Steps...\n";
            
            if (!running) break;
            
            // Search this segment using BSGS (baby steps already computed)
            uint64_t keysInSegment = searchSegmentBSGS(m, segmentOffset, totalKeysChecked);
            
            if (keysInSegment == 0xFFFFFFFFFFFFFFFFULL) { // Special value for found
                return true;
            }
            
            totalKeysChecked += keysInSegment;
            
            // Progress update every segment
            if (segmentsSearched % 1 == 0) {
                auto now = std::chrono::high_resolution_clock::now();
                double elapsed = std::chrono::duration<double>(now - globalStart).count();
                double rate = totalKeysChecked / elapsed;
                
                // Format hash rate with appropriate unit
                std::string rateStr;
                if (rate >= 1e21) {
                    rateStr = std::to_string((int)(rate / 1e21)) + " ZH/s";
                } else if (rate >= 1e18) {
                    rateStr = std::to_string((int)(rate / 1e18)) + " EH/s";
                } else if (rate >= 1e15) {
                    rateStr = std::to_string((int)(rate / 1e15)) + " PH/s";
                } else if (rate >= 1e12) {
                    rateStr = std::to_string((int)(rate / 1e12)) + " TH/s";
                } else if (rate >= 1e9) {
                    rateStr = std::to_string((int)(rate / 1e9)) + " GH/s";
                } else if (rate >= 1e6) {
                    rateStr = std::to_string((int)(rate / 1e6)) + " MH/s";
                } else if (rate >= 1e3) {
                    rateStr = std::to_string((int)(rate / 1e3)) + " kH/s";
                } else {
                    rateStr = std::to_string((int)rate) + " H/s";
                }
                
                std::cout << "\n[BSGS] Progress: " << segmentsSearched << " segments searched | "
                          << totalKeysChecked << " keys checked | "
                          << rateStr << " | "
                          << "Elapsed: " << (int)(elapsed / 60) << "m " << (int)(elapsed) % 60 << "s\n";
            }
        }
        
        std::cout << "\n\n[BSGS] Key not found after " << segmentsSearched << " random BSGS segments.\n";
        std::cout << "Total keys checked: " << totalKeysChecked << "\n";
        std::cout << "Total time: ";
        auto globalEnd = std::chrono::high_resolution_clock::now();
        double totalElapsed = std::chrono::duration<double>(globalEnd - globalStart).count();
        if (totalElapsed > 3600) {
            std::cout << (int)(totalElapsed / 3600) << "h " << ((int)(totalElapsed) % 3600) / 60 << "m\n";
        } else {
            std::cout << (int)(totalElapsed / 60) << "m " << (int)(totalElapsed) % 60 << "s\n";
        }
        
        return false;
    }
    
    // Search a single segment using BSGS algorithm (baby steps already computed)
    // Returns number of keys checked, or special value 0xFFFFFFFFFFFFFFFFULL if found
    uint64_t searchSegmentBSGS(uint64_t m, const uint256_t& segmentOffset, uint64_t& totalKeysCounter) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Use GPU if available and requested
        if (cfg.bsgsUseGPU && gpuIntegration.isActive()) {
            std::cout << "[BSGS-GPU] Launching GPU giant steps for segment...\n";
            
            // Launch GPU kernel for giant steps
            if (gpuIntegration.launchGiantSteps(0, m)) {
                gpuIntegration.synchronize();
                
                uint64_t foundI = 0, foundJ = 0;
                if (gpuIntegration.checkResult(foundI, foundJ)) {
                    uint256_t k = uint256_t(foundI) * uint256_t(m) + uint256_t(foundJ) + segmentOffset;
                    
                    std::cout << "\n" << std::string(70, '=') << std::endl;
                    std::cout << "  [BSGS-GPU] PRIVATE KEY FOUND!\n";
                    std::cout << "  Key: " << k << std::endl;
                    std::cout << std::string(70, '=') << "\n" << std::endl;
                    
                    // Save to file
                    std::ofstream outFile("bsgs_found.txt", std::ios::app);
                    if (outFile.is_open()) {
                        outFile << "=== BSGS PRIVATE KEY FOUND (GPU Mode) ===\n";
                        outFile << "Public Key: " << cfg.bsgsPub << "\n";
                        outFile << "Private Key: " << k << "\n";
                        outFile << "==============================\n\n";
                    }
                    
                    return 0xFFFFFFFFFFFFFFFFULL;
                }
                
                totalKeysCounter += m * m;
                return m * m;
            }
            
            std::cout << "[BSGS-GPU] GPU launch failed, falling back to CPU\n";
        }
        
        // CPU fallback
        std::atomic<bool> localFound(false);
        uint256_t foundKey = 0;
        std::atomic<uint64_t> stepsDone(0);
        
        int numThreads = cfg.bsgsThreads;
        if (numThreads <= 0) {
            numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0) numThreads = 4;
        }
        
        // Precompute m*G for giant step increment
        secp256k1_pubkey mG;
        if (!computePubKey(m, mG)) {
            std::cerr << "[BSGS ERROR] Failed to compute m*G\n";
            return 0;
        }
        
        std::vector<std::thread> threads;
        uint64_t chunkSize = m / numThreads + 1;
        
        for (int t = 0; t < numThreads; t++) {
            uint64_t startI = t * chunkSize;
            uint64_t endI = std::min((t + 1) * chunkSize, m);
            
            threads.emplace_back([&, startI, endI]() {
                secp256k1_context* localCtx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
                
                uint64_t startK = startI * m;
                secp256k1_pubkey giantPoint;
                if (!computePubKey(startK, giantPoint)) {
                    secp256k1_context_destroy(localCtx);
                    return;
                }
                
                for (uint64_t i = startI; i < endI && running && !localFound; i++) {
                    // Compute Q - giantPoint = Q + (-giantPoint)
                    secp256k1_pubkey negGiant;
                    if (!negatePoint(giantPoint, negGiant)) continue;
                    
                    // Add Q + (-giantPoint)
                    const secp256k1_pubkey* pts[2] = { &targetPub, &negGiant };
                    secp256k1_pubkey candidate;
                    if (!secp256k1_ec_pubkey_combine(localCtx, &candidate, pts, 2)) {
                        // Move to next giant step
                        if (i < endI - 1) {
                            const secp256k1_pubkey* addPts[2] = { &giantPoint, &mG };
                            secp256k1_pubkey nextGiant;
                            if (secp256k1_ec_pubkey_combine(localCtx, &nextGiant, addPts, 2)) {
                                giantPoint = nextGiant;
                            }
                        }
                        continue;
                    }
                    
                    // Check if candidate is in baby steps table
                    std::string candidateKey = serializePubkey(candidate);
                    auto it = babySteps.find(candidateKey);
                    if (it != babySteps.end()) {
                        uint64_t j = it->second;
                        uint256_t k = uint256_t(i) * uint256_t(m) + uint256_t(j) + segmentOffset;
                        
                        localFound = true;
                        foundKey = k;
                        
                        // Also check against bloom filter addresses if enabled
                        if (bloomIntegration.isActive()) {
                            secp256k1_pubkey foundPubKey;
                            if (computePubKey(k, foundPubKey)) {
                                bloomIntegration.checkAndReport(foundPubKey, k);
                            }
                        }
                    }
                    
                    // Move to next giant step: giantPoint += m*G
                    if (i < endI - 1) {
                        const secp256k1_pubkey* addPts[2] = { &giantPoint, &mG };
                        secp256k1_pubkey nextGiant;
                        if (secp256k1_ec_pubkey_combine(localCtx, &nextGiant, addPts, 2)) {
                            giantPoint = nextGiant;
                        }
                    }
                    
                    stepsDone++;
                }
                
                secp256k1_context_destroy(localCtx);
            });
        }
        
        for (auto& th : threads) {
            th.join();
        }
        
        if (localFound) {
            std::cout << "\n" << std::string(50, '=') << "\n";
            std::cout << "[BSGS] PRIVATE KEY FOUND!\n";
            std::cout << "Key: " << foundKey << "\n";
            std::cout << "Key (hex): ";
            // Print hex
            uint256_t temp = foundKey;
            bool first = true;
            for (int i = 31; i >= 0; i--) {
                int byte = (int)((temp >> (i * 8)) & 0xFF);
                if (byte != 0 || !first || i == 0) {
                    if (first && byte != 0) first = false;
                    std::cout << std::hex << std::setw(2) << std::setfill('0') << byte;
                }
            }
            std::cout << std::dec << "\n";
            std::cout << std::string(50, '=') << "\n";
            
            // Save to file
            std::ofstream outFile("bsgs_found.txt", std::ios::app);
            if (outFile.is_open()) {
                outFile << "=== BSGS PRIVATE KEY FOUND (Random Mode) ===\n";
                outFile << "Public Key: " << cfg.bsgsPub << "\n";
                outFile << "Private Key: " << foundKey << "\n";
                outFile << "==============================\n\n";
            }
            
            return 0xFFFFFFFFFFFFFFFFULL; // Special value indicating found
        }
        
        return m * m; // Number of keys checked in this segment
    }

    // Main entry point
    void run() {
        // Check for multi-target mode first
        if (!cfg.bsgsTargetsFile.empty()) {
            std::cout << "[BSGS] Multi-target batch search mode\n";
            if (!loadTargetsFromFile(cfg.bsgsTargetsFile)) {
                return;
            }
        } else if (!cfg.bsgsPub.empty()) {
            // Single target mode
            if (!parsePublicKey(cfg.bsgsPub)) {
                return;
            }
            std::cout << "[BSGS] Target public key loaded successfully\n";
        } else if (!cfg.bsgsBloomKeysFiles.empty()) {
            // Bloom-only mode: search for any address in bloom filter
            std::cout << "[BSGS] Bloom filter address search mode\n";
            std::cout << "[BSGS] Searching for addresses matching bloom filter(s)\n";
            runBloomOnlyMode();
            return;
        } else {
            std::cerr << "[BSGS ERROR] No public key specified. Use --bsgs-pub <hex>, --bsgs-targets <file>, or --bloom-keys <file>\n";
            return;
        }
        
        // Determine search range
        uint64_t maxKey64 = cfg.bsgsMax;
        uint256_t maxKey = 0;
        
        // Parse start offset from string (supports large values)
        uint256_t startOffset = 0;
        try {
            startOffset = uint256_t(cfg.bsgsStartStr);
        } catch (...) {
            std::cerr << "[BSGS ERROR] Invalid start offset: " << cfg.bsgsStartStr << "\n";
            return;
        }
        
        int bits = 0;
        bool useLargeRange = false;
        
        // --bsgs-range takes precedence over --bsgs-max
        if (cfg.bsgsRange > 0) {
            if (cfg.bsgsRange > 256) {
                std::cerr << "[BSGS ERROR] Range too large. Maximum is 256 bits.\n";
                return;
            }
            bits = cfg.bsgsRange;
            
            // Calculate maxKey = 2^bits
            maxKey = uint256_t(1) << bits;
            
            if (bits >= 64) {
                useLargeRange = true;
                std::cout << "[BSGS] Using range: 2^" << bits << " = " << maxKey << "\n";
                std::cout << "[BSGS] Using segmented search for large ranges.\n";
                std::cout << "[BSGS WARNING] Range " << bits << " bits requires multiple segments.\n";
                std::cout << "               This will take significant time.\n";
            } else {
                maxKey64 = 1ULL << bits;
                maxKey = maxKey64;
                std::cout << "[BSGS] Using range: 2^" << bits << " = " << maxKey64 << "\n";
            }
            
            // Warn about practical limits
            if (bits > 48 && bits < 64) {
                uint64_t m = (uint64_t)std::sqrt((double)maxKey64) + 1;
                double memMB = (m * 80.0) / (1024 * 1024);
                std::cout << "[BSGS WARNING] Baby steps will require ~" << (memMB / 1024) << " GB memory.\n";
            }
        } else if (maxKey64 == 0) {
            // Default: 2^40 (about 1 trillion)
            maxKey64 = 1099511627776ULL; // 2^40
            maxKey = maxKey64;
            std::cout << "[BSGS] Using default max key: 2^40 = " << maxKey64 << "\n";
        } else {
            maxKey = maxKey64;
        }
        
        // Apply start offset if specified
        if (startOffset > 0) {
            std::cout << "[BSGS] Starting from offset: " << startOffset << "\n";
        }
        
        // Run search
        bool found = false;
        
        // Check if multi-target mode
        if (multiTargetMode) {
            if (cfg.bsgsRange > 64) {
                found = searchMultiTargetLarge(maxKey, startOffset);
            } else {
                found = searchMultiTarget(maxKey64, (uint64_t)startOffset);
            }
        } else {
            // Check if random mode is requested for large ranges
            bool useRandomMode = (cfg.bsgsMode == "random");
            
            if (useRandomMode && cfg.bsgsRange > 64) {
                std::cout << "[BSGS] Using RANDOM sampling mode for large range\n";
                found = searchRandomRange(maxKey, startOffset);
            } else if (useLargeRange) {
                if (useRandomMode) {
                    std::cout << "[BSGS] Note: Random mode is most effective for ranges > 64 bits.\n";
                    std::cout << "        Using standard segmented search for this range.\n";
                }
                found = searchLargeRange(maxKey, startOffset);
            } else {
                found = search(maxKey64, (uint64_t)startOffset);
            }
        }
        
        // Save results for multi-target mode
        if (multiTargetMode) {
            saveMultiTargetResults();
        } else if (!found) {
            std::cout << "\n[BSGS] Search completed. Key not found.\n";
            if (cfg.bsgsRange > 0) {
                std::cout << "  Try increasing --bsgs-range value or use --bsgs-start to continue\n";
            } else {
                std::cout << "  Try increasing --bsgs-max value\n";
            }
        }
    }
    
    // Bloom-only mode: search for any address in bloom filter without specific target
    void runBloomOnlyMode() {
        if (!bloomIntegration.isActive()) {
            std::cerr << "[BSGS-BLOOM ERROR] No bloom filters loaded\n";
            return;
        }
        
        std::cout << "[BSGS-BLOOM] Starting address search in random segments\n";
        std::cout << "[BSGS-BLOOM] Checking all generated addresses against bloom filter(s)\n\n";
        
        // Get m value
        uint64_t m = getBabyStepsM();
        computeBabySteps(m);
        
        if (!running) return;
        
        // Determine range
        uint256_t maxKey;
        if (cfg.bsgsRange > 0) {
            maxKey = uint256_t(1) << cfg.bsgsRange;
        } else {
            maxKey = uint256_t(1) << 135; // Default to 135 bits
        }
        
        // Determine segment size
        uint256_t segmentSize;
        if (cfg.bsgsSegmentBits > 0) {
            segmentSize = uint256_t(1) << cfg.bsgsSegmentBits;
        } else {
            segmentSize = uint256_t(m) * uint256_t(m);
        }
        
        uint64_t segmentsSearched = 0;
        uint64_t maxAttempts = 10000000; // 10 million segments max
        uint64_t totalKeysChecked = 0;
        
        auto globalStart = std::chrono::high_resolution_clock::now();
        
        std::cout << "[BSGS-BLOOM] Baby steps ready: " << m << " entries\n";
        std::cout << "[BSGS-BLOOM] Segment size: " << segmentSize << " keys\n";
        std::cout << "[BSGS-BLOOM] Press Ctrl+C to stop\n\n";
        
        while (running && segmentsSearched < maxAttempts) {
            // Generate random segment offset
            uint256_t randomOffset = randomUint256(maxKey > segmentSize ? maxKey - segmentSize : maxKey);
            uint256_t segmentIndex = randomOffset / segmentSize;
            uint256_t segmentOffset = segmentIndex * segmentSize;
            
            if (segmentOffset >= maxKey) {
                segmentOffset = maxKey - segmentSize;
            }
            
            segmentsSearched++;
            
            // Search this segment with bloom checking
            if (searchSegmentWithBloomCheck(m, segmentOffset, totalKeysChecked)) {
                return; // Found a match
            }
            
            // Progress update
            if (segmentsSearched % 100 == 0) {
                auto now = std::chrono::high_resolution_clock::now();
                double elapsed = std::chrono::duration<double>(now - globalStart).count();
                double rate = totalKeysChecked / elapsed;
                
                std::cout << "[BSGS-BLOOM] Progress: " << segmentsSearched << " segments | "
                          << totalKeysChecked << " keys | "
                          << std::fixed << std::setprecision(1) << rate << " keys/s | "
                          << "Elapsed: " << (int)(elapsed / 60) << "m " << (int)(elapsed) % 60 << "s\n";
            }
        }
        
        std::cout << "\n[BSGS-BLOOM] Search completed. No addresses found after " 
                  << segmentsSearched << " segments.\n";
    }
    
    // Search a segment with bloom filter address checking
    bool searchSegmentWithBloomCheck(uint64_t m, const uint256_t& segmentOffset, uint64_t& totalKeysCounter) {
        int numThreads = cfg.bsgsThreads;
        if (numThreads <= 0) {
            numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0) numThreads = 4;
        }
        
        // Precompute m*G
        secp256k1_pubkey mG;
        if (!computePubKey(m, mG)) return false;
        
        std::atomic<bool> matchFound(false);
        std::atomic<uint64_t> keysChecked(0);
        
        std::vector<std::thread> threads;
        uint64_t chunkSize = m / numThreads + 1;
        
        for (int t = 0; t < numThreads; t++) {
            uint64_t startI = t * chunkSize;
            uint64_t endI = std::min((t + 1) * chunkSize, m);
            
            threads.emplace_back([&, startI, endI]() {
                secp256k1_context* localCtx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
                
                for (uint64_t i = startI; i < endI && running && !matchFound; i++) {
                    // For each giant step, check all baby step combinations
                    for (uint64_t j = 0; j < m && running && !matchFound; j++) {
                        // Calculate potential private key: k = i*m + j + segmentOffset
                        uint256_t k = uint256_t(i) * uint256_t(m) + uint256_t(j) + segmentOffset;
                        
                        // Generate public key for this private key
                        secp256k1_pubkey pubKey;
                        if (!computePubKey(k, pubKey)) continue;
                        
                        // Check against bloom filter
                        if (bloomIntegration.checkAndReport(pubKey, k)) {
                            matchFound = true;
                            break;
                        }
                        
                        keysChecked++;
                    }
                }
                
                secp256k1_context_destroy(localCtx);
            });
        }
        
        for (auto& th : threads) {
            th.join();
        }
        
        totalKeysCounter += keysChecked.load();
        return matchFound.load();
    }
};

// ============================================================================
// GPU BSGS Kernel (CUDA) - Placeholder for future implementation
// ============================================================================
#ifdef USE_CUDA
extern "C" void launch_bsgs_search(
    const uint32_t* targetPubX,
    const uint32_t* targetPubY,
    uint64_t m,
    uint64_t* outKey,
    int* outFound
);
#endif
