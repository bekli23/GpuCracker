# SCAN Mode - Address Scanner

## Overview

The **scan** mode scans blockchain data to find addresses matching specific criteria. It's designed for:

- Finding addresses with specific patterns
- Scanning for vanity addresses
- Bulk address generation and checking
- Blockchain analysis

## Features

### Scanning Methods
- **Linear**: Sequential address generation
- **Random**: Random key generation
- **Pattern**: Specific address patterns
- **Dictionary**: Word-based generation

### Output Options
- Matching addresses only
- All generated addresses
- With private keys
- With derivation paths

## Basic Usage

```bash
# Basic scan
./GpuCracker --mode scan --pattern "1A*"

# Vanity scan
./GpuCracker --mode scan --vanity "1KIMI"

# With bloom filter
./GpuCracker --mode scan --bloom-keys addresses.blf
```

## See Also

- `README_COMMAND_SCAN.md` - 50+ command examples
- `README_MODE_CHECK.md` - Verify found addresses
