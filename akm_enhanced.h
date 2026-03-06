#pragma once
#define _CRT_SECURE_NO_WARNINGS

// =========================================================
// AKM Enhanced v2.0 - Advanced Key Mapper
// Complete implementation with profile management
// =========================================================

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <random>
#include <functional>

// Handle GCC < 8 filesystem support
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 8
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#else
    #include <filesystem>
    namespace fs = std::filesystem;
#endif

#include <cassert>

// =========================================================
// FORWARD DECLARATIONS
// =========================================================
struct AkmRule;
class AkmProfileManager;

// =========================================================
// WORD LIST (512 words) - From AcasaKey
// =========================================================
static const std::vector<std::string> AKM_BASE_WORDS = {
    "abis","acelasi","acoperis","adanc","adapost","adorare","afectiune","aer",
    "ajun","albastru","alb","alge","altar","amintire","amurg","anotimp","apa",
    "apele","apus","apuseni","apusor","aroma","artar","arta","asfintit",
    "asteptare","atingere","aur","aurora","autumn","avion","balta","barca",
    "barza","baterie","batran","bec","bezna","binecuvantare","blandete","boboc",
    "bogatie","bolta","brad","brat","bruma","bucata","bucurie","bujor","burg",
    "camp","campie","cafea","calator","cald","candela","caprioara","caramida",
    "carare","carte","catel","cautare","casa","ceas","cer","cerb","chip",
    "ciocarlie","ciutura","clar","clipa","clopot","coborare","colina","colt",
    "copac","copil","corabie","cord","corn","crang","credinta","crestere",
    "crestet","crin","cuc","cufar","culoare","culme","curcubeu","curte","cupru",
    "cuvant","cutie","daurire","deal","deget","delusor","departare","desert",
    "dimineata","dor","dorinta","drag","draga","drum","drumet","durere",
    "duminica","ecou","efemer","elixir","emisfera","enigma","eter","eternitate",
    "fag","fagure","fantana","farmec","fata","felinar","fenic","fereastra",
    "fericire","feriga","fier","fierar","film","fior","flacara","flamura",
    "floare","fluture","fosnet","fotografie","frag","frate","frezie","frig",
    "fruct","frumusete","frunza","frunte","fulger","furnica","galaxie","galben",
    "gand","gandire","garoafa","gheata","ghetar","ghinda","ghiozdan","glas",
    "glorie","grad","gradina","grai","granita","gust","gura","har","harfa",
    "iarba","iarna","icoana","implinire","inger","insula","insorire","intindere",
    "intuneric","inviere","iubire","iz","izvor","izvoras","joc","jocul","lac",
    "lacrima","laur","lebada","legenda","lemn","leu","libertate","linie","livada",
    "loc","luna","lumina","lume","lunca","lup","lut","manunchi","margine",
    // Extended word list (additional words for profiles)
    "mare","munte","vale","rau","padure","soare","luna","stea","nor","vant",
    "ploaie","zapada","ceata","roua","picatura","val","spuma","nisip","piatra",
    "muscata","trandafir","lalea","zambila","narcisa","toporasi","ghiocei",
    " Brandusa","viorele","maci","mugetar","tulbure","rece","caldut","fierbinte",
    "usor","greu","tare","moale","repede","incet","departe","aproape","sus",
    "jos","stanga","dreapta","inainte","inapoi","primavara","vara","toamna",
    "inghet","dezghet","rasarit","apus","amiaza","miezul_noptii","rasaritul",
    "caldura","racoare","brizna","furtuna","tunet","fulger","curcubeul","arc",
    "punte","pod","turn","cladire","castel","palat","coliba","cort","stana",
    "grajd","hambar","fantanita","izvorasul","paraul","raul","fluviul",
    "lacul","tarnita","iazul","balta","mlastina","mocirla","nisipurile",
    "dunele","stancile","pesterile","pesteri","grota","prapastia","abisul",
    "culmile","varful","poiana","poienita","lunca","lunculita","campia",
    "campul","livezile","padurea","codrul","sesul","dealul","dealusorul",
    "muntele","muntisorul","caraimanul","pietrosul","carpatii","balcani",
    "alpii","pireneii","caucazul","uralii","himalaya","everestul","kilimanjaro",
    "fuji","vesuviu","etna","vulcanul","vulcanii","geizerul","izvoarele",
    "scurgerea","cascada","sarabanda","valurile","torentul","inundatia",
    "seceta","ariditatea","vegetatia","flora","fauna","ecosistemul","biosfera",
    "atmosfera","stratul","ionosfera","ozonul","oxigenul","azotul","hidrogenul",
    "heliul","neonul","argintul","aurul","platinumul","diamantul","rubinul",
    "safirul","smaraldul","topazul","ametistul","opala","perla","coralul",
    "chihlimbarul","turcoazul","lapislazuli","jadul","malachitul","onixul",
    "obsidianul","granitul","marmura","ardezia","calcarul","nisipul","argila",
    "lutul","humusul","solul","terenul","pamantul","globul","planeta",
    "terra","gaia","lumea","universul","cosmosul","galaxia","constelatia",
    "steaua","soarele","luna","satelitul","asteroidul","cometa","meteoritul",
    "pulsarul","quasarul","neutronul","protonul","electronul","fotonul",
    "neutrinoul","atomul","molecula","celula","organismul","fiinta","viata",
    "energia","forta","puterea","taria","vigoarea","vlaga","sufletul",
    "spiritul","mintea","gandul","ideea","conceptul","teoria","legea",
    "regula","principiul","axioma","teorema","demonstratia","dovada",
    "proba","argumentul","rationamentul","logica","filosofia","intelepciunea",
    "cunoasterea","stiinta","invatatura","educatia","cultura","civilizatia",
    "progresul","dezvoltarea","evolutia","revolutia","transformarea",
    "schimbarea","adaptarea","supravietuirea","existenta","fiintarea",
    "esenta","substanta","materia","antimateria","spatiul","timpul",
    "dimensiunea","realitatea","virtualul","simularea","modelul",
    "sistemul","structura","organizarea","ordinea","haosul","entropia",
    "sinele","ego","identitatea","personalitatea","caracterul","temperamentul",
    "natura","firea","inclinatia","pasiunea","entuziasmul","emotia",
    "sentimentul","afectiunea","dragostea","iubirea","adorarea","cultul",
    "veneratia","respectul","onorul","demnitatea","mandria","modestia",
    "smerenia","umilinta","aroganta","egoismul","altruismul","generozitatea",
    "bunatatea","amabilitatea","politetea","cuviinta","decenta","corectitudinea",
    "integritatea","onestitatea","sinceritatea","adevarul","sincerul",
    "transparenta","claritatea","limpiditatea","puritatea","impuritatea",
    "poluarea","contaminarea","infectia","boala","suferinta","durerea",
    "tristetea","melancolia","deprimarea","anxietatea","frica","teroarea",
    "panica","spaima","ingrijorarea","nelinistea","agitatia","furia",
    "mania","ira","ciuda","invidia","gelozia","ura","dusmania","conflicul",
    "razboiul","batalia","lupta","competitia","rivalitatea","opozitia",
    "rezistenta","opunerea","defensiva","protectia","apararea","paza",
    "veghea","atentia","vigilenta","prudenta","precautia","grija",
    "ingrijirea","ocrotirea","salvarea","ajutorul","sprijinul","sustinerea",
    "asistenta","serviciul","beneficiul","folosul","utilitatea",
    "eficienta","eficacitatea","performanta","productivitatea","capacitatea",
    "abilitatea","indemanarea","priceperea","mestesugul","arta","meseria",
    "profesia","ocupatia","activitatea","munca","lucrul","efortul",
    "stradania","staruinta","perseverenta","tenacitatea","hotararea",
    "determinarea","vointa","ambitia","aspiratia","dorinta","visul",
    "speranta","asteptarea","previziunea","prognosticul","predictia",
    "tendinta","directia","orientarea","pozitia","locatia","situatia",
    "contextul","mediul","ambientul","atmosfera","clima","vremea",
    "meteo","sezonul","perioada","etapa","faza","stadiul","nivelul",
    "gradul","scara","treapta","panta","povarnisul","dealul","valea"
};

