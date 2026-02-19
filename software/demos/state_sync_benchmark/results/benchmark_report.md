# ATOMiK State Sync Benchmark Report

*Generated: 2026-02-15 20:14 UTC*

## Summary

| Scenario | ATOMiK Advantage | Key Metric |
|----------|-----------------|------------|
| Sensor Fusion (N=16 streams) | 5.3x faster | 3,143,345 ops/s |
| Rollback (10K deltas) | 1 op vs 100 ops | 6.1 us |
| Distributed Sync (N nodes) | 8 msgs vs 10000 msgs | Order-independent |
| Memory Traffic | 83.1% reduction | 135,284 bytes |

## Scenarios

### Sensor Fusion (N=16 streams)

- **atomik_ops_sec**: 3143345.250195686
- **conventional_ops_sec**: 587650.3849298131
- **atomik_bytes**: 800000
- **conventional_bytes**: 1600000
- **speedup**: 5.349005685703951
- **n_streams**: 16
- **n_updates**: 100000

### Rollback (10K deltas)

- **atomik_undo_ops**: 1
- **conventional_undo_ops**: 100
- **atomik_latency_us**: 6.14600139670074
- **conventional_latency_us**: 100.22099013440311
- **n_deltas**: 10000
- **rollback_steps**: 1

### Distributed Sync (N nodes)

- **atomik_messages**: 8
- **conventional_messages**: 10000
- **atomik_correct**: True
- **conventional_correct**: True
- **atomik_time_us**: 2798.0120066786185
- **conventional_time_us**: 1849023.4249911737
- **n_nodes**: 8
- **n_updates**: 10000

### Memory Traffic

- **atomik_bytes_written**: 135284
- **conventional_bytes_written**: 800000
- **atomik_parallel_bytes**: 135284
- **reduction_pct**: 83.08949999999999
- **n_updates**: 100000
- **state_width**: 64
- **sparsity**: 0.95
