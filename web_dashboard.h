#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <sstream>
#include <functional>
#include <map>
#include <vector>
#include "logger.h"

// =============================================================
// WEB DASHBOARD - HTTP server for remote monitoring
// Simple embedded HTTP server for stats and control
// =============================================================

// Platform-specific socket includes
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    // Windows uses closesocket() instead of close()
    #define CLOSE_SOCKET(s) closesocket(s)
    #define SHUTDOWN_SOCKET(s) closesocket(s)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <fcntl.h>
    #define CLOSE_SOCKET(s) close(s)
    #define SHUTDOWN_SOCKET(s) close(s)
#endif

struct DashboardStats {
    std::atomic<unsigned long long> seedsChecked{0};
    std::atomic<unsigned long long> addressesChecked{0};
    std::atomic<unsigned long long> hitsFound{0};
    std::atomic<double> currentSpeed{0.0};
    std::atomic<int> activeGpus{0};
    std::atomic<int> uptimeSeconds{0};
    
    std::string currentSeed;
    std::string currentPhrase;
    std::vector<std::string> recentHits;
    std::mutex hitsMutex;
    
    std::string toJson() {
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"seeds_checked\": " << seedsChecked.load() << ",\n";
        ss << "  \"addresses_checked\": " << addressesChecked.load() << ",\n";
        ss << "  \"hits_found\": " << hitsFound.load() << ",\n";
        ss << "  \"speed_per_second\": " << currentSpeed.load() << ",\n";
        ss << "  \"active_gpus\": " << activeGpus.load() << ",\n";
        ss << "  \"uptime_seconds\": " << uptimeSeconds.load() << ",\n";
        ss << "  \"current_seed\": \"" << escapeJson(currentSeed) << "\",\n";
        ss << "  \"current_phrase\": \"" << escapeJson(currentPhrase) << "\",\n";
        ss << "  \"recent_hits\": [";
        
        std::lock_guard<std::mutex> lock(hitsMutex);
        for (size_t i = 0; i < recentHits.size(); ++i) {
            ss << "\"" << escapeJson(recentHits[i]) << "\"";
            if (i < recentHits.size() - 1) ss << ", ";
        }
        ss << "]\n";
        ss << "}";
        return ss.str();
    }
    
    static std::string escapeJson(const std::string& s) {
        std::string out;
        for (char c : s) {
            switch (c) {
                case '\"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default: if (c >= 0x20) out += c;
            }
        }
        return out;
    }
    
    void addHit(const std::string& hit) {
        std::lock_guard<std::mutex> lock(hitsMutex);
        recentHits.insert(recentHits.begin(), hit);
        if (recentHits.size() > 10) {
            recentHits.pop_back();
        }
        hitsFound++;
    }
};

class WebDashboard {
private:
    int port;
    std::atomic<bool> running{false};
    std::thread serverThread;
    int serverSocket = -1;
    DashboardStats stats;
    
    std::function<std::string()> customStatsCallback;
    std::function<void(const std::string&)> commandCallback;
    
    bool initSocket() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            LOG_ERROR("Dashboard", "WSAStartup failed");
            return false;
        }
#endif
        
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            LOG_ERROR("Dashboard", "Failed to create socket");
            return false;
        }
        
        // Allow reuse
        int opt = 1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, 
#ifdef _WIN32
                   reinterpret_cast<const char*>(&opt), 
#else
                   &opt, 
