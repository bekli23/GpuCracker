# BRAINWALLET Mode - Brain Wallet Recovery

## Overview

The **brainwallet** mode recovers private keys generated from passwords or passphrases. Brain wallets derive keys directly from SHA256(password) or other KDFs.

## Supported Formats

### Brain Wallet Types
- **Standard**: SHA256(password)
- **Warp**: PBKDF2-based
- **Vanity**: Custom generators
- **Brainv2**: Enhanced with salt

### Dictionary Attacks
- Wordlist-based
- Pattern-based
- Hybrid approaches
- Custom generators

## Security Warning

⚠️ **Brain wallets are cryptographically insecure and should never be used for storing funds.** This mode is for recovery and research only.

## Basic Usage

```bash
# Basic brainwallet recovery
./GpuCracker --mode brainwallet --wordlist passwords.txt

# Specific brainwallet type
./GpuCracker --mode brainwallet --type warp --wordlist passwords.txt

# With target address
./GpuCracker --mode brainwallet --wordlist passwords.txt --target-address 1A1z...
```

## See Also

- `README_COMMAND_BRAINWALLET.md` - 50+ command examples
- `README_MODE_CHECK.md` - Verify recovered keys
