#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <random>
#include <iomanip>
#include <cstring> 

#include <openssl/sha.h>
#include <openssl/ripemd.h>
#ifndef NO_SECP256K1
#include <secp256k1.h>
#endif

#include "utils.h"
#include "bloom.h"
#include "akm.h"
#include "multicoin.h"
#include "coin_database.h"

// Definitie pentru functia CUDA externa (trebuie sa coincida cu GpuCore.cu)
extern "C" void launch_gpu_akm_search(
    unsigned long long startSeed,
    unsigned long long count,
    int blocks,
    int threads,
    int points,
    const void* bloomFilterData,
    size_t bloomFilterSize,
    unsigned long long* outFoundSeeds,
    int* outFoundCount,
    int targetBits,
    bool sequential,
    const void* prefix,
    int prefixLen
);

struct AkmResult {
    struct AddrRow { std::string type, path, addr; std::string status; bool isHit; };
    std::string phrase;
    std::string hexKey;
    std::vector<AddrRow> rows;
};

class AkmTool {
private:
    AkmLogic akm;
    secp256k1_context* ctx;
    std::vector<std::string> activeCoins;  // Multi-coin support
    
    // Stocare pentru partea superioara a cheii (peste 64 biti)
    // Util pentru puzzle-uri unde stim prefixul (ex: 50xxxxxx...)
    std::vector<uint8_t> highBytes;

    static constexpr const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    static constexpr const char* bech32_charset = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

    // --- Crypto Helpers ---
    void Hash160(const std::vector<uint8_t>& in, std::vector<uint8_t>& out) {
        uint8_t sha[SHA256_DIGEST_LENGTH]; SHA256(in.data(), in.size(), sha);
        uint8_t rip[RIPEMD160_DIGEST_LENGTH]; RIPEMD160(sha, SHA256_DIGEST_LENGTH, rip);
        out.resize(20); memcpy(out.data(), rip, 20);
    }

    std::string EncodeBase58Check(const std::vector<uint8_t>& vchIn) {
        std::vector<uint8_t> vch(vchIn);
        uint8_t hash[SHA256_DIGEST_LENGTH], hash2[SHA256_DIGEST_LENGTH];
        SHA256(vch.data(), vch.size(), hash); SHA256(hash, SHA256_DIGEST_LENGTH, hash2);
        vch.push_back(hash2[0]); vch.push_back(hash2[1]); vch.push_back(hash2[2]); vch.push_back(hash2[3]);
        
        const unsigned char* pbegin = vch.data();
        const unsigned char* pend = vch.data() + vch.size();
        int zeros = 0; while (pbegin != pend && *pbegin == 0) { pbegin++; zeros++; }
        std::vector<unsigned char> b58((pend - pbegin) * 138 / 100 + 1);
        std::vector<unsigned char>::iterator it = b58.begin(); *it = 0;
        while (pbegin != pend) {
            int carry = *pbegin;
            for (std::vector<unsigned char>::iterator i = b58.begin(); i != it + 1; ++i) {
                carry += 256 * (*i); *i = carry % 58; carry /= 58;
            }
            while (carry != 0) { it++; *it = carry % 58; carry /= 58; }
            pbegin++;
        }
        std::string str; str.reserve(zeros + (it - b58.begin() + 1)); str.assign(zeros, '1');
        for (std::vector<unsigned char>::reverse_iterator i = b58.rbegin() + (b58.size() - 1 - (it - b58.begin())); i != b58.rend(); ++i) str += pszBase58[*i];
        return str;
    }

