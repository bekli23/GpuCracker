# MNEMONIC Mode - 50+ Command Examples

## Basic Commands (1-10)

### 1. Basic 12-word mnemonic scan
```bash
./GpuCracker --mode mnemonic --words 12
```
Default English wordlist, checks standard BIP44/49/84 paths for Bitcoin.

### 2. 24-word mnemonic with custom path file
```bash
./GpuCracker --mode mnemonic --words 24 --path-file paths_bitcoin_comprehensive.txt
```
Uses comprehensive path list with 150+ derivation paths.

### 3. Multi-coin mnemonic check
```bash
./GpuCracker --mode mnemonic --words 12 --multi-coin BTC,ETH,LTC,DOGE,DASH
```
Checks same mnemonic against 5 different cryptocurrencies.

### 4. Specific language wordlist
```bash
./GpuCracker --mode mnemonic --words 12 --langs spanish
```
Uses Spanish BIP39 wordlist instead of English.

### 5. With bloom filter for address checking
```bash
./GpuCracker --mode mnemonic --words 12 --bloom-keys btc_addresses.blf
```
Only reports mnemonics that generate addresses in the bloom filter.

### 6. Schematic pattern generation
```bash
./GpuCracker --mode mnemonic --words 12 --mnemonic-order schematic --akm-file pattern.txt
```
Generates mnemonics based on schematic patterns.

### 7. Sequential entropy generation
```bash
./GpuCracker --mode mnemonic --words 12 --mnemonic-order sequential --continue 0x1000
```
Starts from specific entropy offset and increments.

### 8. Random mnemonic generation
```bash
./GpuCracker --mode mnemonic --words 12 --mnemonic-order random --count 1000000
```
Generates 1 million random mnemonics.

### 9. With custom wordlist file
```bash
./GpuCracker --mode mnemonic --words 12 --akm-file my_wordlist.txt
```
Uses custom wordlist instead of standard BIP39.

### 10. GPU-accelerated mnemonic check
```bash
./GpuCracker --mode mnemonic --words 12 --gpu-type cuda --device-id 0
```
Uses NVIDIA GPU for faster key derivation.

## Intermediate Commands (11-30)

### 11. Check with 100+ coins
```bash
./GpuCracker --mode mnemonic --words 12 --multi-coin BTC,ETH,BCH,LTC,DOGE,DASH,ZEC,ETC,BNB,MATIC,AVAX,FTM,CRO,ATOM,DOT,SOL,ADA,ALGO,XTZ,XRP,XLM,TRX,NEO,VET,FIL,THETA,XTZ,HBAR,EGLD,NEAR,FTM,ONE,RUNE,SNX,AAVE,UNI,LINK,MKR,COMP,YFI,CRV,BAL,1INCH,SUSHI,KNC,REN,LRC,GRT,ENJ,MANA,SAND,AXS,CHZ,FLOW,ICP,MINA,ROSE,CELO,SCRT,INJ,AVAX,ARB,OP,IMX,STX
```

### 12. High-performance GPU with all coins
```bash
./GpuCracker --mode mnemonic --words 12 --multi-coin ALL --gpu-type cuda --cuda-blocks 512 --cuda-threads 256
```

### 13. Deep scan with extended paths
```bash
./GpuCracker --mode mnemonic --words 12 --path-file paths_deep_scan.txt --threads 32
```
Scans high index ranges (0-1000000).

### 14. With blockchain directory
```bash
./GpuCracker --mode mnemonic --words 12 --blockchain-dir /mnt/bitcoin/blocks --index-dir /mnt/bitcoin/index
```
Checks against local blockchain data.

### 15. Resume interrupted scan
```bash
./GpuCracker --mode mnemonic --words 12 --continue 0x5F3A2B1C
```
Continues from specific entropy offset.

### 16. Limited count scan
```bash
./GpuCracker --mode mnemonic --words 12 --count 10000000
```
Stops after 10 million attempts.

