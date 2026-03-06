#pragma once

// ============================================================================
// BITCOIN BLOCKCHAIN READER - Reads blk*.dat files directly
// ============================================================================

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <memory>
#include <thread>
#include <mutex>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
#else
    #include <dirent.h>
    #include <sys/types.h>
#endif

#include <openssl/sha.h>
#include <openssl/ripemd.h>

namespace BlockchainReader {

// UTXO structure for tracking unspent outputs
struct UTXO {
    unsigned char hash[32];
    uint32_t vout;
    uint64_t value;
    std::string address;
    uint64_t height;
    
    bool operator<(const UTXO& other) const {
        int cmp = memcmp(hash, other.hash, 32);
        if (cmp != 0) return cmp < 0;
        return vout < other.vout;
    }
};

// Address balance information
struct AddressBalance {
    uint64_t balanceSatoshis = 0;
    uint64_t receivedSatoshis = 0;
    uint64_t sentSatoshis = 0;
    uint64_t txCount = 0;
    uint64_t firstSeenHeight = 0;
    uint64_t lastSeenHeight = 0;
};

struct uint256 {
    unsigned char data[32];
    
    uint256() { memset(data, 0, 32); }
    
    std::string ToString() const {
        char hex[65];
        for (int i = 0; i < 32; i++) {
            sprintf(hex + i * 2, "%02x", data[31 - i]);
        }
        hex[64] = 0;
        return std::string(hex);
    }
    
    bool operator<(const uint256& other) const {
        return memcmp(data, other.data, 32) < 0;
    }
    
    bool operator==(const uint256& other) const {
        return memcmp(data, other.data, 32) == 0;
    }
};

// ============================================================================
// Simple Bitcoin Script Parser
// ============================================================================
class ScriptParser {
public:
    static std::string ExtractAddress(const std::vector<unsigned char>& script) {
        if (script.empty()) return "";
        
        // P2PKH: OP_DUP OP_HASH160 <20 bytes> OP_EQUALVERIFY OP_CHECKSIG
        if (script.size() == 25 && script[0] == 0x76 && script[1] == 0xa9 && 
            script[2] == 0x14 && script[23] == 0x88 && script[24] == 0xac) {
            std::vector<unsigned char> hash160(script.begin() + 3, script.begin() + 23);
            return Hash160ToAddress(hash160, 0x00);
        }
        
        // P2SH: OP_HASH160 <20 bytes> OP_EQUAL
        if (script.size() == 23 && script[0] == 0xa9 && 
            script[1] == 0x14 && script[22] == 0x87) {
            std::vector<unsigned char> hash160(script.begin() + 2, script.begin() + 22);
            return Hash160ToAddress(hash160, 0x05);
        }
        
        // P2WPKH (Bech32): 0x00 0x14 <20 bytes>
        if (script.size() == 22 && script[0] == 0x00 && script[1] == 0x14) {
            std::vector<unsigned char> hash160(script.begin() + 2, script.begin() + 22);
            return Hash160ToBech32(hash160, 0);
        }
        
        return "";
    }

private:
    static std::string Hash160ToAddress(const std::vector<unsigned char>& hash160, unsigned char version) {
        std::vector<unsigned char> data;
        data.push_back(version);
        data.insert(data.end(), hash160.begin(), hash160.end());
        
        unsigned char hash1[32], hash2[32];
        SHA256(data.data(), data.size(), hash1);
        SHA256(hash1, 32, hash2);
        
        data.push_back(hash2[0]);
        data.push_back(hash2[1]);
        data.push_back(hash2[2]);
        data.push_back(hash2[3]);
        
        return Base58Encode(data);
    }
    
    static std::string Hash160ToBech32(const std::vector<unsigned char>& hash160, int witness_version) {
        std::string result = "bc1q";
        for (size_t i = 0; i < hash160.size(); i++) {
            result += "qpzry9x8gf2tvdw0s3jn54khce6mua7l"[(hash160[i] >> 3) & 31];
            result += "qpzry9x8gf2tvdw0s3jn54khce6mua7l"[hash160[i] & 31];
        }
        return result;
    }
    
