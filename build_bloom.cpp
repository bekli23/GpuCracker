// Windows conflict fix - MUST be first
#include "win_fix.h"

#define _CRT_SECURE_NO_WARNINGS

// Suppress OpenSSL deprecation warnings (SHA256_* is deprecated since 3.0)
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <cstring>
#include <sstream>
#include <map>

// OpenSSL - Necesar pentru validarea checksum-ului adreselor
#include <openssl/sha.h>
#include <openssl/ripemd.h>

// --- CONFIGURARE BLOOM ---
#define DEFAULT_FP_RATE 0.000000001 // 1e-9 (Foarte precis, 1 la un miliard sanse de eroare)
#define MIN_BLOOM_SIZE_BITS (8 * 1024 * 1024) // Minim 8 milioane biti (1MB)

// Limita de siguranta pentru hash-ul de 32-bit (aprox 3.5 miliarde biti / ~420MB)
#define BLOOM_32BIT_LIMIT 3500000000ULL 

#ifdef _WIN32
    typedef unsigned long long ulong;
    typedef unsigned int uint;
#else
    #include <sys/types.h>
#endif

// ============================================================================
//  MULTI-COIN DATABASE
// ============================================================================

struct CoinPrefixes {
    std::string symbol;
    std::string name;
    // Base58 prefixes
    char legacyPrefix;      // e.g., '1' for BTC
    char p2shPrefix;        // e.g., '3' for BTC
    // Bech32 HRP
    std::string bech32Hrp;  // e.g., "bc1" for BTC, "ltc1" for LTC
    // ETH-based
    bool isEthBased;
    // Additional prefixes for special coins
    std::vector<char> extraPrefixes;
};

static std::vector<CoinPrefixes> g_coinPrefixes = {
    // Bitcoin derivatives with Legacy + Segwit
    {"BTC", "Bitcoin", '1', '3', "bc1", false, {}},
    {"LTC", "Litecoin", 'L', 'M', "ltc1", false, {}},
    {"DOGE", "Dogecoin", 'D', '9', "", false, {}},
    {"DASH", "Dash", 'X', '7', "", false, {}},
    {"BCH", "Bitcoin Cash", '1', '3', "", false, {}},
    {"BSV", "Bitcoin SV", '1', '3', "", false, {}},
    {"BTG", "Bitcoin Gold", 'G', 'A', "btg1", false, {}},
    {"ZEC", "Zcash", 't', 't', "", false, {}},  // t1, t3
    
    // Additional Bitcoin forks
    {"VTC", "Vertcoin", 'V', '3', "vtc1", false, {}},
    {"DGB", "DigiByte", 'D', 'S', "dgb1", false, {}},
    {"RVN", "Ravencoin", 'R', 'r', "", false, {}},
    {"PIVX", "PIVX", 'D', '3', "", false, {}},
    {"MONA", "Monacoin", 'M', 'P', "mona1", false, {}},
    {"XMY", "Myriadcoin", 'M', '4', "", false, {}},
    {"FTC", "Feathercoin", '6', '3', "fc1", false, {}},
    {"XZC", "Firo", 'a', 'Z', "", false, {}},
    {"KMD", "Komodo", 'R', 'b', "", false, {}},
    {"ZEN", "Horizen", 'z', 'z', "", false, {}},
    {"XZC", "Zcoin", 'a', 'Z', "", false, {}},
    {"NMC", "Namecoin", 'M', '3', "", false, {}},
    {"FLO", "Flo", 'F', '3', "", false, {}},
    {"UNO", "Unobtanium", 'u', 'e', "", false, {}},
    {"ABY", "Artbyte", 'A', '3', "", false, {}},
    {"IXC", "Ixcoin", 'x', 'X', "", false, {}},
    {"NLG", "Gulden", 'G', '5', "", false, {}},
    {"SIB", "Sibcoin", 'S', 'X', "", false, {}},
    {"MEC", "Megacoin", 'M', 'N', "", false, {}},
    {"EMC2", "Einsteinium", 'E', 'e', "", false, {}},
    {"GAME", "GameCredits", 'G', '3', "", false, {}},
    {"XVG", "Verge", 'D', '3', "", false, {}},
    {"POT", "Potcoin", 'P', '3', "", false, {}},
    {"BLK", "Blackcoin", 'B', 'b', "", false, {}},
    {"GRS", "Groestlcoin", 'F', '3', "grs1", false, {}},
    {"SMART", "Smartcash", 'S', '3', "", false, {}},
    {"VIA", "Viacoin", 'V', '3', "via1", false, {}},
    {"UBQ", "Ubiq", 'U', '3', "", false, {}},
    {"DCR", "Decred", 'D', 'S', "", false, {}},
    {"BTX", "Bitcore", '2', '3', "btx1", false, {}},
    {"CDN", "Canadaecoin", 'C', '3', "", false, {}},
    {"TAU", "Lamden", 'T', '3', "", false, {}},
    {"MOBI", "Mobius", 'M', '3', "", false, {}},
    {"BPA", "Bitcoin Pizza", 'P', '3', "", false, {}},
    {"BTCP", "Bitcoin Private", 'b', 'B', "", false, {}},
    {"BCA", "Bitcoin Atom", 'P', 'p', "", false, {}},
    {"BTC", "Bitcoin", '1', '3', "bc1", false, {}},
    
    // Ethereum-based coins (all use 0x addresses)
    {"ETH", "Ethereum", 0, 0, "", true, {}},
    {"ETC", "Ethereum Classic", 0, 0, "", true, {}},
    {"BNB", "Binance Chain", 0, 0, "", true, {}},
    {"MATIC", "Polygon", 0, 0, "", true, {}},
    {"AVAX", "Avalanche", 0, 0, "", true, {}},
    {"FTM", "Fantom", 0, 0, "", true, {}},
    {"CRO", "Cronos", 0, 0, "", true, {}},
    {"HT", "Huobi Token", 0, 0, "", true, {}},
    {"OKT", "OKExChain", 0, 0, "", true, {}},
    {"MOVR", "Moonriver", 0, 0, "", true, {}},
    {"GLMR", "Moonbeam", 0, 0, "", true, {}},
    {"ONE", "Harmony", 0, 0, "", true, {}},
    {"KCS", "KuCoin Token", 0, 0, "", true, {}},
    {"CELO", "Celo", 0, 0, "", true, {}},
    {"METIS", "Metis", 0, 0, "", true, {}},
    {"BOBA", "Boba Network", 0, 0, "", true, {}},
    {"RSK", "Rootstock", 0, 0, "", true, {}},
    {"xDAI", "Gnosis Chain", 0, 0, "", true, {}},
    {"PALM", "Palm", 0, 0, "", true, {}},
    {"TLOS", "Telos EVM", 0, 0, "", true, {}},
    {"EVMOS", "Evmos", 0, 0, "", true, {}},
    {"KAVA", "Kava EVM", 0, 0, "", true, {}},
    {"CANTO", "Canto", 0, 0, "", true, {}},
    {"DFK", "Defi Kingdoms", 0, 0, "", true, {}},
    {"WEMIX", "Wemix", 0, 0, "", true, {}},
    {"SONE", "Soneium", 0, 0, "", true, {}},
};

