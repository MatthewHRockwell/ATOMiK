# ATOMiK: Linux Userspace Validation Proof

**Date**: April 6, 2026
**Baseline tag**: [`zynq-linux-v1`](https://github.com/MatthewHRockwell/ATOMiK/releases/tag/zynq-linux-v1)

## What was tested

The ATOMiK delta-state accelerator was exercised from a Linux userspace
process running on a RISC-V soft-core (VexRiscv SMP) inside a Xilinx Zynq
XC7Z020 FPGA. The test verifies the full path:

```
User process (S-mode) → Linux 6.9 kernel → /dev/mem mmap → Wishbone CSR bus → ATOMiK core
```

This is the first validation of ATOMiK through a real operating system with
virtual memory, page tables, and supervisor-mode privilege separation.
Previous hardware validations were performed from bare-metal firmware
(M-mode, no MMU).

## What passed

All 16 algebraic property checks passed with zero failures:

| Test | Operation | Property |
|------|-----------|----------|
| T1 | STATUS read | Version = 2, banks = 1 |
| T2 | CONFIG read | Core enabled |
| T3 | LOAD + READ | State stored and reconstructed correctly |
| T4 | ACCUM + READ | XOR delta applied to state |
| T5 | Double ACCUM | Self-inverse: A ⊕ B ⊕ B = A |
| T6 | Zero ACCUM | Identity: A ⊕ 0 = A |
| T7 | Cross ACCUM | Commutativity: A ⊕ B = B ⊕ A |
| T8 | Status flag | Accumulator-zero flag correct |
| T9 | Address 42 | Independent state table entries |

Results were verified via two independent paths:
- **Console output**: Test binary printed `DDR OK`, `CSR OK`, `DONE`
- **JTAG readback**: Result buffer at DDR 0x20000000 confirmed `pass=16, fail=0`

## Ordering constraint discovered

A `fence iorw, iorw` instruction plus a dummy STATUS register read is
required between CSR write operations and subsequent state reads. Without
this ordering barrier, the first STATE_LO read after a LOAD returns stale
data due to Wishbone bus pipeline latency.

This is a real MMIO ordering requirement, not a test artifact. It applies to
any software (driver, runtime, benchmark) that accesses ATOMiK CSRs from
Linux userspace or kernel space on VexRiscv SMP.

## Environment

| Component | Detail |
|-----------|--------|
| FPGA | Xilinx XC7Z020-2CLG484I (HamGeek RK-ZYNQ7020-F) |
| Soft CPU | VexRiscv SMP, rv32ima, Sv32 MMU, 100 MHz |
| ATOMiK | LiteX Migen CSR module, single-bank, 256×64-bit state table |
| Kernel | Linux 6.9.0 (Buildroot 2023.02.5 + 20 linux-on-litex patches) |
| OpenSBI | 1.3.1 (litex-hub fork, UART readl/writel fix) |
| Rootfs | BusyBox 1.36.1, glibc 2.36, rv32ima |
| Console | LiteUART direct (ttyLXU0, IRQ 12) |
| Test binary | 2,724 bytes, nostdlib, raw Linux syscalls |

## Reproduction

```bash
cd hardware/zynq

# Program FPGA and load images via JTAG (~10s)
make boot-fast

# At serial terminal:
boot 0x40f00000              # Boot Linux
root                         # Login
mknod /dev/mem c 1 1         # Create device node
/root/atomik_test            # Run test — expect: DDR OK, CSR OK, DONE
```

SHA-256 checksums for all images are recorded in
[`hardware/zynq/BASELINE.md`](../hardware/zynq/BASELINE.md).

## Benchmark: Multi-Buffer Change Detection

After algebraic correctness was confirmed, a workload benchmark measured
ATOMiK's change detection against software memcmp across tracked memory
regions. ATOMiK detection uses a single register read (O(1) per region);
memcmp scans the full buffer (O(n) per region).

| Workload | Software (memcmp) | ATOMiK (detect) | Speedup |
|----------|-------------------|-----------------|---------|
| 8 x 256B, 25% changed | 16,572 cy | 1,107 cy | 15x |
| 8 x 1KB, 25% changed | 157,539 cy | 1,088 cy | 145x |
| 8 x 4KB, 25% changed | 6,539,617 cy | 1,093 cy | 5,983x |
| 32 x 256B, 10% changed | 174,263 cy | 2,874 cy | 61x |
| 64 x 1KB, 5% changed | 1,314,615 cy | 5,243 cy | 251x |

ATOMiK detection time scales with **region count**, not region size — the
8x4KB workload takes the same ~1,090 cycles as 8x256B. Software memcmp
scales with N * size. The sharp memcmp degradation above 4KB is the D-cache
boundary (VexRiscv SMP: 4KB D-cache, no L2).

ATOMiK monitoring rate: **1.2 million regions/second** at 64 contexts.

## Significance

This result proves ATOMiK's delta-state algebra survives the full
hardware/software stack: user process → kernel virtual memory → physical bus
→ FPGA accelerator, and delivers measurable performance advantages on a
real change-detection workload.
