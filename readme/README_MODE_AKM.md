# AKM Mode - Advanced Keyspace Manager

## Overview

The **AKM** (Advanced Keyspace Manager) mode provides intelligent, targeted scanning of specific keyspaces rather than brute-force enumeration. It's designed for:

- Recovering partial private keys
- Scanning known pattern ranges
- Distributed keyspace partitioning
- High-performance targeted attacks

## Key Features

### Keyspace Partitioning
- Divide massive search spaces across multiple nodes
- Configurable partition size and overlap
- Automatic load balancing
- Resume capability per partition

### Pattern Recognition
- Hex pattern matching
- Known prefix/suffix scanning
- Custom rule definitions
- AI-assisted pattern prediction

### Performance
- Optimized for large contiguous ranges
- Memory-mapped file support
- Zero-copy operations
- Lock-free threading

## Use Cases

### Case 1: Partial Key Recovery
You have 30 of 64 hex characters from a private key:
```bash
./GpuCracker --mode akm --pattern "1a3f*8b2c*5d4e*..."
```

### Case 2: Distributed Search
Search 2^128 range across 100 nodes:
```bash
# Node 0
./GpuCracker --mode akm --start 0x0000...0000 --end 0x0000...ffff --partition 0 --total-partitions 100

# Node 1
./GpuCracker --mode akm --start 0x0000...0000 --end 0x0000...ffff --partition 1 --total-partitions 100
```

### Case 3: Pattern-Based Scan
Search for keys with specific patterns:
```bash
./GpuCracker --mode akm --pattern-file common_patterns.txt
```

## Command Structure

```bash
./GpuCracker --mode akm [OPTIONS]

Options:
  --start HEX              Starting private key (hex)
  --end HEX                Ending private key (hex)
  --range BITS             Search range as 2^BITS
  --partition N            Partition number for distributed search
  --total-partitions M     Total partitions to divide range
  --pattern PATTERN        Hex pattern with wildcards (*)
  --pattern-file FILE      File with multiple patterns
  --akm-profile FILE       Load AKM profile configuration
  --akm-mode MODE          Profile mode (scan|analyze|predict)
```

## See Also

- `README_COMMAND_AKM.md` - 50+ command examples
- `README_MODE_CHECK.md` - Verify recovered keys
- `README_MODE_BSGS.md` - For ECDLP-based recovery
