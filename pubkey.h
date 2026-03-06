#pragma once

// ============================================================================
// PUBKEY MODE - Public Key Recovery / Private Key Search
// ============================================================================
// Caută cheia privată pentru o adresă Bitcoin dată, într-un interval specific de biți
// Exemplu: --mode pubkey --pub-address 13zb1hQbWVsc2S7ZTZnP2G4undNNpdh5so --pub-bit 71

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

#include <openssl/sha.h>
#include <openssl/ripemd.h>
#ifndef NO_SECP256K1
#include <secp256k1.h>
#endif

#include "utils.h"
#include "bloom.h"
#include "args.h"

// ============================================================================
// Big Integer (256-bit) for large bit ranges
// ============================================================================
class Uint256 {
private:
    uint64_t data[4]; // 4 * 64 = 256 bits
    
public:
    Uint256() { memset(data, 0, sizeof(data)); }
    Uint256(uint64_t val) { 
        memset(data, 0, sizeof(data)); 
        data[0] = val; 
    }
    
    // Initialize from bit position (2^bit)
    static Uint256 fromBit(int bit) {
        Uint256 result;
        if (bit < 64) {
            result.data[0] = 1ULL << bit;
        } else if (bit < 128) {
            result.data[1] = 1ULL << (bit - 64);
        } else if (bit < 192) {
            result.data[2] = 1ULL << (bit - 128);
        } else if (bit < 256) {
            result.data[3] = 1ULL << (bit - 192);
        }
        return result;
    }
    
    uint64_t* getData() { return data; }
    const uint64_t* getData() const { return data; }
    
    // Increment
    void increment() {
        for (int i = 0; i < 4; i++) {
            if (++data[i] != 0) break; // No carry
        }
    }
    
    // Add value
    void add(uint64_t val) {
        uint64_t carry = val;
        for (int i = 0; i < 4 && carry; i++) {
            uint64_t sum = data[i] + carry;
            carry = (sum < data[i]) ? 1 : 0;
            data[i] = sum;
        }
    }
    
    // Compare
    bool operator<(const Uint256& other) const {
        for (int i = 3; i >= 0; i--) {
            if (data[i] != other.data[i]) return data[i] < other.data[i];
        }
        return false;
    }
    bool operator!=(const Uint256& other) const {
        for (int i = 0; i < 4; i++) {
            if (data[i] != other.data[i]) return true;
        }
        return false;
    }
    bool isZero() const {
        for (int i = 0; i < 4; i++) {
            if (data[i] != 0) return false;
        }
        return true;
    }
    
    // Convert to 32-byte private key (big-endian)
    void toPrivateKey(unsigned char* privKey) const {
        memset(privKey, 0, 32);
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 8; j++) {
                int idx = 31 - (i * 8 + j);
                if (idx >= 0) {
                    privKey[idx] = (data[i] >> (j * 8)) & 0xFF;
                }
            }
        }
    }
    
    // Convert from 32-byte array
    static Uint256 fromBytes(const unsigned char* bytes) {
        Uint256 result;
        for (int i = 0; i < 4; i++) {
            result.data[i] = 0;
            for (int j = 0; j < 8; j++) {
                int idx = 31 - (i * 8 + j);
                if (idx >= 0) {
                    result.data[i] |= ((uint64_t)bytes[idx] << (j * 8));
                }
            }
        }
        return result;
    }
    
    // Convert to hex string
    std::string toHex() const {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        bool leading = true;
        for (int i = 3; i >= 0; i--) {
            if (data[i] != 0 || !leading || i == 0) {
                if (leading) {
                    ss << data[i];
                    leading = false;
                } else {
                    ss << std::setw(16) << data[i];
                }
            }
        }
        return ss.str();
    }
};

// ============================================================================
// CUDA Forward Declarations (if CUDA available)
// ============================================================================
#ifdef USE_CUDA
#include <cuda_runtime.h>

extern "C" void launch_gpu_pubkey_search(
    const uint32_t* startKey,
    const uint32_t* endKey,
    const uint8_t* targetHash160,
    int blocks,
    int threads,
    int points,
    unsigned long long* outFoundKeysLow,
    unsigned long long* outFoundKeysHigh,
    int* outFoundCount,
    int maxResults
);
#endif

