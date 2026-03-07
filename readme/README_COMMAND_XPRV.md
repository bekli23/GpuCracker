# XPRV Mode - 50+ Command Examples

## Basic Operations (1-10)

### 1. Validate xprv
```bash
./GpuCracker --mode xprv --xprv xprv9s21ZrQH143K3GJpoapnV8SFfu1N7cPGq6Z6J... --validate
```

### 2. Get xpub from xprv
```bash
./GpuCracker --mode xprv --xprv <XPRV> --get-xpub
```

### 3. Derive single address
```bash
./GpuCracker --mode xprv --xprv <XPRV> --derive "m/44'/0'/0'/0/0"
```

### 4. Derive range
```bash
./GpuCracker --mode xprv --xprv <XPRV> --derive "m/44'/0'/0'/0/0-100"
```

### 5. Account level derivation
```bash
./GpuCracker --mode xprv --xprv <XPRV> --derive "m/44'/0'/0'"
```

### 6. Get master key info
```bash
./GpuCracker --mode xprv --xprv <XPRV> --info
```

### 7. Derive hardened only
```bash
./GpuCracker --mode xprv --xprv <XPRV> --derive "m/44'/0'/0'" --hardened-only
```

### 8. BIP49 path
```bash
./GpuCracker --mode xprv --xprv <XPRV> --derive "m/49'/0'/0'/0/0"
```

### 9. BIP84 path
```bash
./GpuCracker --mode xprv --xprv <XPRV> --derive "m/84'/0'/0'/0/0"
```

### 10. BIP86 Taproot
```bash
./GpuCracker --mode xprv --xprv <XPRV> --derive "m/86'/0'/0'/0/0"
```

## Batch Operations (11-20)

### 11. Multiple paths from file
```bash
./GpuCracker --mode xprv --xprv <XPRV> --path-file paths.txt
```

### 12. Derive all standard paths
```bash
./GpuCracker --mode xprv --xprv <XPRV> --all-standard-paths
```

### 13. Export key tree
```bash
./GpuCracker --mode xprv --xprv <XPRV> --export-tree --depth 3
```

### 14. Batch xprv validation
```bash
./GpuCracker --mode xprv --file xprvs.txt --validate
```

### 15. Convert all to xpub
```bash
./GpuCracker --mode xprv --file xprvs.txt --get-xpub --output xpubs.txt
```

### 16. Derive for multiple coins
```bash
./GpuCracker --mode xprv --xprv <XPRV> --multi-coin BTC,LTC,DOGE
```

### 17. Complete wallet export
```bash
./GpuCracker --mode xprv --xprv <XPRV> --export-wallet wallet.json
```

### 18. Gap limit scan
```bash
./GpuCracker --mode xprv --xprv <XPRV> --gap-limit 100
```

### 19. Used address discovery
```bash
./GpuCracker --mode xprv --xprv <XPRV> --discover-used --api electrum
```

### 20. Balance check
```bash
./GpuCracker --mode xprv --xprv <XPRV> --check-balances
```

## Format Conversions (21-30)

### 21. To WIF
```bash
./GpuCracker --mode xprv --xprv <XPRV> --derive "m/44'/0'/0'/0/0" --output-format wif
```

### 22. To hex
```bash
./GpuCracker --mode xprv --xprv <XPRV> --derive "m/44'/0'/0'/0/0" --output-format hex
```

### 23. To mnemonic (if possible)
```bash
./GpuCracker --mode xprv --xprv <XPRV> --to-mnemonic
```

### 24. Different coin versions
```bash
./GpuCracker --mode xprv --xprv <BTC_XPRV> --convert-to LTC
```

### 25. Testnet conversion
```bash
./GpuCracker --mode xprv --xprv <XPRV> --testnet
```

### 26. Ypub (BIP49)
```bash
./GpuCracker --mode xprv --xprv <XPRV> --output-format ypub
```

### 27. Zpub (BIP84)
```bash
./GpuCracker --mode xprv --xprv <XPRV> --output-format zpub
```

