#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <cstdint>

// --- CROSS-PLATFORM SYSTEM HEADERS ---
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <wininet.h>
    #pragma comment(lib, "wininet.lib")
    
    // Windows doesn't always define these, so we guard them
    typedef unsigned long long ulong;
    typedef unsigned int uint;
    typedef unsigned char uchar;
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/types.h>
    // On Linux, ulong/uint/uchar are already defined in sys/types.h
#endif

// External Libraries for Cryptography
#ifndef NO_SECP256K1
#include <secp256k1.h>
#endif
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>

// =============================================================
// SECTION 1: BECH32 IMPLEMENTATION (Native SegWit)
// =============================================================
namespace Bech32 {
    inline constexpr char const* charset = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
    
    inline uint32_t polymod(const std::vector<uint8_t>& values) {
        static const uint32_t GEN[5] = {0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3};
        uint32_t chk = 1;
        for (uint8_t v : values) {
            uint8_t b = chk >> 25;
            chk = ((chk & 0x1ffffff) << 5) ^ v;
            for (int i = 0; i < 5; ++i) {
                if ((b >> i) & 1) chk ^= GEN[i];
            }
        }
        return chk;
    }

    inline std::vector<uint8_t> expand_hrp(const std::string& hrp) {
        std::vector<uint8_t> ret;
        ret.reserve(hrp.size() * 2 + 1);
        for (char c : hrp) ret.push_back(static_cast<uint8_t>(c) >> 5);
        ret.push_back(0);
        for (char c : hrp) ret.push_back(static_cast<uint8_t>(c) & 31);
        return ret;
    }

    inline std::string encode(const std::string& hrp, const std::vector<uint8_t>& values) {
        std::vector<uint8_t> checksum_input = expand_hrp(hrp);
        checksum_input.insert(checksum_input.end(), values.begin(), values.end());
        checksum_input.resize(checksum_input.size() + 6, 0);
        uint32_t mod = polymod(checksum_input) ^ 1;
        std::string ret = hrp + "1";
        for (uint8_t v : values) ret += charset[v];
        for (int i = 0; i < 6; ++i) ret += charset[(mod >> (5 * (5 - i))) & 31];
        return ret;
    }

    inline bool convert_bits(const std::vector<uint8_t>& in, int fromBits, int toBits, bool pad, std::vector<uint8_t>& out) {
        int acc = 0; int bits = 0; 
        int maxv = (1 << toBits) - 1; 
        int max_acc = (1 << (fromBits + toBits - 1)) - 1;
        for (uint8_t value : in) {
            acc = ((acc << fromBits) | value) & max_acc;
            bits += fromBits;
            while (bits >= toBits) { 
                bits -= toBits; 
                out.push_back(static_cast<uint8_t>((acc >> bits) & maxv)); 
            }
        }
        if (pad) { 
            if (bits > 0) out.push_back(static_cast<uint8_t>((acc << (toBits - bits)) & maxv)); 
        }
        else if (bits >= fromBits || ((acc << (toBits - bits)) & maxv)) return false;
        return true;
    }

    inline std::string encode_segwit(const std::string& hrp, int witver, const std::vector<uint8_t>& program) {
        std::vector<uint8_t> data; 
        data.push_back(static_cast<uint8_t>(witver));
        convert_bits(program, 8, 5, true, data);
        return encode(hrp, data);
    }
    
