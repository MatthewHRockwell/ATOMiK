# ATOMiK State Watch Service

## Problem

Any system tracking mutable state — agent memory, edge sensor buffers, config blocks — needs to answer: "did anything change?" The software approach rescans every byte: **O(N x region_size)** per check cycle. As tracked state grows, monitoring becomes the bottleneck.

## Solution

`atomik-watchd` uses ATOMiK hardware to make change detection **O(1) per region** regardless of region size. Deltas accumulate in hardware at write time. Detection is a single register read per region.

## How It Works

```
Every tick:
  For each monitored region:
    ATOMiK: check one hardware flag → changed or not  (O(1))
    Software: rescan all bytes against shadow copy     (O(size))
  Emit JSON: which regions changed, detection latency, speedup
```

## Live Hardware Result

Captured on Zynq XC7Z020 (VexRiscv SMP @ 100 MHz, Linux 6.9):

| Workload | Software | ATOMiK | Speedup |
|----------|----------|--------|--------:|
| 8 regions x 4KB, 25% changed | 6,955,438 cy | 1,223 cy | **5,687x** |
| 64 regions x 4KB, 5% changed | 55,125,636 cy | 11,837 cy | **4,657x** |
| 64KB region, no change | 13,932,657 cy | 65,846 cy | **212x** |

Software cost grows with region size. ATOMiK cost is constant.

## Output Format

One JSON line per tick (pipe to jq, log aggregator, or dashboard):
```json
{"tick":1,"n_regions":8,"changed":[0,2],"n_changed":2,"detect_cy":1234,"baseline_cy":56789,"speedup":46.0}
```

## Use Cases

- **Edge monitoring**: detect sensor/actuator state changes without rescanning
- **Agent memory**: know which agent state regions changed since last checkpoint
- **Config tracking**: constant-time tamper detection on tracked memory
- **Incremental sync**: identify dirty regions for selective transfer

## Runtime

Built on `libatomik` 1.0 (C, 3 hardware backends). Runs on Linux via `/dev/mem`. No kernel module required. Mock mode for development without hardware.

```
atomik-watchd -n 8 -s 4096 -t 10
```
