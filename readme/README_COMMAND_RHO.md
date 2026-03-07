# RHO Mode - 50+ Command Examples

## Basic RHO Operations (1-10)

### 1. Basic Rho search
```bash
./GpuCracker --mode rho --rho-pub 0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798 --rho-range 80
```
Search 2^80 range using Pollard's Rho.

### 2. With GPU acceleration
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --rho-gpu
```

### 3. CUDA specifically
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --rho-gpu --gpu-type cuda
```

### 4. Multiple CPU instances
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --rho-instances 64
```
Run 64 parallel Rho walks.

### 5. With distinguished point threshold
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --rho-distinguished-bits 24
```
1 in 2^24 points are distinguished.

### 6. Save state file
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --rho-state rho_state.bin
```

### 7. Resume from state
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-resume rho_state.bin
```

### 8. Multi-GPU setup
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --rho-gpu --gpu-devices 0,1,2,3
```

### 9. Mixed CPU/GPU
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --rho-gpu --rho-instances 32
```

### 10. With progress display
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --progress-interval 1000000
```

## Intermediate RHO (11-30)

### 11. Custom random seed
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --rho-seed 12345
```

### 12. Iteration limit
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --rho-max-iterations 1000000000
```

### 13. Time limit
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --rho-time-limit 86400
```
24 hour limit.

### 14. Distinguished point file
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --dp-file distinguished.txt
```

### 15. DP compression
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --dp-compress
```

### 16. Multi-target Rho
```bash
./GpuCracker --mode rho --rho-targets targets.txt --rho-range 80
```

### 17. Load distinguished points
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-load-dp dp_file.bin
```

### 18. Merge state files
```bash
./GpuCracker --mode rho --rho-merge-states state1.bin,state2.bin --output merged.bin
```

### 19. Check state health
```bash
./GpuCracker --mode rho --rho-state rho_state.bin --check-state
```

### 20. Export distinguished points
```bash
./GpuCracker --mode rho --rho-state rho_state.bin --export-dp dp_export.txt
```

### 21. Custom iteration function
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --rho-function radding-walking
```

### 22. Tame/wild kangaroo
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --rho-variant kangaroo
```

### 23. Parallel collision search
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --parallel-collision
```

### 24. Negation map optimization
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --negation-map
```
√2 speedup.

### 25. Batch distinguished points
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --dp-batch-size 1000
```

### 26. Auto-tune instances
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --auto-tune
```

### 27. Memory limit
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --rho-mem-limit 2G
```

### 28. State interval
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --save-interval 300
```
Save every 5 minutes.

### 29. JSON output
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --output-format json
```

### 30. Silent mode
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --silent --output result.txt
```

## Advanced RHO (31-50)

### 31. Benchmark mode
```bash
./GpuCracker --mode rho --benchmark --rho-range 40
```

### 32. Estimate iterations
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --estimate-iterations
```

### 33. Expected time
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --expected-time
```

### 34. Statistical analysis
```bash
./GpuCracker --mode rho --rho-state rho_state.bin --statistics
```

### 35. DP collision check
```bash
./GpuCracker --mode rho --rho-load-dp dp1.bin --rho-check-collision dp2.bin
```

### 36. Distributed coordination server
```bash
./GpuCracker --mode rho --server --port 12345 --rho-range 100
```

### 37. Distributed worker
```bash
./GpuCracker --mode rho --worker --server 10.0.0.1:12345 --worker-id 0
```

### 38. With bloom filter check
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --bloom-keys addresses.blf
```

### 39. Start offset
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --rho-start 0x10000000000
```

### 40. Dynamic adjustment
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --dynamic-adjust
```

### 41. Thread affinity
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 80 --rho-instances 32 --cpu-affinity
```

### 42. GPU thread config
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --rho-gpu --cuda-blocks 256 --cuda-threads 256
```

### 43. OpenCL kernel options
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --rho-gpu --gpu-type opencl --cl-workgroup 128
```

### 44. FPGA acceleration
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 100 --fpga-device /dev/fpga0
```

### 45. ASIC mode
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 110 --asic-device /dev/asic0
```

### 46. Web dashboard
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --web-dashboard --port 8080
```

### 47. Email notification
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --email-on-found user@example.com
```

### 48. Webhook notification
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --webhook https://api.example.com/found
```

### 49. Thermal protection
```bash
./GpuCracker --mode rho --rho-pub <PUB> --rho-range 90 --thermal-limit 80
```

### 50. Complete RHO deployment
```bash
./GpuCracker --mode rho \
  --rho-targets high_value_pubkeys.txt \
  --rho-range 100 \
  --rho-gpu \
  --gpu-type cuda \
  --gpu-devices 0,1,2,3 \
  --rho-instances 256 \
  --rho-distinguished-bits 26 \
  --rho-state rho_100.bin \
  --save-interval 600 \
  --negation-map \
  --dp-compress \
  --progress-interval 10000000 \
  --web-dashboard \
  --port 8080 \
  --webhook https://secure.company.com/alert
```

## Algorithm Variants

### 1. Classic Rho
Standard Floyd's cycle-finding.

### 2. Distinguished Points
Save points with specific prefix.

### 3. Radding-Walking
Improved random walk.

### 4. Tame/Wild Kangaroo
For known smaller range.

### 5. Parallel Collision Search
Van Oorschot-Wiener.

## Performance Guide

### Optimal Distinguished Bits
| Range | DP Bits | Memory per instance |
|-------|---------|---------------------|
| 2^60 | 20 | 1 MB |
| 2^80 | 24 | 16 MB |
| 2^100 | 28 | 256 MB |
| 2^120 | 32 | 4 GB |

### Expected Iterations
| Range | Expected iterations (±50%) |
|-------|---------------------------|
| 2^60 | 2^30 ≈ 1 billion |
| 2^80 | 2^40 ≈ 1 trillion |
| 2^100 | 2^50 ≈ 1 quadrillion |
| 2^120 | 2^60 ≈ 1 quintillion |

### Speed Estimates (RTX 4090)
| Instances | Iterations/sec | 2^80 time (expected) |
|-----------|---------------|----------------------|
| 1 | 10M | ~3 years |
| 64 | 640M | ~17 days |
| 256 | 2.5B | ~4 days |
| 1024 | 10B | ~1 day |

## Tips

1. **Use negation map**: Automatic √2 speedup
2. **More instances**: Linear speedup with parallelism
3. **Save state**: For long-running searches
4. **Distinguished bits**: Balance memory vs collision detection
5. **GPU for large ranges**: Much faster than CPU
