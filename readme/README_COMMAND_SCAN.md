# SCAN Mode - 50+ Command Examples

## Basic Scanning (1-10)

### 1. Basic vanity search
```bash
./GpuCracker --mode scan --vanity "1BTC"
```
Finds address starting with 1BTC.

### 2. Pattern with wildcards
```bash
./GpuCracker --mode scan --pattern "1A*Z"
```
Starts with 1A, ends with Z.

### 3. Case-insensitive vanity
```bash
./GpuCracker --mode scan --vanity "kimi" --case-insensitive
```
Matches KIMI, Kimi, kiMI, etc.

### 4. Multiple patterns
```bash
./GpuCracker --mode scan --patterns "1A*,1B*,1C*"
```
Search for multiple prefixes.

### 5. With bloom filter
```bash
./GpuCracker --mode scan --bloom-keys btc_addresses.blf
```
Only report if in bloom filter.

### 6. GPU-accelerated scan
```bash
./GpuCracker --mode scan --vanity "1BTC" --gpu-type cuda
```

### 7. Multi-GPU scan
```bash
./GpuCracker --mode scan --vanity "1BTC" --gpu-type cuda --device-id 0,1,2
```

### 8. With output file
```bash
./GpuCracker --mode scan --vanity "1KIMI" --output results.txt
```

### 9. Silent mode
```bash
./GpuCracker --mode scan --vanity "1BTC" --silent --output-found-only
```

### 10. Count limit
```bash
./GpuCracker --mode scan --vanity "1BTC" --count 10
```
Stop after 10 matches.

## Intermediate Scanning (11-30)

### 11. Specific address type
```bash
./GpuCracker --mode scan --vanity "bc1" --address-type bech32
```

### 12. SegWit vanity
```bash
./GpuCracker --mode scan --vanity "bc1qabc" --address-type bech32
```

### 13. Taproot vanity
```bash
./GpuCracker --mode scan --vanity "bc1p" --address-type taproot
```

### 14. Multi-coin scan
```bash
./GpuCracker --mode scan --vanity "1BTC" --multi-coin BTC,LTC,DOGE
```

### 15. With checkpoint
```bash
./GpuCracker --mode scan --vanity "1KIMIKOD" --checkpoint scan.chk
```

### 16. Resume scan
```bash
./GpuCracker --mode scan --vanity "1KIMIKOD" --resume scan.chk
```

### 17. Progress display
```bash
./GpuCracker --mode scan --vanity "1KIMIKOD" --show-rate
```

### 18. Probability estimate
```bash
./GpuCracker --mode scan --vanity "1KIMIKOD" --estimate-time
```

### 19. Difficulty check
```bash
./GpuCracker --mode scan --vanity "1KIMIKOD" --difficulty-only
```

### 20. Parallel threads
```bash
./GpuCracker --mode scan --vanity "1BTC" --threads 64
```

### 21. CPU affinity
```bash
./GpuCracker --mode scan --vanity "1BTC" --cpu-affinity 0-31
```

### 22. OpenCL GPU
```bash
./GpuCracker --mode scan --vanity "1BTC" --gpu-type opencl
```

### 23. Mixed GPU/CPU
```bash
./GpuCracker --mode scan --vanity "1BTC" --hybrid-mode
```

### 24. Specific derivation path
```bash
./GpuCracker --mode scan --vanity "1BTC" --path-file paths_custom.txt
```

### 25. Multiple bloom filters
```bash
./GpuCracker --mode scan --bloom-keys "a.blf,b.blf,c.blf"
```

### 26. Scan from xprv
```bash
./GpuCracker --mode scan --xprv <XPRV> --vanity "1BTC"
```

### 27. Scan from mnemonic
```bash
./GpuCracker --mode scan --mnemonic "word1 word2 ..." --vanity "1BTC"
```

### 28. Random scan
```bash
./GpuCracker --mode scan --random --count 1000000
```