    static std::string Base58Encode(const std::vector<unsigned char>& data) {
        static const char* b58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
        
        int leadingZeros = 0;
        while (leadingZeros < (int)data.size() && data[leadingZeros] == 0) {
            leadingZeros++;
        }
        
        std::vector<unsigned char> result;
        for (size_t i = 0; i < data.size(); i++) {
            int carry = data[i];
            for (size_t j = 0; j < result.size(); j++) {
                carry += 256 * result[j];
                result[j] = carry % 58;
                carry /= 58;
            }
            while (carry > 0) {
                result.push_back(carry % 58);
                carry /= 58;
            }
        }
        
        std::string encoded(leadingZeros, '1');
        for (auto it = result.rbegin(); it != result.rend(); ++it) {
            encoded += b58[*it];
        }
        
        return encoded;
    }
};

// ============================================================================
// Block File Reader
// ============================================================================
class BlockFileReader {
private:
    std::string dataDir;
    std::string indexDir;
    std::map<std::string, AddressBalance> addressBalances;
    std::map<uint256, uint64_t> blockIndex;
    std::set<std::string> processedFileNames;
    std::mutex indexMutex;
    bool stopIndexing = false;
    double indexingProgress = 0.0;
    size_t totalBlockFiles = 0;
    size_t processedFiles = 0;
    
    static constexpr size_t MIN_INDEX_FILE_SIZE = 100ULL * 1024 * 1024;
    static constexpr size_t MAX_INDEX_FILE_SIZE = 128ULL * 1024 * 1024;
    
    std::map<std::string, AddressBalance> currentBuffer;
    size_t currentBufferSize = 0;
    size_t currentFileIndex = 0;
    std::vector<std::string> indexFiles;
    std::string lastProcessedFile;  // Track last processed file
    
    std::string GetFileName(const std::string& path) {
        size_t pos = path.find_last_of("/\\");
        if (pos != std::string::npos) {
            return path.substr(pos + 1);
        }
        return path;
    }
    
    std::string GetIndexFilename(int chunkNum) {
        std::ostringstream oss;
        oss << "bitcoin_address_index_" << std::setw(3) << std::setfill('0') << chunkNum << ".bin";
        return oss.str();
    }
    
    std::string GetProgressFilename() {
        return indexDir + "/progress.txt";
    }
    
    size_t CalculateEntrySize(const std::string& address, const AddressBalance& bal) {
        return sizeof(size_t) + address.length() + sizeof(AddressBalance);
    }
    
    // Save progress to progress.txt
    void SaveProgress() {
        if (indexDir.empty()) return;
        
        std::string progressFile = GetProgressFilename();
        std::ofstream file(progressFile);
        if (!file) {
            std::cerr << "[Blockchain] Cannot save progress to: " << progressFile << "\n";
            return;
        }
        
        file << "# Bitcoin Blockchain Index Progress\n";
        file << "# This file tracks indexing progress for resume capability\n";
        file << "LAST_PROCESSED_FILE=" << lastProcessedFile << "\n";
        file << "PROCESSED_FILES=" << processedFiles << "\n";
        file << "TOTAL_FILES=" << totalBlockFiles << "\n";
        file << "PROGRESS=" << indexingProgress << "\n";
        file << "ADDRESSES_INDEXED=" << addressBalances.size() << "\n";
        file << "LAST_UPDATE=" << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << "\n";
        file << "\n# Processed files:\n";
        for (const auto& fname : processedFileNames) {
            file << fname << "\n";
        }
        file.close();
    }
    
    // Load progress from progress.txt
    bool LoadProgress() {
        if (indexDir.empty()) return false;
        
        std::string progressFile = GetProgressFilename();
        std::ifstream file(progressFile);
        if (!file) {
            std::cout << "[Blockchain] No progress file found, starting fresh\n";
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            if (line.find("LAST_PROCESSED_FILE=") == 0) {
                lastProcessedFile = line.substr(20);
            } else if (line.find("PROCESSED_FILES=") == 0) {
                processedFiles = std::stoull(line.substr(16));
            } else if (line.find("TOTAL_FILES=") == 0) {
                totalBlockFiles = std::stoull(line.substr(12));
            } else if (line.find("PROGRESS=") == 0) {
                indexingProgress = std::stod(line.substr(9));
            } else if (line.find("ADDRESSES_INDEXED=") == 0) {
                // Just informational
            } else {
                // Regular filename
                processedFileNames.insert(line);
            }
        }
        file.close();
        
        std::cout << "[Blockchain] Loaded progress: " << processedFiles << "/" << totalBlockFiles 
                  << " files (" << (int)(indexingProgress * 100) << "%)\n";
        std::cout << "[Blockchain] Last processed: " << lastProcessedFile << "\n";
        return true;
    }
    
