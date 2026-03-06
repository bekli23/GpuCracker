// bsgs_bloom_x.h - BSGS with Bloom Filter Address Checking
// Checks generated addresses against bloom filter during BSGS search

#ifndef BSGS_BLOOM_X_H
#define BSGS_BLOOM_X_H

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <cstring>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <thread>
#include <atomic>
#include "bloom.h"

// Forward declarations for hash functions
#include <openssl/sha.h>
#include <openssl/ripemd.h>

// Type definition for large integers
#include <boost/multiprecision/cpp_int.hpp>
typedef boost::multiprecision::uint256_t uint256_t;

// Progress tracking for real-time display
// Single line format for maximum compatibility (Windows & Linux)
class BloomCheckProgress {
public:
    std::atomic<uint64_t> addressesChecked{0};
    std::atomic<uint64_t> matchesFound{0};
    std::chrono::steady_clock::time_point startTime;
    std::string currentAddress;
    std::mutex progressMutex;
    uint64_t updateCounter = 0;
    
    BloomCheckProgress() {
        startTime = std::chrono::steady_clock::now();
    }
    
    void incrementChecked() {
        addressesChecked++;
    }
    
    void incrementMatches() {
        matchesFound++;
    }
    
    void addAddress(const std::string& addr) {
        if (addr.empty()) return;
        
        std::lock_guard<std::mutex> lock(progressMutex);
        currentAddress = addr;
        updateCounter++;
        
        // Update display every 100 addresses
        if (updateCounter % 100 == 0) {
            updateDisplay();
        }
    }
    
    void updateDisplay() {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - startTime).count();
        uint64_t checked = addressesChecked.load();
        uint64_t matches = matchesFound.load();
        
        double rate = (elapsed > 0) ? (checked / elapsed) : 0;
        
        std::string rateStr;
        if (rate >= 1e6) {
            rateStr = std::to_string((int)(rate / 1e6)) + "M";
        } else if (rate >= 1e3) {
            rateStr = std::to_string((int)(rate / 1e3)) + "k";
        } else {
            rateStr = std::to_string((int)rate);
        }
        
        // Format: [ADDR] address | [CHECK] Total: X Rate: X/s Matches: X
        // Using \r to stay on same line
        std::cout << "\r[ADDR] " << currentAddress;
        std::cout << " | [CHECK] T:" << checked;
        std::cout << " R:" << rateStr << "/s";
        std::cout << " M:" << matches;
        
        // Pad with spaces to clear leftover characters
        std::cout << "          ";
        std::cout << std::flush;
    }
    
    void printProgress() {
        updateDisplay();
    }
    
    void finish() {
        std::cout << std::endl;
    }
};

// BSGS Bloom Extended - Address checking support
class BSGSBloomChecker {
public:
    struct MatchResult {
        std::string privateKeyHex;
        std::string publicKeyHex;
        std::string address;
        std::string addressType;
        bool found;
        
        MatchResult() : found(false) {}
    };

private:
    std::vector<std::unique_ptr<BloomFilter>> bloomFilters;
    std::vector<std::string> bloomFilterPaths;
    bool initialized;
    std::unique_ptr<BloomCheckProgress> progress;
    std::thread progressThread;
    std::atomic<bool> progressRunning{false};
    
    // Address derivation helpers
    std::string publicKeyToAddress(const std::vector<unsigned char>& pubKey) {
        return pubKeyToP2PKH(pubKey);
    }
    
    // P2PKH address (1...)
    std::string pubKeyToP2PKH(const std::vector<unsigned char>& pubKey) {
        if (pubKey.size() != 33 && pubKey.size() != 65) {
            return "";
        }
        
        unsigned char shaHash[32];
        SHA256(pubKey.data(), pubKey.size(), shaHash);
        
        unsigned char ripemdHash[20];
        RIPEMD160(shaHash, 32, ripemdHash);
        
        std::vector<unsigned char> payload;
        payload.reserve(25);
        payload.push_back(0x00);
        payload.insert(payload.end(), ripemdHash, ripemdHash + 20);
        
        unsigned char hash1[32], hash2[32];
        SHA256(payload.data(), payload.size(), hash1);
        SHA256(hash1, 32, hash2);
        
        payload.push_back(hash2[0]);
        payload.push_back(hash2[1]);
        payload.push_back(hash2[2]);
        payload.push_back(hash2[3]);
        
        if (payload.size() != 25) return "";
        
        std::string address = base58Encode(payload);
        if (!validateAddress(address)) return "";
        
        return address;
    }
    
    // Base58 encoding
    std::string base58Encode(const std::vector<unsigned char>& data) {
        static const char* alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
        
        int leadingZeros = 0;
        while (leadingZeros < (int)data.size() && data[leadingZeros] == 0) {
            leadingZeros++;
        }
        
        std::vector<unsigned char> digits;
        digits.reserve((data.size() - leadingZeros) * 138 / 100 + 1);
        
        for (size_t i = leadingZeros; i < data.size(); i++) {
            unsigned int carry = data[i];
            for (int j = (int)digits.size() - 1; j >= 0; j--) {
                carry += digits[j] * 256;
                digits[j] = carry % 58;
                carry /= 58;
            }
            while (carry > 0) {
                digits.insert(digits.begin(), carry % 58);
                carry /= 58;
            }
        }
        
        std::string result;
        result.reserve(leadingZeros + digits.size());
        
        for (int i = 0; i < leadingZeros; i++) result.push_back('1');
        for (unsigned char digit : digits) result.push_back(alphabet[digit]);
        
        return result;
    }
    