### 29. Sequential with offset
```bash
./GpuCracker --mode scan --start 0x1 --count 1000000
```

### 30. Time-limited scan
```bash
./GpuCracker --mode scan --vanity "1KIMIKOD" --time-limit 3600
```

## Advanced Scanning (31-50)

### 31. Regex pattern
```bash
./GpuCracker --mode scan --regex "^1[A-Z]{5}[0-9]{3}$"
```

### 32. Fuzzy matching
```bash
./GpuCracker --mode scan --vanity "KIMI" --fuzzy 1
```
Allow 1 character difference.

### 33. Multi-prefix search
```bash
./GpuCracker --mode scan --prefixes prefixes.txt
```

### 34. Suffix search
```bash
./GpuCracker --mode scan --suffix "KIMI"
```

### 35. Middle pattern
```bash
./GpuCracker --mode scan --contains "KIMI"
```

### 36. Entropy scan
```bash
./GpuCracker --mode scan --low-entropy --threshold 100
```

### 37. Weak key detection
```bash
./GpuCracker --mode scan --detect-weak-keys
```

### 38. Known vulnerable range
```bash
./GpuCracker --mode scan --vulnerable-range brainwallet
```

### 39. Dictionary scan
```bash
./GpuCracker --mode scan --dictionary words.txt
```

### 40. Levenshtein distance
```bash
./GpuCracker --mode scan --target "KIMI" --levenshtein 2
```

### 41. Distributed scan node
```bash
./GpuCracker --mode scan --vanity "1KIMIKOD" --partition 0 --partitions 100
```

### 42. Network coordinated
```bash
./GpuCracker --mode scan --vanity "1KIMIKOD" --coordinator 10.0.0.1:9000
```

### 43. With web dashboard
```bash
./GpuCracker --mode scan --vanity "1KIMIKOD" --web-dashboard --port 8080
```

### 44. Auto-save results
```bash
./GpuCracker --mode scan --vanity "1KIMIKOD" --auto-save 60
```

### 45. Email notification
```bash
./GpuCracker --mode scan --vanity "1KIMIKOD" --email-on-found user@example.com
```

### 46. Webhook notification
```bash
./GpuCracker --mode scan --vanity "1KIMIKOD" --webhook https://api.example.com/found
```

### 47. Balance threshold
```bash
./GpuCracker --mode scan --bloom-keys addresses.blf --min-balance 0.01
```

### 48. Output formats
```bash
./GpuCracker --mode scan --vanity "1BTC" --output-format json --pretty-print
```

### 49. Paper wallet output
```bash
./GpuCracker --mode scan --vanity "1BTC" --output-format paper-wallet
```

### 50. Complete vanity setup
```bash
./GpuCracker --mode scan \
  --vanity "1KIMIKOD" \
  --gpu-type cuda \
  --device-id 0,1,2,3 \
  --threads 32 \
  --checkpoint vanity.chk \
  --show-rate \
  --estimate-time \
  --output-format json \
  --auto-save 300 \
  --webhook https://api.example.com/found \
  --web-dashboard \
  --port 8080
```

## Difficulty Estimates

| Pattern | Approximate Difficulty | Time @ 1 GH/s |
|---------|----------------------|---------------|
| 1* | 23 | Instant |
| 1A* | 1,400 | Instant |
| 1AB* | 100,000 | Instant |
| 1ABC* | 6,000,000 | 0.006 sec |
| 1ABCD* | 300,000,000 | 0.3 sec |
| 1ABCDE* | 20 billion | 20 sec |
| 1ABCDEF* | 1 trillion | 17 min |
| 1ABCDEFG* | 60 trillion | 17 hours |
| 1ABCDEFGH* | 3 quadrillion | 44 days |

## Tips

1. **Start small**: Test with 2-3 character vanity
2. **Use GPU**: 1000x faster than CPU
3. **Realistic expectations**: 8+ chars needs years
4. **Case matters**: Case-sensitive is 58x harder
5. **Save checkpoints**: For long-running scans