// ============================================================================
// Worker Thread for CPU Search
// ============================================================================
class PubkeyWorker {
private:
    secp256k1_context* ctx;
    std::string targetAddr;
    std::string addrType;
    std::atomic<bool>& running;
    std::atomic<unsigned long long>& totalChecked;
    
public:
    PubkeyWorker(const std::string& target, const std::string& type, std::atomic<bool>& run, 
                 std::atomic<unsigned long long>& checked)
        : targetAddr(target), addrType(type), running(run), totalChecked(checked) {
        ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    }
    
    ~PubkeyWorker() {
        if(ctx) secp256k1_context_destroy(ctx);
    }
    
    std::string deriveAddress(const std::vector<uint8_t>& pubKey) {
        if(addrType == "p2wpkh") {
            return PubKeyToNativeSegwit(pubKey);
        } else if(addrType == "p2sh") {
            return PubKeyToP2SH(pubKey);
        } else {
            return PubKeyToLegacy(pubKey);
        }
    }
    
    void search(Uint256 start, Uint256 end, int threadId, int numThreads, 
                std::vector<Uint256>& foundKeys) {
        Uint256 current = start;
        
        // Add thread offset
        for(int i = 0; i < threadId; i++) {
            current.add(1);
        }
        
        while(current < end && running) {
            unsigned char privKey[32];
            current.toPrivateKey(privKey);
            
            secp256k1_pubkey pub;
            if(secp256k1_ec_pubkey_create(ctx, &pub, privKey)) {
                unsigned char pubKeyC[33];
                size_t pubLen = 33;
                secp256k1_ec_pubkey_serialize(ctx, pubKeyC, &pubLen, &pub, SECP256K1_EC_COMPRESSED);
                std::vector<uint8_t> vPubC(pubKeyC, pubKeyC + 33);
                
                std::string derivedAddr = deriveAddress(vPubC);
                
                if(derivedAddr == targetAddr) {
                    foundKeys.push_back(Uint256::fromBytes(privKey));
                    running = false;
                    break;
                }
            }
            
            totalChecked++;
            
            // Increment by number of threads
            current.add(numThreads);
        }
    }
};

// ============================================================================
// Main Pubkey Mode Class
// ============================================================================
class PubkeyMode {
private:
    ProgramConfig cfg;
    std::atomic<bool>& running;
    
public:
    PubkeyMode(const ProgramConfig& config, std::atomic<bool>& runFlag) 
        : cfg(config), running(runFlag) {}
    
    std::string detectAddressType(const std::string& addr) {
        if(addr.substr(0, 3) == "bc1") return "p2wpkh";
        else if(addr[0] == '3') return "p2sh";
        else if(addr[0] == '1') return "p2pkh";
        return "";
    }
    
    void getHash160FromAddress(const std::string& addr, uint8_t* hash160) {
        if(addr[0] == '1' || addr[0] == '3') {
            // Base58 decode
            std::vector<uint8_t> decoded = DecodeBase58(addr);
            std::cout << "[Debug] Decoded size: " << decoded.size() << " bytes\n";
            if(decoded.size() == 25) {
                memcpy(hash160, decoded.data() + 1, 20);
                std::cout << "[Debug] Hash160 extracted successfully\n";
            } else {
                std::cerr << "[Error] Invalid decoded size: " << decoded.size() << " (expected 25)\n";
            }
        } else if(addr.substr(0, 3) == "bc1") {
            // Bech32 decode
            std::string hrp;
            std::vector<uint8_t> program;
            int witver = Bech32::decode_segwit(addr, hrp, program);
            if(witver == 0 && program.size() == 20) {
                memcpy(hash160, program.data(), 20);
            }
        }
    }
    
