#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cstdint>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <map>

// JSONCPP Support - Optional
#ifdef HAS_JSONCPP
    #include <json/json.h>
#else
    // Try to find jsoncpp in common locations
    #if __has_include(<json/json.h>)
        #include <json/json.h>
        #define HAS_JSONCPP
    #elif __has_include(<jsoncpp/json/json.h>)
        #include <jsoncpp/json/json.h>
        #define HAS_JSONCPP
    #else
    // Minimal JSON fallback for basic parsing when jsoncpp is not available
    // Note: <map> and other required headers are already included above
    namespace Json {
        class Value {
        public:
            enum ValueType { nullValue, intValue, uintValue, realValue, stringValue, booleanValue, arrayValue, objectValue };
            ValueType type_ = nullValue;
            std::string string_;
            double real_ = 0;
            int int_ = 0;
            std::vector<Value> array_;
            std::map<std::string, Value> object_;
            
            Value() = default;
            Value(ValueType t) : type_(t) {}
            
            ValueType type() const { return type_; }
            bool isNull() const { return type_ == nullValue; }
            bool isObject() const { return type_ == objectValue; }
            bool isArray() const { return type_ == arrayValue; }
            bool isString() const { return type_ == stringValue; }
            bool isDouble() const { return type_ == realValue || type_ == intValue; }
            bool isInt() const { return type_ == intValue; }
            bool isMember(const std::string& key) const {
                return type_ == objectValue && object_.find(key) != object_.end();
            }
            
            const Value& operator[](const std::string& key) const {
                static Value nullValue;
                if (type_ != objectValue) return nullValue;
                auto it = object_.find(key);
                return (it != object_.end()) ? it->second : nullValue;
            }
            
            const Value& operator[](size_t index) const {
                static Value nullValue;
                if (type_ != arrayValue || index >= array_.size()) return nullValue;
                return array_[index];
            }
            
            size_t size() const { return type_ == arrayValue ? array_.size() : 0; }
            
            std::string asString() const { return type_ == stringValue ? string_ : ""; }
            double asDouble() const { return type_ == realValue ? real_ : (type_ == intValue ? int_ : 0); }
            int asInt() const { return type_ == intValue ? int_ : static_cast<int>(asDouble()); }
            
            void append(const Value& v) {
                if (type_ != arrayValue) { type_ = arrayValue; array_.clear(); }
                array_.push_back(v);
            }
            
            Value& operator[](const std::string& key) {
                if (type_ != objectValue) type_ = objectValue;
                return object_[key];
            }
            
            static Value null;
            
            // Simple parser
            bool parse(const std::string& text) {
                size_t pos = 0;
                skipWhitespace(text, pos);
                return parseValue(text, pos);
            }
            
        private:
            void skipWhitespace(const std::string& s, size_t& pos) {
                while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\n' || s[pos] == '\r')) pos++;
            }
            
            bool parseValue(const std::string& s, size_t& pos) {
                skipWhitespace(s, pos);
                if (pos >= s.size()) return false;
                
                char c = s[pos];
                if (c == '{') return parseObject(s, pos);
                if (c == '[') return parseArray(s, pos);
                if (c == '"') return parseString(s, pos);
                if (c == 't' || c == 'f') return parseBool(s, pos);
                if (c == 'n') return parseNull(s, pos);
                return parseNumber(s, pos);
            }
            
            bool parseObject(const std::string& s, size_t& pos) {
                type_ = objectValue;
                object_.clear();
                pos++; // skip {
                skipWhitespace(s, pos);
                if (pos < s.size() && s[pos] == '}') { pos++; return true; }
                
                while (pos < s.size()) {
                    skipWhitespace(s, pos);
                    if (pos >= s.size() || s[pos] != '"') return false;
                    
                    std::string key;
                    if (!parseStringRaw(s, pos, key)) return false;
                    
                    skipWhitespace(s, pos);
                    if (pos >= s.size() || s[pos] != ':') return false;
                    pos++;
                    
                    Value val;
                    if (!val.parseValue(s, pos)) return false;
                    object_[key] = val;
                    
                    skipWhitespace(s, pos);
                    if (pos < s.size() && s[pos] == '}') { pos++; return true; }
                    if (pos >= s.size() || s[pos] != ',') return false;
                    pos++;
                }
                return false;
            }
            
            bool parseArray(const std::string& s, size_t& pos) {
                type_ = arrayValue;
                array_.clear();
                pos++; // skip [
                skipWhitespace(s, pos);
                if (pos < s.size() && s[pos] == ']') { pos++; return true; }
                
                while (pos < s.size()) {
                    Value val;
                    if (!val.parseValue(s, pos)) return false;
                    array_.push_back(val);
                    
                    skipWhitespace(s, pos);
                    if (pos < s.size() && s[pos] == ']') { pos++; return true; }
                    if (pos >= s.size() || s[pos] != ',') return false;
                    pos++;
                }
                return false;
            }
            
            bool parseString(const std::string& s, size_t& pos) {
                return parseStringRaw(s, pos, string_) && (type_ = stringValue, true);
            }
            
            bool parseStringRaw(const std::string& s, size_t& pos, std::string& out) {
                pos++; // skip opening quote
                out.clear();
                while (pos < s.size()) {
                    char c = s[pos++];
                    if (c == '"') return true;
                    if (c == '\\' && pos < s.size()) {
                        char next = s[pos++];
                        if (next == 'n') out += '\n';
                        else if (next == 't') out += '\t';
                        else if (next == 'r') out += '\r';
                        else out += next;
                    } else {
                        out += c;
                    }
                }
                return false;
            }
            
            bool parseNumber(const std::string& s, size_t& pos) {
                size_t start = pos;
                bool isFloat = false;
                if (pos < s.size() && (s[pos] == '-' || s[pos] == '+')) pos++;
                while (pos < s.size() && isdigit(s[pos])) pos++;
                if (pos < s.size() && s[pos] == '.') {
                    isFloat = true;
                    pos++;
                    while (pos < s.size() && isdigit(s[pos])) pos++;
                }
                if (pos < s.size() && (s[pos] == 'e' || s[pos] == 'E')) {
                    isFloat = true;
                    pos++;
                    if (pos < s.size() && (s[pos] == '-' || s[pos] == '+')) pos++;
                    while (pos < s.size() && isdigit(s[pos])) pos++;
                }
                
                std::string num = s.substr(start, pos - start);
                if (isFloat) {
                    type_ = realValue;
                    real_ = std::stod(num);
                } else {
                    type_ = intValue;
                    int_ = std::stoi(num);
                }
                return true;
            }
            
            bool parseBool(const std::string& s, size_t& pos) {
                if (s.substr(pos, 4) == "true") { pos += 4; type_ = booleanValue; int_ = 1; return true; }
                if (s.substr(pos, 5) == "false") { pos += 5; type_ = booleanValue; int_ = 0; return true; }
                return false;
            }
            
            bool parseNull(const std::string& s, size_t& pos) {
                if (s.substr(pos, 4) == "null") { pos += 4; type_ = nullValue; return true; }
                return false;
            }
        };
        
        class StreamWriterBuilder {
        public:
            std::string write(const Value& root) {
                return toString(root);
            }
        private:
            std::string toString(const Value& v) {
                switch (v.type_) {
                    case Value::nullValue: return "null";
                    case Value::booleanValue: return v.int_ ? "true" : "false";
                    case Value::intValue: return std::to_string(v.int_);
                    case Value::realValue: return std::to_string(v.real_);
                    case Value::stringValue: return "\"" + escapeString(v.string_) + "\"";
                    case Value::arrayValue: {
                        std::string s = "[";
                        for (size_t i = 0; i < v.array_.size(); i++) {
                            if (i > 0) s += ",";
                            s += toString(v.array_[i]);
                        }
                        return s + "]";
                    }
                    case Value::objectValue: {
                        std::string s = "{";
                        bool first = true;
                        for (const auto& kv : v.object_) {
                            if (!first) s += ",";
                            s += "\"" + escapeString(kv.first) + "\":" + toString(kv.second);
                            first = false;
                        }
                        return s + "}";
                    }
                    default: return "null";
                }
            }
            std::string escapeString(const std::string& s) {
                std::string r;
                for (char c : s) {
                    if (c == '"' || c == '\\') r += '\\';
                    r += c;
                }
                return r;
            }
        };
        
        inline std::string writeString(const StreamWriterBuilder& builder, const Value& root) {
            return builder.write(root);
        }
        
        class CharReader {
        public:
            virtual bool parse(const char* begin, const char* end, Value* root, std::string* errs) = 0;
        };
        
        class CharReaderBuilder {
        public:
            std::unique_ptr<CharReader> newCharReader() {
                return std::make_unique<SimpleCharReader>();
            }
        private:
            class SimpleCharReader : public CharReader {
            public:
                bool parse(const char* begin, const char* end, Value* root, std::string* errs) override {
                    if (!root) return false;
                    std::string text(begin, end);
                    // Skip HTTP headers if present
                    size_t bodyStart = text.find("\r\n\r\n");
                    if (bodyStart == std::string::npos) bodyStart = text.find("\n\n");
                    if (bodyStart != std::string::npos) {
                        text = text.substr(bodyStart + (text[bodyStart+1] == '\n' ? 2 : 4));
                    }
                    bool ok = root->parse(text);
                    if (!ok && errs) *errs = "Parse error";
                    return ok;
                }
            };
        };
        
        inline bool parseFromStream(const CharReaderBuilder& builder, 
                                    std::unique_ptr<std::istream> is,
                                    Value* root, std::string* errs) {
            std::string str((std::istreambuf_iterator<char>(*is)),
                            std::istreambuf_iterator<char>());
            CharReaderBuilder::SimpleCharReader reader;
            return reader.parse(str.c_str(), str.c_str() + str.size(), root, errs);
        }
    } // namespace Json
    #endif