    std::string Bech32Encode(const std::vector<int>& values) {
        uint32_t chk = 1;
        uint32_t gen[] = {0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3};
        auto polymod_step = [&](uint8_t v) {
            uint8_t b = chk >> 25; chk = ((chk & 0x1ffffff) << 5) ^ v;
            for(int i=0; i<5; ++i) if((b>>i)&1) chk ^= gen[i];
        };
        polymod_step(3); polymod_step(3); polymod_step(0); polymod_step(2); polymod_step(3);
        std::string ret = "bc1";
        for(int v : values) { polymod_step(v); ret += bech32_charset[v]; }
        for(int i=0; i<6; ++i) polymod_step(0);
        chk ^= 1; for(int i=0; i<6; ++i) ret += bech32_charset[(chk >> ((5-i)*5)) & 31];
        return ret;
    }

    // --- Address Converters ---
    std::string PubKeyToLegacy(const std::vector<uint8_t>& pubKey) {
        std::vector<uint8_t> hash; Hash160(pubKey, hash);
        std::vector<uint8_t> data = {0x00}; data.insert(data.end(), hash.begin(), hash.end());
        return EncodeBase58Check(data);
    }
    std::string PubKeyToNestedSegwit(const std::vector<uint8_t>& pubKey) {
        std::vector<uint8_t> pubHash; Hash160(pubKey, pubHash);
        std::vector<uint8_t> script = { 0x00, 0x14 }; script.insert(script.end(), pubHash.begin(), pubHash.end());
        std::vector<uint8_t> scriptHash; Hash160(script, scriptHash);
        std::vector<uint8_t> data = {0x05}; data.insert(data.end(), scriptHash.begin(), scriptHash.end());
        return EncodeBase58Check(data);
    }
    std::string PubKeyToNativeSegwit(const std::vector<uint8_t>& pubKey) {
        std::vector<uint8_t> h160; Hash160(pubKey, h160);
        std::vector<int> data5; data5.push_back(0); // Witness version 0
        int acc = 0, bits = 0;
        for(uint8_t b : h160) {
            acc = (acc << 8) | b; bits += 8;
            while(bits >= 5) { data5.push_back((acc >> (bits - 5)) & 31); bits -= 5; }
        }
        if(bits > 0) data5.push_back((acc << (5 - bits)) & 31);
        return Bech32Encode(data5);
    }

    std::string toHex(const std::vector<uint8_t>& d) {
        std::stringstream ss; ss << std::hex << std::setfill('0');
        for(auto b : d) ss << std::setw(2) << (int)b;
        return ss.str();
    }

    // --- LOGICA PUZZLE MATEMATICA ---
    
    // Aduna seed-ul (offset-ul) la cheie, propagand transportul (carry) catre octetii superiori.
    void addSeedToKey(std::vector<uint8_t>& key, unsigned long long seed) {
        unsigned long long carry = 0;
        unsigned long long tempSeed = seed;
        
        // 1. Adunam seed-ul la ultimii 8 bytes (64 biti)
        // key[31] este byte-ul cel mai putin semnificativ (LSB)
        for (int i = 0; i < 8; i++) {
            int idx = 31 - i;
            if (idx < 0) break;

            unsigned long long byteVal = tempSeed & 0xFF;
            tempSeed >>= 8;

            unsigned long long sum = (unsigned long long)key[idx] + byteVal + carry;
            key[idx] = (uint8_t)(sum & 0xFF);
            carry = sum >> 8;
        }

        // 2. Propagam carry-ul in sus (peste 64 biti) daca e cazul
        // Asta permite ca seed-ul sa "umfle" partea superioara (ex: 50 -> 51)
        int idx = 23; // 31 - 8
        while (carry > 0 && idx >= 0) {
            unsigned long long sum = (unsigned long long)key[idx] + carry;
            key[idx] = (uint8_t)(sum & 0xFF);
            carry = sum >> 8;
            idx--;
        }
    }

    void setTargetBit(std::vector<uint8_t>& key, int bits) {
        if (bits <= 0 || bits > 256) return;
        // Calcul bit in format Big Endian (byte 0 = MSB, byte 31 = LSB)
        int bitIndex = bits - 1;
        int byteIndex = 31 - (bitIndex / 8); 
        int bitInByte = bitIndex % 8;
        key[byteIndex] |= (1 << bitInByte);
    }