// ============================================================================
//  1. ALGORITMI DE HASHING (32-BIT si 64-BIT)
// ============================================================================

inline uint32_t rotl32(uint32_t x, int8_t r) { return (x << r) | (x >> (32 - r)); }
inline uint64_t rotl64(uint64_t x, int8_t r) { return (x << r) | (x >> (64 - r)); }

// --- 32-BIT Hash (Murmur3_x86_32) ---
inline uint32_t MurmurHash3_x86_32(const void* key, int len, uint32_t seed) {
    const uint8_t* data = (const uint8_t*)key;
    const int nblocks = len / 4;
    uint32_t h1 = seed;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);
    for (int i = -nblocks; i; i++) {
        uint32_t k1;
        memcpy(&k1, (const uint8_t*)blocks + (i * 4), 4);
        k1 *= c1; k1 = rotl32(k1, 15); k1 *= c2;
        h1 ^= k1; h1 = rotl32(h1, 13); h1 = h1 * 5 + 0xe6546b64;
    }
    const uint8_t* tail = (const uint8_t*)(data + nblocks * 4);
    uint32_t k1 = 0;
    switch (len & 3) {
    case 3: k1 ^= tail[2] << 16;
    case 2: k1 ^= tail[1] << 8;
    case 1: k1 ^= tail[0];
            k1 *= c1; k1 = rotl32(k1, 15); k1 *= c2; h1 ^= k1;
    };
    h1 ^= len; h1 ^= h1 >> 16; h1 *= 0x85ebca6b; h1 ^= h1 >> 13; h1 *= 0xc2b2ae35; h1 ^= h1 >> 16;
    return h1;
}

