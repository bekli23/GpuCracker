#pragma once

#include <string>
#include <vector>
#include <map>
#include <algorithm>

// SLIP-44 Coin Types Database - 100+ coins supported
// https://github.com/satoshilabs/slips/blob/master/slip-0044.md

struct CoinInfo {
    std::string symbol;      // BTC, ETH, etc.
    std::string name;        // Bitcoin, Ethereum, etc.
    int slip44;              // SLIP-44 coin type index
    bool hasLegacy;          // Supports P2PKH (1...)
    bool hasSegwit;          // Supports P2SH (3...) and/or Bech32 (bc1...)
    bool isEthereumBased;    // ETH-based (uses ETH address format)
    std::string bech32Hrp;   // Bech32 human readable part (e.g., "bc" for BTC)
};

class CoinDatabase {
public:
    // Get complete coin database
    static std::vector<CoinInfo> getAllCoins() {
        return {
            // Major cryptocurrencies
            {"btc", "Bitcoin", 0, true, true, false, "bc"},
            {"eth", "Ethereum", 60, false, false, true, ""},
            {"ltc", "Litecoin", 2, true, true, false, "ltc"},
            {"doge", "Dogecoin", 3, true, false, false, ""},
            {"dash", "Dash", 5, true, false, false, ""},
            {"bch", "Bitcoin Cash", 145, true, true, false, ""},
            {"bsv", "Bitcoin SV", 236, true, false, false, ""},
            {"btg", "Bitcoin Gold", 156, true, true, false, ""},
            {"zec", "Zcash", 133, true, false, false, ""},
            
            // Layer 2 and scaling solutions
            {"tbtc", "Bitcoin Testnet", 1, true, true, false, "tb"},
            {"rbtc", "RSK", 137, false, false, true, ""},
            {"xrp", "Ripple", 144, false, false, false, ""},
            {"qtum", "Qtum", 2301, true, true, false, "qc"},
            {"xem", "NEM", 43, false, false, false, ""},
            {"vet", "VeChain", 818, false, false, true, ""},
            
            // Privacy coins
            {"xmr", "Monero", 128, false, false, false, ""},
            {"grs", "Groestlcoin", 17, true, true, false, "grs"},
            {"dgb", "DigiByte", 20, true, true, false, "dgb"},
            {"nav", "NavCoin", 130, true, true, false, "nav"},
            {"pivx", "PIVX", 119, true, false, false, ""},
            {"via", "Viacoin", 14, true, true, false, "via"},
            {"xvg", "Verge", 77, true, false, false, ""},
            {"zen", "Horizen", 121, true, false, false, ""},
            {"kmd", "Komodo", 141, true, false, false, ""},
            {"zcl", "Zclassic", 147, true, false, false, ""},
            {"zlla", "Zilliqa", 119, false, false, true, ""},
            
            // Smart contract platforms
            {"etc", "Ethereum Classic", 61, false, false, true, ""},
            {"avax", "Avalanche", 9000, false, false, true, ""},
            {"matic", "Polygon", 966, false, false, true, ""},
            {"bnb", "Binance Chain", 714, false, false, true, ""},
            {"ftm", "Fantom", 1007, false, false, true, ""},
            {"ht", "Huobi Token", 555, false, false, true, ""},
            {"okt", "OKExChain", 996, false, false, true, ""},
            {"cro", "Cronos", 394, false, false, true, ""},
            {"one", "Harmony", 1023, false, false, true, ""},
            {"mob", "MobileCoin", 127, false, false, false, ""},
            {"stx", "Stacks", 5757, false, false, false, ""},
            {"rsk", "Rootstock", 137, false, false, true, ""},
            {"wan", "Wanchain", 5718350, false, false, true, ""},
            {"poa", "POA Network", 178, false, false, true, ""},
            {"exp", "Expanse", 40, false, false, true, ""},
            {"music", "Musicoin", 184, false, false, true, ""},
            {"ubq", "Ubiq", 108, false, false, true, ""},
            {"ella", "Ellaism", 163, false, false, true, ""},
            {"etsc", "Ethereum Social", 1128, false, false, true, ""},
            {"mix", "Mix Blockchain", 76, false, false, true, ""},
            
            // DeFi tokens (as ETH-based)
            {"aave", "Aave", 60, false, false, true, ""},
            {"uni", "Uniswap", 60, false, false, true, ""},
            {"link", "Chainlink", 60, false, false, true, ""},
            {"mkr", "Maker", 60, false, false, true, ""},
            {"comp", "Compound", 60, false, false, true, ""},
            {"yfi", "Yearn Finance", 60, false, false, true, ""},
            {"snx", "Synthetix", 60, false, false, true, ""},
            {"sushi", "SushiSwap", 60, false, false, true, ""},
            {"crv", "Curve", 60, false, false, true, ""},
            {"1inch", "1inch", 60, false, false, true, ""},
            {"bal", "Balancer", 60, false, false, true, ""},
            {"knc", "Kyber Network", 60, false, false, true, ""},
            {"lrc", "Loopring", 60, false, false, true, ""},
            {"bat", "Basic Attention", 60, false, false, true, ""},
            {"ren", "Ren", 60, false, false, true, ""},
            {"omg", "OMG Network", 60, false, false, true, ""},
            {"zrx", "0x", 60, false, false, true, ""},
            {"rep", "Augur", 60, false, false, true, ""},
            {"enj", "Enjin", 60, false, false, true, ""},
            {"mana", "Decentraland", 60, false, false, true, ""},
            {"sand", "The Sandbox", 60, false, false, true, ""},
            {"axs", "Axie Infinity", 60, false, false, true, ""},
            {"chz", "Chiliz", 60, false, false, true, ""},
            {"shib", "Shiba Inu", 60, false, false, true, ""},
            {"dogelon", "Dogelon Mars", 60, false, false, true, ""},
            {"safemoon", "SafeMoon", 60, false, false, true, ""},
            
            // Stablecoins
            {"usdt", "Tether", 60, false, false, true, ""},
            {"usdc", "USD Coin", 60, false, false, true, ""},
            {"busd", "Binance USD", 60, false, false, true, ""},
            {"dai", "Dai", 60, false, false, true, ""},
            {"tusd", "TrueUSD", 60, false, false, true, ""},
            {"usdp", "Pax Dollar", 60, false, false, true, ""},
            {"gusd", "Gemini Dollar", 60, false, false, true, ""},
            {"frax", "Frax", 60, false, false, true, ""},
            
            // NFT/Metaverse
            {"flow", "Flow", 639, false, false, false, ""},
            {"theta", "Theta", 500, false, false, true, ""},
            {"tezos", "Tezos", 1729, false, false, false, ""},
            {"eos", "EOS", 194, false, false, false, ""},
            {"wax", "WAX", 14001, false, false, false, ""},
            {"enj", "Enjin Coin", 60, false, false, true, ""},
            
            // DAG coins
            {"iota", "IOTA", 4218, false, false, false, ""},
            {"nano", "Nano", 165, false, false, false, ""},
            
            // Exchange tokens
            {"ftt", "FTX Token", 60, false, false, true, ""},
            {"ht", "Huobi Token", 555, false, false, true, ""},
            {"leo", "UNUS SED LEO", 60, false, false, true, ""},
            {"cetus", "Cetus", 60, false, false, true, ""},
            
            // Storage/Compute
            {"fil", "Filecoin", 461, false, false, false, ""},
            {"storj", "Storj", 60, false, false, true, ""},
            {"sc", "Siacoin", 199, false, false, false, ""},
            {"ar", "Arweave", 9999999, false, false, false, ""},
            
            // Oracle
            {"link", "Chainlink", 60, false, false, true, ""},
            {"band", "Band Protocol", 494, false, false, true, ""},
            {"dia", "DIA", 60, false, false, true, ""},
            
            // Interoperability
            {"atom", "Cosmos", 118, false, false, false, ""},
            {"dot", "Polkadot", 354, false, false, false, ""},
            {"ksm", "Kusama", 434, false, false, false, ""},
            {"near", "NEAR Protocol", 397, false, false, false, ""},
            {"sol", "Solana", 501, false, false, false, ""},
            {"ada", "Cardano", 1815, false, false, false, ""},
            {"algo", "Algorand", 283, false, false, false, ""},
            
            // Gaming
            {"mana", "Decentraland", 60, false, false, true, ""},
            {"sand", "The Sandbox", 60, false, false, true, ""},
            {"axs", "Axie Infinity", 60, false, false, true, ""},
            {"enj", "Enjin Coin", 60, false, false, true, ""},
            {"gala", "Gala", 60, false, false, true, ""},
            {"tlm", "Alien Worlds", 60, false, false, true, ""},
            {"mbox", "Mobox", 60, false, false, true, ""},
            {"ygg", "Yield Guild", 60, false, false, true, ""},
            
            // Meme coins (ETH-based)
            {"shib", "Shiba Inu", 60, false, false, true, ""},
            {"elon", "Dogelon Mars", 60, false, false, true, ""},
            {"saitama", "Saitama", 60, false, false, true, ""},
            {"floki", "Floki Inu", 60, false, false, true, ""},
            {"akita", "Akita Inu", 60, false, false, true, ""},
            {"hokk", "Hokkaidu Inu", 60, false, false, true, ""},
            {"kishu", "Kishu Inu", 60, false, false, true, ""},
            {"sanshu", "Sanshu Inu", 60, false, false, true, ""},
            
            // Other major coins
            {"trx", "TRON", 195, false, false, true, ""},
            {"xlm", "Stellar", 148, false, false, false, ""},
            {"btt", "BitTorrent", 60, false, false, true, ""},
            {"wbtc", "Wrapped Bitcoin", 60, false, false, true, ""},
            {"renbtc", "renBTC", 60, false, false, true, ""},
            {"hbtc", "Huobi BTC", 60, false, false, true, ""},
            {"ibtc", "imBTC", 60, false, false, true, ""},
            {"sbtc", "sBTC", 60, false, false, true, ""},
        };
    }
    
