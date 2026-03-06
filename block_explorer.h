#pragma once

// ============================================================================
// MINI BLOCK EXPLORER - Web interface for blockchain monitoring
// ============================================================================

#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <sstream>
#include <iomanip>
#include "args.h"
#include "blockchain_reader.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define CLOSE_SOCKET(s) closesocket(s)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <fcntl.h>
    #define CLOSE_SOCKET(s) close(s)
#endif

namespace BlockExplorer {

static std::atomic<bool> g_explorerRunning{false};
static std::thread g_explorerThread;
static int g_serverSocket = -1;
static int g_explorerPort = 8080;

// Format satoshis to BTC string
inline std::string FormatBTC(uint64_t satoshis) {
    double btc = satoshis / 100000000.0;
    std::stringstream ss;
    ss << std::fixed << std::setprecision(8) << btc << " BTC";
    return ss.str();
}

// Generate HTML dashboard
inline std::string GetExplorerHtml(int port) {
    auto* reader = BlockchainReader::GetBlockchainReader();
    uint64_t totalAddrs = reader ? reader->GetTotalAddresses() : 0;
    size_t filesProcessed = reader ? reader->GetProcessedFileCount() : 0;
    double progress = reader ? reader->GetIndexingProgress() : 0.0;
    std::string totalBalance = reader ? FormatBTC(reader->GetTotalBalance()) : "0.00000000 BTC";
    
    std::stringstream html;
    html << "<!DOCTYPE html>\n<html>\n<head>\n";
    html << "<title>GpuCracker Block Explorer</title>\n";
    html << "<meta http-equiv=\"refresh\" content=\"10\">\n";
    html << "<style>\n";
    html << "body{font-family:Arial,sans-serif;margin:40px;background:#1a1a1a;color:#eee}\n";
    html << "h1{color:#4CAF50}h2{color:#2196F3}\n";
    html << ".stat-box{background:#2d2d2d;padding:20px;margin:10px 0;border-radius:8px}\n";
    html << ".stat-value{font-size:24px;font-weight:bold;color:#4CAF50}\n";
    html << ".stat-label{color:#888}\n";
    html << ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:20px}\n";
    html << "input[type=text]{padding:10px;width:400px;background:#1a1a1a;color:#fff;border:1px solid #555}\n";
    html << "button{padding:10px 20px;background:#4CAF50;color:white;border:none;cursor:pointer}\n";
    html << ".info{background:#333;padding:15px;border-radius:8px;margin:10px 0}\n";
    html << ".balance-positive{color:#4CAF50;font-weight:bold}\n";
    html << ".balance-zero{color:#888}\n";
    html << "pre{background:#222;padding:10px;border-radius:4px;overflow-x:auto}\n";
    html << "</style>\n</head>\n<body>\n";
    html << "<h1>GpuCracker Block Explorer</h1>\n";
    html << "<div class=\"grid\">\n";
    html << "<div class=\"stat-box\"><div class=\"stat-label\">Indexed Addresses</div>";
    html << "<div class=\"stat-value\">" << totalAddrs << "</div></div>\n";
    html << "<div class=\"stat-box\"><div class=\"stat-label\">Files Processed</div>";
    html << "<div class=\"stat-value\">" << filesProcessed << "</div></div>\n";
    html << "<div class=\"stat-box\"><div class=\"stat-label\">Progress</div>";
    html << "<div class=\"stat-value\">" << (int)(progress * 100) << "%</div></div>\n";
    html << "<div class=\"stat-box\"><div class=\"stat-label\">Total Balance</div>";
    html << "<div class=\"stat-value\">" << totalBalance << "</div></div>\n";
    html << "</div>\n";
    html << "<div class=\"info\">\n<h2>Check Address Balance</h2>\n";
    html << "<form onsubmit=\"checkAddress(event)\">\n";
    html << "<input type=\"text\" id=\"addrInput\" placeholder=\"Enter Bitcoin address (1..., 3..., bc1...)\" style=\"width:500px\">\n";
    html << "<button type=\"submit\">Check</button>\n";
    html << "</form>\n";
    html << "<pre id=\"result\" style=\"margin-top:15px;display:none\"></pre>\n";
    html << "</div>\n";
    html << "<script>\n";
    html << "async function checkAddress(e){\n";
    html << "e.preventDefault();\n";
    html << "const addr=document.getElementById('addrInput').value;\n";
    html << "if(!addr)return;\n";
    html << "const res=document.getElementById('result');\n";
    html << "res.style.display='block';\n";
    html << "res.textContent='Checking...';\n";
    html << "try{\n";
    html << "const resp=await fetch('/api/check?address='+encodeURIComponent(addr));\n";
    html << "const data=await resp.json();\n";
    html << "let txt='Address: '+data.address+'\\n';\n";
    html << "txt+='Found: '+(data.found?'YES':'NO')+'\\n';\n";
    html << "txt+='Balance: '+data.balance_btc+' ('+data.balance_satoshis+' sat)\\n';\n";
    html << "txt+='Received: '+data.total_received+'\\n';\n";
    html << "txt+='Sent: '+data.total_sent+'\\n';\n";
    html << "txt+='TX Count: '+data.tx_count;\n";
    html << "res.textContent=txt;\n";
    html << "}catch(err){res.textContent='Error: '+err.message;}\n";
    html << "}\n";
    html << "</script>\n";
    html << "</body>\n</html>";
    return html.str();
}

