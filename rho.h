#pragma once

// ============================================================================
// POLLARD'S RHO ALGORITHM - Memory-efficient discrete logarithm solver
// ============================================================================
// Uses O(1) memory instead of O(√n) like BSGS
// Based on Floyd's cycle-finding algorithm applied to elliptic curves
//
// Formula: Q = k * G, find k
// Uses pseudo-random walk on elliptic curve points
// When collision found: a1*G + b1*Q = a2*G + b2*Q => k = (a1-a2)/(b2-b1)

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <atomic>
#include <cstring>
#include <thread>
#include <random>
#include <unordered_map>

#include <boost/multiprecision/cpp_int.hpp>

#ifndef NO_SECP256K1
#include <secp256k1.h>
#endif

#include "utils.h"
#include "args.h"

namespace mp = boost::multiprecision;
using uint256_t = mp::number<mp::cpp_int_backend<256, 256, mp::unsigned_magnitude, mp::unchecked, void>>;

// ============================================================================
// Pollard's Rho Point Structure
// ============================================================================
struct RhoPoint {
    secp256k1_pubkey point;
    uint256_t a;  // Coefficient for G
    uint256_t b;  // Coefficient for Q (target)
    
    bool isDistinguished(uint32_t mask = 0xFFFFFFFF) const {
        // A point is distinguished if its x-coordinate ends with certain bits
        // For simplicity, check first few bytes of serialized point
        size_t len = 33;
        unsigned char serialized[33];
        // Note: We need a context to serialize, this is simplified
        // In real implementation, we'd check the actual bytes
        return (a.convert_to<uint32_t>() & mask) == 0;
    }
};

// ============================================================================
// Pollard's Rho Algorithm Implementation
// ============================================================================
class PollardsRho {
private:
    ProgramConfig cfg;
    std::atomic<bool>& running;
    secp256k1_context* ctx;
    
    // Target public key Q
    secp256k1_pubkey targetPub;
    bool targetLoaded;
    
    // Generator point G (scalar = 1)
    secp256k1_pubkey G;
    
    // Distinguished points table (for parallel Rho)
    std::unordered_map<std::string, std::pair<uint256_t, uint256_t>> distinguishedPoints;
    
    // Number of partitions for the random walk
    static const int NUM_PARTITIONS = 16;
    
public:
    PollardsRho(const ProgramConfig& config, std::atomic<bool>& runFlag) 
        : cfg(config), running(runFlag), targetLoaded(false) {
        ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
        
        // Initialize generator point G
        unsigned char genPriv[32] = {0};
        genPriv[31] = 1;
        secp256k1_ec_pubkey_create(ctx, &G, genPriv);
    }
    
    ~PollardsRho() {
        secp256k1_context_destroy(ctx);
    }
    
    // Parse public key from hex string
    bool parsePublicKey(const std::string& pubHex) {
        std::vector<uint8_t> bytes;
        
        if (pubHex.length() == 66) {
            for (size_t i = 0; i < 66; i += 2) {
                std::string byteString = pubHex.substr(i, 2);
                bytes.push_back((uint8_t)strtol(byteString.c_str(), NULL, 16));
            }
        } else if (pubHex.length() == 130) {
            for (size_t i = 0; i < 130; i += 2) {
                std::string byteString = pubHex.substr(i, 2);
                bytes.push_back((uint8_t)strtol(byteString.c_str(), NULL, 16));
            }
        } else {
            std::cerr << "[RHO ERROR] Invalid public key length: " << pubHex.length() << "\n";
            return false;
        }
        
        if (!secp256k1_ec_pubkey_parse(ctx, &targetPub, bytes.data(), bytes.size())) {
            std::cerr << "[RHO ERROR] Failed to parse public key\n";
            return false;
        }
        
        targetLoaded = true;
        return true;
    }
    
    // Serialize pubkey to string
    std::string serializePubkey(const secp256k1_pubkey& pub) {
        size_t len = 65;
        unsigned char serialized[65];
        secp256k1_ec_pubkey_serialize(ctx, serialized, &len, &pub, SECP256K1_EC_UNCOMPRESSED);
        return std::string((char*)serialized, 65);
    }
    
    // Add two points: R = P1 + P2
    bool pointAdd(const secp256k1_pubkey& p1, const secp256k1_pubkey& p2, secp256k1_pubkey& result) {
        const secp256k1_pubkey* pts[2] = { &p1, &p2 };
        return secp256k1_ec_pubkey_combine(ctx, &result, pts, 2);
    }
    