    int BigIntDivMod(std::vector<uint8_t>& n, int divisor) {
        int remainder = 0;
        for (size_t i = 0; i < n.size(); i++) {
            unsigned int val = (remainder << 8) + n[i];
            n[i] = val / divisor;
            remainder = val % divisor;
        }
        return remainder;
    }

public:
    AkmTool() { 
        ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY); 
        activeCoins = {"btc"}; // Default to BTC
    }
    ~AkmTool() { if (ctx) secp256k1_context_destroy(ctx); }
	const std::vector<uint8_t>& getHighBytes() const { return highBytes; }

    void init(const std::string& profile, const std::string& wordlistPath) {
        akm.init(profile, wordlistPath);
    }
    
    // Set active coins for multi-coin support
    void setActiveCoins(const std::vector<std::string>& coins) {
        activeCoins = coins;
        // Default to BTC if empty
        if (activeCoins.empty()) {
            activeCoins.push_back("btc");
        }
    }

    // [NOU - Aceasta este functia care lipsea]
    // Functie pentru a seta prefixul Hex (partea de sus, ex: "50")
    // Folosita cand dam --continue cu un offset mare
    void setHighHexPrefix(std::string hex) {
        if (hex.length() % 2 != 0) hex = "0" + hex; 
        highBytes.clear();
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byteString = hex.substr(i, 2);
            uint8_t byte = (uint8_t)strtol(byteString.c_str(), nullptr, 16);
            highBytes.push_back(byte);
        }
    }

    AkmResult processAkmSeed(unsigned long long seed, const std::string& order, int targetBits, int phraseLen, const BloomFilter& bloom) {
        AkmResult result;
        std::vector<uint8_t> directKey(32, 0); 
        const std::vector<std::string>& akmWords = akm.getWordList();
        size_t akmSize = akmWords.size();



        if (akmSize == 0) {
            std::cout << "[ERROR] AKM wordlist is empty!\n";
            return result;
        }

        if (order == "random") {
            // Generare aleatorie completa (ignorand secventialitatea)
            std::mt19937_64 r(seed);
            int byteLen = (targetBits + 7) / 8;
            // If targetBits=0, generate full 32 bytes, otherwise generate up to targetBits
            if (targetBits == 0) {
                byteLen = 32; // Full key random generation
            }
            for(int i=0; i<byteLen; i++) directKey[31-i] = r() % 256;
            if (targetBits > 0) setTargetBit(directKey, targetBits); 
        } else {
            // Mod Schematic (Sequential) - FIX COMPLET:
            
            // 1. Initializam cheia cu partea HIGH (prefixul din --continue)
            // Calculam pozitia: Low 8 bytes sunt pentru seed. High bytes vin deasupra.
            int highCount = (int)highBytes.size();
            int startIdx = 24 - highCount; // 32 total - 8 (seed) - lungime_high
            
            // Umplem bytes de la coada spre capul zonei superioare
            if (startIdx >= 0) {
                for(int i=0; i<highCount; i++) {
                    directKey[startIdx + i] = highBytes[i];
                }
            }

            // 2. Setam bitul de puzzle (doar OR logic, nu distruge prefixul)
            if (targetBits > 0) setTargetBit(directKey, targetBits);
            
            // 3. Adunam seed-ul curent (partea de jos, 64 biti) folosind matematica BigInt
            // Asta va propaga carry-ul daca seed-ul face overflow la zona de 64 biti
            addSeedToKey(directKey, seed);
        }

        result.hexKey = toHex(directKey);
        
        // Convertire cheie -> Cuvinte AKM (Base Conversion)
        std::vector<uint8_t> tempForWords = directKey;
        std::vector<std::string> phraseWords;
        for (int w = 0; w < phraseLen; ++w) {
            int idx = BigIntDivMod(tempForWords, (int)akmSize);
            phraseWords.insert(phraseWords.begin(), akmWords[idx]);
        }
        for (const auto& w : phraseWords) result.phrase += w + " ";

        // Derivare Chei Publice & Adrese (Multi-Coin Support)
        secp256k1_pubkey pubkey;
        if (secp256k1_ec_pubkey_create(ctx, &pubkey, directKey.data())) {
            uint8_t cPub[33], uPub[65];
            size_t len = 33; secp256k1_ec_pubkey_serialize(ctx, cPub, &len, &pubkey, SECP256K1_EC_COMPRESSED);
            len = 65; secp256k1_ec_pubkey_serialize(ctx, uPub, &len, &pubkey, SECP256K1_EC_UNCOMPRESSED);

            std::vector<uint8_t> vPubC(cPub, cPub + 33), vPubU(uPub, uPub + 65);

            // Generate addresses for all active coins
            for (const auto& coinSymbol : activeCoins) {
                std::string coin = coinSymbol;
                std::transform(coin.begin(), coin.end(), coin.begin(), ::tolower);
                
                std::string coinUpper = coin;
                std::transform(coinUpper.begin(), coinUpper.end(), coinUpper.begin(), ::toupper);
                
                CoinInfo coinInfo = CoinDatabase::getCoinBySymbol(coin);

                if (coinInfo.isEthereumBased) {
                    // Ethereum-based coins (ETH, ETC, BNB, MATIC, AVAX, FTM, CRO, DeFi tokens, etc.)
                    std::string ethAddr = MultiCoin::GenEth(vPubU);
                    if (!ethAddr.empty() && ethAddr.substr(0, 2) == "0x") {
                        std::vector<uint8_t> payload;
                        for (size_t i = 2; i < ethAddr.length(); i += 2) {
                            payload.push_back((uint8_t)std::stoi(ethAddr.substr(i, 2), nullptr, 16));
                        }
                        bool isHit = bloom.isLoaded() ? bloom.check_hash160(payload) : false;
                        std::string path = "m/44'/" + std::to_string(coinInfo.slip44) + "'/0'/0/0";
                        result.rows.push_back({coinUpper + "-AKM", path, ethAddr, (isHit ? "HIT" : "-"), isHit});
                    }
                } 
                else if (coin == "atom") {
                    // Cosmos (ATOM)
                    std::string atomAddr = MultiCoin::GenCosmos(vPubC);
                    if (!atomAddr.empty()) {
                        std::vector<uint8_t> h160; MultiCoin::PublicHash160(vPubC, h160);
                        bool isHit = bloom.isLoaded() ? bloom.check_hash160(h160) : false;
                        result.rows.push_back({coinUpper + "-AKM", "m/44'/118'/0'/0/0", atomAddr, (isHit ? "HIT" : "-"), isHit});
                    }
                }
                else if (coin == "dot") {
                    // Polkadot (DOT) - simplified
                    std::string dotAddr = MultiCoin::GenPolkadot(vPubC);
                    if (!dotAddr.empty() && dotAddr.substr(0, 3) != "DOT") {
                        std::vector<uint8_t> h160; MultiCoin::PublicHash160(vPubC, h160);
                        bool isHit = bloom.isLoaded() ? bloom.check_hash160(h160) : false;
                        result.rows.push_back({coinUpper + "-AKM", "m/44'/354'/0'/0/0", dotAddr, (isHit ? "HIT" : "-"), isHit});
                    }
                }
                else if (coin == "sol") {
                    // Solana (SOL)
                    std::string solAddr = MultiCoin::GenSolana(vPubC);
                    if (!solAddr.empty()) {
                        std::vector<uint8_t> h160; MultiCoin::PublicHash160(vPubC, h160);
                        bool isHit = bloom.isLoaded() ? bloom.check_hash160(h160) : false;
                        result.rows.push_back({coinUpper + "-AKM", "m/44'/501'/0'/0'", solAddr, (isHit ? "HIT" : "-"), isHit});
                    }
                }
                else if (coin == "ada") {
                    // Cardano (ADA)
                    std::string adaAddr = MultiCoin::GenCardano(vPubC);
                    if (!adaAddr.empty()) {
                        std::vector<uint8_t> h160; MultiCoin::PublicHash160(vPubC, h160);
                        bool isHit = bloom.isLoaded() ? bloom.check_hash160(h160) : false;
                        result.rows.push_back({coinUpper + "-AKM", "m/1852'/1815'/0'/0/0", adaAddr, (isHit ? "HIT" : "-"), isHit});
                    }
                }
                else {
                    // Bitcoin-derived coins (BTC, LTC, DOGE, DASH, BTG, ZEC, BCH, BSV, etc.)
                    std::string path44 = "m/44'/" + std::to_string(coinInfo.slip44) + "'/0'/0/0";
                    
                    // 1. Legacy (P2PKH)
                    std::string legacyAddr = MultiCoin::GenLegacy(vPubC, coin);
                    if (!legacyAddr.empty()) {
                        std::vector<uint8_t> h160; MultiCoin::PublicHash160(vPubC, h160);
                        bool isHit = bloom.isLoaded() ? bloom.check_hash160(h160) : false;
                        result.rows.push_back({coinUpper + "-AKM [Legacy]", path44, legacyAddr, (isHit ? "HIT" : "-"), isHit});
                    }
                    
                    // 2. Legacy Uncompressed
                    std::string legacyUncompAddr = MultiCoin::GenLegacy(vPubU, coin);
                    if (!legacyUncompAddr.empty() && legacyUncompAddr != legacyAddr) {
                        std::vector<uint8_t> h160; MultiCoin::PublicHash160(vPubU, h160);
                        bool isHit = bloom.isLoaded() ? bloom.check_hash160(h160) : false;
                        result.rows.push_back({coinUpper + "-AKM [Legacy Uncomp]", path44, legacyUncompAddr, (isHit ? "HIT" : "-"), isHit});
                    }
                    
                    // 3. P2SH (Segwit)
                    std::string p2shAddr = MultiCoin::GenP2SH(vPubC, coin);
                    if (!p2shAddr.empty()) {
                        std::vector<uint8_t> pubHash; MultiCoin::PublicHash160(vPubC, pubHash);
                        std::vector<uint8_t> script = { 0x00, 0x14 };
                        script.insert(script.end(), pubHash.begin(), pubHash.end());
                        std::vector<uint8_t> scriptHash; MultiCoin::PublicHash160(script, scriptHash);
                        bool isHit = bloom.isLoaded() ? bloom.check_hash160(scriptHash) : false;
                        std::string path49 = "m/49'/" + std::to_string(coinInfo.slip44) + "'/0'/0/0";
                        result.rows.push_back({coinUpper + "-AKM [P2SH]", path49, p2shAddr, (isHit ? "HIT" : "-"), isHit});
                    }
                    
                    // 4. Bech32 (Native Segwit)
                    std::string bech32Addr = MultiCoin::GenBech32(vPubC, coin);
                    if (!bech32Addr.empty() && bech32Addr.substr(0, 3) != "BCH") {
                        std::vector<uint8_t> h160; MultiCoin::PublicHash160(vPubC, h160);
                        bool isHit = bloom.isLoaded() ? bloom.check_hash160(h160) : false;
                        std::string path84 = "m/84'/" + std::to_string(coinInfo.slip44) + "'/0'/0/0";
                        result.rows.push_back({coinUpper + "-AKM [Bech32]", path84, bech32Addr, (isHit ? "HIT" : "-"), isHit});
                    }
                }
            }
        }
        return result;
    }

    size_t getWordCount() { return akm.getWordCount(); }
    void listProfiles() { akm.listProfiles(); }
    

};