    bool FlushBuffer(bool force = false) {
        if (indexDir.empty()) return false;
        if (currentBuffer.empty()) return true;
        
        if (!force && currentBufferSize < MIN_INDEX_FILE_SIZE) {
            return true;
        }
        
        std::string indexFilename = GetIndexFilename((int)currentFileIndex);
        std::string filepath = indexDir + "/" + indexFilename;
        
        std::ofstream file(filepath, std::ios::binary);
        if (!file) {
            std::cerr << "[Blockchain] Cannot save index to: " << filepath << "\n";
            return false;
        }
        
        size_t count = currentBuffer.size();
        file.write((char*)&count, sizeof(count));
        
        for (const auto& pair : currentBuffer) {
            size_t addrLen = pair.first.length();
            file.write((char*)&addrLen, sizeof(addrLen));
            file.write(pair.first.c_str(), addrLen);
            file.write((char*)&pair.second, sizeof(pair.second));
        }
        
        file.close();
        
        double fileSizeMB = currentBufferSize / (1024.0 * 1024.0);
        std::cout << "[Blockchain] Saved index file: " << indexFilename 
                  << " (" << count << " addresses, " << std::fixed << std::setprecision(1) 
                  << fileSizeMB << " MB)\n";
        
        indexFiles.push_back(indexFilename);
        
        currentBuffer.clear();
        currentBufferSize = sizeof(size_t);
        currentFileIndex++;
        
        return true;
    }
    
    void UpdateAddressBalance(const std::string& address, uint64_t value, bool isInput, uint64_t height) {
        std::lock_guard<std::mutex> lock(indexMutex);
        
        auto& bal = addressBalances[address];
        auto& bufBal = currentBuffer[address];
        
        if (bal.firstSeenHeight == 0) {
            bal.firstSeenHeight = height;
            bufBal.firstSeenHeight = height;
        }
        
        if (isInput) {
            bal.sentSatoshis += value;
            bal.balanceSatoshis -= value;
        } else {
            bal.receivedSatoshis += value;
            bal.balanceSatoshis += value;
        }
        
        bal.txCount++;
        bal.lastSeenHeight = height;
        
        bufBal = bal;
        
        size_t entrySize = CalculateEntrySize(address, bal);
        currentBufferSize += entrySize;
        
        if (currentBufferSize > MAX_INDEX_FILE_SIZE && !currentBuffer.empty()) {
            FlushBuffer(false);
            currentBufferSize = sizeof(size_t);
            for (const auto& pair : currentBuffer) {
                currentBufferSize += CalculateEntrySize(pair.first, pair.second);
            }
        }
    }
    
    uint64_t ReadVarInt(const std::vector<unsigned char>& data, size_t& pos) {
        if (pos >= data.size()) return 0;
        
        uint8_t first = data[pos++];
        
        if (first < 0xFD) {
            return first;
        } else if (first == 0xFD) {
            if (pos + 2 > data.size()) return 0;
            uint16_t val = *(uint16_t*)&data[pos];
            pos += 2;
            return val;
        } else if (first == 0xFE) {
            if (pos + 4 > data.size()) return 0;
            uint32_t val = *(uint32_t*)&data[pos];
            pos += 4;
            return val;
        } else {
            if (pos + 8 > data.size()) return 0;
            uint64_t val = *(uint64_t*)&data[pos];
            pos += 8;
            return val;
        }
    }
    