// =========================================================
// CHECKSUM VERSIONS
// =========================================================
enum class ChecksumVersion {
    NONE,
    V1_9BIT,   // 1 word, 9 bits
    V2_10BIT   // 2 words, 10 bits each
};

// =========================================================
// PROFILE METADATA
// =========================================================
struct ProfileMetadata {
    std::string name;
    std::string description;
    std::string author;
    std::string version;
    std::string created_date;
    ChecksumVersion checksum_ver;
    bool strict_hex;
    std::vector<int> allowed_lengths;
    std::string algorithm;
    std::unordered_map<std::string, std::string> parameters;
};

// =========================================================
// ENHANCED AKM LOGIC
// =========================================================
class AkmLogicEnhanced {
private:
    std::unordered_map<std::string, std::string> customHex;
    std::unordered_map<std::string, AkmRule> specialRules;
    std::vector<std::string> baseWords;
    bool isStrict = false;
    ChecksumVersion checksumVer = ChecksumVersion::V1_9BIT;
    ProfileMetadata metadata;
    
    // Word to index mapping
    std::unordered_map<std::string, int> wordIndex;
    
    bool isValidHex(const std::string& s) {
        if (s.empty()) return false;
        for (char c : s) {
            if (!isxdigit((unsigned char)c)) return false;
        }
        return true;
    }
    