#endif

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET SocketHandle;
    #define INVALID_SOCKET_VAL INVALID_SOCKET
    #define CLOSE_SOCKET(s) closesocket(s)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    typedef int SocketHandle;
    #define INVALID_SOCKET_VAL -1
    #define CLOSE_SOCKET(s) close(s)
#endif

// ============================================================================
// BITCOIN CORE RPC CLIENT
// Compatible with Bitcoin Core 0.17.0+
// ============================================================================
class BitcoinRpcClient {
private:
    std::string host;
    int port;
    std::string username;
    std::string password;
    std::atomic<bool> connected{false};
    std::atomic<int> requestId{0};
    std::mutex rpcMutex;
    
public:
    // Default constructor
    BitcoinRpcClient() : host("127.0.0.1"), port(8332), username(""), password("") {}
    
    // Constructor with parameters
    BitcoinRpcClient(const std::string& h, int p, const std::string& u, const std::string& pass)
        : host(h), port(p), username(u), password(pass) {}
    
    // Disable copy (mutex member makes it non-copyable)
    BitcoinRpcClient(const BitcoinRpcClient&) = delete;
    BitcoinRpcClient& operator=(const BitcoinRpcClient&) = delete;
    
    // Allow move
    BitcoinRpcClient(BitcoinRpcClient&& other) noexcept
        : host(std::move(other.host)), port(other.port), 
          username(std::move(other.username)), password(std::move(other.password)),
          connected(other.connected.load()), requestId(other.requestId.load()) {}
    