    void ParseTransaction(const std::vector<unsigned char>& data, size_t& pos, uint64_t height) {
        if (pos + 4 > data.size()) return;
        
        pos += 4; // Skip version
        
        uint64_t inputCount = ReadVarInt(data, pos);
        
        // Skip inputs
        for (uint64_t i = 0; i < inputCount; i++) {
            pos += 36; // Previous output
            uint64_t scriptLen = ReadVarInt(data, pos);
            pos += scriptLen;
            pos += 4; // Sequence
        }
        
        if (pos >= data.size()) return;
        uint64_t outputCount = ReadVarInt(data, pos);
        
        for (uint64_t i = 0; i < outputCount && pos < data.size(); i++) {
            if (pos + 8 > data.size()) return;
            
            uint64_t value = 0;
            for (int j = 0; j < 8; j++) {
                value |= (uint64_t)data[pos + j] << (j * 8);
            }
            pos += 8;
            
            uint64_t scriptLen = ReadVarInt(data, pos);
            
            if (pos + scriptLen > data.size()) return;
            std::vector<unsigned char> script(data.begin() + pos, data.begin() + pos + scriptLen);
            pos += scriptLen;
            
            std::string address = ScriptParser::ExtractAddress(script);
            if (!address.empty()) {
                UpdateAddressBalance(address, value, false, height);
            }
        }
        
        pos += 4; // Skip lock time
    }
    
    void ParseBlock(const std::vector<unsigned char>& data, uint64_t height) {
        if (data.size() < 80) return;
        
        size_t pos = 80; // Skip header
        
        uint64_t txCount = ReadVarInt(data, pos);
        
        for (uint64_t i = 0; i < txCount && pos < data.size(); i++) {
            ParseTransaction(data, pos, height);
        }
    }
    
    void ProcessBlockFile(const std::string& filename, uint64_t startHeight) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "[Blockchain] Cannot open: " << filename << "\n";
            return;
        }
        
        std::cout << "[Blockchain] Processing: " << filename << "\n";
        
        uint64_t blockCount = 0;
        auto lastPrint = std::chrono::steady_clock::now();
        auto lastSave = std::chrono::steady_clock::now();
        
        // Read file in chunks to find block headers
        std::vector<unsigned char> buffer(8192);
        std::vector<unsigned char> leftover;
        
        while (file && !stopIndexing) {
            // Read next chunk
            file.read((char*)buffer.data(), buffer.size());
            size_t bytesRead = file.gcount();
            if (bytesRead == 0) break;
            
            // Combine with leftover from previous read
            std::vector<unsigned char> data = leftover;
            data.insert(data.end(), buffer.begin(), buffer.begin() + bytesRead);
            
            // Search for block magic in buffer
            size_t pos = 0;
            while (pos + 8 <= data.size()) {
                // Look for magic bytes: 0xf9 0xbe 0xb4 0xd9
                if (data[pos] == 0xf9 && data[pos+1] == 0xbe && 
                    data[pos+2] == 0xb4 && data[pos+3] == 0xd9) {
                    
                    // Found magic, read block size
                    uint32_t blockSize = *(uint32_t*)&data[pos+4];
                    
                    // Check if we have full block in buffer
                    if (pos + 8 + blockSize > data.size()) {
                        // Block continues beyond buffer, save as leftover
                        break;
                    }
                    
                    // Parse block
                    std::vector<unsigned char> blockData(data.begin() + pos + 8, 
                                                         data.begin() + pos + 8 + blockSize);
                    ParseBlock(blockData, startHeight + blockCount);
                    blockCount++;
                    
                    // Move past this block
                    pos += 8 + blockSize;
                } else {
                    pos++;
                }
            }
            
            // Save leftover data that might contain partial block header
            if (pos < data.size()) {
                leftover.assign(data.begin() + pos, data.end());
            } else {
                leftover.clear();
            }
            
            // Print progress every 5 seconds
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastPrint).count() >= 5) {
                std::lock_guard<std::mutex> lock(indexMutex);
                std::cout << "\r[Blockchain] Processed " << blockCount << " blocks... (" 
                          << addressBalances.size() << " addresses)" << std::flush;
                lastPrint = now;
            }
            
            // Save progress every 30 seconds
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastSave).count() >= 30) {
                SaveProgress();
                lastSave = now;
            }
        }
        
        std::cout << "\n[Blockchain] Finished " << filename << ": " << blockCount << " blocks\n";
    }