    // Decode Bech32/Bech32m address (BIP-173/BIP-350)
    // Returns witness version (0-16) or -1 if invalid
    inline int decode_segwit(const std::string& addr, std::string& hrp_out, std::vector<uint8_t>& program) {
        // Find separator '1'
        size_t sep = addr.find_last_of('1');
        if (sep == std::string::npos || sep == 0 || sep + 7 > addr.size()) return -1;
        
        hrp_out = addr.substr(0, sep);
        std::string data_str = addr.substr(sep + 1);
        
        // Create reverse charset map
        static const int8_t charset_rev[128] = {
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            15, -1, 10, 17, 21, 20, 26, 30,  7,  5, -1, -1, -1, -1, -1, -1,
            -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
             1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1,
            -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
             1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1
        };
        
        // Decode characters
        std::vector<uint8_t> data;
        data.reserve(data_str.size());
        for (char c : data_str) {
            if (c < 33 || c > 126) return -1;
            int8_t d = charset_rev[static_cast<uint8_t>(c)];
            if (d < 0) return -1;
            data.push_back(static_cast<uint8_t>(d));
        }
        
        // Verify checksum (Bech32 = 1, Bech32m = 0x2BC830A3)
        std::vector<uint8_t> checksum_input = expand_hrp(hrp_out);
        checksum_input.insert(checksum_input.end(), data.begin(), data.end());
        uint32_t checksum = polymod(checksum_input);
        
        if (checksum != 1 && checksum != 0x2BC830A3) return -1;
        
        // Extract witness version and program
        int witver = data[0];
        if (witver > 16) return -1;
        
        std::vector<uint8_t> data_5bit(data.begin() + 1, data.end() - 6);
        program.clear();
        if (!convert_bits(data_5bit, 5, 8, false, program)) return -1;
        
        // Validate program length
        if (witver == 0) {
            if (program.size() != 20 && program.size() != 32) return -1;
        } else {
            if (program.size() < 2 || program.size() > 40) return -1;
        }
        
        return witver;
    }
}

// =============================================================
// SECTION 2: HASH, BASE58 & HEX UTILITIES
// =============================================================

inline std::string hideString(const std::string& input, int visibleLen = 4) {
    if (input.length() <= (size_t)visibleLen * 2) return input;
    return input.substr(0, visibleLen) + "..." + input.substr(input.length() - visibleLen);
}

inline std::string toHex(const std::vector<uint8_t>& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t b : data) ss << std::setw(2) << (int)b;
    return ss.str();
}

inline std::vector<uint8_t> fromHex(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        bytes.push_back((uint8_t)strtol(hex.substr(i, 2).c_str(), NULL, 16));
    }
    return bytes;
}

inline void Hash160(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
    unsigned char shaResult[SHA256_DIGEST_LENGTH];
    SHA256(input.data(), input.size(), shaResult);
    output.resize(20);
    RIPEMD160(shaResult, SHA256_DIGEST_LENGTH, output.data());
}

inline void DoubleSHA256(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
    unsigned char hash1[SHA256_DIGEST_LENGTH];
    SHA256(input.data(), input.size(), hash1);
    output.resize(SHA256_DIGEST_LENGTH);
    SHA256(hash1, SHA256_DIGEST_LENGTH, output.data());
}

inline std::string EncodeBase58Check(const std::vector<uint8_t>& input) {
    static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    std::vector<uint8_t> vch(input);
    std::vector<uint8_t> hash;
    DoubleSHA256(vch, hash);
    vch.insert(vch.end(), hash.begin(), hash.begin() + 4);
    
    std::vector<uint8_t> digits((vch.size() * 138 / 100) + 1, 0);
    size_t digitslen = 1;
    for (size_t i = 0; i < vch.size(); i++) {
        uint32_t carry = static_cast<uint32_t>(vch[i]);
        for (size_t j = 0; j < digitslen; j++) {
            carry += static_cast<uint32_t>(digits[j]) << 8;
            digits[j] = static_cast<uint8_t>(carry % 58);
            carry /= 58;
        }
        while (carry > 0) { 
            digits[digitslen++] = static_cast<uint8_t>(carry % 58); 
            carry /= 58; 
        }
    }
    std::string result = "";
    for (size_t i = 0; i < vch.size() && vch[i] == 0; i++) result += '1';
    for (size_t i = 0; i < digitslen; i++) result += pszBase58[digits[digitslen - 1 - i]];
    return result;
}

// Decode Base58 string to bytes (standalone version)
inline std::vector<uint8_t> DecodeBase58(const std::string& str) {
    static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    std::vector<uint8_t> vch;
    const char* pbegin = str.c_str();
    const char* pend = pbegin + str.length();
    while (pbegin != pend && isspace(*pbegin)) pbegin++;
    int zeros = 0;
    while (pbegin != pend && *pbegin == '1') { zeros++; pbegin++; }
    int size = (pend - pbegin) * 733 / 1000 + 1;
    std::vector<unsigned char> b256(size);
    while (pbegin != pend && !isspace(*pbegin)) {
        const char* ch = strchr(pszBase58, *pbegin);
        if (ch == NULL) break;
        int carry = ch - pszBase58;
        for (int i = b256.size() - 1; i >= 0; i--) {
            carry += 58 * b256[i];
            b256[i] = carry % 256;
            carry /= 256;
        }
        pbegin++;
    }
    std::vector<unsigned char>::iterator it = b256.begin();
    while (it != b256.end() && *it == 0) it++;
    vch.assign(zeros, 0x00);
    vch.insert(vch.end(), it, b256.end());
    return vch;
}

