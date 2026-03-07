# CHECK Mode - 50+ Command Examples

## Basic Verification (1-10)

### 1. Check WIF private key
```bash
./GpuCracker --mode check --key L1aW4aubDFB7yfras2S1mN3bqg9nwySY8nkoLmJebSLD5BWv3ENZ
```
Verify WIF format and extract details.

### 2. Check hex private key
```bash
./GpuCracker --mode check --key 0000000000000000000000000000000000000000000000000000000000000001 --format hex
```

### 3. Validate mnemonic
```bash
./GpuCracker --mode check --mnemonic "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"
```

### 4. Check compressed public key
```bash
./GpuCracker --mode check --pubkey 0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798
```

### 5. Check uncompressed public key
```bash
./GpuCracker --mode check --pubkey 0479BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8
```

### 6. Verify address format
```bash
./GpuCracker --mode check --address 1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa
```

### 7. Check SegWit address
```bash
./GpuCracker --mode check --address bc1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq
```

### 8. Validate Taproot address
```bash
./GpuCracker --mode check --address bc1p5d7rjq7g6rdk2yhzks9smlaqtedr4dekq08ge8ztwac72sfr9rusxg3297
```

### 9. Check minikey format
```bash
./GpuCracker --mode check --minikey S6c56bnXQiBjk9mqSYE7ykVQ7NzrRy
```

### 10. Verify key-address pair
```bash
./GpuCracker --mode check --key L1aW4aubDFB7yfras2S1mN3bqg9nwySY8nkoLmJebSLD5BWv3ENZ --address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
```

## Batch Operations (11-20)

### 11. Check keys from file
```bash
./GpuCracker --mode check --file keys.txt
```
One key per line.

### 12. Batch address validation
```bash
./GpuCracker --mode check --file addresses.txt --check-addresses
```

### 13. Validate multiple mnemonics
```bash
./GpuCracker --mode check --file mnemonics.txt --check-mnemonics
```

### 14. Check with progress
```bash
./GpuCracker --mode check --file keys.txt --progress
```

### 15. Export valid keys only
```bash
./GpuCracker --mode check --file keys.txt --output valid_keys.txt --filter valid
```

### 16. Check with specific coin
```bash
./GpuCracker --mode check --file keys.txt --multi-coin BTC,LTC,DOGE
```

### 17. Validate all formats
```bash
./GpuCracker --mode check --file mixed.txt --detect-format
```

### 18. Parallel checking
```bash
./GpuCracker --mode check --file keys.txt --threads 32
```

### 19. Check and derive addresses
```bash
./GpuCracker --mode check --file keys.txt --derive-addresses
```

### 20. Verify against blockchain
```bash
./GpuCracker --mode check --file keys.txt --verify-balance --api blockchain.info
```

## Format Conversions (21-30)

### 21. Convert WIF to hex
```bash
./GpuCracker --mode check --key <WIF> --output-format hex
```

### 22. Convert hex to WIF
```bash
./GpuCracker --mode check --key <HEX> --output-format wif
```

### 23. Get compressed from uncompressed
```bash
./GpuCracker --mode check --pubkey <UNCOMPRESSED> --compress
```

### 24. Get uncompressed from compressed
```bash
./GpuCracker --mode check --pubkey <COMPRESSED> --decompress
```

### 25. Convert to all address types
```bash
./GpuCracker --mode check --key <WIF> --all-addresses
```
Generates P2PKH, P2SH, Bech32.

### 26. Export as JSON
```bash
./GpuCracker --mode check --key <WIF> --output-format json --pretty-print
```

### 27. Export as CSV
```bash
./GpuCracker --mode check --file keys.txt --output-format csv
```

### 28. Generate QR codes
```bash
./GpuCracker --mode check --key <WIF> --qr-code
```

### 29. Paper wallet format
```bash
./GpuCracker --mode check --key <WIF> --paper-wallet
```

### 30. Electrum format
```bash
./GpuCracker --mode check --key <WIF> --electrum-format
```

## Derivations (31-40)

### 31. Derive from mnemonic
```bash
./GpuCracker --mode check --mnemonic "word1 word2 ..." --derive-path "m/44'/0'/0'/0/0"
```