### 17. With multiple bloom filters
```bash
./GpuCracker --mode mnemonic --words 12 --bloom-keys btc_2010.blf,btc_2011.blf,btc_2012.blf
```
Checks against multiple historical address sets.

### 18. Japanese wordlist with custom paths
```bash
./GpuCracker --mode mnemonic --words 12 --langs japanese --path-file paths_multicoin.txt
```

### 19. Parallel GPU devices
```bash
./GpuCracker --mode mnemonic --words 12 --gpu-type cuda --device-id 0 &
./GpuCracker --mode mnemonic --words 12 --gpu-type cuda --device-id 1 &
```
Uses 2 GPUs simultaneously.

### 20. Memory-optimized mode
```bash
./GpuCracker --mode mnemonic --words 12 --batch-size 1000 --low-memory
```
Reduces memory usage for systems with limited RAM.

### 21. With verbose logging
```bash
./GpuCracker --mode mnemonic --words 12 --verbose --log-file mnemonic_scan.log
```
Detailed logging for debugging.

### 22. Specific coin with all paths
```bash
./GpuCracker --mode mnemonic --words 12 --multi-coin ETH --path-file paths_ethereum_all.txt
```
Focuses on Ethereum with comprehensive path list.

### 23. Check against known addresses
```bash
./GpuCracker --mode mnemonic --words 12 --pub-address 1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa
```
Looks for mnemonic generating specific address.

### 24. With checkpoint saving
```bash
./GpuCracker --mode mnemonic --words 12 --checkpoint-interval 300 --checkpoint-file mnemonic.chk
```
Saves progress every 5 minutes.

### 25. Random with specific seed
```bash
./GpuCracker --mode mnemonic --words 12 --mnemonic-order random --entropy-seed 12345
```
Reproducible random sequence.

### 26. Prefix search
```bash
./GpuCracker --mode mnemonic --words 12 --mnemonic-prefix "abandon ability"
```
Fixes first two words, searches remaining.

### 27. Suffix search
```bash
./GpuCracker --mode mnemonic --words 12 --mnemonic-suffix "zoo zone"
```
Fixes last two words, searches beginning.

### 28. Pattern with wildcards
```bash
./GpuCracker --mode mnemonic --words 12 --pattern "* ability * * * * * * * * zoo *"
```
Searches with specific word positions.

### 29. Compressed addresses only
```bash
./GpuCracker --mode mnemonic --words 12 --address-type compressed
```
Only checks compressed public key formats.

### 30. SegWit native only
```bash
./GpuCracker --mode mnemonic --words 12 --address-type bech32 --multi-coin BTC
```
Only checks bc1... addresses.

## Advanced Commands (31-50)

### 31. Full automatic mode
```bash
./GpuCracker --mode mnemonic --words auto --auto-detect --scan-all-paths --multi-coin ALL --gpu-type auto
```
Auto-detects best parameters.

### 32. Cluster distributed scan
```bash
./GpuCracker --mode mnemonic --words 12 --cluster-node 0 --cluster-nodes 4 --cluster-master 192.168.1.100
```
Distributed scan across 4 nodes.

### 33. With web dashboard
```bash
./GpuCracker --mode mnemonic --words 12 --web-dashboard --web-port 8080
```
Real-time monitoring via web interface.

### 34. Priority process
```bash
nice -n -20 ./GpuCracker --mode mnemonic --words 12 --realtime-priority
```
Highest system priority.

### 35. RAM disk for bloom filters
```bash
./GpuCracker --mode mnemonic --words 12 --bloom-keys /ramdisk/btc.blf --temp-dir /ramdisk
```
Uses RAM disk for faster bloom filter access.

### 36. Specific derivation indices
```bash
./GpuCracker --mode mnemonic --words 12 --path-file paths_bitcoin_standard.txt --index-start 0 --index-end 1000
```
Checks first 1000 addresses per path.