// --- 64-BIT Hash (Murmur3_x64_128) ---
#define BIG_CONSTANT(x) (x##LLU)
void MurmurHash3_x64_128(const void* key, const int len, const uint32_t seed, void* out) {
    const uint8_t* data = (const uint8_t*)key;
    const int nblocks = len / 16;
    uint64_t h1 = seed;
    uint64_t h2 = seed;
    const uint64_t c1 = BIG_CONSTANT(0x87c37b91114253d5);
    const uint64_t c2 = BIG_CONSTANT(0x4cf5ad432745937f);
    const uint64_t* blocks = (const uint64_t*)(data);

    for (int i = 0; i < nblocks; i++) {
        uint64_t k1 = blocks[i * 2 + 0];
        uint64_t k2 = blocks[i * 2 + 1];
        k1 *= c1; k1 = rotl64(k1, 31); k1 *= c2; h1 ^= k1;
        h1 = rotl64(h1, 27); h1 += h2; h1 = h1 * 5 + 0x52dce729;
        k2 *= c2; k2 = rotl64(k2, 33); k2 *= c1; h2 ^= k2;
        h2 = rotl64(h2, 31); h2 += h1; h2 = h2 * 5 + 0x38495ab5;
    }
    const uint8_t* tail = (const uint8_t*)(data + nblocks * 16);
    uint64_t k1 = 0;
    uint64_t k2 = 0;
    switch (len & 15) {
    case 15: k2 ^= ((uint64_t)tail[14]) << 48;
    case 14: k2 ^= ((uint64_t)tail[13]) << 40;
    case 13: k2 ^= ((uint64_t)tail[12]) << 32;
    case 12: k2 ^= ((uint64_t)tail[11]) << 24;
    case 11: k2 ^= ((uint64_t)tail[10]) << 16;
    case 10: k2 ^= ((uint64_t)tail[9]) << 8;
    case  9: k2 ^= ((uint64_t)tail[8]) << 0;
             k2 *= c2; k2 = rotl64(k2, 33); k2 *= c1; h2 ^= k2;
    case  8: k1 ^= ((uint64_t)tail[7]) << 56;
    case  7: k1 ^= ((uint64_t)tail[6]) << 48;
    case  6: k1 ^= ((uint64_t)tail[5]) << 40;
    case  5: k1 ^= ((uint64_t)tail[4]) << 32;
    case  4: k1 ^= ((uint64_t)tail[3]) << 24;
    case  3: k1 ^= ((uint64_t)tail[2]) << 16;
    case  2: k1 ^= ((uint64_t)tail[1]) << 8;
    case  1: k1 ^= ((uint64_t)tail[0]) << 0;
             k1 *= c1; k1 = rotl64(k1, 31); k1 *= c2; h1 ^= k1;
    };
    h1 ^= len; h2 ^= len;
    h1 += h2; h2 += h1;
    h1 ^= h1 >> 33; h1 *= BIG_CONSTANT(0xff51afd7ed558ccd);
    h1 ^= h1 >> 33; h1 *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
    h1 ^= h1 >> 33;
    h2 ^= h2 >> 33; h2 *= BIG_CONSTANT(0xff51afd7ed558ccd);
    h2 ^= h2 >> 33; h2 *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
    h2 ^= h2 >> 33; h1 += h2; h2 += h1;
    ((uint64_t*)out)[0] = h1;
    ((uint64_t*)out)[1] = h2;
}

// ============================================================================
//  2. BASE58 & CRYPTO UTILS
// ============================================================================
static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
static int8_t mapBase58[256] = {};
static bool base58_initialized = false;

void InitBase58() {
    if (base58_initialized) return;
    memset(mapBase58, -1, sizeof(mapBase58));
    for (int i = 0; i < 58; i++) mapBase58[(uint8_t)pszBase58[i]] = (int8_t)i;
    base58_initialized = true;
}

bool DecodeBase58(const char* psz, std::vector<uint8_t>& vch) {
    // Skip leading spaces
    while (*psz && isspace((uint8_t)*psz)) psz++;
    // Skip and count leading '1's
    int zeroes = 0;
    int length = 0;
    while (*psz == '1') { zeroes++; psz++; }
    int b256_size = (int)strlen(psz) * 733 / 1000 + 1;
    std::vector<uint8_t> b256(b256_size);
    while (*psz && !isspace((uint8_t)*psz)) {
        int carry = mapBase58[(uint8_t)*psz];
        if (carry == -1) return false;
        int i = 0;
        for (std::vector<uint8_t>::reverse_iterator it = b256.rbegin();
             (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
            carry += 58 * (*it);
            *it = carry % 256;
            carry /= 256;
        }
        length = i;
        psz++;
    }
    while (*psz && isspace((uint8_t)*psz)) psz++;
    if (*psz != 0) return false;
    vch.assign(zeroes + length, 0);
    for (int i = 0; i < length; i++) vch[zeroes + i] = b256[b256_size - length + i];
    return true;
}

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4996)  // Disable deprecation warnings on Windows
#endif

