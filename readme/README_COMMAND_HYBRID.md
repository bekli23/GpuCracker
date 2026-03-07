# HYBRID Mode - 50+ Command Examples

## Basic Hybrid Operations (1-10)

### 1. Auto-select algorithm
```bash
./GpuCracker --mode hybrid --target-pub 0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798 --search-range 60
```

### 2. Prefer BSGS
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 50 --prefer-bsgs
```

### 3. Prefer Rho
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 90 --prefer-rho
```

### 4. With GPU
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 70 --use-gpu
```

### 5. Multi-target hybrid
```bash
./GpuCracker --mode hybrid --targets targets.txt --search-range 60
```

### 6. Memory-aware
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 70 --memory-limit 32G
```

### 7. Bloom threshold
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 65 --bloom-threshold 64
```

### 8. Dynamic switching
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 75 --dynamic-switch
```

### 9. Algorithm benchmark
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 60 --benchmark-algorithms
```

### 10. Force algorithm
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 50 --force-algorithm bsgs
```

## Intermediate Hybrid (11-30)

### 11. BSGS bloom hybrid
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 70 --bsgs-bloom --rho-instances 64
```

### 12. Checkpoint for both
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 60 --checkpoint hybrid.chk
```

### 13. Resume hybrid
```bash
./GpuCracker --mode hybrid --resume hybrid.chk
```

### 14. Multi-GPU hybrid
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 80 --use-gpu --gpu-devices 0,1,2,3
```

### 15. CPU/GPU balance
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 70 --cpu-ratio 0.3 --gpu-ratio 0.7
```

### 16. Adaptive range
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 60 --adaptive-range --expand-if-needed
```

### 17. Priority queue
```bash
./GpuCracker --mode hybrid --targets targets.txt --search-range 60 --priority-by value
```

### 18. Time-based switch
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 70 --bsgs-timeout 3600
```

### 19. Memory pressure switch
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 65 --oom-switch-to-rho
```

### 20. Combined statistics
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 60 --combined-stats
```

### 21. Algorithm comparison
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 50 --compare-bsgs-rho
```

### 22. Optimal parameters
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 60 --auto-optimize
```

### 23. Parallel hybrid
```bash
./GpuCracker --mode hybrid --targets targets.txt --search-range 60 --parallel-algorithms
```

### 24. BSGS with Rho fallback
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 70 --bsgs-primary --rho-fallback
```

### 25. GPU-CPU coordination
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 75 --gpu-bsgs --cpu-rho
```

### 26. Multi-node hybrid
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 80 --distributed --node-id 0 --total-nodes 4
```

### 27. Intelligent partition
```bash
./GpuCracker --mode hybrid --targets targets.txt --search-range 70 --intelligent-partition
```

### 28. Resource monitoring
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 60 --monitor-resources
```

### 29. Auto-tune batch size
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 60 --auto-tune-batch
```

### 30. Cache management
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 60 --cache-dir ./cache --cache-size 100G
```

## Advanced Hybrid (31-50)

### 31. Hybrid with bloom
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 70 --use-bloom --bloom-fpp 0.001
```

### 32. Distributed bloom
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 80 --distributed-bloom
```

### 33. Multi-level hybrid
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 100 --multi-level
```
BSGS → Bloom BSGS → Rho cascade.

### 34. Smart checkpoint
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 70 --smart-checkpoint --checkpoint-interval 300
```

### 35. Progress estimation
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 60 --estimate-progress
```

### 36. Result aggregation
```bash
./GpuCracker --mode hybrid --targets targets.txt --search-range 60 --aggregate-results
```

### 37. Load balancing
```bash
./GpuCracker --mode hybrid --targets targets.txt --search-range 70 --dynamic-load-balance
```

### 38. Thermal-aware
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 80 --thermal-aware --max-temp 75
```

### 39. Power-aware
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 80 --power-aware --max-power 300
```

### 40. Network coordinated
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 90 --network-coordination --coordinator 10.0.0.1:9000
```

### 41. Web dashboard
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 70 --web-dashboard --port 8080
```

### 42. Algorithm telemetry
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 60 --telemetry --telemetry-file telemetry.json
```

### 43. Performance profiling
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 60 --profile --profile-output profile.txt
```

### 44. Cost optimization
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 80 --cost-optimize --cost-per-hour 2.5
```

### 45. Deadline aware
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 70 --deadline "2024-12-31T23:59:59"
```

### 46. Success probability
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 60 --calculate-probability
```

### 47. Monte carlo estimate
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 70 --monte-carlo-simulation 10000
```

### 48. Historical analysis
```bash
./GpuCracker --mode hybrid --analyze-historical --historical-data history.json
```

### 49. Predictive switching
```bash
./GpuCracker --mode hybrid --target-pub <PUB> --search-range 75 --predictive-switch --prediction-model ml
```

### 50. Enterprise hybrid deployment
```bash
./GpuCracker --mode hybrid \
  --targets high_value_pubkeys.txt \
  --search-range 80 \
  --prefer-bsgs \
  --use-gpu \
  --gpu-devices 0,1,2,3,4,5,6,7 \
  --memory-limit 128G \
  --dynamic-switch \
  --oom-switch-to-rho \
  --distributed \
  --node-id 0 \
  --total-nodes 8 \
  --cache-dir /fast/cache \
  --smart-checkpoint \
  --monitor-resources \
  --thermal-aware \
  --max-temp 80 \
  --web-dashboard \
  --port 8080 \
  --telemetry \
  --aggregate-results \
  --webhook https://secure.company.com/alert
```

## Decision Matrix

| Scenario | Recommended Algorithm | Hybrid Action |
|----------|----------------------|---------------|
| Range 2^40, 32GB RAM | BSGS | Auto-select BSGS |
| Range 2^60, 128GB RAM | BSGS | Auto-select BSGS |
| Range 2^70, 32GB RAM | Bloom BSGS | Auto-select Bloom |
| Range 2^80, 64GB RAM | Rho | Auto-switch to Rho |
| Range 2^100, any RAM | Rho | Force Rho |
| Multi-target (100+) | BSGS | Prefer BSGS |
| Single target, huge | Rho | Prefer Rho |
| Time critical (<1h) | BSGS | Force BSGS |

## Performance Characteristics

### BSGS Characteristics
- **Pros**: Deterministic, multi-target efficient, fast iterations
- **Cons**: Memory intensive, limited range size

### Rho Characteristics
- **Pros**: Memory efficient, unlimited range, parallelizable
- **Cons**: Probabilistic, single target focus, more iterations

### Hybrid Advantages
- Automatic optimal selection
- Graceful degradation
- Resource-aware scheduling
- Unified interface

## Monitoring

The hybrid mode provides unified statistics:
```
[HYBRID] Selected algorithm: BSGS
[HYBRID] Estimated memory: 45GB
[HYBRID] Progress: 15.3%
[HYBRID] ETA: 4h 23m
[HYBRID] Can switch to Rho if needed
```
