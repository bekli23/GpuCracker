# PUBKEY Mode - 50+ Command Examples

## Basic Operations (1-10)

### 1. Validate compressed public key
```bash
./GpuCracker --mode pubkey --pubkey 0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798 --validate
```

### 2. Decompress public key
```bash
./GpuCracker --mode pubkey --pubkey 0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798 --decompress
```

### 3. Compress public key
```bash
./GpuCracker --mode pubkey --pubkey 0479BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8 --compress
```

### 4. Get P2PKH address
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --address-type p2pkh
```

### 5. Get Bech32 address
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --address-type bech32
```

### 6. Get all address types
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --all-addresses
```

### 7. Batch validate from file
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --validate
```

### 8. Batch derive addresses
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --derive-addresses --output addresses.txt
```

### 9. Convert to base64
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --output-format base64
```

### 10. Get X,Y coordinates
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --coordinates
```

## Batch Processing (11-20)

### 11. Large file processing
```bash
./GpuCracker --mode pubkey --file pubkeys_large.txt --threads 32 --batch-size 10000
```

### 12. With bloom filter check
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --bloom-keys addresses.blf
```

### 13. Multi-coin derivation
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --multi-coin BTC,LTC,DOGE
```

### 14. Progress display
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --progress --progress-interval 1000
```

### 15. JSON output
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --output-format json --output results.json
```

### 16. CSV output
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --output-format csv --output results.csv
```

### 17. Parallel processing
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --parallel --workers 16
```

### 18. GPU-accelerated batch
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --gpu-type cuda
```

### 19. Filter valid only
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --filter valid --output valid.txt
```

### 20. Skip invalid
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --skip-invalid --continue-on-error
```

## Format Conversions (21-30)

### 21. Hex to PEM
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --output-format pem
```

### 22. Hex to DER
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --output-format der
```

### 23. Raw bytes output
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --output-format raw
```

### 24. Hash160 calculation
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --hash160
```

### 25. SHA256 of pubkey
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --sha256
```

### 26. RipeMD160 only
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --ripemd160
```

### 27. Taproot tweak
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --taproot-tweak
```

### 28. Schnorr public key
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --schnorr-format
```

### 29. ECDSA verification
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --verify-sig --message "test" --signature <SIG>
```

### 30. Batch signature verify
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --verify-sigs sigs.txt
```

## Analysis Operations (31-40)

### 31. Point on curve check
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --on-curve
```

### 32. Order calculation
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --calculate-order
```

### 33. Group membership
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --group-check
```

### 34. Batch curve validation
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --validate-curve
```

### 35. Statistical analysis
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --statistics
```

### 36. Entropy calculation
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --entropy
```

### 37. Distribution analysis
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --distribution
```

### 38. Duplicate detection
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --find-duplicates
```

### 39. Sort by X coordinate
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --sort-by x
```

### 40. Filter by prefix
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --filter-prefix "0279BE"
```

## Advanced Operations (41-50)

### 41. Aggregated public key
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --aggregate --mu-sig
```

### 42. Multi-sig script
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --multisig-script 2-of-3
```

### 43. Merkle tree root
```bash
./GpuCracker --mode pubkey --file pubkeys.txt --merkle-root
```

### 44. Proof generation
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --merkle-proof --root <ROOT>
```

### 45. Pedersen commitment
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --pedersen --blinding <BLIND>
```

### 46. Stealth address
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --stealth --ephemeral <EPH>
```

### 47. Diffie-Hellman
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --ecdh --other-pubkey <OTHER>
```

### 48. X-only pubkey
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --x-only
```

### 49. Parity byte
```bash
./GpuCracker --mode pubkey --pubkey 0279BE... --parity
```

### 50. Complete analysis
```bash
./GpuCracker --mode pubkey \
  --file pubkeys.txt \
  --validate \
  --derive-addresses \
  --all-addresses \
  --multi-coin BTC,LTC \
  --statistics \
  --entropy \
  --output-format json \
  --output analysis.json \
  --threads 32
```

## Output Examples

### JSON Output
```json
{
  "pubkey": "0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798",
  "compressed": true,
  "uncompressed": "0479BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8",
  "coordinates": {
    "x": "79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798",
    "y": "483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8"
  },
  "addresses": {
    "p2pkh": "1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH",
    "p2sh": "3JvL6Ymt8MVWiCNHC7oWU6nLeHNJKLZGLN",
    "bech32": "bc1qdx0yxkrq2d6a5pp8xcjv6l6r22dwqksl5npgvq"
  },
  "hash160": "91b24bf9f5288532960ac687abb035127b1d28a5",
  "valid": true
}
```

## Performance

| Operation | Speed (CPU) | Speed (GPU) |
|-----------|-------------|-------------|
| Validation | 1M/s | 100M/s |
| Decompress | 500K/s | 50M/s |
| Address Derive | 800K/s | 80M/s |
| Batch Process | 2M/s | 200M/s |
