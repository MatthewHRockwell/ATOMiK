# ATOMiK Parallel-Bank Throughput — Validated on Hardware

**Date:** 2026-05-30
**Board:** HamGeek RK-ZYNQ7020-F (XC7Z020-2CLG484), NaxRiscv RV64 + LiteX, Ubuntu RV64
**Bitstream:** `litex-build-nax64-bench/gateware/hamgeek_rk7020f.bit`
(1080p30 HDMI framebuffer + 8-bank ATOMiK bench engine, 64.5% LUT, timing met)
**Engine:** `atomik_parallel_bench` @ MMIO `0xF0021000` (separate from the validated
single-bank adapter at `0xF0020000`, which is untouched)

## What this measures

The engine accumulates `count` deterministic deltas across `active_banks` parallel XOR
banks, feeding `active_banks` deltas every cycle, and reports the **exact number of
hardware cycles** the accumulation loop took (read from an on-chip cycle counter, not
estimated). Each delta `d(i) = xorshift3(seed ^ i)` is a stateless function of its index,
so the accumulated XOR is **independent of bank count** — only the cycle count changes.
The on-board tool recomputes the same XOR in software and asserts the hardware result
matches. This makes the throughput curve both real and self-verifying.

## Measured results (raw board output)

### count = 65,537 — seed 0xDEADBEEF0BADF00D
```
banks  hw_cycles   speedup  result              ok
1      65537       1.00     0x17e9fe29b5dc0aad  OK
2      32769       1.99     0x17e9fe29b5dc0aad  OK
4      16385       3.99     0x17e9fe29b5dc0aad  OK
8      8193        7.99     0x17e9fe29b5dc0aad  OK
VERIFY: ALL MATCH
```

### count = 1,000,003 — seed 0xA70A1CBA12345678
```
banks  hw_cycles   speedup  result              ok
1      1000003     1.00     0x2dd570fa93fb0cd3  OK
2      500002      1.99     0x2dd570fa93fb0cd3  OK
4      250001      3.99     0x2dd570fa93fb0cd3  OK
8      125001      7.99     0x2dd570fa93fb0cd3  OK
VERIFY: ALL MATCH
```

### count = 16,000,000 — seed 0xA70A1CBA12345678
```
banks  hw_cycles   speedup  result              ok
1      16000000    1.00     0x0000000000000000  OK
2      8000000     2.00     0x0000000000000000  OK
4      4000000     4.00     0x0000000000000000  OK
8      2000000     8.00     0x0000000000000000  OK
VERIFY: ALL MATCH
```
(The =0 result is mathematically exact: xorshift is linear over GF(2), so the XOR of a
contiguous index range cancels for counts divisible by the relevant power of two; the
software reference computes the same 0, confirming correctness. The 16M count is divisible
by 8, so there is no partial final cycle and the speedup is a clean **8.00×**.)

## Honest interpretation

- **Throughput scales linearly with allocated banks, measured on silicon.** 8 banks
  complete the same accumulation in 1/8 the cycles of 1 bank. At the 100 MHz `sys` clock
  this is **100 Mdeltas/s (1 bank) → 800 Mdeltas/s (8 banks)**, consistent with the
  synthesis sweep's prediction (`hardware/sweep/sweep_results.json`, N=8 ≈ 756 Mops/s est).
- The result is **identical across bank counts and matches software** in every run — banks
  change *how fast*, never *the answer*. This is the literal hardware embodiment of the
  delta-state property that the accumulator is an order-independent shared resource.
- This is the honest, demonstrable core of "dynamic per-workload resource allocation":
  `active_banks` is a single MMIO write, and the cycle count it produces is real.

## Scope / honesty boundary

- This is the **bench engine** (a measurement instrument). The single-bank application
  adapter at `0xF0020000` is unchanged and still drives `atomik_os`.
- A full closed-loop allocator (a scheduler that shifts banks toward the hottest workload
  from live `perf_*` telemetry) is the next step; this artifact proves the substrate.

## Reproduce

1. Boot: `ATOMIK_BITSTREAM=.../litex-build-nax64-bench/gateware/hamgeek_rk7020f.bit \
   python3 hardware/zynq/fsbl_build/jtag_load_all_then_boot.py`
2. Sweep: `ATOMIK_PORT=/dev/ttyUSB2 python3 hardware/zynq/test/run_bench_on_board.py [count] [seed_hex]`
   (creates `/dev/mem` if missing, uploads `atomik_bench_sweep_tiny`, runs, verifies)

RTL: `hardware/zynq/rtl/atomik_parallel_bench.v` wrapping
`hardware/rtl/atomik_parallel_acc.v` + `atomik_delta_acc.v`.
Sim: `hardware/zynq/sim/tb_parallel_bench.v` (all pass: invariance + cycle scaling + ref).
