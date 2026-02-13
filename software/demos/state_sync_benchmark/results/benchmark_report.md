# ATOMiK State Sync Benchmark Report

*Generated: 2026-02-03 03:44 UTC*

## Summary

| Scenario | ATOMiK Advantage | Key Metric |
|----------|-----------------|------------|
| Sensor Fusion (N=16 streams) | 1.1x faster | 2,856,800 ops/s |
| Rollback (10K deltas) | 1 op vs 100 ops | 3.7 us |
| Distributed Sync (N nodes) | 8 msgs vs 10000 msgs | Order-independent |
| Memory Traffic | 83.1% reduction | 135,284 bytes |

## Scenarios

### Sensor Fusion (N=16 streams)

- **atomik_ops_sec**: 2856800.042993339
- **conventional_ops_sec**: 2673725.300841617
- **atomik_bytes**: 800000
- **conventional_bytes**: 1600000
- **speedup**: 1.0684717843280664
- **n_streams**: 16
- **n_updates**: 100000

### Rollback (10K deltas)

- **atomik_undo_ops**: 1
- **conventional_undo_ops**: 100
- **atomik_latency_us**: 3.6999990697950125
- **conventional_latency_us**: 63.99999256245792
- **n_deltas**: 10000
- **rollback_steps**: 1

### Distributed Sync (N nodes)

- **atomik_messages**: 8
- **conventional_messages**: 10000
- **atomik_correct**: True
- **conventional_correct**: True
- **atomik_time_us**: 3041.4999928325415
- **conventional_time_us**: 2675411.3000060897
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
