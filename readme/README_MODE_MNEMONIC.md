# MNEMONIC Mode - BIP39 Recovery

## Overview

The **mnemonic** mode performs brute force and dictionary attacks on BIP39 mnemonic phrases to recover cryptocurrency wallets. It supports all standard BIP39 word counts (12, 15, 18, 21, 24 words) and can check against multiple derivation paths and coins simultaneously.

## Technical Details

### BIP39 Specification
- **Word Lists**: 2048 words per language
- **Checksum**: Last word contains checksum bits
- **Entropy**: 128-256 bits (12-24 words)
- **Seed Generation**: PBKDF2-HMAC-SHA512 with "mnemonic" salt

### Supported Languages
- English (default)
- Chinese (Simplified & Traditional)
- Japanese
- Korean
- Spanish
- French
- Italian
- Portuguese
- Czech

### Derivation Paths
- BIP44 (Legacy): `m/44'/coin'/account'/change/index`
- BIP49 (SegWit): `m/49'/coin'/account'/change/index`
- BIP84 (Native SegWit): `m/84'/coin'/change/index`
- Custom paths supported

## Basic Usage

```bash
# Basic 12-word mnemonic recovery
./GpuCracker --mode mnemonic --words 12

# With specific path file
./GpuCracker --mode mnemonic --words 24 --path-file paths_bitcoin_standard.txt

# Multi-coin check
./GpuCracker --mode mnemonic --words 12 --multi-coin BTC,ETH,LTC
```

## Performance

| Words | Combinations | Time (1M/sec) | Memory |
|-------|--------------|---------------|--------|
| 12 | 2048^11 | ~1000 years | 2 GB |
| 18 | 2048^17 | Impractical | 4 GB |
| 24 | 2048^23 | Impossible | 8 GB |

**Note**: Full brute force is impractical. Use with:
- Partial word knowledge
- Schematic patterns
- Specific wordlist files

## Common Scenarios

### Scenario 1: Lost 1-2 words
```bash
# If you know 11 of 12 words
./GpuCracker --mode mnemonic --words 12 --akm-file known_words.txt
```

### Scenario 2: Wrong word order
```bash
# Permutation attack
./GpuCracker --mode mnemonic --words 12 --mnemonic-order schematic
```

### Scenario 3: Specific language
```bash
# Chinese mnemonics
./GpuCracker --mode mnemonic --words 12 --langs chinese_simplified
```

## Related Files

- `README_COMMAND_MNEMONIC.md` - 30+ example commands
- `bip39/english.txt` - Default wordlist
- `paths_bitcoin_standard.txt` - Standard derivation paths

## See Also

- `README_MODE_CHECK.md` - Verify recovered mnemonics
- `README_MODE_XPRV.md` - Convert mnemonics to xprv
