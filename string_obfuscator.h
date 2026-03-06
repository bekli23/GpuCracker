#pragma once

// String obfuscation to avoid AV signature detection
// Uses XOR encryption for sensitive strings

template<size_t N>
class ObfuscatedString {
private:
    char data[N];
    static constexpr char XOR_KEY = 0x47; // 'G'
    
public:
    constexpr ObfuscatedString(const char (&str)[N]) {
        for (size_t i = 0; i < N; i++) {
            data[i] = str[i] ^ XOR_KEY;
        }
    }
    
    void decrypt(char* out) const {
        for (size_t i = 0; i < N; i++) {
            out[i] = data[i] ^ XOR_KEY;
        }
    }
    
    std::string get() const {
        std::string result;
        result.resize(N - 1);
        decrypt(&result[0]);
        return result;
    }
};

// Macro for easy usage
#define OBF(s) ObfuscatedString<sizeof(s)>(s).get().c_str()

// Alternative: Runtime XOR decryption
inline std::string deobfuscate(const char* data, size_t len, char key = 0x47) {
    std::string result;
    result.reserve(len);
    for (size_t i = 0; i < len; i++) {
        result.push_back(data[i] ^ key);
    }
    return result;
}