### 37. Change addresses only
```bash
./GpuCracker --mode mnemonic --words 12 --address-type change
```
Only checks change addresses (index 1).

### 38. External command integration
```bash
./GpuCracker --mode mnemonic --words 12 --exec "./custom_checker.sh" --pipe-results
```
Pipes results to external script.

### 39. Unix socket input
```bash
./GpuCracker --mode mnemonic --words 12 --socket /tmp/mnemonic_feed.sock
```
Reads seeds from Unix socket.

### 40. Batch file processing
```bash
./GpuCracker --mode mnemonic --words 12 --file wordlists/*.txt --batch-mode
```
Processes multiple wordlist files.

### 41. With custom hash function
```bash
./GpuCracker --mode mnemonic --words 12 --custom-hash ./my_hash_plugin.so
```
Uses custom hash algorithm.

### 42. Time-limited scan
```bash
./GpuCracker --mode mnemonic --words 12 --time-limit 3600
```
Runs for exactly 1 hour.

### 43. Result callback URL
```bash
./GpuCracker --mode mnemonic --words 12 --webhook https://api.example.com/found
```
POSTs results to webhook.

### 44. Email notification
```bash
./GpuCracker --mode mnemonic --words 12 --email-on-found user@example.com --smtp-server mail.example.com
```
Sends email when key found.

### 45. Telegram notification
```bash
./GpuCracker --mode mnemonic --words 12 --telegram-bot TOKEN --telegram-chat CHAT_ID
```
Sends Telegram message on success.

### 46. Multiple GPU types
```bash
./GpuCracker --mode mnemonic --words 12 --gpu-type cuda:0,cuda:1,opencl:0
```
Uses 2 NVIDIA + 1 AMD GPU.

### 47. Adaptive difficulty
```bash
./GpuCracker --mode mnemonic --words 12 --adaptive-mode --start-easy --increase-difficulty
```
Starts with common patterns, gets harder.

### 48. With known xpub
```bash
./GpuCracker --mode mnemonic --words 12 --known-xpub xpub6C...
```
Stops when xpub matches.

### 49. Balance threshold
```bash
./GpuCracker --mode mnemonic --words 12 --min-balance 0.001 --blockchain-api blockchain.info
```
Only reports addresses with balance > 0.001 BTC.

### 50. Complete enterprise setup
```bash
./GpuCracker --mode mnemonic \
  --words 12 \
  --multi-coin ALL \
  --path-file paths_comprehensive.txt \
  --bloom-keys btc_all.blf,eth_all.blf \
  --gpu-type cuda \
  --device-id 0,1,2,3 \
  --threads 64 \
  --web-dashboard \
  --web-port 8080 \
  --log-file /var/log/gpu_cracker.log \
  --checkpoint-interval 600 \
  --email-on-found admin@company.com \
  --webhook https://api.company.com/alerts
```

Full enterprise deployment with monitoring.

## Tips & Tricks

### Performance Optimization
1. Use GPU acceleration when available (10-100x faster)
2. Bloom filters reduce I/O (1000x faster for large address sets)
3. Multi-coin checking adds minimal overhead
4. Local blockchain is faster than API

### Memory Management
1. Large bloom filters (>1GB) should be on SSD or RAM disk
2. Reduce thread count if RAM is limited
3. Use `--low-memory` flag for <16GB systems

### Security
1. Never share recovered keys in logs
2. Use encrypted storage for checkpoints
3. Secure webhook endpoints
4. Monitor for unauthorized access

## Troubleshooting

### Out of Memory
```bash
# Reduce batch size
./GpuCracker --mode mnemonic --words 12 --batch-size 100
```

### GPU Not Detected
```bash
# Check CUDA
nvidia-smi

# Force OpenCL
./GpuCracker --mode mnemonic --words 12 --gpu-type opencl
```

### Slow Performance
```bash
# Check CPU affinity
./GpuCracker --mode mnemonic --words 12 --cpu-affinity 0-15
```