    BitcoinRpcClient& operator=(BitcoinRpcClient&& other) noexcept {
        if (this != &other) {
            host = std::move(other.host);
            port = other.port;
            username = std::move(other.username);
            password = std::move(other.password);
            connected = other.connected.load();
            requestId = other.requestId.load();
        }
        return *this;
    }
    
    ~BitcoinRpcClient() = default;
    
    // Helper for JSON parsing (works with both jsoncpp and fallback)
    bool parseJsonResponse(const std::string& response, Json::Value& root) {
        std::string errors;
        
#ifdef HAS_JSONCPP
        // Use jsoncpp's CharReaderBuilder
        Json::CharReaderBuilder builder;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        return reader->parse(response.c_str(), response.c_str() + response.size(), &root, &errors);
#else
        // Use fallback parser
        Json::CharReaderBuilder builder;
        std::string errs;
        return Json::parseFromStream(builder, 
            std::unique_ptr<std::istream>(new std::istringstream(response)), &root, &errs);
#endif
    }
    
private:
    
    // HTTP Basic Auth base64 encoding
    std::string base64Encode(const std::string& input) {
        static const char* base64_chars = 
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string ret;
        int i = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];
        
        for (size_t in_len = input.size(), pos = 0; in_len--; ) {
            char_array_3[i++] = input[pos++];
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;
                
                for(i = 0; i < 4; i++) ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }
        