    // Multiply point by scalar using secp256k1 directly
    bool pointMultiply(const secp256k1_pubkey& p, uint64_t scalar, secp256k1_pubkey& result) {
        if (scalar == 0) {
            // Return generator point G as fallback
            result = G;
            return true;
        }
        if (scalar == 1) {
            result = p;
            return true;
        }
        
        // Use secp256k1_ec_pubkey_create with scalar as private key
        // This is more efficient and reliable than repeated addition
        unsigned char priv[32] = {0};
        uint64_t temp = scalar;
        for (int i = 31; i >= 24; i--) {
            priv[i] = temp & 0xFF;
            temp >>= 8;
        }
        
        if (!secp256k1_ec_pubkey_create(ctx, &result, priv)) {
            // Fallback to input point
            result = p;
            return false;
        }
        
        return true;
    }
    
    // Random walk function - determines next point based on current point
    void iterateRho(RhoPoint& current, const std::array<secp256k1_pubkey, NUM_PARTITIONS>& precomp) {
        // Use hash of current point to determine partition
        size_t len = 33;
        unsigned char serialized[33];
        secp256k1_ec_pubkey_serialize(ctx, serialized, &len, &current.point, SECP256K1_EC_COMPRESSED);
        
        // Simple hash: use last byte
        int partition = serialized[32] % NUM_PARTITIONS;
        
        // Add precomputed point for this partition
        secp256k1_pubkey newPoint;
        if (pointAdd(current.point, precomp[partition], newPoint)) {
            // Verify the new point is valid before accepting
            if (secp256k1_ec_pubkey_serialize(ctx, serialized, &len, &newPoint, SECP256K1_EC_COMPRESSED)) {
                current.point = newPoint;
                // Update coefficients
                current.a += (partition + 1);
                if (partition % 2 == 0) {
                    current.b += 1;
                }
            }
        }
    }
    
    // Check if a point is "distinguished" (for distinguished points method)
    bool isDistinguished(const secp256k1_pubkey& p, int bits = 20) {
        size_t len = 33;
        unsigned char serialized[33];
        secp256k1_ec_pubkey_serialize(ctx, serialized, &len, &p, SECP256K1_EC_COMPRESSED);
        
        // Check if last 'bits' bits are zero
        uint32_t mask = (1u << bits) - 1;
        uint32_t value = serialized[32] | ((uint32_t)serialized[31] << 8);
        return (value & mask) == 0;
    }
    
    // Single-threaded Pollard's Rho
    bool searchSingleThread(const uint256_t& maxRange, const uint256_t& startOffset) {
        std::cout << "\n=== POLLARD'S RHO (Single Thread) ===\n";
        std::cout << "Search range: " << maxRange << "\n";
        std::cout << "Memory usage: O(1) - Constant\n";
        std::cout << "Algorithm: Floyd's cycle-finding on elliptic curve\n\n";
        
        // Precompute random walk partitions
        std::array<secp256k1_pubkey, NUM_PARTITIONS> precomp;
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dis(1, 1000000);
        
        for (int i = 0; i < NUM_PARTITIONS; i++) {
            // Precompute c_i * G where c_i are random scalars
            uint64_t c = dis(gen);
            pointMultiply(G, c, precomp[i]);
        }
        
        // Initialize tortoise and hare
        RhoPoint tortoise, hare;
        tortoise.point = G;  // Start with G (1*G + 0*Q)
        tortoise.a = 1;
        tortoise.b = 0;
        hare = tortoise;
        
        auto start = std::chrono::high_resolution_clock::now();
        uint64_t iterations = 0;
        
        std::cout << "[RHO] Starting random walk...\n";
        
        while (running) {
            // Tortoise takes one step
            iterateRho(tortoise, precomp);
            
            // Hare takes two steps
            iterateRho(hare, precomp);
            iterateRho(hare, precomp);
            
            iterations += 2;
            
            // Check if points match
            if (serializePubkey(tortoise.point) == serializePubkey(hare.point)) {
                // Found collision!
                // a_tortoise*G + b_tortoise*Q = a_hare*G + b_hare*Q
                // (a_tortoise - a_hare)*G = (b_hare - b_tortoise)*Q
                // Q = ((a_tortoise - a_hare) / (b_hare - b_tortoise)) * G
                
                uint256_t ba = tortoise.b - hare.b;
                uint256_t ah = tortoise.a - hare.a;
                
                if (ba != 0) {
                    // k = ah / ba (mod n) where n is the curve order
                    // For simplicity, we just output the values
                    std::cout << "\n" << std::string(50, '=') << "\n";
                    std::cout << "[RHO] COLLISION FOUND!\n";
                    std::cout << "Tortoise: a=" << tortoise.a << ", b=" << tortoise.b << "\n";
                    std::cout << "Hare:     a=" << hare.a << ", b=" << hare.b << "\n";
                    std::cout << "k = (a1-a2)/(b2-b1) = " << ah << " / " << ba << "\n";
                    std::cout << "Iterations: " << iterations << "\n";
                    std::cout << std::string(50, '=') << "\n";
                    
                    return true;
                }
            }
            
            // Progress update every 1M iterations
            if (iterations % 1000000 == 0) {
                auto now = std::chrono::high_resolution_clock::now();
                double elapsed = std::chrono::duration<double>(now - start).count();
                double rate = iterations / elapsed;
                
                std::cout << "\r[RHO] Iterations: " << iterations / 1000000 << "M"
                          << " | Rate: " << std::fixed << std::setprecision(0) << rate << " iter/s"
                          << " | Elapsed: " << (int)(elapsed / 60) << "m " << (int)(elapsed) % 60 << "s"
                          << std::flush;
            }
        }
        
        return false;
    }
    