void ComputeSHA256(const unsigned char* data, size_t len, unsigned char* out) {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, len);
    SHA256_Final(out, &ctx);
}

#ifdef _WIN32
#pragma warning(pop)
#endif

bool DecodeBase58Check(const std::string& str, std::vector<uint8_t>& vchRet) {
    if (!DecodeBase58(str.c_str(), vchRet)) return false;
    if (vchRet.size() < 4) return false;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    ComputeSHA256(vchRet.data(), vchRet.size() - 4, hash);
    ComputeSHA256(hash, SHA256_DIGEST_LENGTH, hash);
    if (memcmp(&vchRet[vchRet.size() - 4], hash, 4) != 0) return false;
    vchRet.resize(vchRet.size() - 4);
    return true;
}

// ============================================================================
//  3. BECH32 (BCH code) - BIP-173
// ============================================================================
namespace Bech32 {
    static const char* CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
    static const int8_t CHARSET_REV[128] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        15, -1, 10, 17, 21, 20, 26, 30,  7,  5, -1, -1, -1, -1, -1, -1,
        -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
         1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1,
        -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
         1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1
    };
    
    static const uint32_t GENERATOR[5] = {0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3};
    
    inline uint32_t polymod(const std::vector<uint32_t>& values) {
        uint32_t chk = 1;
        for (uint32_t v : values) {
            uint32_t b = chk >> 25;
            chk = (chk & 0x1ffffff) << 5 ^ v;
            for (int i = 0; i < 5; i++) {
                chk ^= ((b >> i) & 1) ? GENERATOR[i] : 0;
            }
        }
        return chk;
    }
    
    inline std::vector<uint32_t> hrp_expand(const std::string& hrp) {
        std::vector<uint32_t> ret;
        ret.reserve(hrp.size() * 2 + 1);
        for (char c : hrp) ret.push_back(c >> 5);
        ret.push_back(0);
        for (char c : hrp) ret.push_back(c & 31);
        return ret;
    }
    
    inline bool convert_bits(const std::vector<uint8_t>& in, int from_bits, int to_bits, bool pad, std::vector<uint8_t>& out) {
        int acc = 0;
        int bits = 0;
        int maxv = (1 << to_bits) - 1;
        int max_acc = (1 << (from_bits + to_bits - 1)) - 1;
        for (uint8_t c : in) {
            if (c >> from_bits) return false;
            acc = ((acc << from_bits) | c) & max_acc;
            bits += from_bits;
            while (bits >= to_bits) {
                bits -= to_bits;
                out.push_back((acc >> bits) & maxv);
            }
        }
        if (pad) {
            if (bits) out.push_back((acc << (to_bits - bits)) & maxv);
        } else if (bits >= from_bits || ((acc << (to_bits - bits)) & maxv)) {
            return false;
        }
        return true;
    }
}

// Bech32 (BIP-173) and Bech32m (BIP-350) decoder
// Returns: 0 = invalid, 1 = Bech32 (P2WPKH v0), 2 = Bech32m (P2TR v1+)
int DecodeBech32(const std::string& addr, std::vector<uint8_t>& vchRet) {
    // Find separator '1'
    size_t pos = addr.find_last_of('1');
    if (pos == std::string::npos || pos == 0 || pos + 7 > addr.size()) return 0;
    
    std::string hrp = addr.substr(0, pos);
    std::string data_str = addr.substr(pos + 1);
    
    if (data_str.size() < 6) return 0;
    
    // Decode data characters
    std::vector<uint8_t> data;
    data.reserve(data_str.size());
    for (char c : data_str) {
        if (c < 33 || c > 126) return 0;
        int8_t d = Bech32::CHARSET_REV[(uint8_t)c];
        if (d < 0) return 0;
        data.push_back(static_cast<uint8_t>(d));
    }
    
    // Verify checksum (Bech32 = 1, Bech32m = 0x2BC830A3)
    std::vector<uint32_t> polymod_data = Bech32::hrp_expand(hrp);
    for (uint8_t d : data) polymod_data.push_back(d);
    uint32_t checksum = Bech32::polymod(polymod_data);
    
    int bechType = 0;
    if (checksum == 1) bechType = 1;           // Bech32 (BIP-173)
    else if (checksum == 0x2BC830A3) bechType = 2; // Bech32m (BIP-350)
    else return 0;
    
    // Extract payload (skip version byte, exclude 6 checksum chars)
    std::vector<uint8_t> payload_5bit(data.begin() + 1, data.end() - 6);
    
    vchRet.clear();
    if (!Bech32::convert_bits(payload_5bit, 5, 8, false, vchRet)) return 0;
    
    // Validate sizes: P2WPKH = 20 bytes, P2WSH/P2TR = 32 bytes
    if (vchRet.size() != 20 && vchRet.size() != 32) return 0;
    
    return bechType;
}

