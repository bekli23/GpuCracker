# GpuCracker v50.0 - General Documentation

## Table of Contents

1. [Introduction](#introduction)
2. [Architecture Overview](#architecture-overview)
3. [Getting Started](#getting-started)
4. [Core Concepts](#core-concepts)
5. [Security Considerations](#security-considerations)
6. [Performance Tuning](#performance-tuning)
7. [Troubleshooting](#troubleshooting)
8. [Contributing](#contributing)
9. [License](#license)

---

## Introduction

**GpuCracker** is a high-performance cryptocurrency recovery and analysis tool designed for security researchers, penetration testers, and legitimate recovery scenarios. It leverages GPU acceleration to perform billions of cryptographic operations per second.

### Key Capabilities

- **Seed Recovery**: BIP39 mnemonic phrase recovery
- **Private Key Search**: Brute-force and intelligent keyspace traversal
- **ECDLP Solving**: BSGS and Pollard's Rho algorithms
- **Address Analysis**: Bloom filter-based mass address checking
- **Multi-Currency**: Support for 100+ cryptocurrencies

### Target Audience

- Security researchers studying wallet implementations
- Penetration testers evaluating wallet security
- Users recovering their own lost wallets
- Cryptocurrency forensic analysts

---

## Architecture Overview

### System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     User Interface                          │
│              (CLI / Web Dashboard / API)                    │
└───────────────────────┬─────────────────────────────────────┘
                        │
┌───────────────────────▼─────────────────────────────────────┐
│                  Core Engine                                │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │  BSGS    │  │   Rho    │  │  AKM     │  │ Mnemonic │  │
│  │  Engine  │  │  Engine  │  │  Engine  │  │  Engine  │  │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘  │
└───────────────────────┬─────────────────────────────────────┘
                        │
┌───────────────────────▼─────────────────────────────────────┐
│              GPU Abstraction Layer                          │
│     ┌──────────┐  ┌──────────┐  ┌──────────┐              │
│     │  CUDA    │  │  OpenCL  │  │  Vulkan  │              │
│     │(NVIDIA)  │  │(AMD/Intel│  │(Universal│              │
│     └──────────┘  └──────────┘  └──────────┘              │
└───────────────────────┬─────────────────────────────────────┘
                        │
┌───────────────────────▼─────────────────────────────────────┐
│                 Cryptographic Primitives                    │
│     secp256k1    │    SHA256    │    RIPEMD160    │        │
│     PBKDF2       │    HMAC      │    Base58       │        │
└─────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Purpose |
|-----------|---------|
| **Core Engine** | Mode-specific logic and algorithms |
| **GPU Layer** | Hardware abstraction for parallel computing |
| **Bloom Filter** | Ultra-fast set membership testing |
| **Key Derivation** | BIP32/39/44/49/84 hierarchy |
| **Address Encoding** | Base58, Bech32, Bech32m formats |

---

## Getting Started

### Quick Start Checklist

- [ ] Download/Build GpuCracker for your platform
- [ ] Verify GPU drivers (NVIDIA: 550+, AMD: ROCm 5.0+)
- [ ] Prepare target data (addresses, public keys)
- [ ] Configure bloom filters for target addresses
- [ ] Run initial benchmark: `./GpuCracker --benchmark`
- [ ] Start with small test range
- [ ] Scale up based on results

### First Run

```bash
# Check system
./GpuCracker --system-info

# List available hardware
./GpuCracker --list-devices

# Run benchmark
./GpuCracker --benchmark --duration 60

# Test with demo
./GpuCracker --mode check --demo
```

---

## Core Concepts

### Bloom Filters

Bloom filters provide probabilistic set membership testing:

- **Advantage**: 1000x faster than hash tables for large datasets
- **Trade-off**: Small false positive rate (~0.1%)
- **Memory**: ~10 bits per element

```
Without Bloom:  O(n) lookup,  high memory
With Bloom:     O(1) lookup,  low memory
```

### BSGS Algorithm

Baby-Step Giant-Step for solving ECDLP:

```
Given: Q = d × G (public key Q, generator G, unknown d)

Baby Steps:  Store j×G for j = 0 to m-1
Giant Steps: Compute Q - i×m×G
Match:       d = i×m + j when Q - i×m×G = j×G

Time:   O(√n)
Memory: O(√n)
```

### Pollard's Rho

Memory-efficient alternative to BSGS:

```
Uses Floyd's cycle finding with pseudorandom walk
No memory requirements for baby steps
Parallelizable with distinguished points

Time:   O(√n) expected
Memory: O(1)
```

### Hybrid Strategy

Automatically selects optimal algorithm:

```
if (range <= 2^60 && memory_available):
    use BSGS (fast, deterministic)
elif (range <= 2^80 && memory_available):
    use Bloom BSGS (memory efficient)
else:
    use Pollard's Rho (unlimited range)
```

---

## Security Considerations

### Responsible Use

⚠️ **This tool is for legitimate security research and personal recovery only.**

#### Acceptable Uses
- Recovering your own lost wallets
- Security auditing your own systems
- Academic research on cryptographic algorithms
- Penetration testing with explicit authorization

#### Prohibited Uses
- Stealing funds from others
- Unauthorized access to systems
- Any illegal activity

### Data Security

1. **Isolate Sensitive Data**
   ```bash
   # Use dedicated secure environment
   chmod 700 /secure/gpucracker
   ./GpuCracker --secure-mode --data-dir /secure/gpucracker
   ```

2. **Secure Memory**
   ```bash
   # Lock pages in RAM (prevent swap)
   ./GpuCracker --lock-memory
   
   # Clear memory on exit
   ./GpuCracker --secure-exit
   ```

3. **Encrypted Results**
   ```bash
   # Encrypt found keys
   ./GpuCracker --encrypt-results --key-file /secure/key.pem
   ```

### Best Practices

- Never run on shared systems with sensitive data
- Use air-gapped machines for high-value targets
- Verify results on isolated systems before use
- Keep software updated for security patches

---

## Performance Tuning

### GPU Optimization

#### NVIDIA (CUDA)

```bash
# Persistence mode
sudo nvidia-smi -pm 1

# Performance mode
sudo nvidia-smi -ac 877,1530  # Memory, Graphics clocks

# Power limit (adjust for your GPU)
sudo nvidia-smi -pl 300

# Compute-only mode (Tesla/Quadro)
sudo nvidia-smi -c EXCLUSIVE_THREAD
```

#### AMD (OpenCL)

```bash
# Performance mode
export GPU_PERF_MODE=high

# Disable power saving
echo "high" | sudo tee /sys/class/drm/card0/device/power_dpm_force_performance_level
```

### CPU Optimization

```bash
# CPU governor
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Disable hyperthreading (for some workloads)
echo 0 | sudo tee /sys/devices/system/cpu/cpu{1,3,5,7,9,11,13,15}/online

# Huge pages
echo 1024 | sudo tee /proc/sys/vm/nr_hugepages
```

### Algorithm Tuning

#### BSGS Parameters

| Range | Optimal m | Memory Required |
|-------|-----------|-----------------|
| 2^40 | 2^20 | 22 GB |
| 2^50 | 2^25 | 720 GB |
| 2^60 | 2^30 | 23 TB |

Use `--bsgs-bloom` to reduce memory by 10x.

#### Rho Parameters

| Distinguished Bits | DP Rate | Memory/Instance |
|-------------------|---------|-----------------|
| 20 | 1/1M | 1 MB |
| 24 | 1/16M | 16 MB |
| 28 | 1/256M | 256 MB |

Higher bits = less memory, slower collision detection.

---

## Troubleshooting

### Common Issues

#### Out of Memory

```bash
# Reduce batch size
./GpuCracker --batch-size 1000

# Use bloom filter
./GpuCracker --bsgs-bloom

# Reduce threads
./GpuCracker --threads 8
```

#### GPU Not Detected

```bash
# Check drivers
nvidia-smi  # NVIDIA
clinfo      # OpenCL

# Check permissions
sudo usermod -aG video $USER

# Verify CUDA
nvcc --version
```

#### Slow Performance

```bash
# Check thermal throttling
nvidia-smi dmon

# Verify GPU utilization
nvidia-smi pmon

# Profile application
./GpuCracker --profile --profile-output perf.txt
```

### Debug Mode

```bash
# Verbose output
./GpuCracker --verbose --debug

# GPU debug
./GpuCracker --cuda-debug --opencl-debug

# Memory debug
./GpuCracker --memory-debug
```

---

## Contributing

### Development Setup

```bash
# Clone repository
git clone https://github.com/yourusername/GpuCracker.git
cd GpuCracker

# Install dependencies (see build guides)

# Create feature branch
git checkout -b feature/my-feature

# Build and test
make clean && make -j$(nproc)
./GpuCracker --self-test
```

### Code Style

- C++17 standard
- 4 spaces indentation
- Maximum line length: 120 characters
- Use `snake_case` for functions/variables
- Use `PascalCase` for classes

### Testing

```bash
# Run all tests
./GpuCracker --test-all

# Run specific test suite
./GpuCracker --test-suite crypto
./GpuCracker --test-suite gpu
./GpuCracker --test-suite modes
```

### Pull Request Process

1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Create Pull Request

---

## License

This project is licensed under the MIT License - see LICENSE file for details.

### Third-Party Licenses

- secp256k1: MIT License
- Boost: Boost Software License
- OpenSSL: Apache License 2.0
- CUDA: NVIDIA Software License

### Disclaimer

This software is provided "as is", without warranty of any kind. Use at your own risk. The authors assume no liability for misuse or damages arising from use of this software.

---

## Additional Resources

- `README_BUILD_LINUX.md` - Linux build instructions
- `README_BUILD_WIN.md` - Windows build instructions
- `README_MODE_*.md` - Mode-specific documentation
- `README_COMMAND_*.md` - Command reference

## Support

- GitHub Issues: https://github.com/yourusername/GpuCracker/issues
- Documentation: https://gpucracker.readthedocs.io
- Community Discord: [Invite Link]

---

**Version**: GpuCracker v50.0  
**Last Updated**: 2026-03-04  
**Maintained by**: GpuCracker Team