#endif
                   sizeof(opt));
        
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(serverSocket, (sockaddr*)&addr, sizeof(addr)) < 0) {
            LOG_ERROR("Dashboard", "Failed to bind to port " + std::to_string(port));
            return false;
        }
        
        if (listen(serverSocket, 5) < 0) {
            LOG_ERROR("Dashboard", "Failed to listen");
            return false;
        }
        
        return true;
    }
    
    void handleClient(int clientSocket) {
        char buffer[4096];
        int received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        if (received <= 0) {
            CLOSE_SOCKET(clientSocket);
            return;
        }
        
        buffer[received] = '\0';
        std::string request(buffer);
        
        // Parse request line
        size_t firstSpace = request.find(' ');
        size_t secondSpace = request.find(' ', firstSpace + 1);
        
        std::string method = request.substr(0, firstSpace);
        std::string path = request.substr(firstSpace + 1, secondSpace - firstSpace - 1);
        
        std::string response;
        std::string contentType = "application/json";
        int statusCode = 200;
        
        if (path == "/status" || path == "/api/status") {
            response = stats.toJson();
        }
        else if (path == "/api/config") {
            response = "{\"status\": \"ok\", \"port\": " + std::to_string(port) + "}";
        }
        else if (path == "/api/stop" && method == "POST") {
            if (commandCallback) {
                commandCallback("stop");
            }
            response = "{\"status\": \"stopping\"}";
        }
        else if (path == "/" || path == "/index.html") {
            contentType = "text/html";
            response = getHtmlDashboard();
        }
        else {
            statusCode = 404;
            response = "{\"error\": \"Not found\"}";
        }
        
        // Send HTTP response
        std::stringstream httpResponse;
        httpResponse << "HTTP/1.1 " << statusCode << " OK\r\n";
        httpResponse << "Content-Type: " << contentType << "\r\n";
        httpResponse << "Access-Control-Allow-Origin: *\r\n";
        httpResponse << "Content-Length: " << response.length() << "\r\n";
        httpResponse << "Connection: close\r\n\r\n";
        httpResponse << response;
        
        std::string responseStr = httpResponse.str();
        send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
        
        CLOSE_SOCKET(clientSocket);
    }
    
    void serverLoop() {
        while (running) {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(serverSocket, &readfds);
            
            timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            
            int activity = select(serverSocket + 1, &readfds, nullptr, nullptr, &tv);
            
            if (activity > 0 && FD_ISSET(serverSocket, &readfds)) {
                sockaddr_in clientAddr;
                socklen_t clientLen = sizeof(clientAddr);
                int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
                
                if (clientSocket >= 0) {
                    handleClient(clientSocket);
                }
            }
        }
    }
    
    std::string getHtmlDashboard() {
        return R"(<!DOCTYPE html>
<html>
<head>
    <title>GpuCracker Dashboard</title>
    <meta http-equiv="refresh" content="5">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #1a1a1a; color: #eee; }
        h1 { color: #4CAF50; }
        .stat-box { background: #2d2d2d; padding: 20px; margin: 10px 0; border-radius: 8px; }
        .stat-value { font-size: 24px; font-weight: bold; color: #4CAF50; }
        .stat-label { color: #888; }
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; }
        pre { background: #333; padding: 10px; overflow-x: auto; }
        .hit { background: #2e7d32; padding: 10px; margin: 5px 0; border-radius: 4px; }
    </style>
</head>
<body>
    <h1>🚀 GpuCracker Dashboard</h1>
    <div class="grid">
        <div class="stat-box">
            <div class="stat-label">Seeds Checked</div>
            <div class="stat-value" id="seeds">Loading...</div>
        </div>
        <div class="stat-box">
            <div class="stat-label">Speed (addr/s)</div>
            <div class="stat-value" id="speed">Loading...</div>
        </div>
        <div class="stat-box">
            <div class="stat-label">Hits Found</div>
            <div class="stat-value" id="hits">Loading...</div>
        </div>
        <div class="stat-box">
            <div class="stat-label">Active GPUs</div>
            <div class="stat-value" id="gpus">Loading...</div>
        </div>
    </div>
    <div class="stat-box">
        <div class="stat-label">Current Seed</div>
        <pre id="seed">Loading...</pre>
    </div>
    <div class="stat-box">
        <div class="stat-label">Recent Hits</div>
        <div id="recent-hits">No hits yet</div>
    </div>
    <script>
        async function updateStats() {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();
                document.getElementById('seeds').textContent = data.seeds_checked.toLocaleString();
                document.getElementById('speed').textContent = data.speed_per_second.toLocaleString();
                document.getElementById('hits').textContent = data.hits_found.toLocaleString();
                document.getElementById('gpus').textContent = data.active_gpus;
                document.getElementById('seed').textContent = data.current_phrase || 'N/A';
                
                const hitsDiv = document.getElementById('recent-hits');
                if (data.recent_hits && data.recent_hits.length > 0) {
                    hitsDiv.innerHTML = data.recent_hits.map(h => '<div class="hit">' + h + '</div>').join('');
                }
            } catch (e) {
                console.error('Failed to update stats:', e);
            }
        }
        updateStats();
        setInterval(updateStats, 5000);
    </script>
</body>
</html>)";
    }

public:
    WebDashboard(int p = 8080) : port(p) {}
    
    ~WebDashboard() {
        stop();
    }
    
    bool start() {
        if (running) return true;
        
        if (!initSocket()) {
            return false;
        }
        
        running = true;
        serverThread = std::thread(&WebDashboard::serverLoop, this);
        
        LOG_INFO("Dashboard", "Web dashboard started on http://localhost:" + std::to_string(port));
        return true;
    }
    
    void stop() {
        if (!running) return;
        
        running = false;
        
        if (serverThread.joinable()) {
            serverThread.join();
        }
        
        if (serverSocket >= 0) {
            CLOSE_SOCKET(serverSocket);
            serverSocket = -1;
        }
        
#ifdef _WIN32
        WSACleanup();
#endif
        
        LOG_INFO("Dashboard", "Web dashboard stopped");
    }
    
    DashboardStats& getStats() {
        return stats;
    }
    
    void setCustomStatsCallback(std::function<std::string()> callback) {
        customStatsCallback = callback;
    }
    
    void setCommandCallback(std::function<void(const std::string&)> callback) {
        commandCallback = callback;
    }
    
    bool isRunning() const {
        return running;
    }
};

// Global dashboard instance
extern std::unique_ptr<WebDashboard> g_dashboard;

void initDashboard(int port = 8080);
void stopDashboard();