        if (i) {
            for(int j = i; j < 3; j++) char_array_3[j] = '\0';
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            
            for (int j = 0; j < (i + 1); j++) ret += base64_chars[char_array_4[j]];
            while((i++ < 3)) ret += '=';
        }
        return ret;
    }
    
    // Simple HTTP POST request with verbose error reporting
    // Uses blocking sockets with timeout for maximum compatibility
    std::string httpPost(const std::string& requestBody, std::string* outError = nullptr) {
        std::lock_guard<std::mutex> lock(rpcMutex);
        
        std::cerr << "[RPC Debug] Starting httpPost to " << host << ":" << port << "\n";
        
        // Initialize Windows Sockets
        #ifdef _WIN32
            static bool wsaInitialized = false;
            if (!wsaInitialized) {
                WSADATA wsaData;
                int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
                if (wsaResult != 0) {
                    std::cerr << "[RPC Debug] WSAStartup failed: " << wsaResult << "\n";
                    if (outError) *outError = "WSAStartup failed: " + std::to_string(wsaResult);
                    return "";
                }
                std::cerr << "[RPC Debug] WSAStartup OK\n";
                wsaInitialized = true;
            }
        #endif
        
        // Create socket
        SocketHandle sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET_VAL) {
            std::cerr << "[RPC Debug] socket() failed\n";
            if (outError) {
                #ifdef _WIN32
                    *outError = "socket() failed: " + std::to_string(WSAGetLastError());
                #else
                    *outError = "socket() failed";
                #endif
            }
            return "";
        }
        std::cerr << "[RPC Debug] Socket created\n";
        
        // Set socket timeouts (5 seconds for connect, 10 for receive)
        #ifdef _WIN32
            DWORD timeoutMs = 10000;  // 10 seconds
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
            timeoutMs = 5000;  // 5 seconds
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
        #else
            struct timeval tv;
            tv.tv_sec = 10;
            tv.tv_usec = 0;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            tv.tv_sec = 5;
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        #endif
        
        // Setup server address
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        
        // Use direct IP for localhost, resolve for others
        if (host == "127.0.0.1" || host == "localhost") {
            serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
            std::cerr << "[RPC Debug] Using 127.0.0.1 for localhost\n";
        } else {
            struct hostent* hostEntry = gethostbyname(host.c_str());
            if (!hostEntry) {
                std::cerr << "[RPC Debug] Failed to resolve hostname: " << host << "\n";
                if (outError) *outError = "Failed to resolve hostname: " + host;
                CLOSE_SOCKET(sock);
                return "";
            }
            memcpy(&serverAddr.sin_addr, hostEntry->h_addr_list[0], hostEntry->h_length);
        }
        
        // Connect (blocking with timeout due to SO_SNDTIMEO)
        std::cerr << "[RPC Debug] Connecting to " << host << ":" << port << "...\n";
        int connResult = connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (connResult < 0) {
            int err = 0;
            #ifdef _WIN32
                err = WSAGetLastError();
                std::cerr << "[RPC Debug] Connect failed with error: " << err << "\n";
            #else
                err = errno;
                std::cerr << "[RPC Debug] Connect failed: " << strerror(err) << "\n";
            #endif
            if (outError) {
                #ifdef _WIN32
                    if (err == WSAETIMEDOUT) {
                        *outError = "Connection timeout - Bitcoin Core not responding on port " + std::to_string(port);
                    } else if (err == WSAECONNREFUSED) {
                        *outError = "Connection refused - No server listening on port " + std::to_string(port);
                    } else {
                        *outError = "connect() failed: " + std::to_string(err);
                    }
                #else
                    if (err == ETIMEDOUT) {
                        *outError = "Connection timeout - Bitcoin Core not responding";
                    } else if (err == ECONNREFUSED) {
                        *outError = "Connection refused - No server listening";
                    } else {
                        *outError = "connect() failed: " + std::string(strerror(err));
                    }
                #endif
            }
            CLOSE_SOCKET(sock);
            return "";
        }
        std::cerr << "[RPC Debug] Connected!\n";
        
        // Build HTTP request
        std::string auth = username + ":" + password;
        std::string authB64 = base64Encode(auth);
        
        std::string httpRequest = 
            "POST / HTTP/1.1\r\n"
            "Host: " + host + ":" + std::to_string(port) + "\r\n"
            "Authorization: Basic " + authB64 + "\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + std::to_string(requestBody.length()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            requestBody;
        
        // Send request (blocking with timeout)
        std::cerr << "[RPC Debug] Sending request...\n";
        int totalSent = 0;
        while (totalSent < (int)httpRequest.length()) {
            int sent = send(sock, httpRequest.c_str() + totalSent, (int)httpRequest.length() - totalSent, 0);
            if (sent <= 0) {
                std::cerr << "[RPC Debug] Send failed\n";
                if (outError) {
                    #ifdef _WIN32
                        int err = WSAGetLastError();
                        if (err == WSAETIMEDOUT) {
                            *outError = "Send timeout";
                        } else {
                            *outError = "send() failed: " + std::to_string(err);
                        }
                    #else
                        *outError = "send() failed";
                    #endif
                }
                CLOSE_SOCKET(sock);
                return "";
            }
            totalSent += sent;
        }
        std::cerr << "[RPC Debug] Request sent, waiting for response...\n";
        
        // Receive response (blocking with timeout)
        std::string response;
        char buffer[4096];
        int received;
        bool gotData = false;
        
        while ((received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[received] = '\0';
            response += buffer;
            gotData = true;
        }
        
        if (received < 0 && !gotData) {
            std::cerr << "[RPC Debug] Receive failed\n";
            if (outError) {
                #ifdef _WIN32
                    int err = WSAGetLastError();
                    if (err == WSAETIMEDOUT) {
                        *outError = "Receive timeout - server not responding";
                    } else if (err == WSAECONNRESET) {
                        *outError = "Connection reset by server";
                    } else {
                        *outError = "recv() failed: " + std::to_string(err);
                    }
                #else
                    *outError = "recv() failed: " + std::string(strerror(errno));
                #endif
            }
            CLOSE_SOCKET(sock);
            return "";
        }
        
        std::cerr << "[RPC Debug] Got response: " << response.length() << " bytes\n";
        CLOSE_SOCKET(sock);
        
        if (response.empty()) {
            if (outError) *outError = "Empty response from server";
            return "";
        }
        
        // Extract JSON body from HTTP response
        size_t bodyStart = response.find("\r\n\r\n");
        if (bodyStart != std::string::npos) {
            return response.substr(bodyStart + 4);
        }
        
        bodyStart = response.find("\n\n");
        if (bodyStart != std::string::npos) {
            return response.substr(bodyStart + 2);
        }
        
        // If no header separator found, return entire response
        return response;
    }

