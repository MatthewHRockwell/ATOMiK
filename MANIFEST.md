# ATOMiK Artifact Manifest

**Tag:** `zynq-adapter-v1` (commit cc9307e)
**Date:** 2026-04-08

## Build Environment

| Component | Version / Commit |
|-----------|-----------------|
| Vivado | v2025.2 (64-bit) |
| LiteX | 498d1fe82 |
| VexRiscvSMP pythondata | 0a97182 |
| Buildroot | 2023.02.5 (linux-on-litex external tree) |
| Linux kernel | 6.9.0 (rv32ima, buildroot-built) |
| OpenSBI | 1.3.1 (litex-hub fork, readl/writel + EV_PENDING fix) |
| Cross-compiler | riscv32-buildroot-linux-gnu-gcc 11.4.0 |
| Host | Kubuntu 24.04, Ryzen 7 5700U |

## Bitstreams

### April 5th SoC (proven Linux boot, CSR ATOMiK)
- **File:** `hardware/zynq/litex-build/build/gateware/hamgeek_rk7020f.bit`
- **Size:** 4,045,691 bytes
- **MD5:** `79927d3534011f9017d12e83ea2c4818`
- **Date:** 2026-04-05T18:27:18
- **Build:** `python3 -m litex_boards.targets.hamgeek_rk7020f --build --cpu-type=vexriscv_smp --cpu-variant=linux --cpu-count=1`
- **Target:** XC7Z020CLG484-2 (HamGeek RK-ZYNQ7020-F)
- **CPU:** VexRiscvSMP linux variant, 1 core, 100 MHz
- **ATOMiK:** Migen CSR module at 0xF0000000

### Adapter SoC (CSR + CFU adapter)
- **File:** `hardware/zynq/litex-build-adapter/gateware/hamgeek_rk7020f.bit`
- **Size:** 4,045,691 bytes
- **MD5:** `e6e8436b031d8f64fab24bc4a5fa9621`
- **Date:** 2026-04-07T11:25:28
- **Build:** `python3 hardware/zynq/litex/soc_smp_adapter.py --build --cpu-type=vexriscv_smp --cpu-variant=linux --cpu-count=1 --output-dir=litex-build-adapter`
- **ATOMiK CSR:** 0xF0000000 (Migen module)
- **ATOMiK Adapter:** 0xF0020000 (Wishbone-wrapped CFU adapter)
- **LUT:** 3,007 / 53,200 (5.65%), WNS: +0.523 ns

## Boot Images

| File | Size | MD5 | Description |
|------|------|-----|-------------|
| `litex-build/Image69` | 8,543,024 | `bd252da1e77228dc6e60ac9df246c455` | Linux 6.9.0 kernel (rv32ima, no compressed insns) |
| `litex-build/fw_jump69.bin` | 263,652 | `e00ae03bd93d64f6b945e61b6ec183f5` | OpenSBI 1.3.1 fw_jump (litex-hub fork) |
| `litex-build/rootfs69.cpio` | 8,539,648 | `9f6957a6eda9966913078b060ca13210` | BusyBox rootfs (buildroot, glibc 2.36) |
| `litex-build/linux69.dtb` | 2,624 | `3c061a5737a20ee5263944ca96b8738a` | DTB for CSR SoC (UART 0xF0001000) |
| `litex-build/rootfs69_demo.cpio` | 9,190,400 | `941383d61200db3cf5716c3c832bb787` | rootfs + demo binaries in /root/ |
| `litex-build/linux69_demo.dtb` | 2,624 | `e28e3e194d4035137d936ce95ab5d713` | DTB matching demo rootfs initrd-end |

## Demo Binaries (in rootfs69_demo.cpio at /root/)

| Binary | Source | Description |
|--------|--------|-------------|
| `demo_state_monitor` | `software/libatomik/demo_state_monitor.c` | Application-shaped demo (7/7 correct) |
| `bench_change_detect` | `hardware/zynq/test/bench_change_detect.c` | Three-way benchmark (CSR/adapter/SW) |
| `workload_change_detect` | `hardware/zynq/test/workload_change_detect.c` | Multi-buffer workload (15x-4,657x) |
| `example_atomik` | `software/libatomik/example_atomik.c` | libatomik API example |

## Captured Results

| File | Date | Description |
|------|------|-------------|
| `hardware/zynq/results/workload_csr_20260408.txt` | 2026-04-08 | CSR workload: 15x-4,657x speedup |
| `hardware/zynq/results/demo_state_monitor_20260408.txt` | 2026-04-08 | State monitor: 7/7 correct, up to 211x |

## JTAG Load Addresses

| Image | PS DDR Address | VexRiscv Address |
|-------|---------------|-----------------|
| Kernel (Image69) | 0x00100000 | 0x40000000 |
| Rootfs (rootfs69_demo.cpio) | 0x02100000 | 0x42000000 |
| DTB (linux69_demo.dtb) | 0x00FF0000 | 0x40EF0000 |
| OpenSBI (fw_jump69.bin) | 0x01000000 | 0x40F00000 |

## Critical Build Flags

- **`--cpu-variant=linux`**: REQUIRED for VexRiscvSMP. Without it, BIOS skips OpenSBI init and boot hangs after "Liftoff!" silently.
- **`--cpu-count=1`**: Single-core SMP (includes CLINT + PLIC).