    // Base58 decoding
    std::vector<unsigned char> base58Decode(const std::string& str) {
        static const char* alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
        
        int leadingOnes = 0;
        while (leadingOnes < (int)str.size() && str[leadingOnes] == '1') {
            leadingOnes++;
        }
        
        std::vector<unsigned char> result;
        for (int i = leadingOnes; i < (int)str.size(); i++) {
            const char* p = strchr(alphabet, str[i]);
            if (p == nullptr) return {};
            
            int carry = p - alphabet;
            for (int j = (int)result.size() - 1; j >= 0; j--) {
                carry += 58 * result[j];
                result[j] = carry % 256;
                carry /= 256;
            }
            while (carry > 0) {
                result.insert(result.begin(), carry % 256);
                carry /= 256;
            }
        }
        
        result.insert(result.begin(), leadingOnes, 0);
        return result;
    }
    
    // Validate address
    bool validateAddress(const std::string& address) {
        if (address.empty() || address[0] != '1') return false;
        if (address.length() < 25 || address.length() > 35) return false;
        
        std::vector<unsigned char> decoded = base58Decode(address);
        if (decoded.size() != 25) return false;
        
        if (decoded[0] != 0x00) return false;
        
        unsigned char hash1[32], hash2[32];
        SHA256(decoded.data(), 21, hash1);
        SHA256(hash1, 32, hash2);
        
        for (int i = 0; i < 4; i++) {
            if (decoded[21 + i] != hash2[i]) return false;
        }
        
        return true;
    }

public:
    BSGSBloomChecker() : initialized(false), progressRunning(false) {}
    
    ~BSGSBloomChecker() {
        stopProgressThread();
    }
    
    void startProgressThread() {
        if (progressRunning) return;
        progress = std::make_unique<BloomCheckProgress>();
        progressRunning = true;
        progressThread = std::thread([this]() {
            while (progressRunning) {
                if (progress) progress->printProgress();
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        });
    }
    
    void stopProgressThread() {
        progressRunning = false;
        if (progressThread.joinable()) progressThread.join();
        if (progress) progress->finish();
    }
    
    bool loadBloomFilters(const std::vector<std::string>& paths) {
        bloomFilterPaths = paths;
        bloomFilters.clear();
        
        for (const auto& path : paths) {
            auto bloom = std::make_unique<BloomFilter>();
            if (!bloom->load(path)) {
                std::cerr << "[BSGS-BLOOM] Failed to load: " << path << std::endl;
                continue;
            }
            std::cout << "[BSGS-BLOOM] Loaded: " << path << std::endl;
            bloomFilters.push_back(std::move(bloom));
        }
        
        initialized = !bloomFilters.empty();
        if (initialized) startProgressThread();
        return initialized;
    }
    
    bool isActive() const {
        return initialized && !bloomFilters.empty();
    }
    
    MatchResult checkPublicKey(const secp256k1_pubkey& pubKey, const uint256_t& privateKey) {
        MatchResult result;
        
        if (!isActive()) return result;
        
        std::vector<unsigned char> pubKeyCompressed(33);
        std::vector<unsigned char> pubKeyUncompressed(65);
        
        secp256k1_context* tempCtx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
        
        size_t len = 33;
        secp256k1_ec_pubkey_serialize(tempCtx, pubKeyCompressed.data(), &len, &pubKey, SECP256K1_EC_COMPRESSED);
        len = 65;
        secp256k1_ec_pubkey_serialize(tempCtx, pubKeyUncompressed.data(), &len, &pubKey, SECP256K1_EC_UNCOMPRESSED);
        
        secp256k1_context_destroy(tempCtx);
        
        std::string addressComp = pubKeyToP2PKH(pubKeyCompressed);
        
        if (progress) {
            progress->incrementChecked();
            progress->addAddress(addressComp);
        }
        
        unsigned char shaHash[32];
        SHA256(pubKeyCompressed.data(), pubKeyCompressed.size(), shaHash);
        unsigned char hash160Comp[20];
        RIPEMD160(shaHash, 32, hash160Comp);
        std::vector<uint8_t> hash160VecComp(hash160Comp, hash160Comp + 20);
        
        for (const auto& bloom : bloomFilters) {
            if (bloom->check_hash160(hash160VecComp)) {
                result.found = true;
                result.address = addressComp;
                result.addressType = "P2PKH";
                result.publicKeyHex = bytesToHex(pubKeyCompressed);
                result.privateKeyHex = uint256ToHex(privateKey);
                if (progress) progress->incrementMatches();
                return result;
            }
        }
        
        SHA256(pubKeyUncompressed.data(), pubKeyUncompressed.size(), shaHash);
        unsigned char hash160Uncomp[20];
        RIPEMD160(shaHash, 32, hash160Uncomp);
        std::vector<uint8_t> hash160VecUncomp(hash160Uncomp, hash160Uncomp + 20);
        
        for (const auto& bloom : bloomFilters) {
            if (bloom->check_hash160(hash160VecUncomp)) {
                result.found = true;
                result.address = pubKeyToP2PKH(pubKeyUncompressed);
                result.addressType = "P2PKH-Legacy";
                result.publicKeyHex = bytesToHex(pubKeyUncompressed);
                result.privateKeyHex = uint256ToHex(privateKey);
                if (progress) progress->incrementMatches();
                return result;
            }
        }
        
        return result;
    }
    
    void printFinalStats() {
        if (progress) {
            stopProgressThread();
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - progress->startTime).count();
            uint64_t checked = progress->addressesChecked.load();
            uint64_t matches = progress->matchesFound.load();
            
            std::cout << "\n[BLOOM-CHECK] Final Statistics:\n";
            std::cout << "  Total addresses checked: " << checked << "\n";
            std::cout << "  Total matches found: " << matches << "\n";
            std::cout << "  Time elapsed: " << (int)(elapsed / 60) << "m " << (int)(elapsed) % 60 << "s\n";
            std::cout << "  Average rate: " << (int)(checked / elapsed) << " addr/s\n";
        }
    }
    