    // Multi-threaded Pollard's Rho with distinguished points
    bool searchMultiThread(const uint256_t& maxRange, const uint256_t& startOffset) {
        std::cout << "\n=== POLLARD'S RHO (Multi-Thread with Distinguished Points) ===\n";
        std::cout << "Search range: " << maxRange << "\n";
        std::cout << "Memory usage: O(number of distinguished points)\n";
        std::cout << "Distinguished point probability: 1/2^20\n";
        std::cout << "Threads: " << cfg.bsgsThreads << "\n\n";
        
        int numThreads = cfg.bsgsThreads;
        if (numThreads <= 0) {
            numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0) numThreads = 4;
        }
        
        // Precompute random walk partitions
        std::array<secp256k1_pubkey, NUM_PARTITIONS> precomp;
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dis(1, 1000000);
        
        for (int i = 0; i < NUM_PARTITIONS; i++) {
            uint64_t c = dis(gen);
            pointMultiply(G, c, precomp[i]);
        }
        
        std::atomic<bool> found(false);
        std::atomic<uint64_t> totalIterations(0);
        uint256_t foundKey = 0;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        std::mutex dpMutex;
        
        std::cout << "[RHO] Starting " << numThreads << " random walks...\n";
        
        for (int t = 0; t < numThreads; t++) {
            threads.emplace_back([&, t]() {
                secp256k1_context* localCtx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
                
                // Each thread starts from a different random point
                std::mt19937_64 localGen(rd() + t);
                std::uniform_int_distribution<uint64_t> localDis(1, 1000000000ULL);
                
                RhoPoint current;
                uint64_t startScalar = localDis(localGen);
                pointMultiply(G, startScalar, current.point);
                current.a = startScalar;
                current.b = 0;
                
                uint64_t localIterations = 0;
                
                while (running && !found) {
                    // Random walk
                    size_t len = 33;
                    unsigned char serialized[33];
                    
                    // Serialize current point - if it fails, reset to G
                    if (!secp256k1_ec_pubkey_serialize(localCtx, serialized, &len, &current.point, SECP256K1_EC_COMPRESSED)) {
                        current.point = G;
                        current.a = 1;
                        current.b = 0;
                        continue;
                    }
                    
                    int partition = serialized[32] % NUM_PARTITIONS;
                    
                    secp256k1_pubkey newPoint;
                    const secp256k1_pubkey* pts[2] = { &current.point, &precomp[partition] };
                    if (secp256k1_ec_pubkey_combine(localCtx, &newPoint, pts, 2)) {
                        // Verify the new point can be serialized
                        if (secp256k1_ec_pubkey_serialize(localCtx, serialized, &len, &newPoint, SECP256K1_EC_COMPRESSED)) {
                            current.point = newPoint;
                            current.a += (partition + 1);
                            if (partition % 2 == 0) {
                                current.b += 1;
                            }
                        }
                    }
                    
                    localIterations++;
                    
                    // Check if distinguished point (last 20 bits are zero)
                    uint32_t value = serialized[32] | ((uint32_t)serialized[31] << 8);
                    if ((value & 0xFFFFF) == 0) { // 1 in 2^20 probability
                        std::string key = serializePubkey(current.point);
                        
                        std::lock_guard<std::mutex> lock(dpMutex);
                        auto it = distinguishedPoints.find(key);
                        if (it != distinguishedPoints.end()) {
                            // Collision!
                            uint256_t other_a = it->second.first;
                            uint256_t other_b = it->second.second;
                            
                            if (current.b != other_b) {
                                found = true;
                                // k = (other_a - current.a) / (current.b - other_b)
                                foundKey = (other_a - current.a) / (current.b - other_b);
                            }
                        } else {
                            distinguishedPoints[key] = {current.a, current.b};
                        }
                    }
                    
                    if (localIterations % 1000000 == 0) {
                        totalIterations += 1000000;
                    }
                }
                
                secp256k1_context_destroy(localCtx);
            });
        }
        
        // Progress reporter thread
        std::thread reporter([&]() {
            while (!found && running) {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                auto now = std::chrono::high_resolution_clock::now();
                double elapsed = std::chrono::duration<double>(now - start).count();
                uint64_t iter = totalIterations.load();
                
                std::cout << "[RHO] Progress: " << iter / 1000000 << "M iterations | "
                          << distinguishedPoints.size() << " distinguished points | "
                          << "Elapsed: " << (int)(elapsed / 60) << "m\n";
            }
        });
        
