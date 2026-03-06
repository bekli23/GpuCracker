#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <cstring>
#include "logger.h"

// =============================================================
// CHECKPOINT MANAGER - Resume Capability with SQLite-like format
// =============================================================

struct SearchState {
    std::string mode;
    unsigned long long seedsChecked;
    unsigned long long currentOffset;
    std::string lastEntropy;
    std::string startTime;
    std::string lastSaveTime;
    std::vector<int> targetBits;
    std::string profile;
    int words;
    bool running;
    
    // Constructor with defaults
    SearchState() : seedsChecked(0), currentOffset(0), words(12), running(true) {}
};

class CheckpointManager {
private:
    std::string checkpointFile;
    int saveIntervalSeconds;
    std::chrono::time_point<std::chrono::steady_clock> lastSave;
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    // Simple serialization format (key=value pairs)
    std::string serialize(const SearchState& state) {
        std::stringstream ss;
        ss << "[Checkpoint]\n";
        ss << "version=2\n";
        ss << "mode=" << state.mode << "\n";
        ss << "seeds_checked=" << state.seedsChecked << "\n";
        ss << "current_offset=" << state.currentOffset << "\n";
        ss << "last_entropy=" << state.lastEntropy << "\n";
        ss << "start_time=" << state.startTime << "\n";
        ss << "last_save=" << getCurrentTimestamp() << "\n";
        ss << "profile=" << state.profile << "\n";
        ss << "words=" << state.words << "\n";
        ss << "running=" << (state.running ? "1" : "0") << "\n";
        ss << "target_bits=";
        for (size_t i = 0; i < state.targetBits.size(); ++i) {
            ss << state.targetBits[i];
            if (i < state.targetBits.size() - 1) ss << ",";
        }
        ss << "\n";
        return ss.str();
    }

public:
    CheckpointManager(const std::string& file = "checkpoint.dat", int intervalSec = 30)
        : checkpointFile(file), saveIntervalSeconds(intervalSec) 
    {
        lastSave = std::chrono::steady_clock::now();
    }
    
    void setFile(const std::string& file) {
        checkpointFile = file;
    }
    
    bool save(const SearchState& state) {
        // Create backup of existing checkpoint
        if (std::ifstream(checkpointFile).good()) {
            std::string backupFile = checkpointFile + ".bak";
            std::ifstream src(checkpointFile, std::ios::binary);
            std::ofstream dst(backupFile, std::ios::binary);
            dst << src.rdbuf();
        }
        
        std::ofstream file(checkpointFile);
        if (!file.is_open()) {
            LOG_ERROR("Checkpoint", "Failed to save checkpoint to: " + checkpointFile);
            return false;
        }
        
        SearchState stateToSave = state;
        if (stateToSave.startTime.empty()) {
            stateToSave.startTime = getCurrentTimestamp();
        }
        
        file << serialize(stateToSave);
        file.close();
        
        lastSave = std::chrono::steady_clock::now();
        LOG_DEBUG("Checkpoint", "Saved checkpoint: offset=" + std::to_string(state.currentOffset));
        return true;
    }
    
    bool load(SearchState& state) {
        std::ifstream file(checkpointFile);
        if (!file.is_open()) {
            LOG_INFO("Checkpoint", "No existing checkpoint found");
            return false;
        }
        
        LOG_INFO("Checkpoint", "Loading checkpoint from: " + checkpointFile);
        
        std::string line;
        std::string currentSection;
        
        while (std::getline(file, line)) {
            // Trim whitespace
            size_t start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            size_t end = line.find_last_not_of(" \t\r\n");
            line = line.substr(start, end - start + 1);
            
            if (line.empty() || line[0] == '#') continue;
            
            if (line[0] == '[' && line[line.length()-1] == ']') {
                currentSection = line.substr(1, line.length()-2);
                continue;
            }
            
            size_t eqPos = line.find('=');
            if (eqPos == std::string::npos) continue;
            
            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);
            
            // Trim key and value
            key = key.substr(key.find_first_not_of(" \t"));
            key = key.substr(0, key.find_last_not_of(" \t") + 1);
            value = value.substr(value.find_first_not_of(" \t"));
            value = value.substr(0, value.find_last_not_of(" \t") + 1);
            
            if (key == "mode") state.mode = value;
            else if (key == "seeds_checked") state.seedsChecked = std::stoull(value);
            else if (key == "current_offset") state.currentOffset = std::stoull(value);
            else if (key == "last_entropy") state.lastEntropy = value;
            else if (key == "start_time") state.startTime = value;
            else if (key == "last_save") state.lastSaveTime = value;
            else if (key == "profile") state.profile = value;
            else if (key == "words") state.words = std::stoi(value);
            else if (key == "running") state.running = (value == "1" || value == "true");
            else if (key == "target_bits") {
                state.targetBits.clear();
                std::stringstream ss(value);
                std::string bit;
                while (std::getline(ss, bit, ',')) {
                    if (!bit.empty()) state.targetBits.push_back(std::stoi(bit));
                }
            }
        }
        
        file.close();
        
        LOG_INFO("Checkpoint", "Resuming from offset: " + std::to_string(state.currentOffset));
        return true;
    }
    
    bool shouldSave() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastSave).count();
        return elapsed >= saveIntervalSeconds;
    }
    
    void forceSave(const SearchState& state) {
        save(state);
    }
    
    void autoSave(const SearchState& state) {
        if (shouldSave()) {
            save(state);
        }
    }
    
    bool exists() {
        std::ifstream file(checkpointFile);
        return file.good();
    }
    
    void deleteCheckpoint() {
        std::remove(checkpointFile.c_str());
        std::remove((checkpointFile + ".bak").c_str());
        LOG_INFO("Checkpoint", "Checkpoint deleted");
    }
    
    // Export checkpoint info as JSON for external tools
    std::string exportJson() {
        SearchState state;
        if (!load(state)) return "{}";
        
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"mode\": \"" << state.mode << "\",\n";
        ss << "  \"seeds_checked\": " << state.seedsChecked << ",\n";
        ss << "  \"current_offset\": " << state.currentOffset << ",\n";
        ss << "  \"profile\": \"" << state.profile << "\",\n";
        ss << "  \"words\": " << state.words << ",\n";
        ss << "  \"start_time\": \"" << state.startTime << "\",\n";
        ss << "  \"last_save\": \"" << state.lastSaveTime << "\"\n";
        ss << "}";
        return ss.str();
    }
};

// Global checkpoint manager
extern CheckpointManager g_checkpoint;
