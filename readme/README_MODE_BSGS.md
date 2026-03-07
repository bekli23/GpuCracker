# BSGS Mode - Baby-Step Giant-Step Algorithm

## Overview

The **BSGS** (Baby-Step Giant-Step) mode implements the classic BSGS algorithm for solving the Elliptic Curve Discrete Logarithm Problem (ECDLP). It's the most efficient algorithm for searching known key ranges when you have the public key.

## Mathematical Background

### The ECDLP Problem
Given public key Q and generator G, find private key d such that:
```
Q = d × G
```

### BSGS Algorithm
1. **Baby Steps**: Precompute and store j×G for j = 0 to m-1
2. **Giant Steps**: For i = 0, 1, 2... compute Q - i×m×G
3. **Match**: When Q - i×m×G = j×G, then d = i×m + j

### Complexity
- **Time**: O(√n) where n is the search range
- **Memory**: O(√n) for baby steps table
- **Optimal m**: m = √n

## Features

### Multi-Target Search
- Process multiple public keys simultaneously
- Shared baby steps calculation
- Linear speedup with target count

### Baby Steps Caching
- Automatic cache save/load
- Named cache files
- Version checking

### GPU Acceleration
- CUDA support (NVIDIA)
- OpenCL support (AMD/Intel)
- 10-100x speedup

### Bloom Filter Optimization
- 10x memory reduction
- ~1% false positive rate
- For very large ranges

### Distributed Computing
- Cluster support with node partitioning
- Split giant steps across nodes
- Linear scalability

## Memory Requirements

| Range | Standard Memory | With Bloom |
|-------|-----------------|------------|
| 2^40 | 22 GB | 2.2 GB |
| 2^50 | 720 GB | 72 GB |
| 2^60 | 23 TB | 2.3 TB |

## Basic Usage

```bash
# Basic BSGS search
./GpuCracker --mode bsgs --bsgs-pub 0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798 --bsgs-range 50

# With GPU
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 60 --bsgs-gpu

# Multi-target
./GpuCracker --mode bsgs --bsgs-targets targets.txt --bsgs-range 50
```

## See Also

- `README_COMMAND_BSGS.md` - 50+ command examples
- `README_MODE_RHO.md` - Pollard's Rho algorithm
- `README_MODE_HYBRID.md` - Auto-select algorithm
