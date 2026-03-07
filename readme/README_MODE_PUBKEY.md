# PUBKEY Mode - Public Key Operations

## Overview

The **pubkey** mode performs operations on public keys including derivation, conversion, and analysis. It's useful for:

- Converting between compressed/uncompressed formats
- Deriving addresses from public keys
- Batch processing public keys
- Vanity address generation from public keys

## Supported Operations

### Format Conversions
- Compressed ↔ Uncompressed
- Hex ↔ Base64
- Raw bytes representation

### Address Derivation
- P2PKH (Legacy)
- P2SH (SegWit wrapped)
- Bech32 (Native SegWit)
- Bech32m (Taproot)

### Analysis
- Point validation
- Curve membership
- Batch verification

## Basic Usage

```bash
# Convert compressed to uncompressed
./GpuCracker --mode pubkey --pubkey 0279BE... --decompress

# Derive all address types
./GpuCracker --mode pubkey --pubkey 0279BE... --all-addresses

# Batch process
./GpuCracker --mode pubkey --file pubkeys.txt --derive-addresses
```

## See Also

- `README_COMMAND_PUBKEY.md` - 50+ command examples
- `README_MODE_CHECK.md` - Validate public keys
