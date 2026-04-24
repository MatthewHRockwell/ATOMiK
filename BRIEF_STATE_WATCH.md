# ATOMiK State Watch Service

## Problem

Any system tracking mutable state — agent memory, edge sensor buffers, config blocks — needs to answer: "did anything change?" The software approach rescans every byte: **O(N x region_size)** per check cycle. As tracked state grows, monitoring becomes the bottleneck.

## What atomik-watchd Does

`atomik-watchd` monitors N memory regions and detects which changed each tick. It compares two approaches side-by-side:

- **Software**: `memcmp` each region against a shadow copy — O(N x size)
- **ATOMiK**: recompute XOR fingerprint, compare via hardware `acc_zero` — O(N x size) for fingerprint computation, O(1) for the comparison

Both methods are measured and reported per tick as JSON.

## The Production Model (Not Yet Implemented)

In production, deltas accumulate in ATOMiK hardware **at write time** (one `ACCUM` call per write). Detection then becomes a single `acc_zero` check per region — truly O(1) regardless of region size. This daemon demonstrates the detection model with full-rescan fingerprinting; write-time accumulation requires integration into the application's write path.

The earlier standalone benchmark (`bench_change_detect`) measured only the detection cost after pre-accumulated deltas, which is where the O(1) constant-time result comes from.

## Output Format

One JSON line per tick (pipe to jq, log aggregator, or dashboard):
```json
{"tick":1,"n_regions":8,"changed":[0,2],"n_changed":2,"detect_ticks":1234,"baseline_ticks":56789,"speedup":46.0}
```

Timer units: `ticks` (RISC-V rdtime @ 100 MHz) or `ns` (CLOCK_MONOTONIC on host).

## Related Benchmark Results (Live Zynq Hardware)

The standalone detection benchmark, which measures only the O(1) `acc_zero` check after pre-accumulated deltas, showed:

| Workload | Software | ATOMiK detect | Speedup |
|----------|----------|--------------|--------:|
| 8 regions x 4KB | 6,955,438 cy | 1,223 cy | **5,687x** |
| 64 regions x 4KB | 55,125,636 cy | 11,837 cy | **4,657x** |

These numbers reflect the production model where deltas are accumulated at write time — not the current daemon's full-rescan approach.

## Use Cases

- **Edge monitoring**: detect sensor/actuator state changes
- **Agent memory**: which agent state regions changed since last checkpoint
- **Config tracking**: detect modifications to tracked memory regions
- **Incremental sync**: identify dirty regions for selective transfer

## Runtime

Built on `libatomik` 1.0 (C, 3 hardware backends). Mock mode for development: compile with `-DATOMIK_MOCK`.

```
atomik-watchd -n 8 -s 4096 -t 10
```
