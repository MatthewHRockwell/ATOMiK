# ATOMiK Zynq Linux Userspace Validation — Frozen Baseline

**Tag**: `zynq-linux-v1`
**Date**: 2026-04-06
**Commit**: `9ddfd61f5ebd21de9387fa8231388aeaa4389609`
**Result**: 16/16 PASS

## Validated Configuration

| Component | Version | Details |
|-----------|---------|---------|
| **FPGA** | XC7Z020-2CLG484I | HamGeek RK-ZYNQ7020-F |
| **Soft CPU** | VexRiscv SMP | rv32ima, Sv32 MMU, 4KB D$/I$, 100 MHz |
| **ATOMiK** | v2 (LiteX Migen CSR) | Single-bank, 256x64-bit state table |
| **Linux kernel** | 6.9.0 | Buildroot 2023.02.5 + linux-on-litex patches |
| **OpenSBI** | 1.3.1 | litex-hub fork (readl/writel + EV_PENDING fix) |
| **Rootfs** | BusyBox 1.36.1 | glibc 2.36, rv32ima, /sbin/init |
| **Toolchain** | GCC 11.4.0 | riscv32-buildroot-linux-gnu |
| **Test binary** | atomik_test_linux | nostdlib, raw syscalls, -Wl,--no-relax |

## Image SHA-256 Checksums

```
d17ca5db  Image69           (8,543,024 bytes)  Linux 6.9 kernel
635ce1e1  rootfs69.cpio     (6,599,680 bytes)  BusyBox + atomik_test
f9ecc91f  fw_jump69.bin       (263,652 bytes)  OpenSBI 1.3.1
d24a5963  linux69.dtb           (2,624 bytes)  Device tree
e2a3babb  atomik_test_linux     (2,724 bytes)  Userspace test binary
df12fe93  hamgeek_rk7020f.bit (4,045,691 bytes) FPGA bitstream
```

## Boot Configuration

```json
{
    "Image69":        "0x40000000",
    "rootfs69.cpio":  "0x42000000",
    "linux69.dtb":    "0x40ef0000",
    "fw_jump69.bin":  "0x40f00000"
}
```

Bootargs: `console=liteuart0 earlycon=liteuart,0xf0001000 rdinit=/sbin/init`

## Test Results (16/16 PASS)

| # | Test | Operation | Result |
|---|------|-----------|--------|
| T1 | Status register | version=2, n_banks=1 | PASS |
| T2 | Config register | core_enable=1 | PASS |
| T3 | LOAD + READ | state = 0xCAFEBABE_DEADBEEF | PASS |
| T4 | ACCUM + READ | XOR delta applied correctly | PASS |
| T5 | Self-inverse | A XOR B XOR B = A | PASS |
| T6 | Identity | A XOR 0 = A | PASS |
| T7 | Commutativity | A XOR B = B XOR A | PASS |
| T8 | Accumulator zero | acc_zero flag correct | PASS |
| T9 | Multi-address | Address 42 independent of address 0 | PASS |

## MMIO Ordering Requirement

CSR writes via Wishbone bus require a `fence iorw, iorw` + dummy STATUS read
before reading STATE_LO/HI. Without this, the first state read after a LOAD
returns stale data (bus pipeline latency).

## Reproduction

```bash
cd hardware/zynq

# 1. Program FPGA
xsdb scripts/jtag_load_images.tcl 69    # after FPGA is programmed

# 2. Boot (serial terminal)
boot 0x40f00000

# 3. Login and run test
root
mknod /dev/mem c 1 1; chmod 666 /dev/mem
/root/atomik_test

# 4. Verify via JTAG readback
# Result buffer at PS DDR 0x20000000: magic=0x41544F4D, pass=16, fail=0
```

## Build Locations (not in git)

| Artifact | Path |
|----------|------|
| Buildroot tree | `/home/mattrock/buildroot-litex/` |
| External tree | `/home/mattrock/linux-on-litex-external/` |
| Kernel config | `output/build/linux-6.9/.config` |
| BusyBox config | `output/build/busybox-1.36.1/.config` (CONFIG_TC=n) |