// Get stats JSON for /api/status
inline std::string GetStatsJson() {
    auto* reader = BlockchainReader::GetBlockchainReader();
    std::stringstream json;
    json << "{";
    json << "\"status\":\"ok\",";
    json << "\"indexed_addresses\":" << (reader ? reader->GetTotalAddresses() : 0) << ",";
    json << "\"files_processed\":" << (reader ? reader->GetProcessedFileCount() : 0) << ",";
    json << "\"indexing_progress\":" << (reader ? reader->GetIndexingProgress() : 0.0) << ",";
    json << "\"total_balance_btc\":\"" << (reader ? FormatBTC(reader->GetTotalBalance()) : "0.00000000 BTC") << "\"";
    json << "}";
    return json.str();
}

// Check address and return JSON
inline std::string CheckAddressJson(const std::string& address) {
    auto* reader = BlockchainReader::GetBlockchainReader();
    if (!reader) {
        return "{\"error\":\"Reader not initialized\"}";
    }
    
    std::stringstream json;
    auto bal = reader->GetAddressBalance(address);
    bool found = reader->HasAddress(address);
    
    json << "{";
    json << "\"address\":\"" << address << "\",";
    json << "\"found\":" << (found ? "true" : "false") << ",";
    json << "\"balance_satoshis\":" << bal.balanceSatoshis << ",";
    json << "\"balance_btc\":\"" << FormatBTC(bal.balanceSatoshis) << "\",";
    json << "\"total_received\":\"" << FormatBTC(bal.receivedSatoshis) << "\",";
    json << "\"total_sent\":\"" << FormatBTC(bal.sentSatoshis) << "\",";
    json << "\"tx_count\":" << bal.txCount << "}";
    return json.str();
}

// Handle HTTP client
inline void HandleClient(int clientSocket) {
    char buffer[4096];
    int received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
        CLOSE_SOCKET(clientSocket);
        return;
    }
    
    buffer[received] = '\0';
    std::string request(buffer);
    
    // Parse request
    size_t firstSpace = request.find(' ');
    size_t secondSpace = request.find(' ', firstSpace + 1);
    std::string path = request.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    
    std::string response;
    std::string contentType = "text/html";
    
    if (path == "/" || path == "/index.html") {
        response = GetExplorerHtml(g_explorerPort);
    }
    else if (path == "/api/status" || path == "/api/stats") {
        contentType = "application/json";
        response = GetStatsJson();
    }
    else if (path.find("/check?address=") == 0 || path.find("/api/check?address=") == 0) {
        size_t addrStart = path.find("address=") + 8;
        std::string address = path.substr(addrStart);
        contentType = "application/json";
        response = CheckAddressJson(address);
    }
    else {
        response = "<!DOCTYPE html><html><body><h1>404 Not Found</h1></body></html>";
    }
    
    // Send response
    std::stringstream httpResponse;
    httpResponse << "HTTP/1.1 200 OK\r\n";
    httpResponse << "Content-Type: " << contentType << "\r\n";
    httpResponse << "Access-Control-Allow-Origin: *\r\n";
    httpResponse << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    httpResponse << "Content-Length: " << response.length() << "\r\n";
    httpResponse << "Connection: close\r\n\r\n";
    httpResponse << response;
    
    std::string respStr = httpResponse.str();
    send(clientSocket, respStr.c_str(), respStr.length(), 0);
    CLOSE_SOCKET(clientSocket);
}

// Server loop
inline void ExplorerServerLoop(int port) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    
    g_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_serverSocket < 0) return;
    
    int opt = 1;
    setsockopt(g_serverSocket, SOL_SOCKET, SO_REUSEADDR, 
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
    
    if (bind(g_serverSocket, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[Explorer] Failed to bind to port " << port << "\n";
        CLOSE_SOCKET(g_serverSocket);
        return;
    }
    
    if (listen(g_serverSocket, 5) < 0) {
        CLOSE_SOCKET(g_serverSocket);
        return;
    }
    
    std::cout << "[Explorer] Web server started on http://localhost:" << port << "\n";
    
    while (g_explorerRunning) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(g_serverSocket, &readfds);
        
        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        int activity = select(g_serverSocket + 1, &readfds, nullptr, nullptr, &tv);
        if (activity > 0 && FD_ISSET(g_serverSocket, &readfds)) {
            sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int clientSocket = accept(g_serverSocket, (sockaddr*)&clientAddr, &clientLen);
            if (clientSocket >= 0) {
                HandleClient(clientSocket);
            }
        }
    }
    
    CLOSE_SOCKET(g_serverSocket);
#ifdef _WIN32
    WSACleanup();
#endif
}

// Start block explorer
inline void startBlockExplorer(const ProgramConfig& cfg) {
    if (g_explorerRunning) return;
    
    g_explorerPort = cfg.explorerPort;
    g_explorerRunning = true;
    g_explorerThread = std::thread(ExplorerServerLoop, g_explorerPort);
}

// Stop block explorer
inline void stopBlockExplorer() {
    if (!g_explorerRunning) return;
    
    g_explorerRunning = false;
    if (g_explorerThread.joinable()) {
        g_explorerThread.join();
    }
}

} // namespace BlockExplorer