### 28. To descriptors
```bash
./GpuCracker --mode xprv --xprv <XPRV> --to-descriptor
```

### 29. To wallet format
```bash
./GpuCracker --mode xprv --xprv <XPRV> --to-wallet-format electrum
```

### 30. Minimize key
```bash
./GpuCracker --mode xprv --xprv <XPRV> --minimize
```

## Security Operations (31-40)

### 31. Verify derivation path
```bash
./GpuCracker --mode xprv --xprv <XPRV> --verify-path "m/44'/0'/0'/0/0"
```

### 32. Check fingerprint
```bash
./GpuCracker --mode xprv --xprv <XPRV> --fingerprint
```

### 33. Parent fingerprint
```bash
./GpuCracker --mode xprv --xprv <XPRV> --parent-fingerprint
```

### 34. Chain code extract
```bash
./GpuCracker --mode xprv --xprv <XPRV> --chain-code
```

### 35. Neutered check
```bash
./GpuCracker --mode xprv --xpub <XPUB> --is-neutered
```

### 36. Depth check
```bash
./GpuCracker --mode xprv --xprv <XPRV> --depth
```

### 37. Child number
```bash
./GpuCracker --mode xprv --xprv <XPRV> --child-number
```

### 38. Master fingerprint
```bash
./GpuCracker --mode xprv --xprv <XPRV> --master-fingerprint
```

### 39. Encrypt xprv
```bash
./GpuCracker --mode xprv --xprv <XPRV> --encrypt --password "secure"
```

### 40. Decrypt xprv
```bash
./GpuCracker --mode xprv --xprv <ENCRYPTED> --decrypt --password "secure"
```

## Advanced Operations (41-50)

### 41. Custom derivation scheme
```bash
./GpuCracker --mode xprv --xprv <XPRV> --custom-scheme ./scheme.json
```

### 42. Multisig derivation
```bash
./GpuCracker --mode xprv --xprv <XPRV1,XPRV2,XPRV3> --multisig 2-of-3
```

### 43. Shamir split
```bash
./GpuCracker --mode xprv --xprv <XPRV> --shamir-split 3-of-5
```

### 44. Shamir combine
```bash
./GpuCracker --mode xprv --shamir-combine share1.txt,share2.txt,share3.txt
```

### 45. Seed extraction
```bash
./GpuCracker --mode xprv --xprv <XPRV> --extract-seed
```

### 46. Key origin info
```bash
./GpuCracker --mode xprv --xprv <XPRV> --key-origin
```

### 47. PSBT compatible
```bash
./GpuCracker --mode xprv --xprv <XPRV> --psbt-export psbt.json
```

### 48. Hardware wallet format
```bash
./GpuCracker --mode xprv --xprv <XPRV> --to-hardware-format ledger
```

### 49. Sweep to xprv
```bash
./GpuCracker --mode xprv --sweep-keys keys.txt --output-xprv new.xprv
```

### 50. Complete xprv analysis
```bash
./GpuCracker --mode xprv \
  --xprv <XPRV> \
  --info \
  --fingerprint \
  --all-standard-paths \
  --gap-limit 100 \
  --discover-used \
  --check-balances \
  --export-wallet wallet_full.json \
  --output-format json \
  --pretty-print
```

## Output Format Examples

### JSON Output
```json
{
  "xprv": "xprv9s21ZrQH143K3GJpoapnV8SFfu1N7cPGq6Z6J...",
  "xpub": "xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8",
  "fingerprint": "00000000",
  "chain_code": "...",
  "depth": 0,
  "derived_keys": [
    {
      "path": "m/44'/0'/0'/0/0",
      "address": "1A...",
      "wif": "K..."
    }
  ]
}
```

## Best Practices

1. **Validate before use**: Always run `--validate` first
2. **Secure storage**: Use `--encrypt` for saved keys
3. **Backup derivation paths**: Save with `--export-tree`
4. **Check balances**: Use `--check-balances` before sweeping
5. **Gap limit**: Use appropriate gap limit for wallet type