// Convert private key to WIF format
inline std::string privKeyToWIF(const unsigned char* privKey, bool compressed = true) {
    std::vector<uint8_t> data;
    // Add version byte (0x80 for mainnet)
    data.push_back(0x80);
    // Add private key (32 bytes)
    data.insert(data.end(), privKey, privKey + 32);
    // Add compression flag if compressed
    if (compressed) {
        data.push_back(0x01);
    }
    return EncodeBase58Check(data);
}

// Convert private key hex string to WIF
inline std::string privKeyHexToWIF(const std::string& hexKey, bool compressed = true) {
    std::vector<uint8_t> privKey = fromHex(hexKey);
    if (privKey.size() != 32) return "";
    return privKeyToWIF(privKey.data(), compressed);
}

// Convert public key to P2SH-Segwit address (3...)
inline std::string PubKeyToP2SH(const std::vector<uint8_t>& pubKey) {
    unsigned char sha256_res[SHA256_DIGEST_LENGTH];
    SHA256(pubKey.data(), pubKey.size(), sha256_res);
    unsigned char ripemd160_res[RIPEMD160_DIGEST_LENGTH];
    RIPEMD160(sha256_res, SHA256_DIGEST_LENGTH, ripemd160_res);

    std::vector<uint8_t> redeemScript;
    redeemScript.push_back(0x00);
    redeemScript.push_back(0x14); 
    redeemScript.insert(redeemScript.end(), ripemd160_res, ripemd160_res + 20);

    SHA256(redeemScript.data(), redeemScript.size(), sha256_res);
    RIPEMD160(sha256_res, SHA256_DIGEST_LENGTH, ripemd160_res);

    std::vector<uint8_t> addrBytes;
    addrBytes.push_back(0x05);  // P2SH version byte
    addrBytes.insert(addrBytes.end(), ripemd160_res, ripemd160_res + 20);
    return EncodeBase58Check(addrBytes);
}

// =============================================================
// SECTION 3: CRYPTO CORE (BIP32)
// =============================================================

struct ExtendedKey {
    std::vector<uint8_t> key; 
    std::vector<uint8_t> chainCode; 
    int depth = 0; 
    uint32_t childIndex = 0;
};

inline void HmacSha512(const std::vector<uint8_t>& key, const std::vector<uint8_t>& data, std::vector<uint8_t>& output) {
    output.resize(64); 
    unsigned int len = 64;
    HMAC(EVP_sha512(), key.data(), static_cast<int>(key.size()), data.data(), data.size(), output.data(), &len);
}

inline ExtendedKey GetMasterKey(const std::vector<uint8_t>& seed) {
    std::vector<uint8_t> output; 
    const std::string salt = "Bitcoin seed";
    std::vector<uint8_t> saltBytes(salt.begin(), salt.end());
    HmacSha512(saltBytes, seed, output);
    ExtendedKey k; 
    k.key.assign(output.begin(), output.begin() + 32); 
    k.chainCode.assign(output.begin() + 32, output.end());
    k.depth = 0; 
    k.childIndex = 0;
    return k;
}

