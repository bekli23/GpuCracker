#pragma once

// =========================================================
// AKM Profile Loader v2.0
// Dynamic profile loading from akm_profile/ directory
// =========================================================

#include <string>
#include <vector>
#include <unordered_map>
// Handle GCC < 8 filesystem support
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 8
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#else
    #include <filesystem>
    namespace fs = std::filesystem;
#endif
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <random>
#include "akm_enhanced.h"

class AkmProfileLoader {
private:
    std::string profilesDir = "akm_profile";
    std::vector<std::string> availableProfiles;
    std::unordered_map<std::string, std::string> profileNameToFile;
    
    std::string cleanString(const std::string& input) {
        std::string s = input;
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c){ 
            return std::isspace(c) || c == '\r' || c == '\n'; 
        }), s.end());
        return s;
    }

public:
    AkmProfileLoader(const std::string& dir = "akm_profile") : profilesDir(dir) {
        scanProfiles();
    }
    
    void setDirectory(const std::string& dir) {
        profilesDir = dir;
        scanProfiles();
    }
    
    // Scan directory for profile files
    void scanProfiles() {
        availableProfiles.clear();
        profileNameToFile.clear();
        
        if (!fs::exists(profilesDir)) {
            std::cerr << "[AKM ProfileLoader] Directory not found: " << profilesDir << "\n";
            return;
        }
        
        for (const auto& entry : fs::directory_iterator(profilesDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                std::string filename = entry.path().filename().string();
                
                // Skip index and non-profile files
                if (filename[0] == '_') continue;
                
                // Extract profile name from file
                std::ifstream file(entry.path());
                if (file.is_open()) {
                    std::string line;
                    while (std::getline(file, line)) {
                        if (line.find("name=") == 0) {
                            std::string name = cleanString(line.substr(5));
                            if (!name.empty()) {
                                availableProfiles.push_back(name);
                                profileNameToFile[name] = entry.path().string();
                                break;
                            }
                        }
                    }
                    file.close();
                }
            }
        }
        
        std::sort(availableProfiles.begin(), availableProfiles.end());
        
        std::cout << "[AKM ProfileLoader] Found " << availableProfiles.size() 
                  << " profiles in " << profilesDir << "/\n";
    }
    
    // Load profile into AkmLogicEnhanced
    bool loadProfile(const std::string& profileName, AkmLogicEnhanced& logic) {
        auto it = profileNameToFile.find(profileName);
        if (it == profileNameToFile.end()) {
            std::cerr << "[AKM ProfileLoader] Profile not found: " << profileName << "\n";
            return false;
        }
        
        return logic.loadProfile(it->second);
    }
    
    // Load profile by index (1-based)
    bool loadProfileByIndex(int index, AkmLogicEnhanced& logic) {
        if (index < 1 || index > (int)availableProfiles.size()) {
            std::cerr << "[AKM ProfileLoader] Invalid profile index: " << index << "\n";
            return false;
        }
        return loadProfile(availableProfiles[index - 1], logic);
    }
    
    // Get list of available profiles
    const std::vector<std::string>& getAvailableProfiles() const {
        return availableProfiles;
    }
    
    // Print available profiles
    void listProfiles() const {
        std::cout << "\n=== Available AKM Profiles ===\n";
        std::cout << "Total: " << availableProfiles.size() << "\n\n";
        
        for (size_t i = 0; i < availableProfiles.size(); i++) {
            std::cout << "  " << (i + 1) << ". " << availableProfiles[i] << "\n";
            if ((i + 1) % 20 == 0 && i < availableProfiles.size() - 1) {
                std::cout << "  ...\n";
            }
        }
        std::cout << "\n";
    }
    
    // Get profile info
    ProfileMetadata getProfileInfo(const std::string& profileName) {
        ProfileMetadata meta;
        
        auto it = profileNameToFile.find(profileName);
        if (it == profileNameToFile.end()) return meta;
        
        std::ifstream file(it->second);
        if (!file.is_open()) return meta;
        
        std::string line;
        bool inMetadata = false;
        
        while (std::getline(file, line)) {
            std::string cleanL = cleanString(line);
            if (cleanL.empty()) continue;
            
            if (cleanL == "#METADATA") {
                inMetadata = true;
                continue;
            }
            if (cleanL[0] == '#' && inMetadata) break;
            if (!inMetadata) continue;
            
            size_t eqPos = cleanL.find('=');
            if (eqPos == std::string::npos) continue;
            
            std::string key = cleanL.substr(0, eqPos);
            std::string val = cleanL.substr(eqPos + 1);
            
            if (key == "name") meta.name = val;
            else if (key == "description") meta.description = val;
            else if (key == "author") meta.author = val;
            else if (key == "version") meta.version = val;
            else if (key == "created") meta.created_date = val;
            else if (key == "checksum") {
                if (val == "v1") meta.checksum_ver = ChecksumVersion::V1_9BIT;
                else if (val == "v2") meta.checksum_ver = ChecksumVersion::V2_10BIT;
                else meta.checksum_ver = ChecksumVersion::NONE;
            }
            else if (key == "strict") meta.strict_hex = (val == "true");
        }
        
        file.close();
        return meta;
    }
    
    // Select random profile
    std::string getRandomProfile() const {
        if (availableProfiles.empty()) return "";
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, (int)availableProfiles.size() - 1);
        
        return availableProfiles[dis(gen)];
    }
    
    // Search profiles by keyword
    std::vector<std::string> searchProfiles(const std::string& keyword) const {
        std::vector<std::string> results;
        std::string lowerKeyword = keyword;
        std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);
        
        for (const auto& name : availableProfiles) {
            std::string lowerName = name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            if (lowerName.find(lowerKeyword) != std::string::npos) {
                results.push_back(name);
            }
        }
        
        return results;
    }
    
    // Get total count
    size_t getCount() const {
        return availableProfiles.size();
    }
};

// =========================================================
// BACKWARD COMPATIBILITY
// Keep existing load_akm_extra_profile function
// =========================================================

#include "akm_extra.h"  // Original file

// Enhanced version that also checks profile files
inline void load_akm_extra_profile_enhanced(const std::string& name,
                                            std::unordered_map<std::string, std::string>& customHex,
                                            std::unordered_map<std::string, AkmRule>& specialRules) {
    // First try original function
    load_akm_extra_profile(name, customHex, specialRules);
    
    // If nothing loaded, try profile files
    if (customHex.empty() && specialRules.empty()) {
        AkmProfileLoader loader;
        AkmLogicEnhanced logic;
        
        if (loader.loadProfile(name, logic)) {
            // Copy loaded data
            customHex = logic.customHex;
            // Note: specialRules would need accessor in AkmLogicEnhanced
        }
    }
}