    std::string cleanString(const std::string& input) {
        std::string s = input;
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c){ 
            return std::isspace(c) || c == '\r' || c == '\n'; 
        }), s.end());
        return s;
    }

public:
    AkmLogicEnhanced() {
        baseWords = AKM_BASE_WORDS;
        // Build word index
        for (size_t i = 0; i < baseWords.size(); i++) {
            wordIndex[baseWords[i]] = (int)i;
        }
    }
    
    // Load profile from file
    bool loadProfile(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "[AKM] Cannot open profile: " << filePath << "\n";
            return false;
        }
        
        // Reset state
        customHex.clear();
        specialRules.clear();
        isStrict = false;
        
        std::string line;
        bool inMetadata = false;
        bool inWords = false;
        bool inRules = false;
        
        while (std::getline(file, line)) {
            std::string cleanL = cleanString(line);
            if (cleanL.empty()) continue;
            if (cleanL[0] == '#') {
                // Check for header markers
                if (cleanL.find("#METADATA") != std::string::npos) {
                    inMetadata = true;
                    inWords = false;
                    inRules = false;
                    continue;
                }
                if (cleanL.find("#WORDS") != std::string::npos) {
                    inMetadata = false;
                    inWords = true;
                    inRules = false;
                    continue;
                }
                if (cleanL.find("#RULES") != std::string::npos) {
                    inMetadata = false;
                    inWords = false;
                    inRules = true;
                    continue;
                }
                if (cleanL.find("#STRICT_HEX") != std::string::npos) {
                    isStrict = true;
                    continue;
                }
                continue;
            }
            
            if (inMetadata) {
                // Parse metadata: key=value
                size_t eqPos = cleanL.find('=');
                if (eqPos != std::string::npos) {
                    std::string key = cleanL.substr(0, eqPos);
                    std::string val = cleanL.substr(eqPos + 1);
                    if (key == "name") metadata.name = val;
                    else if (key == "description") metadata.description = val;
                    else if (key == "version") metadata.version = val;
                    else if (key == "author") metadata.author = val;
                    else if (key == "created") metadata.created_date = val;
                    else if (key == "checksum") {
                        if (val == "v1") checksumVer = ChecksumVersion::V1_9BIT;
                        else if (val == "v2") checksumVer = ChecksumVersion::V2_10BIT;
                        else if (val == "none") checksumVer = ChecksumVersion::NONE;
                    }
                    else if (key == "strict") isStrict = (val == "true");
                }
            }
            else if (inWords) {
                // Parse word=hex mappings
                size_t eqPos = cleanL.find('=');
                if (eqPos != std::string::npos) {
                    std::string word = cleanL.substr(0, eqPos);
                    std::string hex = cleanL.substr(eqPos + 1);
                    
                    if (isStrict && !isValidHex(hex)) {
                        std::cerr << "[AKM] Invalid hex for word '" << word << "': " << hex << "\n";
                        continue;
                    }
                    
                    std::transform(hex.begin(), hex.end(), hex.begin(), ::tolower);
                    customHex[word] = hex;
                } else {
                    baseWords.push_back(cleanL);
                    wordIndex[cleanL] = (int)baseWords.size() - 1;
                }
            }
            else if (inRules) {
                // Parse special rules: word|fixed8|fixed1|fixed2|fixed3|pad|repeat
                std::stringstream ss(cleanL);
                std::string word, fixed8, fixed1, fixed2, fixed3, pad, repeat;
                
                std::getline(ss, word, '|');
                std::getline(ss, fixed8, '|');
                std::getline(ss, fixed1, '|');
                std::getline(ss, fixed2, '|');
                std::getline(ss, fixed3, '|');
                std::getline(ss, pad, '|');
                std::getline(ss, repeat, '|');
                
                AkmRule rule;
                rule.fixed8 = fixed8;
                rule.fixed1 = fixed1;
                rule.fixed2 = fixed2;
                rule.fixed3 = fixed3;
                rule.pad_nibble = pad;
                rule.repeat_last = (repeat == "true" || repeat == "1");
                specialRules[word] = rule;
            }
        }
        
        file.close();
        
        std::cout << "[AKM] Loaded profile: " << metadata.name << " v" << metadata.version << "\n";
        std::cout << "[AKM] Author: " << metadata.author << "\n";
        std::cout << "[AKM] Words: " << baseWords.size() << ", Custom hex: " << customHex.size() 
                  << ", Rules: " << specialRules.size() << "\n";
        
        return true;
    }
    
    // Generate token for word
    std::string getToken8(const std::string& word) {
        // Check custom hex first
        auto it = customHex.find(word);
        if (it != customHex.end()) {
            std::string hex = it->second;
            // Pad to 8 hex digits
            while (hex.length() < 8) hex = "0" + hex;
            if (hex.length() > 8) hex = hex.substr(0, 8);
            return hex;
        }
        
        // Check special rules
        auto ruleIt = specialRules.find(word);
        if (ruleIt != specialRules.end()) {
            const AkmRule& rule = ruleIt->second;
            if (!rule.fixed8.empty()) return rule.fixed8;
            
            // Build from fixed parts
            std::string token;
            if (!rule.fixed1.empty()) token += rule.fixed1;
            if (!rule.fixed2.empty()) token += rule.fixed2;
            if (!rule.fixed3.empty()) token += rule.fixed3;
            if (!rule.pad_nibble.empty()) token += rule.pad_nibble;
            
            while (token.length() < 8) token += "0";
            if (token.length() > 8) token = token.substr(0, 8);
            return token;
        }
        
        // Generate from SHA256
        if (isStrict) return "";
        
        uint8_t hash[SHA256_DIGEST_LENGTH];
        SHA256((const uint8_t*)word.c_str(), word.length(), hash);
        
        char buf[17];
        sprintf(buf, "%02x%02x%02x%02x%02x%02x%02x%02x",
                hash[0], hash[1], hash[2], hash[3],
                hash[4], hash[5], hash[6], hash[7]);
        return std::string(buf);
    }
    
    // Convert phrase to private key
    std::vector<uint8_t> phraseToKey(const std::vector<std::string>& words) {
        std::string bigHex = "";
        for (const auto& w : words) {
            bigHex += getToken8(w);
            if (bigHex.length() >= 64) break;
        }
        
        while (bigHex.length() < 64) bigHex = "0" + bigHex;
        if (bigHex.length() > 64) bigHex = bigHex.substr(bigHex.length() - 64);
        
        std::vector<uint8_t> key;
        for (size_t i = 0; i < 64; i += 2) {
            std::string byteStr = bigHex.substr(i, 2);
            key.push_back((uint8_t)strtol(byteStr.c_str(), nullptr, 16));
        }
        return key;
    }
    
    // Calculate checksum V1 (9-bit)
    std::string calculateChecksumV1(const std::vector<std::string>& coreWords) {
        auto key = phraseToKey(coreWords);
        
        // Use first 9 bits of key hash
        uint8_t hash[SHA256_DIGEST_LENGTH];
        SHA256(key.data(), key.size(), hash);
        
        int idx = ((hash[0] << 1) | (hash[1] >> 7)) & 0x1FF; // 9 bits
        idx = idx % baseWords.size();
        
        return baseWords[idx];
    }
    
    // Calculate checksum V2 (10-bit x 2)
    std::vector<std::string> calculateChecksumV2(const std::vector<std::string>& coreWords) {
        auto key = phraseToKey(coreWords);
        
        uint8_t hash[SHA256_DIGEST_LENGTH];
        SHA256(key.data(), key.size(), hash);
        
        int idx1 = ((hash[0] << 2) | (hash[1] >> 6)) & 0x3FF; // 10 bits
        int idx2 = ((hash[2] << 2) | (hash[3] >> 6)) & 0x3FF; // 10 bits
        
        idx1 = idx1 % baseWords.size();
        idx2 = idx2 % baseWords.size();
        
        return {baseWords[idx1], baseWords[idx2]};
    }
    
    // Verify checksum
    bool verifyChecksum(const std::vector<std::string>& phrase) {
        if (phrase.empty()) return false;
        
        if (checksumVer == ChecksumVersion::V1_9BIT) {
            std::vector<std::string> core(phrase.begin(), phrase.end() - 1);
            std::string expected = calculateChecksumV1(core);
            return phrase.back() == expected;
        }
        else if (checksumVer == ChecksumVersion::V2_10BIT) {
            if (phrase.size() < 3) return false;
            std::vector<std::string> core(phrase.begin(), phrase.end() - 2);
            auto expected = calculateChecksumV2(core);
            return phrase[phrase.size()-2] == expected[0] && phrase.back() == expected[1];
        }
        
        return true; // NONE
    }
    
    // Getters
    const std::vector<std::string>& getWordList() const { return baseWords; }
    size_t getWordCount() const { return baseWords.size(); }
    bool hasWord(const std::string& word) const { return wordIndex.count(word) > 0; }
    int getWordIndex(const std::string& word) const {
        auto it = wordIndex.find(word);
        return it != wordIndex.end() ? it->second : -1;
    }
    const ProfileMetadata& getMetadata() const { return metadata; }
    ChecksumVersion getChecksumVersion() const { return checksumVer; }
};

// =========================================================
// RULE STRUCT (for compatibility)
// =========================================================
struct AkmRule {
    std::string fixed8;
    std::string fixed1;
    std::string fixed2;
    std::string fixed3;
    std::string pad_nibble;
    bool repeat_last = false;
};
