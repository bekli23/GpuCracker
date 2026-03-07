# HYBRID Mode - Intelligent Algorithm Selection

## Overview

The **hybrid** mode automatically selects the optimal algorithm (BSGS or Rho) based on available resources, range size, and system configuration. It provides the best of both worlds - BSGS speed for ranges that fit in memory, and Rho efficiency for extremely large ranges.

## Auto-Selection Logic

### Selection Criteria
| Factor | BSGS Preferred | Rho Preferred |
|--------|---------------|---------------|
| Range size | 2^60 or less | 2^70 or more |
| Available RAM | > 2× required | < required |
| Multi-target | Yes | No |
| Deterministic | Required | Not critical |
| Time limit | Short | Long |

### Hybrid Strategy
1. **Analyze** range, memory, targets
2. **Estimate** completion time for each algorithm
3. **Select** optimal algorithm
4. **Switch** if conditions change

## Features

### Dynamic Switching
- Monitor memory pressure
- Auto-switch to Rho if OOM
- Resume capability across algorithms

### Resource Management
- Automatic GPU allocation
- CPU/GPU load balancing
- Memory-aware scheduling

### Progress Tracking
- Unified progress format
- Cross-algorithm resume
- Comprehensive statistics

## Basic Usage

```bash
# Automatic mode selection
./GpuCracker --mode hybrid --target-pub 0279BE... --search-range 60

# With preference
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 80 --prefer-bsgs

# Multi-target hybrid
./GpuCracker --mode hybrid --targets targets.txt --search-range 70
```

## See Also

- `README_COMMAND_HYBRID.md` - 50+ command examples
- `README_MODE_BSGS.md` - Baby-step Giant-step algorithm
- `README_MODE_RHO.md` - Pollard's Rho algorithm