public:
    // Note: Constructor, destructor, move operators are defined at class start
    // Test connection to Bitcoin Core with verbose error reporting
    bool testConnection(std::string* outError = nullptr) {
        Json::Value request(Json::objectValue);
        request["jsonrpc"] = "1.0";
        request["id"] = requestId++;
        request["method"] = "getblockchaininfo";
        request["params"] = Json::Value(Json::arrayValue);
        
        Json::StreamWriterBuilder builder;
        std::string requestStr = Json::writeString(builder, request);
        
        std::string response = httpPost(requestStr, outError);
        if (response.empty()) {
            if (outError && outError->empty()) *outError = "Empty response from server";
            return false;
        }
        
        Json::Value root;
        if (!parseJsonResponse(response, root)) {
            if (outError) *outError = "Failed to parse JSON response";
            return false;
        }
        
        if (root.isMember("error") && !root["error"].isNull()) {
            if (outError) {
                std::string errMsg = root["error"]["message"].asString();
                int errCode = root["error"]["code"].asInt();
                *outError = "RPC Error " + std::to_string(errCode) + ": " + errMsg;
            }
            return false;
        }
        
        if (root.isMember("result") && !root["result"].isNull()) {
            connected = true;
            return true;
        }
        
        if (outError) *outError = "Invalid response format";
        return false;
    }
    
    // Abort any running scantxoutset scan
    void abortScan() {
        Json::Value request(Json::objectValue);
        request["jsonrpc"] = "1.0";
        request["id"] = requestId++;
        request["method"] = "scantxoutset";
        
        Json::Value params(Json::arrayValue);
        params.append("abort");
        request["params"] = params;
        
        Json::StreamWriterBuilder builder;
        std::string requestStr = Json::writeString(builder, request);
        httpPost(requestStr); // Don't care about response
    }
    
    // Get address balance using scantxoutset (0.17.0+)
    // Returns balance in satoshis, -1 if error, 0 if no balance
    int64_t getAddressBalance(const std::string& address, std::string* debugInfo = nullptr) {
        if (address.empty()) return -1;
        
        // Try scantxoutset first (0.17.0+, scans blockchain UTXO set)
        Json::Value request(Json::objectValue);
        request["jsonrpc"] = "1.0";
        request["id"] = requestId++;
        request["method"] = "scantxoutset";
        
        Json::Value params(Json::arrayValue);
        params.append("start");
        Json::Value scanObjects(Json::arrayValue);
        scanObjects.append("addr(" + address + ")");
        params.append(scanObjects);
        
        request["params"] = params;
        
        Json::StreamWriterBuilder builder;
        std::string requestStr = Json::writeString(builder, request);
        
        if (debugInfo) *debugInfo += "Request: " + requestStr + "\n";
        
        std::string response = httpPost(requestStr);
        if (response.empty()) {
            if (debugInfo) *debugInfo += "Empty response from server\n";
            return -1;
        }
        
        if (debugInfo) *debugInfo += "Response: " + response.substr(0, 500) + "\n";
        
        Json::Value root;
        if (!parseJsonResponse(response, root)) {
            if (debugInfo) *debugInfo += "Failed to parse JSON\n";
            return -1;
        }
        
        if (root.isMember("error") && !root["error"].isNull()) {
            std::string errMsg = root["error"]["message"].asString();
            int errCode = root["error"]["code"].asInt();
            if (debugInfo) *debugInfo += "RPC Error " + std::to_string(errCode) + ": " + errMsg + "\n";
            
            // If scan is already in progress, abort it and retry
            if (errCode == -8 && errMsg.find("Scan already in progress") != std::string::npos) {
                if (debugInfo) *debugInfo += "Aborting stuck scan and retrying...\n";
                abortScan();
                // Retry after abort
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return getAddressBalance(address, debugInfo);
            }
            
            // scantxoutset not available or other error
            return -1;
        }
        
        if (root.isMember("result") && root["result"].isMember("total_amount")) {
            double btc = root["result"]["total_amount"].asDouble();
            int64_t satoshis = static_cast<int64_t>(btc * 100000000);
            if (debugInfo) *debugInfo += "Balance: " + std::to_string(satoshis) + " satoshis\n";
            return satoshis;
        }
        
        if (debugInfo) *debugInfo += "No total_amount in result\n";
        return 0; // No UTXOs found
    }
    
    // Fallback using listunspent (slower, for older versions)
    int64_t getBalanceListUnspent(const std::string& address) {
        Json::Value request(Json::objectValue);
        request["jsonrpc"] = "1.0";
        request["id"] = requestId++;
        request["method"] = "listunspent";
        
        Json::Value params(Json::arrayValue);
        params.append(0);     // minconf
        params.append(9999999); // maxconf
        Json::Value addresses(Json::arrayValue);
        addresses.append(address);
        Json::Value options(Json::objectValue);
        options["addresses"] = addresses;
        params.append(options);
        
        request["params"] = params;
        
        Json::StreamWriterBuilder builder;
        std::string requestStr = Json::writeString(builder, request);
        
        std::string response = httpPost(requestStr);
        if (response.empty()) return -1;
        
        Json::Value root;
        if (!parseJsonResponse(response, root)) return -1;
        
        if (root.isMember("error") && !root["error"].isNull()) {
            return -1; // Method failed
        }
        
        if (root.isMember("result") && root["result"].isArray()) {
            int64_t totalSatoshis = 0;
            for (const auto& utxo : root["result"]) {
                if (utxo.isMember("amount")) {
                    double btc = utxo["amount"].asDouble();
                    totalSatoshis += static_cast<int64_t>(btc * 100000000);
                }
            }
            return totalSatoshis;
        }
        
        return -1;
    }
    
    // Batch check multiple addresses (more efficient)
    std::vector<std::pair<std::string, int64_t>> getAddressesBalanceBatch(
        const std::vector<std::string>& addresses) {
        
        std::vector<std::pair<std::string, int64_t>> results;
        
        // Try scantxoutset with multiple addresses
        Json::Value request(Json::objectValue);
        request["jsonrpc"] = "1.0";
        request["id"] = requestId++;
        request["method"] = "scantxoutset";
        
        Json::Value params(Json::arrayValue);
        params.append("start");
        Json::Value scanObjects(Json::arrayValue);
        for (const auto& addr : addresses) {
            scanObjects.append("addr(" + addr + ")");
        }
        params.append(scanObjects);
        
        request["params"] = params;
        
        Json::StreamWriterBuilder builder;
        std::string requestStr = Json::writeString(builder, request);
        
        std::string response = httpPost(requestStr);
        if (response.empty()) {
            // Fallback to individual checks
            for (const auto& addr : addresses) {
                int64_t bal = getAddressBalance(addr);
                results.push_back({addr, bal});
            }
            return results;
        }
        
        Json::Value root;
        if (!parseJsonResponse(response, root)) {
            // Fallback to individual checks
            for (const auto& addr : addresses) {
                int64_t bal = getAddressBalance(addr);
                results.push_back({addr, bal});
            }
            return results;
        }
        
        if (root.isMember("error") && !root["error"].isNull()) {
            // Fallback to individual checks
            for (const auto& addr : addresses) {
                int64_t bal = getAddressBalance(addr);
                results.push_back({addr, bal});
            }
            return results;
        }
        
        // Parse results
        if (root.isMember("result") && root["result"].isMember("unspents")) {
            const Json::Value& unspents = root["result"]["unspents"];
            std::map<std::string, int64_t> addrBalances;
            
            for (const auto& utxo : unspents) {
                if (utxo.isMember("desc") && utxo.isMember("amount")) {
                    std::string desc = utxo["desc"].asString();
                    // Extract address from descriptor like "addr(1A...)"
                    size_t addrStart = desc.find("addr(");
                    if (addrStart != std::string::npos) {
                        addrStart += 5;
                        size_t addrEnd = desc.find(")", addrStart);
                        if (addrEnd != std::string::npos) {
                            std::string addr = desc.substr(addrStart, addrEnd - addrStart);
                            double btc = utxo["amount"].asDouble();
                            addrBalances[addr] += static_cast<int64_t>(btc * 100000000);
                        }
                    }
                }
            }
            
            // Return results for all requested addresses
            for (const auto& addr : addresses) {
                auto it = addrBalances.find(addr);
                if (it != addrBalances.end()) {
                    results.push_back({addr, it->second});
                } else {
                    results.push_back({addr, 0}); // No balance
                }
            }
        } else {
            // No UTXOs found for any address
            for (const auto& addr : addresses) {
                results.push_back({addr, 0});
            }
        }
        
        return results;
    }
    
    bool isConnected() const { return connected.load(); }
    
    std::string getHost() const { return host; }
    int getPort() const { return port; }
};

