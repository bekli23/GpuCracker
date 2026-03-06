#pragma once

// ============================================================================
// BSGS BLOOM FILTER - Probabilistic data structure for memory-efficient lookup
// ============================================================================
// Uses 10x less memory than hash table with <1% false positive rate
// False positives are handled by verification

#include <vector>
#include <functional>
#include <cstdint>
#include <cmath>
#include <string>

class BSGSBloomFilter {
private:
    std::vector<uint8_t> bits;
    size_t numBits;
    size_t numHashFunctions;
    size_t numElements;
    
    // Simple hash functions
    static uint64_t hash1(const std::string& key) {
        uint64_t hash = 0xcbf29ce484222325;
        for (char c : key) {
            hash ^= static_cast<uint8_t>(c);
            hash *= 0x100000001b3;
        }
        return hash;
    }
    
    static uint64_t hash2(const std::string& key) {
        uint64_t hash = 0x14650fb123456789;
        for (char c : key) {
            hash = (hash << 5) + hash + static_cast<uint8_t>(c);
        }
        return hash;
    }
    
    static uint64_t hash3(const std::string& key) {
        uint64_t hash = 0xdeadbeefcafebabe;
        for (size_t i = 0; i < key.size(); i++) {
            hash ^= static_cast<uint8_t>(key[i]) << ((i % 8) * 8);
            hash = (hash << 13) | (hash >> 51);
        }
        return hash;
    }
    
    std::vector<size_t> getPositions(const std::string& key) const {
        std::vector<size_t> positions;
        uint64_t h1 = hash1(key);
        uint64_t h2 = hash2(key);
        uint64_t h3 = hash3(key);
        
        positions.push_back(h1 % numBits);
        positions.push_back(h2 % numBits);
        positions.push_back(h3 % numBits);
        
        // Additional hashes if needed
        for (size_t i = 3; i < numHashFunctions; i++) {
            positions.push_back((h1 + i * h2 + i * i * h3) % numBits);
        }
        
        return positions;
    }
    
public:
    BSGSBloomFilter(size_t expectedElements, double falsePositiveRate = 0.01) 
        : numElements(0) {
        // Calculate optimal size and number of hash functions
        // m = -n * ln(p) / (ln(2)^2)
        // k = m/n * ln(2)
        
        double ln2 = std::log(2.0);
        numBits = static_cast<size_t>(-static_cast<double>(expectedElements) * 
                                      std::log(falsePositiveRate) / (ln2 * ln2));
        numHashFunctions = static_cast<size_t>(static_cast<double>(numBits) / 
                                               static_cast<double>(expectedElements) * ln2);
        
        // Ensure at least 3 hash functions and at most 10
        if (numHashFunctions < 3) numHashFunctions = 3;
        if (numHashFunctions > 10) numHashFunctions = 10;
        
        // Round up to nearest byte
        size_t numBytes = (numBits + 7) / 8;
        bits.resize(numBytes, 0);
    }
    
    // Add element to filter
    void add(const std::string& key) {
        for (size_t pos : getPositions(key)) {
            bits[pos / 8] |= (1 << (pos % 8));
        }
        numElements++;
    }
    
    // Check if element might be in set (may return false positives)
    bool possiblyContains(const std::string& key) const {
        for (size_t pos : getPositions(key)) {
            if ((bits[pos / 8] & (1 << (pos % 8))) == 0) {
                return false; // Definitely not in set
            }
        }
        return true; // Possibly in set (may be false positive)
    }
    
    // Get memory usage in bytes
    size_t getMemoryUsage() const {
        return bits.size();
    }
    
    // Get number of elements added
    size_t getNumElements() const {
        return numElements;
    }
    
    // Get number of bits
    size_t getNumBits() const {
        return numBits;
    }
    
    // Get number of hash functions
    size_t getNumHashFunctions() const {
        return numHashFunctions;
    }
    
    // Estimated false positive rate
    double getFalsePositiveRate() const {
        return std::pow(1.0 - std::exp(-static_cast<double>(numHashFunctions) * 
                                       static_cast<double>(numElements) / 
                                       static_cast<double>(numBits)), 
                        static_cast<double>(numHashFunctions));
    }
    
    // Clear all elements
    void clear() {
        std::fill(bits.begin(), bits.end(), 0);
        numElements = 0;
    }
};