    void runCPU(const std::string& targetAddr, const std::string& addrType, 
                const Uint256& startKey, const Uint256& endKey) {
        // Determine number of threads
        int numThreads = cfg.cpuCores;
        if(numThreads <= 0) {
            numThreads = std::thread::hardware_concurrency();
            if(numThreads == 0) numThreads = 4; // Fallback
        }
        
        std::cout << "[CPU Mode] Starting search with " << numThreads << " threads...\n\n";
        
        auto startTime = std::chrono::high_resolution_clock::now();
        std::atomic<unsigned long long> totalChecked{0};
        std::vector<std::thread> threads;
        std::vector<std::vector<Uint256>> foundKeysPerThread(numThreads);
        
        // Launch worker threads
        for(int i = 0; i < numThreads; i++) {
            threads.emplace_back([&, i, numThreads]() {
                PubkeyWorker worker(targetAddr, addrType, running, totalChecked);
                worker.search(startKey, endKey, i, numThreads, foundKeysPerThread[i]);
            });
        }
        
        // Progress monitor thread
        std::thread monitor([&]() {
            auto lastTime = std::chrono::high_resolution_clock::now();
            unsigned long long lastChecked = 0;
            
            while(running) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                
                auto now = std::chrono::high_resolution_clock::now();
                double elapsed = std::chrono::duration<double>(now - lastTime).count();
                unsigned long long checked = totalChecked.load();
                unsigned long long diff = checked - lastChecked;
                double speed = diff / elapsed;
                
                std::cout << "\r[CPU] Keys: " << checked 
                          << " | Speed: " << std::fixed << std::setprecision(0) << speed << " k/s"
                          << std::flush;
                
                lastChecked = checked;
                lastTime = now;
            }
        });
        
        // Wait for all workers
        for(auto& t : threads) {
            t.join();
        }
        monitor.join();
        
        auto endTime = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(endTime - startTime).count();
        
        // Check if any key was found
        bool found = false;
        Uint256 foundKey;
        for(const auto& keys : foundKeysPerThread) {
            if(!keys.empty()) {
                found = true;
                foundKey = keys[0];
                break;
            }
        }
        
        std::cout << "\n\n=== Search Complete ===\n";
        std::cout << "Keys Checked: " << totalChecked.load() << "\n";
        std::cout << "Time:         " << elapsed << " seconds\n";
        if(elapsed > 0) {
            std::cout << "Avg Speed:    " << (totalChecked.load() / elapsed) << " keys/second\n";
        }
        
        if(found) {
            std::cout << "\n*** KEY FOUND! ***\n";
            unsigned char privKey[32];
            foundKey.toPrivateKey(privKey);
            std::string privHex = BytesToHex(std::vector<uint8_t>(privKey, privKey + 32));
            std::string wif = privKeyToWIF(privKey, false);
            
            std::cout << "Private Key (HEX): " << privHex << "\n";
            std::cout << "Private Key (WIF): " << wif << "\n";
            std::cout << "********************\n";
            
            // Save to file
            std::ofstream outFile("pubkey_found.txt", std::ios::app);
            if(outFile.is_open()) {
                outFile << "=== PUBKEY MODE HIT ===\n";
                outFile << "Target: " << targetAddr << "\n";
                outFile << "Private Key (HEX): " << privHex << "\n";
                outFile << "Private Key (WIF): " << wif << "\n";
                outFile << "===================\n\n";
            }
        } else {
            std::cout << "Result: KEY NOT FOUND\n";
        }
        std::cout << "\n";
    }
    