// ============================================================================
//  4. ADDRESS PARSING
// ============================================================================

std::vector<uint8_t> ParseEthAddress(const std::string& addr) {
    std::vector<uint8_t> result;
    if (addr.size() != 42 || addr.substr(0, 2) != "0x") return result;
    for (size_t i = 2; i < addr.size(); i += 2) {
        std::string byte = addr.substr(i, 2);
        result.push_back((uint8_t)strtol(byte.c_str(), nullptr, 16));
    }
    return result;
}

// Detect coin type from address prefix
std::string DetectCoinFromAddress(const std::string& addr) {
    if (addr.empty()) return "";
    
    // ETH-based addresses (all start with 0x)
    if (addr.size() == 42 && addr.substr(0, 2) == "0x") {
        return "ETH_BASED";
    }
    
    // Bech32 detection
    if (addr.substr(0, 3) == "bc1") return "BTC";
    if (addr.substr(0, 4) == "ltc1") return "LTC";
    if (addr.substr(0, 4) == "btg1") return "BTG";
    if (addr.substr(0, 4) == "vtc1") return "VTC";
    if (addr.substr(0, 4) == "dgb1") return "DGB";
    if (addr.substr(0, 4) == "mona") return "MONA";
    if (addr.substr(0, 4) == "grs1") return "GRS";
    if (addr.substr(0, 4) == "via1") return "VIA";
    if (addr.substr(0, 4) == "btx1") return "BTX";
    if (addr.substr(0, 3) == "fc1") return "FTC";
    
    // Base58 detection based on first character
    char first = addr[0];
    switch (first) {
        case '1': case '3': return "BTC";  // Also BCH, BSV
        case 'L': case 'M': return "LTC";
        case 'D': 
            // Could be DOGE (D), DASH (X), DGB (D), DCR (D)
            if (addr.size() >= 2 && addr[1] == 'R') return "DCR";
            if (addr.size() >= 2 && (addr[1] == 'C' || addr[1] == 'S')) return "DGB";
            return "DOGE";
        case 'X': return "DASH";
        case 'G': 
            // Could be BTG (G), GAME (G), NLG (G)
            if (addr.size() >= 2 && addr[1] == 'S') return "GS";
            return "BTG";
        case 't':
            // Zcash (t1, t3)
            if (addr.size() >= 2 && (addr[1] == '1' || addr[1] == '3')) return "ZEC";
            break;
        case 'R': return "RVN";
        case 'P':
            // PIVX or POT
            if (addr.size() >= 2 && addr[1] == 'W') return "PIVX";
            return "POT";
        case 'V': return "VTC";
        case 'A': return "ABY";
        case 'F':
            // FLO or FTC
            if (addr.size() >= 2 && addr[1] == 'T') return "FLO";
            return "FTC";
        case 'E': return "EMC2";
        case 'B': return "BLK";
        case 'S':
            // SMART or SIB
            if (addr.size() >= 2 && addr[1] == 'X') return "SIB";
            return "SMART";
        case 'a': return "XZC";
        case 'b': return "BTCP";
        case 'z': return "ZEN";
        case 'u': return "UNO";
        case 'x': return "IXC";
        case '2': return "BTX";
        case 'U': return "UBQ";
        case 'C': return "CDN";
        case 'T': return "TAU";
        case 'K': return "KMD";
    }
    
    return "";
}

// ============================================================================
//  MAIN
// ============================================================================

struct CoinStats {
    std::string symbol;
    size_t count = 0;
};

