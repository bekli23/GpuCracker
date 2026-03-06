#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <chrono>

// Handle GCC < 8 filesystem support
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 8
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#else
    #include <filesystem>
    namespace fs = std::filesystem;
#endif

#include <algorithm>
#include <numeric>  // for std::iota
#include <fstream>
#include <sstream>
#include <regex>

// AKM Profile Manager - Handles loading and rotating profiles from akm_profile folder
class AkmProfileManager {
private:
    std::vector<std::string> profileFiles;     // Full paths to profile files
    std::vector<std::string> profileNames;     // Profile names (without path and extension)
    std::string profileFolder;
    std::mt19937 rng;
    
    // Extract profile name from filename (e.g., "profile_001_rapid_cipher_v0.0.txt" -> "rapid_cipher_v0.0")
    std::string extractProfileName(const std::string& filename) {
        // Remove path
        std::string name = filename;
        size_t lastSlash = name.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            name = name.substr(lastSlash + 1);
        }
        
        // Remove extension
        size_t lastDot = name.find_last_of('.');
        if (lastDot != std::string::npos) {
            name = name.substr(0, lastDot);
        }
        
        // Extract profile name part (after profile_XXX_)
        std::regex pattern(R"(profile_\d+_(.+))");
        std::smatch match;
        if (std::regex_match(name, match, pattern)) {
            return match[1].str();
        }
        
        // If no match, return the name as-is
        return name;
    }
    
    // Check if file is a valid profile file
    bool isProfileFile(const std::string& filename) {
        // Must start with "profile_" and end with ".txt"
        std::string name = filename;
        size_t lastSlash = name.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            name = name.substr(lastSlash + 1);
        }
        return name.substr(0, 8) == "profile_" && 
               name.length() > 4 && 
               name.substr(name.length() - 4) == ".txt";
    }

public:
    AkmProfileManager(const std::string& folder = "akm_profile") 
        : profileFolder(folder), rng(std::random_device{}()) {
        scanProfiles();
    }
    
    // Scan folder for profile files
    void scanProfiles() {
        profileFiles.clear();
        profileNames.clear();
        
        try {
            if (!fs::exists(profileFolder)) {
                std::cerr << "[WARN] Profile folder not found: " << profileFolder << std::endl;
                return;
            }
            
            for (const auto& entry : fs::directory_iterator(profileFolder)) {
                try {
                    if (fs::is_regular_file(entry)) {
                        std::string path = entry.path().string();
                        if (isProfileFile(path)) {
                            profileFiles.push_back(path);
                            profileNames.push_back(extractProfileName(path));
                        }
                    }
                } catch (const std::exception& fileEx) {
                    // Skip files with encoding issues (Unicode characters in filenames)
                    continue;
                }
            }
            
            // Sort by name for consistent ordering
            std::vector<size_t> indices(profileFiles.size());
            std::iota(indices.begin(), indices.end(), 0);
            std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                return profileNames[a] < profileNames[b];
            });
            
            std::vector<std::string> sortedFiles, sortedNames;
            for (size_t idx : indices) {
                sortedFiles.push_back(profileFiles[idx]);
                sortedNames.push_back(profileNames[idx]);
            }
            profileFiles = std::move(sortedFiles);
            profileNames = std::move(sortedNames);
            
        } catch (const std::exception& e) {
            std::cerr << "[WARN] Error scanning profiles: " << e.what() << std::endl;
        }
    }
    
    // Get count of available profiles
    size_t getProfileCount() const {
        return profileFiles.size();
    }
    
    // Get list of all profile names
    std::vector<std::string> getProfileNames() const {
        return profileNames;
    }
    
    // Get profile path by name
    std::string getProfilePath(const std::string& name) const {
        // First check if it's a direct filename match
        for (size_t i = 0; i < profileFiles.size(); i++) {
            if (profileNames[i] == name) {
                return profileFiles[i];
            }
        }
        
        // Check if it's a full filename
        for (const auto& path : profileFiles) {
            if (path.find(name) != std::string::npos) {
                return path;
            }
        }
        
        // Return empty if not found
        return "";
    }
    
    // Get random profile
    std::string getRandomProfile(std::string* outName = nullptr) {
        if (profileFiles.empty()) {
            return "";
        }
        
        std::uniform_int_distribution<size_t> dist(0, profileFiles.size() - 1);
        size_t idx = dist(rng);
        
        if (outName) {
            *outName = profileNames[idx];
        }
        
        return profileFiles[idx];
    }
    
    // Get random profile excluding specific indices (to avoid repeats)
    std::string getRandomProfileExcluding(const std::vector<size_t>& exclude, std::string* outName = nullptr) {
        if (profileFiles.empty()) {
            return "";
        }
        
        std::vector<size_t> available;
        for (size_t i = 0; i < profileFiles.size(); i++) {
            if (std::find(exclude.begin(), exclude.end(), i) == exclude.end()) {
                available.push_back(i);
            }
        }
        
        if (available.empty()) {
            // All excluded, just pick random
            return getRandomProfile(outName);
        }
        
        std::uniform_int_distribution<size_t> dist(0, available.size() - 1);
        size_t idx = available[dist(rng)];
        
        if (outName) {
            *outName = profileNames[idx];
        }
        
        return profileFiles[idx];
    }
    
    // List all available profiles
    void listProfiles() const {
        std::cout << "\n=== Available AKM Profiles ===\n";
        std::cout << "Folder: " << profileFolder << "\n";
        std::cout << "Total: " << profileFiles.size() << " profiles\n\n";
        
        for (size_t i = 0; i < profileFiles.size(); i++) {
            std::cout << "  " << (i + 1) << ". " << profileNames[i] << "\n";
            std::cout << "     File: " << profileFiles[i] << "\n";
        }
        std::cout << "\n";
    }
    
    // Check if a profile exists
    bool hasProfile(const std::string& name) const {
        return !getProfilePath(name).empty();
    }
    
    // Get full profile path with fallback logic:
    // 1. If name is a full path that exists, return it
    // 2. If name is a profile name in akm_profile/, return full path
    // 3. If name is a filename in akm_profile/, return full path
    // 4. Return empty string if not found
    std::string resolveProfilePath(const std::string& name) const {
        // 1. Check if it's already a valid path
        if (fs::exists(name)) {
            return name;
        }
        
        // 2. Check in profile folder
        std::string path = getProfilePath(name);
        if (!path.empty()) {
            return path;
        }
        
        // 3. Try to construct path from name
        std::string constructedPath = profileFolder + "/" + name;
        if (fs::exists(constructedPath)) {
            return constructedPath;
        }
        
        // 4. Try with .txt extension
        if (name.length() < 4 || name.substr(name.length() - 4) != ".txt") {
            constructedPath += ".txt";
            if (fs::exists(constructedPath)) {
                return constructedPath;
            }
        }
        
        return "";
    }
};

