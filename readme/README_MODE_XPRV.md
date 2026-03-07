# XPRV Mode - Extended Key Operations

## Overview

The **xprv** mode handles extended private keys (xprv) and extended public keys (xpub) for Hierarchical Deterministic (HD) wallets as defined in BIP32.

## Key Features

### Operations
- Derive child keys
- Convert xprv to xpub
- Validate extended keys
- Export key hierarchies

### Supported Standards
- BIP32 - HD Wallets
- BIP44 - Multi-Account Hierarchy
- BIP49 - SegWit
- BIP84 - Native SegWit
- BIP86 - Taproot

## Basic Usage

```bash
# Derive child key
./GpuCracker --mode xprv --xprv xprv9s21ZrQH143K3GJpoapnV8SFfu1N7cPGq6Z6J... --derive "m/44'/0'/0'/0/0"

# Get xpub from xprv
./GpuCracker --mode xprv --xprv <XPRV> --get-xpub

# Validate xprv
./GpuCracker --mode xprv --xprv <XPRV> --validate
```

## See Also

- `README_COMMAND_XPRV.md` - 50+ command examples
- `README_MODE_MNEMONIC.md` - Generate xprv from mnemonic
- `README_MODE_CHECK.md` - Validate derived keys