#ifdef USE_CUDA
    void runGPU(const std::string& targetAddr, const std::string& addrType,
                const Uint256& startKey, const Uint256& endKey) {
        std::cout << "[GPU Mode] Initializing CUDA...\n";
        
        // Get target hash160
        uint8_t targetHash160[20] = {0};
        getHash160FromAddress(targetAddr, targetHash160);
        
        // Debug: Print target hash160
        std::cout << "[Debug] Target Hash160: ";
        for(int i = 0; i < 20; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)targetHash160[i];
        }
        std::cout << std::dec << "\n\n";
        
        // Test all keys from 1 to 7 to verify which one matches
        std::cout << "[Debug] Testing all keys 1-7...\n";
        secp256k1_context* testCtx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
        for(int testKey = 1; testKey <= 7; testKey++) {
            unsigned char testPrivKey[32] = {0};
            testPrivKey[31] = testKey;
            
            secp256k1_pubkey testPub;
            if(secp256k1_ec_pubkey_create(testCtx, &testPub, testPrivKey)) {
                unsigned char testPubKeyC[33];
                size_t testPubLen = 33;
                secp256k1_ec_pubkey_serialize(testCtx, testPubKeyC, &testPubLen, &testPub, SECP256K1_EC_COMPRESSED);
                std::vector<uint8_t> vTestPubC(testPubKeyC, testPubKeyC + 33);
                std::string testAddr = PubKeyToLegacy(vTestPubC);
                
                // Calculate hash160
                unsigned char shaResult[SHA256_DIGEST_LENGTH];
                SHA256(testPubKeyC, 33, shaResult);
                unsigned char ripemdResult[RIPEMD160_DIGEST_LENGTH];
                RIPEMD160(shaResult, SHA256_DIGEST_LENGTH, ripemdResult);
                
                // Print hash160
                std::cout << "  hash160: ";
                for(int i = 0; i < 20; i++) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)ripemdResult[i];
                }
                std::cout << std::dec;
                
                std::cout << " -> " << testAddr;
                if(testAddr == targetAddr) std::cout << " *** MATCH ***";
                std::cout << "\n";
            }
        }
        std::cout << "\n";
        
        // Test startKey hash160 for GPU verification
        std::cout << "[Debug] Testing startKey hash160 with CPU...\n";
        unsigned char startPrivKey[32] = {0};
        startKey.toPrivateKey(startPrivKey);
        
        std::cout << "[CPU] startKey bytes: ";
        for(int i = 0; i < 32; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)startPrivKey[i];
        }
        std::cout << std::dec << "\n";
        
        secp256k1_pubkey startPub;
        if(secp256k1_ec_pubkey_create(testCtx, &startPub, startPrivKey)) {
            unsigned char startPubKeyC[33];
            size_t startPubLen = 33;
            secp256k1_ec_pubkey_serialize(testCtx, startPubKeyC, &startPubLen, &startPub, SECP256K1_EC_COMPRESSED);
            
            unsigned char shaResult[SHA256_DIGEST_LENGTH];
            SHA256(startPubKeyC, 33, shaResult);
            unsigned char ripemdResult[RIPEMD160_DIGEST_LENGTH];
            RIPEMD160(shaResult, SHA256_DIGEST_LENGTH, ripemdResult);
            
            std::cout << "[CPU] startKey (2^70) hash160: ";
            for(int i = 0; i < 20; i++) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)ripemdResult[i];
            }
            std::cout << std::dec << "\n";
            
            // Compare with target
            bool match = true;
            for(int i = 0; i < 20; i++) {
                if(ripemdResult[i] != targetHash160[i]) {
                    match = false;
                    break;
                }
            }
            if(match) {
                std::cout << "*** STARTKEY MATCHES TARGET! ***\n";
            }
        }
        std::cout << "\n";
        
        // Allocate device memory for results (needed for tests)
        unsigned long long* d_foundKeysLow = nullptr;
        unsigned long long* d_foundKeysHigh = nullptr;
        int* d_foundCount = nullptr;
        
        cudaMalloc(&d_foundKeysLow, 1024 * sizeof(unsigned long long));
        cudaMalloc(&d_foundKeysHigh, 1024 * sizeof(unsigned long long));
        cudaMalloc(&d_foundCount, sizeof(int));
        
        // Test for key=1 first (known result) - MUST be before destroying testCtx
        std::cout << "\n[Debug] Testing key=1 (should give G point)...\n";
        {
            uint32_t key1Data[8] = {1,0,0,0,0,0,0,0};  // Key = 1 in u256 format
            
            // CPU calculation for key=1
            unsigned char priv1[32] = {0};
            priv1[31] = 1;  // Key = 1
            
            secp256k1_pubkey pub1;
            unsigned char pub1C[33];
            size_t pub1Len = 33;
            secp256k1_ec_pubkey_create(testCtx, &pub1, priv1);
            secp256k1_ec_pubkey_serialize(testCtx, pub1C, &pub1Len, &pub1, SECP256K1_EC_COMPRESSED);
            
            unsigned char sha1[32], rmd1[20];
            SHA256(pub1C, 33, sha1);
            RIPEMD160(sha1, 32, rmd1);
            
            std::cout << "[CPU] Key=1, PubKey: ";
            for(int i = 0; i < 33; i++) std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)pub1C[i];
            std::cout << "\n[CPU] Key=1, hash160: ";
            for(int i = 0; i < 20; i++) std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)rmd1[i];
            std::cout << std::dec << "\n";
            
            // GPU test for key=1
            std::cout << "[GPU] Running key=1 test...\n";
            cudaMemset(d_foundCount, 0, sizeof(int));
            launch_gpu_pubkey_search(
                key1Data, key1Data,
                targetHash160,
                1, 1, 1,
                d_foundKeysLow, d_foundKeysHigh, d_foundCount, 1024
            );
            cudaDeviceSynchronize();
        }
        
        // Quick test: Compare GPU and CPU for first key
        std::cout << "\n[Debug] Quick GPU vs CPU comparison for startKey...\n";
        {
            uint32_t testKeyData[8] = {0};
            const uint64_t* start64 = (const uint64_t*)startKey.getData();
            for(int i = 0; i < 4; i++) {
                testKeyData[i*2]   = (uint32_t)(start64[i] & 0xFFFFFFFF);
                testKeyData[i*2+1] = (uint32_t)(start64[i] >> 32);
            }
            
            // Calculate expected hash160 with CPU for comparison
            unsigned char testPrivKey[32] = {0};
            startKey.toPrivateKey(testPrivKey);
            
            secp256k1_pubkey testPub;
            unsigned char testPubKeyC[33];
            size_t testPubLen = 33;
            secp256k1_ec_pubkey_create(testCtx, &testPub, testPrivKey);
            secp256k1_ec_pubkey_serialize(testCtx, testPubKeyC, &testPubLen, &testPub, SECP256K1_EC_COMPRESSED);
            
            unsigned char shaResult[SHA256_DIGEST_LENGTH];
            SHA256(testPubKeyC, 33, shaResult);
            unsigned char rmdResult[RIPEMD160_DIGEST_LENGTH];
            RIPEMD160(shaResult, SHA256_DIGEST_LENGTH, rmdResult);
            
            std::cout << "[CPU] startKey hash160: ";
            for(int i = 0; i < 20; i++) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)rmdResult[i];
            }
            std::cout << std::dec << "\n";
            
            // Run one GPU iteration
            std::cout << "[GPU] Running single key test...\n\n";
            cudaMemset(d_foundCount, 0, sizeof(int));
            launch_gpu_pubkey_search(
                testKeyData, testKeyData,  // Same start and end to test single key
                targetHash160,
                1, 1, 1,  // Single block, thread, point
                d_foundKeysLow, d_foundKeysHigh, d_foundCount, 1024
            );
            cudaDeviceSynchronize();
            std::cout << "\n";
        }
        
        // Destroy test context after all tests are done
        secp256k1_context_destroy(testCtx);
        
        auto startTime = std::chrono::high_resolution_clock::now();
        auto lastPrintTime = startTime;
        unsigned long long totalChecked = 0;
        Uint256 currentKey = startKey;
        bool found = false;
        Uint256 foundKey;
        
        std::cout << "Starting GPU search... (Press Ctrl+C to stop)\n\n";
        
        // Calculate keys per launch (default)
        unsigned long long keysPerLaunch = (unsigned long long)cfg.cudaBlocks * 
                                            (unsigned long long)cfg.cudaThreads * 
                                            (unsigned long long)cfg.pointsPerThread;
        
        // Reduce grid size to avoid "too many resources requested for launch" error
        // New mod_mul uses more registers, need smaller grid
        int adjustedBlocks = cfg.cudaBlocks;
        int adjustedThreads = cfg.cudaThreads;
        int adjustedPoints = cfg.pointsPerThread;
        
        // Limit threads if grid is too large
        if(adjustedBlocks * adjustedThreads > 65536) {
            adjustedBlocks = 128;
            adjustedThreads = 512;
            keysPerLaunch = (unsigned long long)adjustedBlocks * adjustedThreads * adjustedPoints;
            std::cout << "[GPU] Reduced grid to avoid resource error: " 
                      << adjustedBlocks << "x" << adjustedThreads << "x" << adjustedPoints
                      << " = " << keysPerLaunch << " keys/launch\n";
        }
        
        // Calculate actual range in 64-bit (for ranges that fit in 64 bits)
        uint64_t startLow = ((const uint64_t*)startKey.getData())[0];
        uint64_t endLow = ((const uint64_t*)endKey.getData())[0];
        uint64_t rangeSize = endLow - startLow;
        
        if(rangeSize < 10000 && ((const uint64_t*)endKey.getData())[1] == ((const uint64_t*)startKey.getData())[1]) {
            // Small range - use smaller grid
            adjustedBlocks = 4;
            adjustedThreads = 64;
            adjustedPoints = 32;
            keysPerLaunch = (unsigned long long)adjustedBlocks * adjustedThreads * adjustedPoints;
            std::cout << "[GPU] Small range detected (" << rangeSize << " keys), using reduced grid: " 
                      << adjustedBlocks << "x" << adjustedThreads << "x" << adjustedPoints 
                      << " = " << keysPerLaunch << " keys/launch\n\n";
        }
        
        // Main search loop
        while(currentKey < endKey && running && !found) {
            // Reset found count
            cudaMemset(d_foundCount, 0, sizeof(int));
            
            // Convert current key to uint32_t array (correct byte order)
            // Uint256 uses uint64_t[4] with LSW at index 0
            // GPU u256 expects uint32_t[8] with LSW at index 0
            uint32_t startKeyData[8];
            uint32_t endKeyData[8];
            
            const uint64_t* start64 = (const uint64_t*)currentKey.getData();
            const uint64_t* end64 = (const uint64_t*)endKey.getData();
            
            // Convert each uint64_t to two uint32_t (little-endian)
            for(int i = 0; i < 4; i++) {
                startKeyData[i*2]   = (uint32_t)(start64[i] & 0xFFFFFFFF);
                startKeyData[i*2+1] = (uint32_t)(start64[i] >> 32);
                endKeyData[i*2]     = (uint32_t)(end64[i] & 0xFFFFFFFF);
                endKeyData[i*2+1]   = (uint32_t)(end64[i] >> 32);
            }
            
            // Debug: Print what we're sending
            std::cout << "[Host] startKeyData: ";
            for(int i = 7; i >= 0; i--) {
                std::cout << std::hex << std::setw(8) << std::setfill('0') << startKeyData[i];
            }
            std::cout << std::dec << "\n";
            
            // Launch kernel with adjusted parameters
            launch_gpu_pubkey_search(
                startKeyData,
                endKeyData,
                targetHash160,
                adjustedBlocks,
                adjustedThreads,
                adjustedPoints,
                d_foundKeysLow,
                d_foundKeysHigh,
                d_foundCount,
                1024
            );
            
            // Check for CUDA errors
            cudaError_t err = cudaGetLastError();
            if(err != cudaSuccess) {
                std::cerr << "[GPU ERROR] " << cudaGetErrorString(err) << "\n";
                break;
            }
            
            // Check if key was found
            int h_foundCount = 0;
            cudaMemcpy(&h_foundCount, d_foundCount, sizeof(int), cudaMemcpyDeviceToHost);
            
            if(h_foundCount > 0) {
                unsigned long long h_keyLow, h_keyHigh;
                cudaMemcpy(&h_keyLow, d_foundKeysLow, sizeof(unsigned long long), cudaMemcpyDeviceToHost);
                cudaMemcpy(&h_keyHigh, d_foundKeysHigh, sizeof(unsigned long long), cudaMemcpyDeviceToHost);
                
                unsigned char privKey[32] = {0};
                memcpy(privKey + 16, &h_keyHigh, 8);
                memcpy(privKey + 24, &h_keyLow, 8);
                
                found = true;
                foundKey = Uint256::fromBytes(privKey);
                break;
            }
            
            // Update statistics
            unsigned long long actualKeysPerLaunch = (unsigned long long)adjustedBlocks * 
                                                      adjustedThreads * adjustedPoints;
            totalChecked += actualKeysPerLaunch;
            
            // Advance current key
            for(unsigned long long i = 0; i < actualKeysPerLaunch && currentKey < endKey; i++) {
                currentKey.increment();
            }
            
            // Print progress every second
            auto now = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration<double>(now - lastPrintTime).count();
            if(elapsed >= 1.0) {
                double totalElapsed = std::chrono::duration<double>(now - startTime).count();
                double speed = totalChecked / totalElapsed;
                
                std::cout << "\r[GPU] Checked: " << totalChecked 
                          << " | Speed: " << std::fixed << std::setprecision(0) << speed << " keys/s"
                          << " | Current: 0x" << currentKey.toHex()
                          << std::flush;
                
                lastPrintTime = now;
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        double totalElapsed = std::chrono::duration<double>(endTime - startTime).count();
        
        std::cout << "\n\n=== GPU Search Complete ===\n";
        std::cout << "Total Keys Checked: " << totalChecked << "\n";
        std::cout << "Time: " << totalElapsed << " seconds\n";
        if(totalElapsed > 0) {
            std::cout << "Average Speed: " << (totalChecked / totalElapsed) << " keys/second\n";
        }
        
        if(found) {
            std::cout << "\n*** KEY FOUND! ***\n";
            unsigned char privKey[32];
            foundKey.toPrivateKey(privKey);
            std::string privHex = BytesToHex(std::vector<uint8_t>(privKey, privKey + 32));
            std::string wif = privKeyToWIF(privKey, false);
            
            std::cout << "Private Key (HEX): " << privHex << "\n";
            std::cout << "Private Key (WIF): " << wif << "\n";
            std::cout << "********************\n";
            
            // Save to file
            std::ofstream outFile("pubkey_found.txt", std::ios::app);
            if(outFile.is_open()) {
                outFile << "=== PUBKEY MODE HIT ===\n";
                outFile << "Target: " << targetAddr << "\n";
                outFile << "Private Key (HEX): " << privHex << "\n";
                outFile << "Private Key (WIF): " << wif << "\n";
                outFile << "===================\n\n";
            }
        } else {
            std::cout << "Result: KEY NOT FOUND\n";
        }
        std::cout << "\n";
        
        cudaFree(d_foundKeysLow);
        cudaFree(d_foundKeysHigh);
        cudaFree(d_foundCount);
    }
#endif
    
    void run() {
        // Validate configuration
        if(cfg.pubAddress.empty()) {
            std::cerr << "[ERROR] Pubkey mode requires --pub-address <target_address>\n";
            return;
        }
        if(cfg.pubBitEnd <= 0) {
            std::cerr << "[ERROR] Pubkey mode requires --pub-bit <N> or --pub-bit-end <N>\n";
            return;
        }
        if(cfg.pubBitEnd > 256) {
            std::cerr << "[ERROR] Maximum supported bit range is 256\n";
            return;
        }
        
        // Detect address type
        std::string targetAddr = cfg.pubAddress;
        std::string addrType = cfg.pubKeyType;
        if(addrType == "auto") {
            addrType = detectAddressType(targetAddr);
            if(addrType.empty()) {
                std::cerr << "[ERROR] Cannot auto-detect address type for: " << targetAddr << "\n";
                return;
            }
        }
        
        std::cout << "\n=== PUBKEY MODE (Public Key Recovery) ===\n";
        std::cout << "Target Address: " << targetAddr << "\n";
        std::cout << "Address Type:   " << addrType << "\n";
        std::cout << "Bit Range:      " << cfg.pubBitStart << " - " << cfg.pubBitEnd << " bits\n";
        
        // Calculate search range
        Uint256 startKey = Uint256::fromBit(cfg.pubBitStart);
        if(cfg.pubBitStart == 0) startKey = Uint256(1);
        Uint256 endKey = Uint256::fromBit(cfg.pubBitEnd);
        
        std::cout << "Start Key:      0x" << startKey.toHex() << "\n";
        std::cout << "End Key:        0x" << endKey.toHex() << "\n";
        std::cout << "Search Space:   2^" << cfg.pubBitEnd << " keys\n";
        
        // Determine execution mode
        bool useGPU = false;
        int numThreads = cfg.cpuCores;
        if(numThreads <= 0) {
            numThreads = std::thread::hardware_concurrency();
            if(numThreads == 0) numThreads = 4;
        }
        
#ifdef USE_CUDA
        if(cfg.gpuType == "cuda" || cfg.gpuType == "auto") {
            int deviceCount = 0;
            cudaError_t err = cudaGetDeviceCount(&deviceCount);
            if(err == cudaSuccess && deviceCount > 0) {
                useGPU = true;
                std::cout << "Mode:           GPU (CUDA)\n";
                std::cout << "GPU Devices:    " << deviceCount << "\n";
            } else {
                std::cout << "Mode:           CPU (CUDA not available, " << numThreads << " threads)\n";
            }
        } else {
            std::cout << "Mode:           CPU (" << numThreads << " threads)\n";
        }
#else
        std::cout << "Mode:           CPU (" << numThreads << " threads) [CUDA not compiled]\n";
#endif
        std::cout << "\n";
        
        // Run search
#ifdef USE_CUDA
        if(useGPU) {
            runGPU(targetAddr, addrType, startKey, endKey);
        } else {
            runCPU(targetAddr, addrType, startKey, endKey);
        }
#else
        runCPU(targetAddr, addrType, startKey, endKey);
#endif
    }
};
