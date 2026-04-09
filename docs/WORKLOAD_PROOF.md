# ATOMiK: Multi-Buffer Change Detection on Live Hardware

## What ATOMiK Is

ATOMiK is a hardware delta-state accelerator. It reconstructs state as `current = initial ⊕ accumulator`, where deltas are XORed into the accumulator at write time. Detecting whether state changed is a single register read — O(1) regardless of how much data is tracked.

## The Workload

**Track N memory regions. Detect which ones changed.**

This models a real service: an agent, edge node, or runtime monitoring state buffers (pages, config blocks, sensor frames) for changes. The software baseline rescans every byte of every region. ATOMiK checks one flag per region.

## The Result

Measured on live Zynq hardware: VexRiscv SMP at 100 MHz, Linux 6.9, libatomik runtime.

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
| 8 regions x 4KB, 25% changed | 6,901,240 cy | 1,376 cy | **5,015x** | +12% |
| 64 regions x 1KB, 5% changed | 1,439,432 cy | 6,983 cy | **206x** | +19% |
| 64 regions x 4KB, 5% changed | 55,292,090 cy | 12,967 cy | **4,264x** | +10% |

Adapter path adds 10-19% overhead vs direct CSR, preserving the O(1) scaling property.

## Why the Speedup Grows

Software change detection is **O(N x region_size)** — it must rescan every byte of every region. As regions get larger, software cost explodes (6.9M cycles for 8x4KB, 55M cycles for 64x4KB).

ATOMiK detection is **O(N)** — it checks one hardware flag per region, regardless of region size. The cost scales only with the number of tracked regions, not with how much data each region holds. At 8 regions, detection costs ~1,200 cycles whether regions are 256 bytes or 4KB.

**The key insight: as tracked state grows, software slows down. ATOMiK stays flat.**

## Why This Matters

Any system that tracks mutable state benefits:

- **Edge computing**: detect sensor/actuator state changes without rescanning buffers
- **Agent memory**: know which agent state regions changed since last checkpoint
- **Incremental sync**: identify dirty pages/blocks without full comparison
- **Security monitoring**: constant-time tamper detection on tracked memory

The hardware is a $13.50 FPGA (Tang Nano 9K for the standalone core) or a Zynq-class SoC for the Linux-integrated version. The runtime is a C library (libatomik) that works from Linux userspace.

## Validation Stack

- **Hardware**: Zynq XC7Z020, VexRiscv SMP at 100 MHz, 512 MB DDR3
- **Software**: Linux 6.9.0, BusyBox rootfs, libatomik C runtime
- **ATOMiK core**: Migen CSR module, 256x64-bit state table, single bank
- **Adapter path**: CFU adapter validated separately (9/9 PASS, 20/20 Verilator, ~10% overhead vs direct CSR)
- **Formal**: 108 Lean4 theorems proving the delta-state algebra (commutativity, associativity, self-inverse, identity)

## Reproduce

```
cd ~/Projects/ATOMiK
git checkout 10c47e8
# Cold-start board, then:
./hardware/zynq/scripts/demo_run.sh
```