// ============================================================================
// RPC BALANCE CHECKER WRAPPER
// Replaces BloomFilter functionality with RPC balance checks
// ============================================================================
class RpcBalanceChecker {
private:
    BitcoinRpcClient rpcClient;
    std::atomic<bool> enabled{false};
    std::atomic<int64_t> minBalanceSatoshis{0}; // Minimum balance to report (default 0)
    std::mutex checkerMutex;
    
public:
    RpcBalanceChecker() = default;
    
    RpcBalanceChecker(const std::string& host, int port, 
                      const std::string& user, const std::string& pass,
                      int64_t minBalance = 0)
        : rpcClient(host, port, user, pass), minBalanceSatoshis(minBalance) {
    }
    
    bool initialize(const std::string& host, int port, 
                    const std::string& user, const std::string& pass,
                    int64_t minBalance = 0) {
        rpcClient = BitcoinRpcClient(host, port, user, pass);
        minBalanceSatoshis = minBalance;
        
        std::cout << "[RPC] Connecting to Bitcoin Core at " << host << ":" << port << "...\n";
        std::cout << "[RPC] Make sure Bitcoin Core has finished loading (check debug.log)\n";
        
        std::string errorMsg;
        if (rpcClient.testConnection(&errorMsg)) {
            enabled = true;
            std::cout << "[RPC] Connected successfully to Bitcoin Core!\n";
            
            // Test if scantxoutset is working
            std::cout << "[RPC] Testing scantxoutset (blockchain scan)...\n";
            std::string debugInfo;
            // Test with a known address (Genesis block address)
            int64_t testBalance = rpcClient.getAddressBalance("1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa", &debugInfo);
            if (testBalance < 0) {
                std::cerr << "[RPC] Warning: scantxoutset test failed!\n";
                std::cerr << "[RPC] Debug info: " << debugInfo << "\n";
                std::cerr << "[RPC] This might mean:\n";
                std::cerr << "  - scantxoutset is not available (needs Bitcoin Core 0.17.0+)\n";
                std::cerr << "  - txindex is not enabled (add txindex=1 to bitcoin.conf and reindex)\n";
                std::cerr << "[RPC] Continuing anyway, but balance checks might not work...\n";
            } else {
                std::cout << "[RPC] scantxoutset is working! (test address balance: " << formatSatoshis(testBalance) << ")\n";
            }
            return true;
        } else {
            std::cerr << "[RPC] Failed to connect!\n";
            if (!errorMsg.empty()) {
                std::cerr << "[RPC] Technical details: " << errorMsg << "\n";
            }
            std::cerr << "\n[RPC] Troubleshooting checklist:\n";
            std::cerr << "  1. Is Bitcoin Core running? Check Task Manager for bitcoind.exe\n";
            std::cerr << "  2. Has Bitcoin Core finished loading? Look for 'Done loading' in debug.log\n";
            std::cerr << "  3. Is bitcoin.conf in the correct location?\n";
            std::cerr << "     - Default: %APPDATA%\\Bitcoin\\bitcoin.conf\n";
            std::cerr << "     - Custom datadir: <datadir>\\bitcoin.conf\n";
            std::cerr << "  4. Does bitcoin.conf contain these lines?\n";
            std::cerr << "       server=1\n";
            std::cerr << "       rpcuser=" << user << "\n";
            std::cerr << "       rpcpassword=<your_password>\n";
            std::cerr << "       rpcport=" << port << "\n";
            std::cerr << "       rpcallowip=127.0.0.1\n";
            std::cerr << "  5. Test with bitcoin-cli (if available):\n";
            std::cerr << "       bitcoin-cli.exe -datadir=F:\\bitcoin -rpcuser=" << user << " -rpcpassword=<password> getblockchaininfo\n";
            std::cerr << "  6. Test with PowerShell:\n";
            std::cerr << "       $cred = [Convert]::ToBase64String([Text.Encoding]::ASCII.GetBytes('" << user << ":<password>'))\n";
            std::cerr << "       Invoke-RestMethod -Uri 'http://" << host << ":" << port << "/' -Headers @{Authorization=\"Basic $cred\"} -Body '{\"jsonrpc\":\"1.0\",\"method\":\"getblockchaininfo\",\"params\":[]}' -ContentType 'application/json'\n";
            std::cerr << "\n[RPC] Note: If bitcoin-cli works but GpuCracker doesn't, there may be a firewall or network configuration issue.\n";
            return false;
        }
    }
    
