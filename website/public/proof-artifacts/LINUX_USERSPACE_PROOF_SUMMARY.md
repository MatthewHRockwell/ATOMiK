# Linux Userspace-to-FPGA Proof Summary

Evidence label: HARDWARE_VALIDATED

## What Was Tested

ATOMiK was exercised from a Linux userspace process running on a RISC-V VexRiscv SMP soft-core inside a Xilinx Zynq XC7Z020 FPGA.

Tested path:

`User process -> Linux 6.9 kernel -> /dev/mem mmap -> Wishbone CSR bus -> ATOMiK core`

## Environment

- Board: HamGeek RK-ZYNQ7020-F / Xilinx XC7Z020-2CLG484I
- Soft CPU: VexRiscv SMP, rv32ima, Sv32 MMU, 100 MHz
- Kernel: Linux 6.9.0, Buildroot 2023.02.5 plus linux-on-litex patches
- ATOMiK core: LiteX Migen CSR module, single-bank, 256 x 64-bit state table
- Baseline tag: zynq-linux-v1
- Baseline context: see `ZYNQ_BASELINE.md`

## Result

- 16/16 algebraic property checks passed.
- Zero failures recorded.
- Console output reported `DDR OK`, `CSR OK`, and `DONE`.
- JTAG readback confirmed result buffer values including pass=16 and fail=0.

## Ordering Requirement Discovered

CSR writes require `fence iorw, iorw` plus a dummy STATUS read before reading STATE_LO/HI. Without the ordering barrier, the first state read after LOAD can return stale data because of Wishbone bus pipeline latency.

## What This Proves

- The ATOMiK primitive path can be reached from Linux userspace on the documented FPGA configuration.
- The algebraic checks survived a real OS/software/hardware path.
- A real MMIO ordering requirement was discovered and documented.

## What This Does Not Prove

- Customer workload value.
- Battery, heat, cooling, water, power-bill, footprint, or customer savings.
- Production readiness.
- Universal speedup.
- Future SD-boot workload validation.

## Reproduction Command

See `LINUX_USERSPACE_PROOF.md` and `ZYNQ_BASELINE.md` for full reproduction context.

Short version:

```bash
cd hardware/zynq
make boot-fast
# Serial terminal:
boot 0x40f00000
root
mknod /dev/mem c 1 1
/root/atomik_test
```

Use with caveat: reproduce on the documented frozen baseline, and preserve the MMIO ordering requirement.