public:
    BlockFileReader(const std::string& datadir) : dataDir(datadir) {}
    
    void SetIndexDir(const std::string& dir) {
        indexDir = dir;
    }
    
    std::vector<std::string> GetBlockFiles() {
        std::vector<std::string> files;
        
#ifdef _WIN32
        std::string searchPath = dataDir + "\\blk*.dat";
        WIN32_FIND_DATA findData;
        HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);
        
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                std::string name = findData.cFileName;
                files.push_back(dataDir + "\\" + name);
            } while (FindNextFile(hFind, &findData));
            FindClose(hFind);
        }
#else
        DIR* dir = opendir(dataDir.c_str());
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                std::string name = entry->d_name;
                if (name.find("blk") == 0 && name.find(".dat") != std::string::npos) {
                    files.push_back(dataDir + "/" + name);
                }
            }
            closedir(dir);
        }
#endif
        
        // Sort numerically
        std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
            auto extractNum = [](const std::string& path) -> int {
                size_t start = path.find("blk");
                if (start == std::string::npos) return 0;
                start += 3;
                size_t end = path.find(".dat", start);
                if (end == std::string::npos) return 0;
                try {
                    return std::stoi(path.substr(start, end - start));
                } catch (...) {
                    return 0;
                }
            };
            return extractNum(a) < extractNum(b);
        });
        
        return files;
    }
    
    void StartIndexing() {
        try {
            auto files = GetBlockFiles();
            totalBlockFiles = files.size();
            
            if (files.empty()) {
                std::cerr << "[Blockchain] ERROR: No block files found in directory!\n";
                std::cerr << "[Blockchain] Make sure --blockchain-dir points to the correct location.\n";
                indexingProgress = 1.0;  // Mark as complete to avoid infinite wait
                return;
            }
            
            currentBufferSize = sizeof(size_t);
            currentFileIndex = 0;
            
            // Load previous progress if exists
            if (!indexDir.empty()) {
                LoadProgress();
                
                // Scan for existing index files
                for (int i = 0; i < 10000; i++) {
                    std::string testPath = indexDir + "/" + GetIndexFilename(i);
                    std::ifstream testFile(testPath, std::ios::binary);
                    if (testFile.good()) {
                        indexFiles.push_back(GetIndexFilename(i));
                        testFile.close();
                        currentFileIndex++;
                    } else {
                        break;
                    }
                }
            }
            
            // Filter out already processed files
            std::vector<std::string> filesToProcess;
            for (const auto& file : files) {
                std::string fname = GetFileName(file);
                if (processedFileNames.find(fname) == processedFileNames.end()) {
                    filesToProcess.push_back(file);
                }
            }
            
            if (processedFiles > 0) {
                std::cout << "[Blockchain] Found " << files.size() << " block files (" 
                          << processedFiles << " already indexed, " 
                          << filesToProcess.size() << " new to process)\n";
            } else {
                std::cout << "[Blockchain] Found " << files.size() << " block files\n";
            }
            
            if (filesToProcess.empty()) {
                std::cout << "[Blockchain] All files already indexed!\n";
                indexingProgress = 1.0;
                return;
            }
            
            uint64_t height = processedFiles * 100000;
            size_t fileIdx = processedFiles;
            
            std::cout << "[Blockchain] Starting to process " << filesToProcess.size() << " files...\n";
            
            for (const auto& file : filesToProcess) {
                if (stopIndexing) {
                    std::cout << "[Blockchain] Indexing stopped by user. Saving progress...\n";
                    break;
                }
                
                std::string blkFilename = GetFileName(file);
                lastProcessedFile = blkFilename;
                
                ProcessBlockFile(file, height);
                height += 100000;
                
                {
                    std::lock_guard<std::mutex> lock(indexMutex);
                    processedFileNames.insert(blkFilename);
                }
                
                fileIdx++;
                processedFiles = fileIdx;
                indexingProgress = (double)fileIdx / (double)totalBlockFiles;
                
                // Save progress after each file
                SaveProgress();
                
                if (fileIdx % 10 == 0) {
                    double bufSizeMB = currentBufferSize / (1024.0 * 1024.0);
                    std::cout << "[Blockchain] Progress: " << fileIdx << "/" << totalBlockFiles 
                              << " files (" << (int)(indexingProgress * 100) << "%), "
                              << "Buffer: " << std::fixed << std::setprecision(1) << bufSizeMB << " MB\n";
                }
            }
            
            if (fileIdx >= totalBlockFiles) {
                indexingProgress = 1.0;
            }
            
            // Final flush and save
            if (!currentBuffer.empty()) {
                FlushBuffer(true);
            }
            SaveProgress();
            
            std::cout << "[Blockchain] Indexing complete. Total addresses: " << addressBalances.size() << "\n";
            std::cout << "[Blockchain] Total index files: " << indexFiles.size() << "\n";
            
        } catch (const std::exception& e) {
            std::cerr << "[Blockchain] ERROR during indexing: " << e.what() << "\n";
            SaveProgress();  // Save whatever progress we have
        } catch (...) {
            std::cerr << "[Blockchain] UNKNOWN ERROR during indexing\n";
            SaveProgress();  // Save whatever progress we have
        }
    }
    
    void Stop() {
        stopIndexing = true;
    }
    
    size_t GetAddressCount() {
        std::lock_guard<std::mutex> lock(indexMutex);
        return addressBalances.size();
    }
    
    double GetIndexingProgress() {
        return indexingProgress;
    }
    
    size_t GetTotalBlockFiles() {
        return totalBlockFiles;
    }
    
    size_t GetProcessedFiles() {
        return processedFiles;
    }
    
    bool HasAddress(const std::string& address) {
        std::lock_guard<std::mutex> lock(indexMutex);
        return addressBalances.find(address) != addressBalances.end();
    }
    
    AddressBalance GetAddressBalance(const std::string& address) {
        std::lock_guard<std::mutex> lock(indexMutex);
        auto it = addressBalances.find(address);
        if (it != addressBalances.end()) {
            return it->second;
        }
        return AddressBalance{};
    }
    
    uint64_t GetAddressHeight(const std::string& address) {
        std::lock_guard<std::mutex> lock(indexMutex);
        auto it = addressBalances.find(address);
        if (it != addressBalances.end()) {
            return it->second.firstSeenHeight;
        }
        return 0;
    }
    
    // Get total number of indexed addresses
    size_t GetTotalAddresses() {
        std::lock_guard<std::mutex> lock(indexMutex);
        return addressBalances.size();
    }
    
    // Get number of processed block files
    size_t GetProcessedFileCount() {
        return processedFiles;
    }
    
    // Get total balance of all addresses (in satoshis)
    uint64_t GetTotalBalance() {
        std::lock_guard<std::mutex> lock(indexMutex);
        uint64_t total = 0;
        for (const auto& pair : addressBalances) {
            total += pair.second.balanceSatoshis;
        }
        return total;
    }
};

