#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <atomic>

// =============================================================
// STRUCTURED LOGGER - Output JSON pentru parsing automat
// =============================================================

enum class LogLevel { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3, CRITICAL = 4 };

class Logger {
private:
    static Logger* instance;
    static std::mutex instanceMutex;
    
    std::ofstream logFile;
    std::mutex logMutex;
    LogLevel minLevel = LogLevel::INFO;
    bool consoleOutput = true;
    bool jsonFormat = false;
    std::string componentName = "GpuCracker";
    
    Logger() {}
    
    std::string levelToString(LogLevel level) {
        switch(level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARN: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::CRITICAL: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
    
    std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
    std::string escapeJson(const std::string& s) {
        std::string out;
        for (char c : s) {
            switch (c) {
                case '\"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default: out += c;
            }
        }
        return out;
    }

public:
    static Logger* getInstance() {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (instance == nullptr) {
            instance = new Logger();
        }
        return instance;
    }
    
    void init(const std::string& filename = "", LogLevel level = LogLevel::INFO, 
              bool useJson = false, bool console = true) {
        std::lock_guard<std::mutex> lock(logMutex);
        minLevel = level;
        jsonFormat = useJson;
        consoleOutput = console;
        
        if (!filename.empty()) {
            logFile.open(filename, std::ios::app);
        }
    }
    
    void setComponent(const std::string& comp) {
        componentName = comp;
    }
    
    void log(LogLevel level, const std::string& component, const std::string& message) {
        if (level < minLevel) return;
        
        std::lock_guard<std::mutex> lock(logMutex);
        
        std::stringstream output;
        
        if (jsonFormat) {
            output << "{"
                   << "\"timestamp\":\"" << getTimestamp() << "\","
                   << "\"level\":\"" << levelToString(level) << "\","
                   << "\"component\":\"" << escapeJson(component) << "\","
                   << "\"message\":\"" << escapeJson(message) << "\""
                   << "}\n";
        } else {
            output << "[" << getTimestamp() << "] "
                   << "[" << levelToString(level) << "] "
                   << "[" << component << "] "
                   << message << "\n";
        }
        
        std::string logLine = output.str();
        
        if (consoleOutput) {
            // Culori pentru console
            switch(level) {
                case LogLevel::DEBUG: std::cout << "\033[90m"; break;
                case LogLevel::INFO: std::cout << "\033[0m"; break;
                case LogLevel::WARN: std::cout << "\033[33m"; break;
                case LogLevel::ERROR: std::cout << "\033[31m"; break;
                case LogLevel::CRITICAL: std::cout << "\033[1;31m"; break;
            }
            std::cout << logLine << "\033[0m";
        }
        
        if (logFile.is_open()) {
            logFile << logLine;
            logFile.flush();
        }
    }
    
    // Helper methods
    void debug(const std::string& msg) { log(LogLevel::DEBUG, componentName, msg); }
    void info(const std::string& msg) { log(LogLevel::INFO, componentName, msg); }
    void warn(const std::string& msg) { log(LogLevel::WARN, componentName, msg); }
    void error(const std::string& msg) { log(LogLevel::ERROR, componentName, msg); }
    void critical(const std::string& msg) { log(LogLevel::CRITICAL, componentName, msg); }
    
    void debug(const std::string& comp, const std::string& msg) { log(LogLevel::DEBUG, comp, msg); }
    void info(const std::string& comp, const std::string& msg) { log(LogLevel::INFO, comp, msg); }
    void warn(const std::string& comp, const std::string& msg) { log(LogLevel::WARN, comp, msg); }
    void error(const std::string& comp, const std::string& msg) { log(LogLevel::ERROR, comp, msg); }
    void critical(const std::string& comp, const std::string& msg) { log(LogLevel::CRITICAL, comp, msg); }
};

// Macro-uri pentru logging convenabil
#define LOG_INIT(file, level, json, console) Logger::getInstance()->init(file, level, json, console)
#define LOG_SET_COMP(comp) Logger::getInstance()->setComponent(comp)

// Undefine any conflicting macros from other headers (e.g., config_manager.h)
#ifdef LOG_INFO
    #undef LOG_INFO
#endif
#ifdef LOG_WARN
    #undef LOG_WARN
#endif
#ifdef LOG_ERROR
    #undef LOG_ERROR
#endif
#ifdef LOG_INFO_COMP
    #undef LOG_INFO_COMP
#endif

// Variadic macros that work with 1 or 2 arguments
#define LOG_DEBUG(...) Logger::getInstance()->debug(__VA_ARGS__)
#define LOG_INFO(...) Logger::getInstance()->info(__VA_ARGS__)
#define LOG_WARN(...) Logger::getInstance()->warn(__VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) Logger::getInstance()->critical(__VA_ARGS__)

// Static members are defined in logger.cpp (or use inline for header-only)
// For header-only with C++17, use inline:
// inline Logger* Logger::instance = nullptr;
// inline std::mutex Logger::instanceMutex;
// OR define in logger.cpp
