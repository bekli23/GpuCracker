# AKM Mode - 50+ Command Examples

## Basic Commands (1-10)

### 1. Basic range scan
```bash
./GpuCracker --mode akm --start 0x00000000000000000000000000000001 --end 0x00000000000000000000000fffffffff
```
Scan small range for testing.

### 2. Large range with partition
```bash
./GpuCracker --mode akm --range 80 --partition 0 --total-partitions 100
```
Search 2^80 range, node 0 of 100.

### 3. Pattern with wildcards
```bash
./GpuCracker --mode akm --pattern "1a*3b*5c*7d*9e*"
```
Search for keys matching hex pattern.

### 4. With specific public key target
```bash
./GpuCracker --mode akm --range 60 --target-pubkey 0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798
```

### 5. GPU-accelerated AKM
```bash
./GpuCracker --mode akm --range 70 --gpu-type cuda --cuda-blocks 256
```
Use NVIDIA GPU for calculation.

### 6. OpenCL for AMD
```bash
./GpuCracker --mode akm --range 70 --gpu-type opencl --device-id 0
```
Use AMD GPU via OpenCL.

### 7. Multi-GPU setup
```bash
./GpuCracker --mode akm --range 80 --gpu-type cuda --device-id 0,1,2,3
```
Use 4 GPUs simultaneously.

### 8. With bloom filter
```bash
./GpuCracker --mode akm --range 64 --bloom-keys btc_addresses.blf
```
Check generated addresses against bloom filter.

### 9. Pattern file scan
```bash
./GpuCracker --mode akm --pattern-file patterns_hex.txt
```
Load multiple patterns from file.

### 10. Resume from checkpoint
```bash
./GpuCracker --mode akm --range 80 --checkpoint-file akm_80.chk --continue
```

## Intermediate Commands (11-30)

### 11. Distributed cluster node 5
```bash
./GpuCracker --mode akm --range 100 --partition 5 --total-partitions 1000 --cluster-master 10.0.0.1
```

### 12. With AKM profile
```bash
./GpuCracker --mode akm --akm-profile profile_v500.json
```
Use pre-configured profile.

### 13. Profile analysis mode
```bash
./GpuCracker --mode akm --akm-mode analyze --akm-profile profile_v500.json
```
Analyze profile effectiveness.

### 14. Adaptive range scanning
```bash
./GpuCracker --mode akm --adaptive-range --start 0x1 --max-range 128
```
Automatically expand range.

### 15. Custom step size
```bash
./GpuCracker --mode akm --start 0x1 --end 0xffffffff --step 0x1000
```
Skip by custom increment.

### 16. Reverse scan direction
```bash
./GpuCracker --mode akm --start 0xffffffff --end 0x1 --reverse
```
Search from high to low.

### 17. Random sampling
```bash
./GpuCracker --mode akm --range 128 --random-sample 1000000
```
Random samples from large range.

### 18. With progress callback
```bash
./GpuCracker --mode akm --range 80 --progress-callback 1000
```
Call every 1000 keys.

### 19. Silent mode
```bash
./GpuCracker --mode akm --range 80 --silent --output-file results.txt
```
No console output, file only.

### 20. Verbose debugging
```bash
./GpuCracker --mode akm --range 40 --verbose --debug-key-calc
```
Detailed calculation logging.

### 21. Compare with reference
```bash
./GpuCracker --mode akm --range 40 --reference-key REFKEY.txt
```
Verify against known result.

### 22. Statistical analysis
```bash
./GpuCracker --mode akm --range 60 --stats-only --stats-file stats.json
```
Gather statistics without full scan.

### 23. Memory-mapped mode
```bash
./GpuCracker --mode akm --range 90 --mmap --mmap-file /mnt/fast/akm.map
```
Use memory mapping for large ranges.

### 24. Low memory mode
```bash
./GpuCracker --mode akm --range 70 --low-memory --batch-size 10000
```
For systems with <8GB RAM.

### 25. Thread affinity
```bash
./GpuCracker --mode akm --range 70 --threads 32 --cpu-affinity 0-31
```
Pin threads to specific cores.

### 26. Real-time priority
```bash
sudo nice -n -20 ./GpuCracker --mode akm --range 80 --realtime-priority
```
Maximum system priority.

### 27. GPU memory limit
```bash
./GpuCracker --mode akm --range 80 --gpu-type cuda --gpu-mem-limit 4G
```
Limit GPU memory usage.

### 28. Multi-target search
```bash
./GpuCracker --mode akm --range 70 --targets-file pubkeys.txt
```
Search for multiple public keys.