// Global instance
static std::unique_ptr<BlockFileReader> g_blockchainReader;
static std::string g_indexDir;

inline bool InitializeBlockchainReader(const std::string& datadir, const std::string& indexdir = "") {
    g_blockchainReader = std::make_unique<BlockFileReader>(datadir);
    g_indexDir = indexdir;
    g_blockchainReader->SetIndexDir(indexdir);
    return true;
}

inline BlockFileReader* GetBlockchainReader() {
    return g_blockchainReader.get();
}

inline bool CheckAddressInBlockchain(const std::string& address) {
    if (!g_blockchainReader) return false;
    return g_blockchainReader->HasAddress(address);
}

inline double GetBlockchainIndexingProgress() {
    if (!g_blockchainReader) return 0.0;
    return g_blockchainReader->GetIndexingProgress();
}

// Wait for blockchain indexing to reach a minimum progress level
// Returns true if target reached, false if timeout
inline bool WaitForBlockchainIndexing(double minProgress, int timeoutSeconds) {
    if (!g_blockchainReader) return false;
    
    auto startTime = std::chrono::steady_clock::now();
    
    while (true) {
        double progress = g_blockchainReader->GetIndexingProgress();
        if (progress >= minProgress) {
            return true;
        }
        
        // Check timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed >= timeoutSeconds) {
            return false; // Timeout
        }
        
        // Print progress every 5 seconds
        if ((int)elapsed % 5 == 0) {
            std::cout << "[Blockchain] Indexing progress: " << (int)(progress * 100) << "%\n";
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

} // namespace BlockchainReader
