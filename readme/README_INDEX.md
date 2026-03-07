# GpuCracker v50.0 - Documentation Index

Welcome to the GpuCracker documentation! This directory contains comprehensive documentation for all operational modes.

## Quick Start

New users should start with:
1. `../README.md` - Main project README
2. `README_MODE_MNEMONIC.md` or `README_MODE_BSGS.md` - Most common modes
3. `README_COMMAND_*.md` files - Detailed command examples

## Documentation Structure

### Mode Documentation

Each mode has two documentation files:
- `README_MODE_<MODE>.md` - Overview and technical details
- `README_COMMAND_<MODE>.md` - 50+ command examples

#### Available Modes

| Mode | Purpose | Difficulty |
|------|---------|------------|
| [MNEMONIC](README_MODE_MNEMONIC.md) | BIP39 mnemonic recovery | ⭐⭐⭐ |
| [AKM](README_MODE_AKM.md) | Advanced keyspace management | ⭐⭐⭐⭐ |
| [CHECK](README_MODE_CHECK.md) | Key/address verification | ⭐⭐ |
| [SCAN](README_MODE_SCAN.md) | Address/pattern scanning | ⭐⭐⭐ |
| [XPRV](README_MODE_XPRV.md) | Extended key operations | ⭐⭐⭐ |
| [BRAINWALLET](README_MODE_BRAINWALLET.md) | Brain wallet recovery | ⭐⭐ |
| [PUBKEY](README_MODE_PUBKEY.md) | Public key operations | ⭐⭐ |
| [BSGS](README_MODE_BSGS.md) | Baby-step Giant-step algorithm | ⭐⭐⭐⭐⭐ |
| [RHO](README_MODE_RHO.md) | Pollard's Rho algorithm | ⭐⭐⭐⭐⭐ |
| [HYBRID](README_MODE_HYBRID.md) | Intelligent algorithm selection | ⭐⭐⭐⭐ |

### Command Reference Files

- [MNEMONIC Commands](README_COMMAND_MNEMONIC.md) - 50+ mnemonic commands
- [AKM Commands](README_COMMAND_AKM.md) - 50+ AKM commands
- [CHECK Commands](README_COMMAND_CHECK.md) - 50+ check commands
- [SCAN Commands](README_COMMAND_SCAN.md) - 50+ scan commands
- [XPRV Commands](README_COMMAND_XPRV.md) - 50+ xprv commands
- [BRAINWALLET Commands](README_COMMAND_BRAINWALLET.md) - 50+ brainwallet commands
- [PUBKEY Commands](README_COMMAND_PUBKEY.md) - 50+ pubkey commands
- [BSGS Commands](README_COMMAND_BSGS.md) - 50+ BSGS commands
- [RHO Commands](README_COMMAND_RHO.md) - 50+ Rho commands
- [HYBRID Commands](README_COMMAND_HYBRID.md) - 50+ hybrid commands

## Version Information

**Current Version**: GpuCracker v50.0

### Version History Highlights
- v50.0 - Complete BSGS/Rho/Hybrid ECDLP implementation
- v49.x - GPU acceleration improvements
- v48.x - Multi-coin support expansion
- v47.x - Bloom filter optimization
- v42.2 - Previous stable release

## Common Workflows

### Workflow 1: Recover Lost Mnemonic
```
1. README_MODE_MNEMONIC.md - Understand the mode
2. README_COMMAND_MNEMONIC.md - Find appropriate commands
3. Start with command #1 (basic scan)
4. Progress to advanced commands as needed
```

### Workflow 2: ECDLP Key Recovery
```
1. README_MODE_HYBRID.md - Understand auto-selection
2. README_COMMAND_HYBRID.md - Deploy hybrid search
3. If memory limited: README_MODE_RHO.md
4. If memory available: README_MODE_BSGS.md
```

### Workflow 3: Key Verification
```
1. README_MODE_CHECK.md - Quick overview
2. README_COMMAND_CHECK.md - Find validation commands
```

## Support

For issues and questions:
1. Check relevant mode documentation
2. Review command examples
3. See ../TROUBLESHOOTING.md (if available)
4. Check ../FAQ.md (if available)

## Contributing

Documentation improvements welcome! Please ensure:
- Commands are tested before adding
- Examples use realistic parameters
- Performance data is accurate
- Security warnings are prominent

---

**Total Commands Documented**: 500+
**Last Updated**: 2026-03-04
**Maintained by**: GpuCracker Team