// Profile rotation tracker
class AkmProfileRotator {
private:
    AkmProfileManager manager;
    int rotationSeconds;
    std::chrono::steady_clock::time_point lastRotation;
    std::string currentProfilePath;
    std::string currentProfileName;
    std::vector<size_t> usedProfiles;  // Track recently used to avoid repeats
    size_t maxHistory = 10;  // Keep last 10 profiles in history
    
public:
    AkmProfileRotator(int seconds = 0, const std::string& folder = "akm_profile")
        : manager(folder), rotationSeconds(seconds) {
        lastRotation = std::chrono::steady_clock::now();
    }
    
    // Initialize with specific profile
    bool initWithProfile(const std::string& profileName) {
        currentProfilePath = manager.resolveProfilePath(profileName);
        if (!currentProfilePath.empty()) {
            // Extract name from path
            for (size_t i = 0; i < manager.getProfileCount(); i++) {
                if (manager.getProfileNames()[i] == profileName) {
                    currentProfileName = profileName;
                    usedProfiles.push_back(i);
                    break;
                }
            }
            if (currentProfileName.empty()) {
                currentProfileName = profileName;
            }
            return true;
        }
        return false;
    }
    
    // Resolve profile path (delegates to manager)
    std::string resolveProfilePath(const std::string& name) const {
        return manager.resolveProfilePath(name);
    }
    
    // Initialize with random profile
    bool initRandom() {
        currentProfilePath = manager.getRandomProfile(&currentProfileName);
        if (!currentProfilePath.empty()) {
            // Find index and add to used
            for (size_t i = 0; i < manager.getProfileCount(); i++) {
                if (manager.getProfileNames()[i] == currentProfileName) {
                    usedProfiles.push_back(i);
                    break;
                }
            }
            return true;
        }
        return false;
    }
    
    // Check if it's time to rotate
    bool shouldRotate() const {
        if (rotationSeconds <= 0) {
            return false;  // No rotation configured
        }
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastRotation).count();
        return elapsed >= rotationSeconds;
    }
    
    // Rotate to next profile
    bool rotate() {
        if (manager.getProfileCount() == 0) {
            return false;
        }
        
        // Trim history if too long
        if (usedProfiles.size() > maxHistory) {
            usedProfiles.erase(usedProfiles.begin(), usedProfiles.end() - maxHistory);
        }
        
        // Get random profile excluding recent ones
        std::string newName;
        currentProfilePath = manager.getRandomProfileExcluding(usedProfiles, &newName);
        
        if (currentProfilePath.empty()) {
            return false;
        }
        
        currentProfileName = newName;
        lastRotation = std::chrono::steady_clock::now();
        
        // Add to used profiles
        for (size_t i = 0; i < manager.getProfileCount(); i++) {
            if (manager.getProfileNames()[i] == currentProfileName) {
                usedProfiles.push_back(i);
                break;
            }
        }
        
        return true;
    }
    
    // Force rotation now
    bool forceRotate() {
        lastRotation = std::chrono::steady_clock::now() - std::chrono::seconds(rotationSeconds);
        return rotate();
    }
    
    // Get current profile path
    std::string getCurrentPath() const {
        return currentProfilePath;
    }
    
    // Get current profile name
    std::string getCurrentName() const {
        return currentProfileName;
    }
    
    // Get time until next rotation in seconds
    int getTimeUntilRotation() const {
        if (rotationSeconds <= 0) {
            return -1;  // No rotation
        }
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastRotation).count();
        int remaining = rotationSeconds - (int)elapsed;
        return std::max(0, remaining);
    }
    
    // Get rotation interval in seconds
    int getRotationSeconds() const {
        return rotationSeconds;
    }
    
    // List all available profiles
    void listProfiles() const {
        manager.listProfiles();
    }
    
    // Get total profile count
    size_t getProfileCount() const {
        return manager.getProfileCount();
    }
};
