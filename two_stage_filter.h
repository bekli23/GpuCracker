#pragma once
#include "bloom.h"
#include <unordered_set>
#include <vector>
#include <cstdint>
#include <string>
#include <mutex>
#include <fstream>
#include "logger.h"

// =============================================================
// TWO-STAGE FILTER: Bloom (fast) + HashSet (accurate)
// Reduces false positives to effectively zero
// =============================================================

class TwoStageFilter {
private:
    BloomFilter bloomFilter;
    std::unordered_set<std::string> confirmedSet;
    std::mutex setMutex;
    bool useSecondStage = true;
    size_t maxSetSize = 10000000; // 10M entries max in RAM
    std::string secondaryFilterFile;
    
    // Convert hash160 to string key
    std::string hashToKey(const std::vector<uint8_t>& hash160) {
        return std::string(hash160.begin(), hash160.end());
    }
    
    std::string hashToHex(const std::vector<uint8_t>& hash) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (auto b : hash) ss << std::setw(2) << (int)b;
        return ss.str();
    }

public:
    TwoStageFilter(bool useConfirmSet = true, size_t maxConfirmEntries = 10000000)
        : useSecondStage(useConfirmSet), maxSetSize(maxConfirmEntries) 
    {
    }
    
    void setSecondaryFile(const std::string& file) {
        secondaryFilterFile = file;
    }
    
    // Load bloom filter (first stage)
    bool loadBloom(const std::vector<std::string>& files) {
        return bloomFilter.load(files);
    }
    
    bool loadBloom(const std::string& file) {
        return bloomFilter.load(file);
    }
    
    // Load confirmed set from file (second stage)
    bool loadConfirmedSet(const std::string& file) {
        if (!useSecondStage) return false;
        
        std::ifstream infile(file, std::ios::binary);
        if (!infile.is_open()) {
            LOG_WARN("Filter", "Confirmed set file not found: " + file);
            return false;
        }
        
        LOG_INFO("Filter", "Loading confirmed set from: " + file);
        
        std::lock_guard<std::mutex> lock(setMutex);
        confirmedSet.clear();
        
        std::vector<uint8_t> buffer(20);
        size_t count = 0;
        while (infile.read(reinterpret_cast<char*>(buffer.data()), 20)) {
            confirmedSet.insert(hashToKey(buffer));
            count++;
            if (count % 1000000 == 0) {
                LOG_INFO("Filter", "Loaded " + std::to_string(count) + " confirmed hashes...");
            }
        }
        
        LOG_INFO("Filter", "Loaded " + std::to_string(confirmedSet.size()) + " confirmed hashes");
        return true;
    }
    
    // Save confirmed set to file
    bool saveConfirmedSet(const std::string& file) {
        if (!useSecondStage) return false;
        
        std::lock_guard<std::mutex> lock(setMutex);
        
        std::ofstream outfile(file, std::ios::binary);
        if (!outfile.is_open()) {
            LOG_ERROR("Filter", "Cannot write confirmed set to: " + file);
            return false;
        }
        
        for (const auto& key : confirmedSet) {
            outfile.write(key.data(), key.length());
        }
        
        LOG_INFO("Filter", "Saved " + std::to_string(confirmedSet.size()) + " confirmed hashes");
        return true;
    }
    
    // Add to confirmed set
    void addConfirmed(const std::vector<uint8_t>& hash160) {
        if (!useSecondStage) return;
        
        std::lock_guard<std::mutex> lock(setMutex);
        if (confirmedSet.size() < maxSetSize) {
            confirmedSet.insert(hashToKey(hash160));
        }
    }
    
    // Check hash160 - Two stage process
    // Returns: 0 = Not found, 1 = Maybe (Bloom only), 2 = Confirmed (in set)
    int check(const std::vector<uint8_t>& hash160) {
        // Stage 1: Fast bloom filter check
        if (!bloomFilter.check_hash160(hash160)) {
            return 0; // Definitely not present
        }
        
        // Stage 2: Accurate check (if enabled)
        if (useSecondStage) {
            std::lock_guard<std::mutex> lock(setMutex);
            if (confirmedSet.find(hashToKey(hash160)) != confirmedSet.end()) {
                return 2; // Confirmed match
            }
        }
        
        return 1; // Maybe (Bloom says yes, set says no or not using set)
    }
    
    // Convenience method - returns true only if confirmed
    bool checkConfirmed(const std::vector<uint8_t>& hash160) {
        return check(hash160) == 2;
    }
    
    // Check if bloom filter is loaded
    bool isLoaded() const {
        return bloomFilter.isLoaded();
    }
    
    // Get stats
    size_t getBloomSize() const {
        return bloomFilter.getLayerCount();
    }
    
    size_t getConfirmedCount() const {
        if (!useSecondStage) return 0;
        std::lock_guard<std::mutex> lock(setMutex);
        return confirmedSet.size();
    }
    
    // Disable second stage (bloom only mode)
    void disableSecondStage() {
        useSecondStage = false;
        std::lock_guard<std::mutex> lock(setMutex);
        confirmedSet.clear();
    }
    
    // Enable second stage
    void enableSecondStage() {
        useSecondStage = true;
    }
    
    // Export stats as JSON
    std::string getStatsJson() {
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"bloom_layers\": " << getBloomSize() << ",\n";
        ss << "  \"confirmed_entries\": " << getConfirmedCount() << ",\n";
        ss << "  \"second_stage_enabled\": " << (useSecondStage ? "true" : "false") << "\n";
        ss << "}";
        return ss.str();
    }
    
    // For backward compatibility - expose underlying bloom filter
    BloomFilter& getBloomFilter() { return bloomFilter; }
};

// Global instance
extern TwoStageFilter g_filter;