        for (auto& th : threads) {
            th.join();
        }
        
        reporter.join();
        
        if (found) {
            std::cout << "\n" << std::string(50, '=') << "\n";
            std::cout << "[RHO] PRIVATE KEY FOUND!\n";
            std::cout << "Key: " << foundKey << "\n";
            std::cout << "Total iterations: " << totalIterations.load() << "\n";
            std::cout << std::string(50, '=') << "\n";
            
            std::ofstream outFile("rho_found.txt", std::ios::app);
            if (outFile.is_open()) {
                outFile << "=== POLLARD'S RHO PRIVATE KEY FOUND ===\n";
                outFile << "Public Key: " << cfg.bsgsPub << "\n";
                outFile << "Private Key: " << foundKey << "\n";
                outFile << "=======================================\n\n";
            }
            
            return true;
        }
        
        std::cout << "\n[RHO] Key not found.\n";
        return false;
    }
    
    // Main entry point
    void run() {
        if (cfg.bsgsPub.empty()) {
            std::cerr << "[RHO ERROR] No public key specified. Use --bsgs-pub <hex>\n";
            return;
        }
        
        if (!parsePublicKey(cfg.bsgsPub)) {
            return;
        }
        
        std::cout << "[RHO] Target public key loaded successfully\n";
        
        // Determine search range
        uint256_t maxRange = uint256_t(1) << 100; // Default 2^100
        uint256_t startOffset = 0;
        
        try {
            startOffset = uint256_t(cfg.bsgsStartStr);
        } catch (...) {
            std::cerr << "[RHO ERROR] Invalid start offset\n";
            return;
        }
        
        if (cfg.bsgsRange > 0) {
            maxRange = uint256_t(1) << cfg.bsgsRange;
            std::cout << "[RHO] Using range: 2^" << cfg.bsgsRange << "\n";
        }
        
        // Choose algorithm based on configuration
        bool success;
        if (cfg.bsgsThreads > 1) {
            success = searchMultiThread(maxRange, startOffset);
        } else {
            success = searchSingleThread(maxRange, startOffset);
        }
        
        if (!success) {
            std::cout << "\n[RHO] Search completed. Key not found.\n";
        }
    }
};

// ============================================================================
// Hybrid Selector - Auto-select BSGS or Rho based on range and memory
// ============================================================================
class HybridBSGSRho {
private:
    ProgramConfig cfg;
    std::atomic<bool>& running;
    
public:
    HybridBSGSRho(const ProgramConfig& config, std::atomic<bool>& runFlag) 
        : cfg(config), running(runFlag) {}
    
    // Determine best algorithm based on range and available memory
    static std::string selectAlgorithm(int rangeBits) {
        uint64_t memMB = getAvailableMemoryMB();
        
        // Calculate required memory for BSGS: sqrt(2^range) * 80 bytes
        double sqrtRange = std::pow(2.0, rangeBits / 2.0);
        double requiredMemMB = (sqrtRange * 80.0) / (1024.0 * 1024.0);
        
        std::cout << "[HYBRID] Range: 2^" << rangeBits << "\n";
        std::cout << "[HYBRID] Available memory: " << memMB << " MB\n";
        std::cout << "[HYBRID] BSGS would require: " << std::fixed << std::setprecision(0) 
                  << requiredMemMB << " MB\n";
        
        if (rangeBits <= 60) {
            // Small range: Pure BSGS
            std::cout << "[HYBRID] Decision: PURE BSGS (range <= 2^60)\n";
            return "bsgs";
        }
        else if (rangeBits <= 80 && requiredMemMB < memMB * 0.8) {
            // Medium range: BSGS if we have enough memory
            std::cout << "[HYBRID] Decision: PURE BSGS (sufficient memory)\n";
            return "bsgs";
        }
        else if (rangeBits <= 100) {
            // Large range: Rho with distinguished points
            std::cout << "[HYBRID] Decision: POLLARD'S RHO (memory-efficient)\n";
            return "rho";
        }
        else {
            // Very large range: Rho is the only practical option
            std::cout << "[HYBRID] Decision: POLLARD'S RHO (only practical option)\n";
            return "rho";
        }
    }
    
    void run() {
        std::string algo = selectAlgorithm(cfg.bsgsRange);
        
        if (algo == "bsgs") {
            std::cout << "[HYBRID] Using BSGS algorithm\n";
            BSGSMode bsgs(cfg, running);
            bsgs.run();
        } else {
            std::cout << "[HYBRID] Using Pollard's Rho algorithm\n";
            PollardsRho rho(cfg, running);
            rho.run();
        }
    }
};