### 29. Result formatting
```bash
./GpuCracker --mode akm --range 60 --output-format json --pretty-print
```
JSON output format.

### 30. CSV export
```bash
./GpuCracker --mode akm --range 60 --output-format csv --csv-header
```
Export to CSV.

## Advanced Commands (31-50)

### 31. Hybrid GPU/CPU
```bash
./GpuCracker --mode akm --range 80 --hybrid --gpu-ratio 0.8
```
80% GPU, 20% CPU work.

### 32. Dynamic load balancing
```bash
./GpuCracker --mode akm --range 100 --dynamic-balance --balance-interval 60
```
Rebalance every minute.

### 33. Network distributed
```bash
./GpuCracker --mode akm --range 120 --network-mode server --port 12345
```
Server mode for network distribution.

### 34. Client mode
```bash
./GpuCracker --mode akm --network-mode client --server 10.0.0.1:12345
```
Connect to distribution server.

### 35. Encrypted results
```bash
./GpuCracker --mode akm --range 80 --encrypt-results --encrypt-key KEYFILE.pem
```
Encrypt found keys.

### 36. Secure memory
```bash
./GpuCracker --mode akm --range 80 --secure-memory --lock-pages
```
Prevent swapping of sensitive data.

### 37. Hardware token
```bash
./GpuCracker --mode akm --range 80 --hsm-device /dev/tpm0
```
Use hardware security module.

### 38. Thermal throttling
```bash
./GpuCracker --mode akm --range 80 --thermal-limit 75
```
Pause if temp > 75°C.

### 39. Power limit
```bash
./GpuCracker --mode akm --range 80 --power-limit 200
```
200W power cap.

### 40. Benchmark mode
```bash
./GpuCracker --mode akm --benchmark --range 40
```
Performance testing only.

### 41. Profile generation
```bash
./GpuCracker --mode akm --generate-profile --profile-output new_profile.json
```
Create new AKM profile.

### 42. Profile validation
```bash
./GpuCracker --mode akm --validate-profile profile.json
```
Check profile integrity.

### 43. Auto-tune parameters
```bash
./GpuCracker --mode akm --range 80 --auto-tune --tune-duration 60
```
Auto-optimize for 60 seconds.

### 44. Custom kernel
```bash
./GpuCracker --mode akm --range 80 --custom-kernel ak_custom.cu
```
Use custom CUDA kernel.

### 45. Shader compilation
```bash
./GpuCracker --mode akm --range 80 --gpu-type opencl --compile-shaders
```
Pre-compile OpenCL shaders.

### 46. Kernel profiling
```bash
./GpuCracker --mode akm --range 40 --profile-kernel --profile-output kernel.prof
```
Detailed kernel performance data.

### 47. Memory bandwidth test
```bash
./GpuCracker --mode akm --memory-test --test-duration 30
```
Test GPU memory bandwidth.

### 48. PCIe bandwidth test
```bash
./GpuCracker --mode akm --pcie-test
```
Test host-device transfer speed.

### 49. Full system test
```bash
./GpuCracker --mode akm --system-test --test-suite full
```
Complete system validation.

### 50. Production deployment
```bash
./GpuCracker --mode akm \
  --range 100 \
  --partition 0 \
  --total-partitions 1000 \
  --gpu-type cuda \
  --device-id 0,1,2,3,4,5,6,7 \
  --hybrid \
  --bloom-keys btc_mainnet.blf \
  --targets-file high_value_pubkeys.txt \
  --dynamic-balance \
  --thermal-limit 80 \
  --checkpoint-interval 300 \
  --web-dashboard \
  --encrypt-results \
  --secure-memory \
  --webhook https://secure.company.com/found
```

Enterprise-grade distributed search.

## Performance Tips

### GPU Optimization
- Use latest NVIDIA drivers for CUDA
- Enable persistence mode: `nvidia-smi -pm 1`
- Set power limit appropriately
- Monitor with `nvidia-smi dmon`

### CPU Optimization
- Use physical cores only (disable hyperthreading for best results)
- Set CPU affinity for consistent performance
- Use NUMA-aware memory allocation

### Memory Optimization
- Use huge pages: `echo 1024 > /proc/sys/vm/nr_hugepages`
- Enable swap only for non-critical data
- Use tmpfs for temporary files

## Troubleshooting

### GPU Out of Memory
```bash
./GpuCracker --mode akm --range 80 --gpu-mem-limit 2G --batch-size 1000
```

### Slow CPU Performance
```bash
# Check CPU governor
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
# Set to performance
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

### Network Issues
```bash
# Test connectivity
./GpuCracker --mode akm --network-test --server 10.0.0.1:12345
```
