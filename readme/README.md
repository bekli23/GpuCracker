# GpuCracker v50.0 - Complete Documentation

## Overview

GpuCracker is a high-performance cryptocurrency security research tool designed for:
- **Mnemonic phrase recovery** (BIP39)
- **Private key recovery** from public keys (BSGS, Pollard's Rho)
- **Address checking** against blockchain data
- **Brain wallet** analysis
- **Extended public key** (xpub/xprv) derivation
- **Multi-coin support** (100+ cryptocurrencies)

## Architecture

```
GpuCracker v50.0
├── Core Engine (CPU/GPU Hybrid)
│   ├── CUDA Support (NVIDIA GPUs)
│   ├── OpenCL Support (AMD/Intel GPUs)
│   └── CPU Multi-threading (OpenMP)
│
├── Operating Modes (10 modes)
│   ├── mnemonic     - BIP39 mnemonic brute force
│   ├── akm          - Adaptive Key Management profiles
│   ├── check        - Address/seed verification
│   ├── scan         - Directory/file scanning
│   ├── xprv-mode    - Extended private key operations
│   ├── brainwallet  - Brain wallet analysis
│   ├── pubkey       - Public key to private key
│   ├── bsgs         - Baby Step Giant Step algorithm
│   ├── rho          - Pollard's Rho algorithm
│   └── hybrid       - Auto-select optimal algorithm
│
└── Blockchain Integration
    ├── Block Explorer API
    ├── Local blk*.dat reader
    └── Bloom filter support
```

## Quick Start

```bash
# Basic mnemonic recovery
./GpuCracker --mode mnemonic --words 12 --path-file paths_bitcoin_standard.txt

# BSGS with GPU acceleration
./GpuCracker --mode bsgs --bsgs-pub PUBKEY --bsgs-m 10000000 --bsgs-gpu

# Check addresses against bloom filter
./GpuCracker --mode check --bloom-keys addresses.blf --file seeds.txt
```

## System Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| CPU | 4 cores | 16+ cores |
| RAM | 8 GB | 64+ GB |
| GPU | Optional | NVIDIA RTX 3090+ |
| Storage | 10 GB | 1 TB (for blockchain) |
| OS | Linux/Windows | Linux x64 |

## GPU Acceleration

### Supported Algorithms
- **BSGS**: 50-100x speedup with CUDA
- **Pollard's Rho**: 20-50x speedup
- **Brain Wallet**: 10-30x speedup

### GPU Requirements
- NVIDIA: CUDA 11.0+ (Compute Capability 6.0+)
- AMD: OpenCL 2.0+
- Intel: OpenCL 1.2+

## Documentation Structure

```
readme/
├── README.md                          # This file
├── README_INSTALL.md                  # Installation guide
├── README_GPU_SETUP.md                # GPU configuration
├── README_TROUBLESHOOTING.md          # Common issues
│
├── README_MODE_MNEMONIC.md            # Mode: mnemonic
├── README_MODE_AKM.md                 # Mode: akm
├── README_MODE_CHECK.md               # Mode: check
├── README_MODE_SCAN.md                # Mode: scan
├── README_MODE_XPRV.md                # Mode: xprv-mode
├── README_MODE_BRAINWALLET.md         # Mode: brainwallet
├── README_MODE_PUBKEY.md              # Mode: pubkey
├── README_MODE_BSGS.md                # Mode: bsgs
├── README_MODE_RHO.md                 # Mode: rho
├── README_MODE_HYBRID.md              # Mode: hybrid
│
├── README_COMMAND_MNEMONIC.md         # 30+ mnemonic commands
├── README_COMMAND_AKM.md              # 30+ AKM commands
├── README_COMMAND_CHECK.md            # 30+ check commands
├── README_COMMAND_SCAN.md             # 30+ scan commands
├── README_COMMAND_XPRV.md             # 30+ xprv commands
├── README_COMMAND_BRAINWALLET.md      # 30+ brainwallet commands
├── README_COMMAND_PUBKEY.md           # 30+ pubkey commands
├── README_COMMAND_BSGS.md             # 30+ BSGS commands
├── README_COMMAND_RHO.md              # 30+ Rho commands
├── README_COMMAND_HYBRID.md           # 30+ hybrid commands
│
└── README_ADVANCED.md                 # Advanced techniques
```

## Performance Benchmarks

### CPU Only (AMD Ryzen 9 5950X)
| Mode | Keys/Second | Memory |
|------|-------------|--------|
| Mnemonic | 50,000 | 2 GB |
| BSGS (m=1M) | 100,000 | 1 GB |
| Pollard's Rho | 500,000 | 500 MB |

### With GPU (NVIDIA RTX 3090)
| Mode | Keys/Second | Speedup |
|------|-------------|---------|
| BSGS (m=10M) | 10,000,000,000 | 100x |
| Pollard's Rho | 5,000,000,000 | 50x |
| Brain Wallet | 500,000,000 | 30x |

## Security Notes

⚠️ **WARNING**: This tool is for legitimate security research only:
- Recovery of your own lost wallets
- Penetration testing with explicit permission
- Academic research on cryptographic algorithms
- Security audit of your own systems

**Illegal use** (unauthorized access to others' funds) is strictly prohibited and may result in criminal prosecution.

## Version History

### v50.0 (Current)
- Added GPU acceleration for BSGS and Pollard's Rho
- New Bloom filter address checking
- Multi-target BSGS batch search
- 100+ coin support
- Block explorer integration

### v50.0 (Current)
- Complete BSGS/Rho/Hybrid ECDLP implementation
- GPU acceleration for giant steps
- Multi-target batch search
- Bloom filter optimization
- Distributed cluster support

## Support

For issues and feature requests:
- Check `README_TROUBLESHOOTING.md`
- Review mode-specific documentation
- Examine example commands in `README_COMMAND_*.md`

## License

See LICENSE file for details.
