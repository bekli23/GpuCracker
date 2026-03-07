# BSGS Mode - 50+ Command Examples

## Basic BSGS Operations (1-10)

### 1. Basic BSGS with range
```bash
./GpuCracker --mode bsgs --bsgs-pub 0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798 --bsgs-range 40
```
Search 2^40 range for public key.

### 2. With specific m value
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 50 --bsgs-m 1048576
```
Set baby steps table size explicitly.

### 3. GPU-accelerated BSGS
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 60 --bsgs-gpu
```
Use GPU for giant steps.

### 4. CUDA specifically
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 60 --bsgs-gpu --gpu-type cuda
```

### 5. OpenCL for AMD
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 60 --bsgs-gpu --gpu-type opencl
```

### 6. With bloom filter (memory efficient)
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 70 --bsgs-bloom
```
10x less memory, ~1% FPP.

### 7. Custom bloom parameters
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 70 --bsgs-bloom --bloom-fpp 0.001
```

### 8. Multi-target from file
```bash
./GpuCracker --mode bsgs --bsgs-targets pubkeys.txt --bsgs-range 50
```
Search for multiple targets with shared baby steps.

### 9. With cache file
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 50 --bsgs-cache baby_steps_50.cache
```

### 10. Save cache only (precompute)
```bash
./GpuCracker --mode bsgs --bsgs-range 50 --bsgs-cache-only --bsgs-cache baby_steps_50.cache
```

## Advanced BSGS (11-30)

### 11. Random segment mode
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-mode random --bsgs-range 80
```
Random sampling for large ranges.

### 12. With segment bits
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-mode random --bsgs-range 80 --bsgs-segment-bits 20
```
2^20 size segments.

### 13. Distributed node 0
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 60 --bsgs-node-id 0 --bsgs-total-nodes 4
```

### 14. Distributed node 1
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 60 --bsgs-node-id 1 --bsgs-total-nodes 4
```

### 15. With bloom filter address check
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 50 --bsgs-bloom-keys addresses.blf
```

### 16. Custom start offset
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-start 0x10000000000 --bsgs-range 40
```

### 17. Specific threads
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 50 --bsgs-threads 32
```

### 18. Progress interval
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 50 --progress-interval 1000000
```

### 19. Silent mode
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 50 --silent --output result.txt
```

### 20. JSON output
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 50 --output-format json
```

### 21. With checkpoint
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 60 --checkpoint bsgs.chk
```

### 22. Resume from checkpoint
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --resume bsgs.chk
```

### 23. Multi-GPU setup
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 60 --bsgs-gpu --gpu-devices 0,1,2,3
```

### 24. GPU memory limit
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 60 --bsgs-gpu --gpu-mem 4G
```

### 25. CPU fallback
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 50 --cpu-fallback
```

### 26. With seed for random
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-mode random --bsgs-seed 12345
```

### 27. Adaptive m value
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 50 --adaptive-m
```

### 28. Memory-mapped cache
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 60 --mmap-cache
```

### 29. Compressed cache
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 60 --compress-cache
```

### 30. Cache validation
```bash
./GpuCracker --mode bsgs --bsgs-cache baby_steps.cache --validate-cache
```

## Expert BSGS (31-50)

### 31. Benchmark mode
```bash
./GpuCracker --mode bsgs --benchmark --bsgs-range 40
```

### 32. Estimate time
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 60 --estimate-only
```

### 33. Memory estimate
```bash
./GpuCracker --mode bsgs --bsgs-range 60 --memory-estimate
```

### 34. Optimal m calculation
```bash
./GpuCracker --mode bsgs --bsgs-range 60 --calculate-optimal-m
```

### 35. Test baby steps generation
```bash
./GpuCracker --mode bsgs --bsgs-range 40 --test-baby-steps
```

### 36. Verify cache integrity
```bash
./GpuCracker --mode bsgs --bsgs-cache cache.bin --verify-cache
```

### 37. Merge cache files
```bash
./GpuCracker --mode bsgs --merge-caches cache1.bin,cache2.bin --output merged.bin
```

### 38. Split cache for distribution
```bash
./GpuCracker --mode bsgs --bsgs-cache cache.bin --split 4 --output-prefix node
```

### 39. With public key file
```bash
./GpuCracker --mode bsgs --bsgs-pub-file pubkeys.txt --bsgs-range 50
```

### 40. Priority targets first
```bash
./GpuCracker --mode bsgs --bsgs-targets targets.txt --priority-file priority.txt
```

### 41. Dynamic range expansion
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 50 --dynamic-expand
```

### 42. Time-limited search
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 60 --time-limit 3600
```

### 43. Iteration limit
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 60 --iteration-limit 1000000
```

### 44. Hybrid threshold
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 70 --hybrid-threshold 8
```

### 45. Network distributed server
```bash
./GpuCracker --mode bsgs --server --port 12345 --bsgs-range 60
```

### 46. Network distributed client
```bash
./GpuCracker --mode bsgs --client --server 10.0.0.1:12345
```

### 47. With web dashboard
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 60 --web-dashboard --port 8080
```

### 48. Email notification
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 60 --email-on-found user@example.com
```

### 49. Webhook notification
```bash
./GpuCracker --mode bsgs --bsgs-pub <PUB> --bsgs-range 60 --webhook https://api.example.com/found
```

### 50. Complete BSGS deployment
```bash
./GpuCracker --mode bsgs \
  --bsgs-targets high_value_pubkeys.txt \
  --bsgs-range 60 \
  --bsgs-m 1073741824 \
  --bsgs-bloom \
  --bsgs-gpu \
  --gpu-type cuda \
  --gpu-devices 0,1,2,3 \
  --bsgs-node-id 0 \
  --bsgs-total-nodes 8 \
  --bsgs-cache bsgs_60.cache \
  --checkpoint bsgs_progress.chk \
  --progress-interval 10000000 \
  --web-dashboard \
  --port 8080 \
  --webhook https://secure.company.com/alert
```

## Performance Guide

### Optimal m Values
| Range | Optimal m | Memory (Standard) | Memory (Bloom) |
|-------|-----------|-------------------|----------------|
| 2^40 | 2^20 | 22 GB | 2.2 GB |
| 2^50 | 2^25 | 720 GB | 72 GB |
| 2^60 | 2^30 | 23 TB | 2.3 TB |

### GPU Speedup
| Hardware | Speedup |
|----------|---------|
| NVIDIA RTX 4090 | 100x |
| NVIDIA RTX 3090 | 80x |
| AMD RX 7900 XTX | 40x |
| Intel Arc A770 | 15x |

### Time Estimates (1M targets, 2^50 range)
| Configuration | Time |
|---------------|------|
| CPU only (32 cores) | ~50 days |
| 1x RTX 4090 | ~12 hours |
| 4x RTX 4090 | ~3 hours |
| 8 nodes × 4 GPU | ~20 minutes |

## Troubleshooting

### Out of Memory
```bash
# Use bloom filter
./GpuCracker --mode bsgs --bsgs-range 60 --bsgs-bloom

# Or reduce m value
./GpuCracker --mode bsgs --bsgs-range 50 --bsgs-m 524288
```

### Cache File Too Large
```bash
# Compress cache
./GpuCracker --mode bsgs --bsgs-cache cache.bin --compress-cache

# Or use memory mapping
./GpuCracker --mode bsgs --bsgs-cache cache.bin --mmap-cache
```

### Slow Performance
```bash
# Check GPU utilization
nvidia-smi dmon

# Optimize threads
./GpuCracker --mode bsgs --bsgs-range 50 --bsgs-threads 64
```
