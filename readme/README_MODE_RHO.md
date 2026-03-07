# RHO Mode - Pollard's Rho Algorithm

## Overview

The **rho** mode implements Pollard's Rho algorithm for solving the Elliptic Curve Discrete Logarithm Problem (ECDLP). Unlike BSGS, Rho uses O(1) memory, making it suitable for extremely large ranges where memory is limited.

## Mathematical Background

### Pollard's Rho Algorithm
Uses Floyd's cycle-finding with a pseudorandom walk:
```
a_{n+1} = f(a_n)  where f is pseudorandom function
```

When a_i = a_j with i ≠ j, we can solve for d.

### Distinguished Points
To parallelize and save progress:
- Define "distinguished" points (e.g., first 20 bits are zero)
- Save only distinguished points
- Collision occurs when same distinguished point reached via different paths

### Complexity
- **Time**: O(√n) expected
- **Memory**: O(1) (for single instance)
- **Parallel**: Linear speedup with multiple instances

## Comparison with BSGS

| Aspect | BSGS | Rho |
|--------|------|-----|
| Memory | O(√n) | O(1) |
| Time | O(√n) | O(√n) |
| Deterministic | Yes | Probabilistic |
| Parallel | Good | Excellent |
| Large ranges | Memory limited | Memory efficient |

## Features

### Distinguished Points
- Configurable threshold
- Automatic state saving
- Multiple formats

### Parallel Instances
- Multi-thread support
- Multi-GPU support
- Distributed coordination

### State Management
- Auto-save progress
- Resume capability
- State merging

## Basic Usage

```bash
# Basic Rho search
./GpuCracker --mode rho --rho-pub 0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798 --rho-range 80

# With GPU
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --rho-gpu

# Multiple parallel instances
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --rho-instances 100
```

## See Also

- `README_COMMAND_RHO.md` - 50+ command examples
- `README_MODE_BSGS.md` - BSGS algorithm
- `README_MODE_HYBRID.md` - Auto-select algorithm