#ifndef NO_SECP256K1
inline ExtendedKey Derive(const ExtendedKey& parent, uint32_t index, secp256k1_context* ctx) {
    std::vector<uint8_t> data;
    if (index >= 0x80000000) {
        data.push_back(0x00); 
        data.insert(data.end(), parent.key.begin(), parent.key.end());
    } else {
        secp256k1_pubkey pub;
        if (secp256k1_ec_pubkey_create(ctx, &pub, parent.key.data())) {
            uint8_t comp[33]; 
            size_t len = 33;
            secp256k1_ec_pubkey_serialize(ctx, comp, &len, &pub, SECP256K1_EC_COMPRESSED);
            data.insert(data.end(), comp, comp + 33);
        }
    }
    data.push_back(static_cast<uint8_t>((index >> 24) & 0xFF)); 
    data.push_back(static_cast<uint8_t>((index >> 16) & 0xFF));
    data.push_back(static_cast<uint8_t>((index >> 8) & 0xFF)); 
    data.push_back(static_cast<uint8_t>(index & 0xFF));
    
    std::vector<uint8_t> i_out; 
    HmacSha512(parent.chainCode, data, i_out);
    ExtendedKey child; 
    child.key.assign(i_out.begin(), i_out.begin() + 32);
    secp256k1_ec_privkey_tweak_add(ctx, child.key.data(), parent.key.data());
    child.chainCode.assign(i_out.begin() + 32, i_out.end()); 
    child.depth = parent.depth + 1; 
    child.childIndex = index;
    return child;
}
#else
inline ExtendedKey Derive(const ExtendedKey& parent, uint32_t index, void* ctx = nullptr) {
    (void)ctx; (void)index;
    // Stub implementation when secp256k1 is not available
    return parent;
}
#endif

// =============================================================
// SECTION 4: ADDRESS CONVERTERS
// =============================================================

inline std::string PubKeyToLegacy(const std::vector<uint8_t>& pubKeyBytes) {
    std::vector<uint8_t> keyHash; Hash160(pubKeyBytes, keyHash);
    std::vector<uint8_t> addr; 
    addr.push_back(0x00); 
    addr.insert(addr.end(), keyHash.begin(), keyHash.end());
    return EncodeBase58Check(addr);
}

inline std::string PubKeyToNestedSegwit(const std::vector<uint8_t>& pubKeyBytes) {
    std::vector<uint8_t> keyHash; Hash160(pubKeyBytes, keyHash);
    std::vector<uint8_t> redeem; 
    redeem.push_back(0x00); 
    redeem.push_back(0x14); 
    redeem.insert(redeem.end(), keyHash.begin(), keyHash.end());
    std::vector<uint8_t> scriptHash; Hash160(redeem, scriptHash);
    std::vector<uint8_t> addr; 
    addr.push_back(0x05); 
    addr.insert(addr.end(), scriptHash.begin(), scriptHash.end());
    return EncodeBase58Check(addr);
}

inline std::string PubKeyToNativeSegwit(const std::vector<uint8_t>& pubKeyBytes) {
    std::vector<uint8_t> keyHash; Hash160(pubKeyBytes, keyHash);
    return Bech32::encode_segwit("bc", 0, keyHash);
}

// =============================================================
// SECTION 5: ONLINE API CHECKER (Windows Only)
// =============================================================

struct OnlineInfo {
    std::string totalReceived = "0";
    std::string txCount = "0";
    bool success = false;
    std::string source = "Blockchain.info"; 
};

inline OnlineInfo CheckAddressOnline(const std::string& address) {
    OnlineInfo info;
#ifdef _WIN32
    HINTERNET hInt = InternetOpenA("GpuCracker/4.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInt) return info;

    std::string url = "https://blockchain.info/rawaddr/" + address;
    HINTERNET hFile = InternetOpenUrlA(hInt, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);
    if (!hFile) {
        InternetCloseHandle(hInt);
        return info;
    }

    std::string response;
    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        response.append(buffer, bytesRead);
    }
    
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInt);
    if (response.empty()) return info;

    auto extract = [&](const std::string& key) -> std::string {
        size_t pos = response.find(key);
        if (pos == std::string::npos) return "0";
        size_t start = response.find(":", pos) + 1;
        size_t end = response.find_first_of(",}", start);
        if (start >= response.length() || end == std::string::npos) return "0";
        std::string val = response.substr(start, end - start);
        val.erase(std::remove(val.begin(), val.end(), ' '), val.end());
        return val;
    };

    info.totalReceived = extract("\"total_received\"");
    info.txCount = extract("\"n_tx\"");
    info.success = true;
#else
    // Fallback for Linux (requires libcurl implementation if needed)
    info.success = false;
#endif
    return info;
}