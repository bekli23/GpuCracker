#pragma once
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

// Simple log macros to avoid logger dependency (only define if not already defined)
#ifndef LOG_INFO
#define LOG_INFO(tag, msg) std::cout << "[" << tag << "] " << msg << std::endl
#endif
#ifndef LOG_WARN
#define LOG_WARN(tag, msg) std::cerr << "[" << tag << "] WARN: " << msg << std::endl
#endif
#ifndef LOG_ERROR
#define LOG_ERROR(tag, msg) std::cerr << "[" << tag << "] ERROR: " << msg << std::endl
#endif
#ifndef LOG_INFO_COMP
#define LOG_INFO_COMP(tag, msg) std::cout << "[" << tag << "] " << msg << std::endl
#endif

// =============================================================
// CONFIGURATION MANAGER - JSON/YAML Style Config
// =============================================================

class ConfigManager {
private:
    std::map<std::string, std::string> stringValues;
    std::map<std::string, int> intValues;
    std::map<std::string, double> doubleValues;
    std::map<std::string, bool> boolValues;
    std::map<std::string, std::vector<std::string>> listValues;
    
    std::string configFile;
    bool loaded = false;
    
    std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }
    
    std::vector<std::string> splitList(const std::string& s) {
        std::vector<std::string> result;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, ',')) {
            item = trim(item);
            if (!item.empty()) result.push_back(item);
        }
        return result;
    }

public:
    ConfigManager() {}
    
    bool load(const std::string& filename) {
        configFile = filename;
        std::ifstream file(filename);
        if (!file.is_open()) {
            LOG_WARN("Config", "Could not open config file: " + filename);
            return false;
        }
        
        LOG_INFO("Config", "Loading configuration from: " + filename);
        
        std::string line;
        std::string currentSection;
        
        while (std::getline(file, line)) {
            line = trim(line);
            
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;
            
            // Section header [section]
            if (line[0] == '[' && line[line.length()-1] == ']') {
                currentSection = line.substr(1, line.length()-2);
                continue;
            }
            
            // Key = Value pairs
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = trim(line.substr(0, eqPos));
                std::string value = trim(line.substr(eqPos + 1));
                
                if (!currentSection.empty()) {
                    key = currentSection + "." + key;
                }
                
                // Detect type and store
                if (value == "true" || value == "True" || value == "TRUE") {
                    boolValues[key] = true;
                } else if (value == "false" || value == "False" || value == "FALSE") {
                    boolValues[key] = false;
                } else if (value[0] == '[' && value[value.length()-1] == ']') {
                    // List value
                    listValues[key] = splitList(value.substr(1, value.length()-2));
                } else {
                    // Try int first
                    try {
                        size_t pos;
                        int intVal = std::stoi(value, &pos);
                        if (pos == value.length()) {
                            intValues[key] = intVal;
                        } else {
                            // Try double
                            double doubleVal = std::stod(value, &pos);
                            if (pos == value.length()) {
                                doubleValues[key] = doubleVal;
                            } else {
                                stringValues[key] = value;
                            }
                        }
                    } catch (...) {
                        stringValues[key] = value;
                    }
                }
            }
        }
        
        file.close();
        loaded = true;
        LOG_INFO("Config", "Configuration loaded successfully");
        return true;
    }
    
    void save(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            LOG_ERROR("Config", "Cannot write config file: " + filename);
            return;
        }
        
        file << "# GpuCracker Configuration File\n";
        file << "# Generated automatically\n\n";
        
        // Write by sections
        std::map<std::string, std::vector<std::pair<std::string, std::string>>> sections;
        std::vector<std::pair<std::string, std::string>> noSection;
        
        auto addToSection = [&](const std::string& key, const std::string& val) {
            size_t dotPos = key.find('.');
            if (dotPos != std::string::npos) {
                std::string section = key.substr(0, dotPos);
                std::string subKey = key.substr(dotPos + 1);
                sections[section].push_back({subKey, val});
            } else {
                noSection.push_back({key, val});
            }
        };
        
        for (const auto& kv : stringValues) {
            addToSection(kv.first, kv.second);
        }
        for (const auto& kv : intValues) {
            addToSection(kv.first, std::to_string(kv.second));
        }
        for (const auto& kv : doubleValues) {
            addToSection(kv.first, std::to_string(kv.second));
        }
        for (const auto& kv : boolValues) {
            addToSection(kv.first, kv.second ? "true" : "false");
        }
        for (const auto& kv : listValues) {
            std::string listStr = "[";
            for (size_t i = 0; i < kv.second.size(); ++i) {
                listStr += kv.second[i];
                if (i < kv.second.size() - 1) listStr += ", ";
            }
            listStr += "]";
            addToSection(kv.first, listStr);
        }
        
        // Write no section items first
        for (const auto& kv : noSection) {
            file << kv.first << " = " << kv.second << "\n";
        }
        if (!noSection.empty()) file << "\n";
        
        // Write sections
        for (const auto& sec : sections) {
            file << "[" << sec.first << "]\n";
            for (const auto& kv : sec.second) {
                file << kv.first << " = " << kv.second << "\n";
            }
            file << "\n";
        }
        
        file.close();
    }
    
    // Getters with defaults
    std::string getString(const std::string& key, const std::string& defaultVal = "") {
        auto it = stringValues.find(key);
        return it != stringValues.end() ? it->second : defaultVal;
    }
    
    int getInt(const std::string& key, int defaultVal = 0) {
        auto it = intValues.find(key);
        return it != intValues.end() ? it->second : defaultVal;
    }
    
    double getDouble(const std::string& key, double defaultVal = 0.0) {
        auto it = doubleValues.find(key);
        return it != doubleValues.end() ? it->second : defaultVal;
    }
    
    bool getBool(const std::string& key, bool defaultVal = false) {
        auto it = boolValues.find(key);
        return it != boolValues.end() ? it->second : defaultVal;
    }
    
    std::vector<std::string> getList(const std::string& key) {
        auto it = listValues.find(key);
        return it != listValues.end() ? it->second : std::vector<std::string>();
    }
    
    // Setters
    void setString(const std::string& key, const std::string& value) {
        stringValues[key] = value;
    }
    
    void setInt(const std::string& key, int value) {
        intValues[key] = value;
    }
    
    void setDouble(const std::string& key, double value) {
        doubleValues[key] = value;
    }
    
    void setBool(const std::string& key, bool value) {
        boolValues[key] = value;
    }
    
    void setList(const std::string& key, const std::vector<std::string>& value) {
        listValues[key] = value;
    }
    
    bool isLoaded() const { return loaded; }
    
    void printConfig() {
        LOG_INFO("Config", "=== Current Configuration ===");
        for (const auto& kv : stringValues) {
            LOG_INFO_COMP("Config", kv.first + " = " + kv.second);
        }
        for (const auto& kv : intValues) {
            LOG_INFO_COMP("Config", kv.first + " = " + std::to_string(kv.second));
        }
        for (const auto& kv : boolValues) {
            LOG_INFO_COMP("Config", kv.first + " = " + std::string(kv.second ? "true" : "false"));
        }
    }
};

// Global config instance
extern ConfigManager g_config;