    std::string bytesToHex(const std::vector<unsigned char>& data) {
        static const char hexDigits[] = "0123456789abcdef";
        std::string result;
        result.reserve(data.size() * 2);
        for (auto b : data) {
            result.push_back(hexDigits[b >> 4]);
            result.push_back(hexDigits[b & 0xF]);
        }
        return result;
    }
    
    std::string uint256ToHex(const uint256_t& value) {
        if (value == 0) return "0";
        uint256_t temp = value;
        std::string result;
        while (temp > 0) {
            int digit = (int)(temp & 0xF);
            result = "0123456789abcdef"[digit] + result;
            temp >>= 4;
        }
        return result;
    }
    
    static void printMatch(const MatchResult& match) {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "  [BSGS-BLOOM] ADDRESS MATCH FOUND!" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        std::cout << "  Address:     " << match.address << std::endl;
        std::cout << "  Type:        " << match.addressType << std::endl;
        std::cout << "  Public Key:  " << match.publicKeyHex << std::endl;
        std::cout << "  Private Key: " << match.privateKeyHex << std::endl;
        std::cout << std::string(70, '=') << "\n" << std::endl;
        saveMatchToFile(match);
    }
    
    static void saveMatchToFile(const MatchResult& match) {
        std::ofstream out("found_keys.txt", std::ios::app);
        if (out.is_open()) {
            out << "Found: " << match.address << std::endl;
            out << "  Type: " << match.addressType << std::endl;
            out << "  PubKey: " << match.publicKeyHex << std::endl;
            out << "  PrivKey: " << match.privateKeyHex << std::endl;
            out << "  Timestamp: " << getTimestamp() << std::endl;
            out << std::string(50, '-') << std::endl;
            out.close();
        }
    }
    
    static std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        #ifdef _WIN32
        struct tm timeinfo;
        localtime_s(&timeinfo, &time);
        ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
        #else
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        #endif
        return ss.str();
    }
};

// Integration class for BSGS mode
class BSGSBloomIntegration {
public:
    BSGSBloomChecker checker;
    bool useBloomAddressCheck;
    
    BSGSBloomIntegration() : useBloomAddressCheck(false) {}
    
    ~BSGSBloomIntegration() {
        if (useBloomAddressCheck) checker.printFinalStats();
    }
    
    bool initialize(const std::vector<std::string>& bloomPaths) {
        if (bloomPaths.empty()) return false;
        
        std::cout << "\n[BSGS-BLOOM] Initializing Bloom Filter Address Check..." << std::endl;
        std::cout << "[BSGS-BLOOM] Real-time progress on single line (ADDR + CHECK)\n" << std::endl;
        
        if (!checker.loadBloomFilters(bloomPaths)) {
            std::cerr << "[BSGS-BLOOM] Failed to load any bloom filters!" << std::endl;
            return false;
        }
        
        useBloomAddressCheck = true;
        std::cout << "[BSGS-BLOOM] Bloom filter(s) loaded successfully\n" << std::endl;
        return true;
    }
    
    bool isActive() const {
        return useBloomAddressCheck && checker.isActive();
    }
    
    bool checkAndReport(const secp256k1_pubkey& pubKey, const uint256_t& privKey) {
        if (!isActive()) return false;
        
        auto match = checker.checkPublicKey(pubKey, privKey);
        if (match.found) {
            BSGSBloomChecker::printMatch(match);
            return true;
        }
        return false;
    }
    
    void printStats() {
        if (useBloomAddressCheck) checker.printFinalStats();
    }
};

#endif // BSGS_BLOOM_X_H