    // Get coin info by symbol
    static CoinInfo getCoinBySymbol(const std::string& symbol) {
        std::string sym = symbol;
        std::transform(sym.begin(), sym.end(), sym.begin(), ::tolower);
        
        auto coins = getAllCoins();
        for (const auto& coin : coins) {
            if (coin.symbol == sym) {
                return coin;
            }
        }
        // Return default (BTC) if not found
        return {"btc", "Bitcoin", 0, true, true, false, "bc"};
    }
    
    // Check if coin exists
    static bool hasCoin(const std::string& symbol) {
        std::string sym = symbol;
        std::transform(sym.begin(), sym.end(), sym.begin(), ::tolower);
        
        auto coins = getAllCoins();
        for (const auto& coin : coins) {
            if (coin.symbol == sym) {
                return true;
            }
        }
        return false;
    }
    
    // Get SLIP-44 index by symbol
    static int getSlip44Index(const std::string& symbol) {
        return getCoinBySymbol(symbol).slip44;
    }
    
    // Get all symbols as list
    static std::vector<std::string> getAllSymbols() {
        std::vector<std::string> symbols;
        auto coins = getAllCoins();
        for (const auto& coin : coins) {
            symbols.push_back(coin.symbol);
        }
        return symbols;
    }
    
    // Parse comma-separated coin list
    static std::vector<std::string> parseCoinList(const std::string& coinListStr) {
        std::vector<std::string> result;
        std::stringstream ss(coinListStr);
        std::string item;
        
        while (std::getline(ss, item, ',')) {
            // Trim whitespace
            item.erase(0, item.find_first_not_of(" \t\r\n"));
            item.erase(item.find_last_not_of(" \t\r\n") + 1);
            
            if (!item.empty() && hasCoin(item)) {
                result.push_back(item);
            }
        }
        
        return result;
    }
};
