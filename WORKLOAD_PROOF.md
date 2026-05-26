# ATOMiK: Multi-Buffer Change Detection on Live Hardware

**Evidence label:** `LIVE_MEASURED` for the measurements listed below when they
are quoted with workload, platform, artifact, and caveat. This document supports
a workload-specific change-detection claim, not a universal performance,
battery, thermal, water, footprint, or production-readiness claim.

## What ATOMiK Is

ATOMiK is a state-aware compute architecture that reconstructs state as
`current = initial XOR accumulator`, where meaningful deltas are XORed into an
accumulator at update time. For this tracked-region change-detection workload,
ATOMiK checks whether each region changed without rescanning every byte of that
region.

## The Workload

**Track N memory regions. Detect which ones changed.**

This models a service, agent, edge node, or runtime monitoring state buffers
such as pages, config blocks, or sensor frames for changes. The software
baseline rescans every byte of every region. ATOMiK checks one change condition
per tracked region.

## The Result

Measured on live Zynq hardware: VexRiscv SMP at 100 MHz, Linux 6.9, libatomik
runtime.

### Direct CSR Path

| Workload | Software (memcmp) | ATOMiK (detect) | Speedup |
|----------|------------------:|----------------:|--------:|
| 8 regions x 256B, 25% changed | 20,316 cy | 1,238 cy | **16x** |
| 8 regions x 4KB, 25% changed | 6,925,319 cy | 1,226 cy | **5,649x** |
| 32 regions x 1KB, 10% changed | 679,582 cy | 9,200 cy | **74x** |
| 64 regions x 1KB, 5% changed | 1,373,630 cy | 5,878 cy | **234x** |
| 64 regions x 4KB, 5% changed | 55,323,599 cy | 11,830 cy | **4,677x** |

### Adapter Path (CFU Wishbone wrapper at 0xF0020000)

| Workload | Software (memcmp) | ATOMiK (detect) | Speedup | Overhead vs CSR |
|----------|------------------:|----------------:|--------:|:---------------:|
| 8 regions x 256B, 25% changed | 11,616 cy | 1,497 cy | **8x** | +21% |
| 8 regions x 4KB, 25% changed | 6,901,240 cy | 1,376 cy | **5,015x** | +12% |
| 32 regions x 1KB, 10% changed | 690,480 cy | 9,725 cy | **71x** | +6% |
| 64 regions x 1KB, 5% changed | 1,439,432 cy | 6,983 cy | **206x** | +19% |
| 64 regions x 4KB, 5% changed | 55,292,090 cy | 12,967 cy | **4,264x** | +10% |

Adapter path adds 6-21% overhead vs direct CSR in this measurement set while
preserving the region-count scaling behavior for this workload.

## Why The Speedup Grows In This Benchmark

Software change detection in this baseline is `O(N x region_size)` because it
rescans every byte of every region. As regions get larger, software cost grows
quickly in this measurement set.

ATOMiK detection in this workload is `O(N)` because it checks one hardware-backed
condition per tracked region. At 8 regions, detection costs about 1,200 cycles
whether regions are 256 bytes or 4KB in the direct CSR measurement.

**The key insight:** in this benchmark, when tracked region size grows while the
number of tracked regions stays fixed, the software scan cost grows and the
ATOMiK detection path remains tied primarily to region count.

## Why This Matters

This result is relevant when the customer workload spends meaningful cost on
repeated scans or full-region comparisons. Candidate areas include:

- **Edge computing:** detect sensor or actuator state changes without rescanning
  full buffers.
- **Agent memory:** identify which state regions changed since last checkpoint.
- **Incremental sync:** identify dirty pages or blocks before transfer.
- **Monitoring:** track integrity or tamper-related state with a clear threat
  model and measurement plan.

Power, thermal, water, footprint, security, and product-readiness outcomes need
separate instrumentation and evidence before they are public claims.

## Validation Stack

- **Hardware:** Zynq XC7Z020, VexRiscv SMP at 100 MHz, 512 MB DDR3.
- **Software:** Linux 6.9.0, BusyBox rootfs, libatomik C runtime.
- **ATOMiK core:** Migen CSR module, 256x64-bit state table, single bank.
- **Adapter path:** CFU adapter validated separately with its own artifacts.
- **Formal proof work:** Lean4 proof work exists in `math/proofs/`; avoid public
  theorem counts unless they are audited across the site, README, deck, and
  proof packet.

## Reproduce

```bash
cd ~/Projects/ATOMiK
git checkout 10c47e8
./hardware/zynq/scripts/demo_run.sh
```