    // Check single address balance
    // Returns: balance in satoshis, or -1 if error
    int64_t checkAddress(const std::string& address) {
        if (!enabled) return -1;
        return rpcClient.getAddressBalance(address);
    }
    
    // Check if address has balance >= minBalance (considered a "hit")
    bool checkAddressHit(const std::string& address) {
        if (!enabled) return false;
        
        int64_t balance = rpcClient.getAddressBalance(address);
        if (balance < 0) return false; // Error
        
        return balance >= minBalanceSatoshis;
    }
    
    // Format satoshis to BTC string
    static std::string formatBtc(int64_t satoshis) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(8) << (satoshis / 100000000.0) << " BTC";
        return ss.str();
    }
    
    // Format satoshis to human readable
    static std::string formatSatoshis(int64_t satoshis) {
        if (satoshis >= 100000000) {
            return formatBtc(satoshis);
        } else if (satoshis >= 100000) {
            return std::to_string(satoshis / 1000) + "k sats";
        } else {
            return std::to_string(satoshis) + " sats";
        }
    }
    
    bool isEnabled() const { return enabled.load(); }
    int64_t getMinBalance() const { return minBalanceSatoshis.load(); }
    void setMinBalance(int64_t minBal) { minBalanceSatoshis = minBal; }
    
    BitcoinRpcClient& getClient() { return rpcClient; }
};