void print_help() {
    std::cout << "\n=======================================================\n";
    std::cout << "   Bloom Builder v8.0 - Multi-Coin (100+ coins)       \n";
    std::cout << "=======================================================\n";
    std::cout << "Usage:\n";
    std::cout << "  build_bloom.exe --input <file> [options]\n\n";
    std::cout << "Arguments:\n";
    std::cout << "  --input <file>   : Input text file containing addresses.\n";
    std::cout << "                     Can be used multiple times for multiple files.\n";
    std::cout << "                     Example: --input btc.txt --input eth.txt\n\n";
    std::cout << "  --out <file>     : Output Bloom Filter file (.blf).\n";
    std::cout << "                     Default: 'out.blf'\n\n";
    std::cout << "  --p <rate>       : Desired False Positive Rate.\n";
    std::cout << "                     Default: 1e-9 (0.000000001)\n\n";
    std::cout << "  --stats          : Show detailed per-coin statistics.\n\n";
    std::cout << "Supported Coins:\n";
    std::cout << "  Bitcoin derivatives: BTC, LTC, DOGE, DASH, BCH, BSV, BTG, ZEC,\n";
    std::cout << "                       VTC, DGB, RVN, PIVX, MONA, FTC, XZC, KMD,\n";
    std::cout << "                       ZEN, NMC, FLO, UNO, ABY, IXC, NLG, SIB,\n";
    std::cout << "                       MEC, EMC2, GAME, XVG, POT, BLK, GRS, SMART,\n";
    std::cout << "                       VIA, UBQ, DCR, BTX, CDN, TAU, MOBI, BPA,\n";
    std::cout << "                       BTCP, BCA\n";
    std::cout << "  Ethereum-based: ETH, ETC, BNB, MATIC, AVAX, FTM, CRO, HT,\n";
    std::cout << "                  OKT, MOVR, GLMR, ONE, KCS, CELO, METIS, BOBA,\n";
    std::cout << "                  RSK, xDAI, PALM, TLOS, EVMOS, KAVA, CANTO,\n";
    std::cout << "                  DFK, WEMIX, SONE\n";
    std::cout << "=======================================================\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) { print_help(); return 0; }
    
    InitBase58(); 
    std::vector<std::string> inputs;
    std::string outputFile = "out.blf";
    double fpRate = DEFAULT_FP_RATE;
    bool showStats = false;

    // Parsare argumente
    for(int i=1; i<argc; ++i) {
        std::string arg = argv[i];
        if(arg=="--input" && i+1<argc) inputs.push_back(argv[++i]);
        else if(arg=="--out" && i+1<argc) outputFile = argv[++i];
        else if(arg=="--p" && i+1<argc) fpRate = std::stod(argv[++i]);
        else if(arg=="--stats") showStats = true;
        else if(arg=="--help" || arg=="-h") { print_help(); return 0; }
        else if (arg.find("--") == std::string::npos) {
            inputs.push_back(arg);
        }
    }

    if(inputs.empty()) { 
        std::cerr << "Error: No input files specified.\n"; 
        print_help();
        return 1; 
    }

    std::cout << "--- Bloom Builder v8.0 (Multi-Coin: 100+ coins) ---\n";

    // 1. Numarare intrari (pentru calcul marime)
    uint64_t n=0;
    std::cout << "[INFO] Counting entries in input files...\n";
    for(const auto& file : inputs) {
        std::ifstream f(file);
        if(!f.is_open()) { std::cerr << "[WARN] Cannot open file: " << file << "\n"; continue; }
        std::string line;
        while(std::getline(f, line)) {
            line.erase(std::remove_if(line.begin(), line.end(), [](unsigned char c){ return std::isspace(c); }), line.end());
            if(!line.empty() && line.length() > 10) n++;
        }
    }
    std::cout << "Total Entries: " << n << "\n";
    if(n==0) { std::cerr << "[ERR] No valid addresses found in input files.\n"; return 1; }

    // 2. Calcul marime necesara (m bits)
    double m_d = -1.0 * n * log(fpRate) / pow(log(2.0), 2.0);
    uint64_t m = (uint64_t)ceil(m_d);
    if (m < MIN_BLOOM_SIZE_BITS) m = MIN_BLOOM_SIZE_BITS;
    m = ((m + 63) / 64) * 64; 
    
    // 3. DECIDERE AUTOMATA ALGORITM (HIBRID)
    bool useHighRes = false;
    uint8_t version = 3;

    if (m > BLOOM_32BIT_LIMIT) {
        useHighRes = true;
        version = 4;
        std::cout << "[AUTO] Large dataset detected (" << (m/8/1024/1024) << " MB).\n";
        std::cout << "[AUTO] Switched to 64-bit MurmurHash3 (High Precision/Collision Free).\n";
    } else {
        useHighRes = false;
        version = 3;
        std::cout << "[AUTO] Small/Medium dataset detected.\n";
        std::cout << "[AUTO] Using Standard 32-bit MurmurHash3 (Faster).\n";
    }

    // Calcul numar functii hash (k)
    uint32_t k = (uint32_t)round(((double)m / n) * log(2.0));
    if (k == 0) k = 1;
    
    std::cout << "Bloom Params: m=" << m << " bits (" << (m/8/1024/1024) << " MB) | k=" << k << " | Ver=" << (int)version << "\n";

    // Alocare memorie
    std::vector<uint8_t> bitarray;
    try {
        bitarray.resize(m/8, 0);
    } catch (const std::bad_alloc& e) {
        std::cerr << "[FATAL] Not enough RAM! Needed approx " << (m/8/1024/1024) << " MB.\n";
        return 1;
    }

    // 4. Procesare fisiere si populare filtru
    size_t addedCount = 0;
    size_t rejectedCount = 0;
    std::map<std::string, CoinStats> coinStats;
    
    // Initialize coin stats
    for (const auto& coin : g_coinPrefixes) {
        coinStats[coin.symbol] = {coin.symbol, 0};
    }
    coinStats["ETH_BASED"] = {"ETH_BASED", 0};
    coinStats["UNKNOWN"] = {"UNKNOWN", 0};

    for(const auto& file : inputs) {
        std::cout << "Processing file: " << file << "...\n";
        std::ifstream f(file);
        std::string line;
        while(std::getline(f, line)) {
            line.erase(std::remove_if(line.begin(), line.end(), [](unsigned char c){ return std::isspace(c); }), line.end());
            if(line.empty()) continue;
            
            std::vector<uint8_t> payload;
            bool valid = false;
            std::string detectedCoin = "";

            // Detectie ETH-based (0x...)
            if (line.size() == 42 && line.substr(0,2) == "0x") {
                payload = ParseEthAddress(line);
                if (payload.size() == 20) { 
                    valid = true; 
                    detectedCoin = "ETH_BASED";
                }
            }
            // Detectie Bech32/Bech32m addresses
            else if (line.substr(0, 3) == "bc1") {
                int bechType = DecodeBech32(line, payload);
                if (bechType > 0 && payload.size() == 20) { // Only P2WPKH (20 bytes) for bloom
                    valid = true; 
                    detectedCoin = "BTC";
                }
            }
            else if (line.substr(0, 4) == "ltc1") {
                int bechType = DecodeBech32(line, payload);
                if (bechType > 0 && payload.size() == 20) { 
                    valid = true; 
                    detectedCoin = "LTC";
                }
            }
            else if (line.substr(0, 4) == "btg1") {
                int bechType = DecodeBech32(line, payload);
                if (bechType > 0 && payload.size() == 20) { 
                    valid = true; 
                    detectedCoin = "BTG";
                }
            }
            else if (line.substr(0, 4) == "vtc1") {
                int bechType = DecodeBech32(line, payload);
                if (bechType > 0 && payload.size() == 20) { 
                    valid = true; 
                    detectedCoin = "VTC";
                }
            }
            else if (line.substr(0, 4) == "dgb1") {
                int bechType = DecodeBech32(line, payload);
                if (bechType > 0 && payload.size() == 20) { 
                    valid = true; 
                    detectedCoin = "DGB";
                }
            }
            else if (line.substr(0, 4) == "grs1") {
                int bechType = DecodeBech32(line, payload);
                if (bechType > 0 && payload.size() == 20) { 
                    valid = true; 
                    detectedCoin = "GRS";
                }
            }
            else if (line.substr(0, 4) == "via1") {
                int bechType = DecodeBech32(line, payload);
                if (bechType > 0 && payload.size() == 20) { 
                    valid = true; 
                    detectedCoin = "VIA";
                }
            }
            else if (line.substr(0, 4) == "btx1") {
                int bechType = DecodeBech32(line, payload);
                if (bechType > 0 && payload.size() == 20) { 
                    valid = true; 
                    detectedCoin = "BTX";
                }
            }
            else if (line.substr(0, 4) == "mona") {
                int bechType = DecodeBech32(line, payload);
                if (bechType > 0 && payload.size() == 20) { 
                    valid = true; 
                    detectedCoin = "MONA";
                }
            }
            else if (line.substr(0, 3) == "fc1") {
                int bechType = DecodeBech32(line, payload);
                if (bechType > 0 && payload.size() == 20) { 
                    valid = true; 
                    detectedCoin = "FTC";
                }
            }
            // Detectie Base58 (Legacy/P2SH) - generic pentru toate monedele
            else {
                if (DecodeBase58Check(line, payload)) {
                    // Base58Check returneaza version+hash (21 bytes pentru majoritatea)
                    if (payload.size() == 21) { 
                        payload.erase(payload.begin()); 
                        valid = true;
                        detectedCoin = DetectCoinFromAddress(line);
                        if (detectedCoin.empty()) detectedCoin = "UNKNOWN";
                    } 
                    else if (payload.size() == 20) { 
                        valid = true;
                        detectedCoin = DetectCoinFromAddress(line);
                        if (detectedCoin.empty()) detectedCoin = "UNKNOWN";
                    }
                }
            }
            
            // Adaugare in bloom filter daca payload-ul are 20 bytes (Hash160)
            if(valid && payload.size() == 20) {
                uint64_t h1, h2;

                if (useHighRes) {
                    uint64_t hash[2];
                    MurmurHash3_x64_128(payload.data(), 20, 0, hash);
                    h1 = hash[0];
                    h2 = hash[1];
                } else {
                    uint32_t r1 = MurmurHash3_x86_32(payload.data(), 20, 0xFBA4C795);
                    uint32_t r2 = MurmurHash3_x86_32(payload.data(), 20, 0x43876932);
                    h1 = (uint64_t)r1;
                    h2 = (uint64_t)r2;
                }

                // Setare biti in filtru
                for(uint32_t i=0; i<k; ++i) {
                    uint64_t idx = (h1 + i * h2) % m;
                    bitarray[idx/8] |= (1<<(idx%8));
                }
                addedCount++;
                
                // Update stats
                if (!detectedCoin.empty()) {
                    coinStats[detectedCoin].count++;
                }
            } else if (!line.empty()) {
                rejectedCount++;
            }
        }
    }

    std::cout << "\n=== BUILD COMPLETE ===\n";
    std::cout << "Successfully added: " << addedCount << " addresses\n";
    std::cout << "Rejected: " << rejectedCount << " addresses\n";
    
    // Afisare statistici pe monede
    if (showStats) {
        std::cout << "\n=== PER-COIN STATISTICS ===\n";
        size_t btcDerivCount = 0;
        size_t ethBasedCount = 0;
        
        for (const auto& pair : coinStats) {
            if (pair.second.count > 0) {
                if (pair.first == "ETH_BASED") {
                    ethBasedCount = pair.second.count;
                } else if (pair.first != "UNKNOWN") {
                    btcDerivCount += pair.second.count;
                    std::cout << "  " << pair.first << ": " << pair.second.count << "\n";
                }
            }
        }
        
        if (ethBasedCount > 0) {
            std::cout << "  ETH-based (all): " << ethBasedCount << "\n";
        }
        if (coinStats["UNKNOWN"].count > 0) {
            std::cout << "  UNKNOWN: " << coinStats["UNKNOWN"].count << "\n";
        }
    } else {
        // Summary only
        size_t btcDerivCount = 0;
        size_t ethBasedCount = coinStats["ETH_BASED"].count;
        
        for (const auto& pair : coinStats) {
            if (pair.first != "ETH_BASED" && pair.first != "UNKNOWN") {
                btcDerivCount += pair.second.count;
            }
        }
        
        std::cout << "Summary: BTC-derived=" << btcDerivCount 
                  << " | ETH-based=" << ethBasedCount;
        if (coinStats["UNKNOWN"].count > 0) {
            std::cout << " | Unknown=" << coinStats["UNKNOWN"].count;
        }
        std::cout << "\n";
    }

    // 5. Salvare pe disc (Format Big-Endian compatibil cu bloom.h)
    std::ofstream out(outputFile, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "[ERR] Could not write to file: " << outputFile << "\n";
        return 1;
    }

    // Helper pentru scriere big-endian
    auto write_be32 = [&out](uint32_t val) {
        uint8_t buf[4] = {(uint8_t)(val >> 24), (uint8_t)(val >> 16), (uint8_t)(val >> 8), (uint8_t)val};
        out.write((char*)buf, 4);
    };
    auto write_be64 = [&out](uint64_t val) {
        uint8_t buf[8] = {(uint8_t)(val >> 56), (uint8_t)(val >> 48), (uint8_t)(val >> 40), (uint8_t)(val >> 32),
                          (uint8_t)(val >> 24), (uint8_t)(val >> 16), (uint8_t)(val >> 8), (uint8_t)val};
        out.write((char*)buf, 8);
    };

    // Header Format: Magic(4) + Ver(1) + m(8) + k(4) + len(8) + data(...)
    out.write("BLM3", 4);
    out.write((char*)&version, 1); 
    write_be64(m);
    write_be32(k);
    
    uint64_t dataLen = bitarray.size();
    write_be64(dataLen);
    out.write((char*)bitarray.data(), dataLen);
    
    out.close();
    
    std::cout << "Output file: " << outputFile << " (" << dataLen << " bytes)\n";
    std::cout << "========================\n";
    
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    
    return 0;
}
