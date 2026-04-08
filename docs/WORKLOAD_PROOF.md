# ATOMiK: Multi-Buffer Change Detection on Live Hardware

## What ATOMiK Is

ATOMiK is a hardware delta-state accelerator. It reconstructs state as `current = initial ⊕ accumulator`, where deltas are XORed into the accumulator at write time. Detecting whether state changed is a single register read — O(1) regardless of how much data is tracked.

## The Workload

**Track N memory regions. Detect which ones changed.**

This models a real service: an agent, edge node, or runtime monitoring state buffers (pages, config blocks, sensor frames) for changes. The software baseline rescans every byte of every region. ATOMiK checks one flag per region.

## The Result

Measured on live Zynq hardware: VexRiscv SMP at 100 MHz, Linux 6.9, libatomik runtime.

| Workload | Software (memcmp) | ATOMiK (detect) | Speedup | Throughput |
|----------|------------------:|----------------:|--------:|-----------:|
| 8 regions x 256B, 25% changed | 18,435 cy | 1,221 cy | **15x** | 655K reg/s |
| 8 regions x 4KB, 25% changed | 6,955,438 cy | 1,223 cy | **5,687x** | 654K reg/s |
| 32 regions x 1KB, 10% changed | 696,709 cy | 3,225 cy | **216x** | 992K reg/s |
| 64 regions x 1KB, 5% changed | 1,392,901 cy | 5,881 cy | **237x** | 1.1M reg/s |
| 64 regions x 4KB, 5% changed | 55,125,636 cy | 11,837 cy | **4,657x** | 541K reg/s |

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
- **Formal**: 92 Lean4 theorems proving the delta-state algebra (commutativity, associativity, self-inverse, identity)

## Reproduce

```
cd ~/Projects/ATOMiK
git checkout 10c47e8
# Cold-start board, then:
./hardware/zynq/scripts/demo_run.sh
```