### 32. Derive multiple paths
```bash
./GpuCracker --mode check --mnemonic "word1 word2 ..." --path-file paths_bitcoin_standard.txt
```

### 33. Get xprv from mnemonic
```bash
./GpuCracker --mode check --mnemonic "word1 word2 ..." --get-xprv
```

### 34. Get xpub from xprv
```bash
./GpuCracker --mode check --xprv <XPRV> --get-xpub
```

### 35. Derive child keys
```bash
./GpuCracker --mode check --xprv <XPRV> --derive-index 0-1000
```

### 36. Derive hardened keys
```bash
./GpuCracker --mode check --xprv <XPRV> --derive-hardened "m/44'/0'/0'"
```

### 37. Check all derivation paths
```bash
./GpuCracker --mode check --mnemonic "word1 word2 ..." --all-derivations
```

### 38. Specific coin derivation
```bash
./GpuCracker --mode check --mnemonic "word1 word2 ..." --multi-coin ETH --derive-path "m/44'/60'/0'/0/0"
```

### 39. Master key info
```bash
./GpuCracker --mode check --mnemonic "word1 word2 ..." --master-info
```

### 40. Account extended keys
```bash
./GpuCracker --mode check --xprv <XPRV> --account-xkeys
```

## Advanced Operations (41-50)

### 41. Verify signature
```bash
./GpuCracker --mode check --verify-sig --message "Hello" --signature <SIG> --pubkey <PUB>
```

### 42. Create signature
```bash
./GpuCracker --mode check --sign-message "Hello" --key <WIF>
```

### 43. Check transaction input
```bash
./GpuCracker --mode check --tx-input <TX_HEX> --verify-key <PUB>
```

### 44. Verify multisig
```bash
./GpuCracker --mode check --multisig 2-of-3 --pubkeys pub1,pub2,pub3 --address <ADDR>
```

### 45. Check HD wallet structure
```bash
./GpuCracker --mode check --xprv <XPRV> --hd-verify
```

### 46. Gap limit check
```bash
./GpuCracker --mode check --xprv <XPRV> --gap-limit 100
```

### 47. Used address discovery
```bash
./GpuCracker --mode check --xprv <XPRV> --discover-used --api electrum
```

### 48. Balance check all addresses
```bash
./GpuCracker --mode check --xprv <XPRV> --check-all-balances
```

### 49. Export wallet summary
```bash
./GpuCracker --mode check --xprv <XPRV> --wallet-summary --output wallet.json
```

### 50. Complete validation suite
```bash
./GpuCracker --mode check \
  --file recovery_candidates.txt \
  --verify-balance \
  --derive-all-paths \
  --multi-coin BTC,ETH,LTC \
  --output-format json \
  --output validated_results.json \
  --threads 32 \
  --progress
```

Full validation with balance checking.

## Output Formats

### Available Formats
- `text` - Human readable (default)
- `json` - JSON structured
- `csv` - Comma separated
- `xml` - XML format
- `binary` - Raw binary

### Example JSON Output
```json
{
  "key": "L1aW4aubDFB7yfras2S1mN3bqg9nwySY8nkoLmJebSLD5BWv3ENZ",
  "hex": "0000000000000000000000000000000000000000000000000000000000000001",
  "public_key": "0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798",
  "addresses": {
    "p2pkh": "1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH",
    "p2sh": "3JvL6Ymt8MVWiCNHC7oWU6nLeHNJKLZGLN",
    "bech32": "bc1qdx0yxkrq2d6a5pp8xcjv6l6r22dwqksl5npgvq"
  },
  "valid": true,
  "compressed": true
}
```

## Error Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Invalid key format |
| 2 | Checksum mismatch |
| 3 | Address mismatch |
| 4 | Invalid mnemonic |
| 5 | Unsupported format |
| 6 | Network error |

## Best Practices

1. **Always verify** recovered keys before use
2. Use `--verify-balance` before importing
3. Check multiple address types (P2PKH, P2SH, Bech32)
4. Export results in JSON for automation
5. Use batch mode for large key sets
