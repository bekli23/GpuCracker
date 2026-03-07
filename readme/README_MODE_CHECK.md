# CHECK Mode - Key Verification

## Overview

The **check** mode verifies and validates recovered private keys, public keys, addresses, and derived values. It's essential for:

- Confirming key correctness
- Converting between formats
- Validating mnemonic-to-key derivation
- Testing address generation

## Supported Verifications

### Private Key Formats
- Raw 32-byte hex
- WIF (Wallet Import Format)
- WIF-compressed
- Minikey

### Public Key Formats
- Uncompressed (65 bytes, 04 prefix)
- Compressed (33 bytes, 02/03 prefix)
- X/Y coordinates

### Address Formats
- P2PKH (Legacy, 1...)
- P2SH (SegWit, 3...)
- Bech32 (Native SegWit, bc1...)
- Bech32m (Taproot, bc1p...)

### Mnemonics
- Validate checksum
- Test all language wordlists
- Verify seed derivation
- Check BIP44/49/84 paths

## Basic Usage

```bash
# Check single private key
./GpuCracker --mode check --key L1aW4aubDFB7yfras2S1mN3bqg9nwySY8nkoLmJebSLD5BWv3ENZ

# Check mnemonic
./GpuCracker --mode check --mnemonic "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"

# Verify address belongs to key
./GpuCracker --mode check --key <WIF> --address 1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa
```

## See Also

- `README_COMMAND_CHECK.md` - 50+ command examples
- `README_MODE_MNEMONIC.md` - Mnemonic recovery
- `README_MODE_XPRV.md` - Extended key operations
