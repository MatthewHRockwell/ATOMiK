# Phase 9.1 — JTAG-direct boot working (2026-04-15)

Authoritative record of the exact configuration that gets Ubuntu to a root
prompt via JTAG-direct DDR load in ~155 s (vs 44 min SFL = 17× end-to-end).

## Entry point

```
python3 /home/mattrock/Projects/ATOMiK/hardware/zynq/ps_loader/jtag_boot.py
```

That script:
1. Runs one xsdb session: rst-system → ps7_init → fpga -file (PATCHED bitstream)
   → ps7_post_config → stop APU core 0 → **`after 5000`** → 5× `dow -data` → con
2. Opens /dev/ttyUSB2 @ 921600, sends `boot 0x40a00000\n`, passthroughs console

## The critical fix

```tcl
targets -set -filter {name =~ "ARM*Cortex-A9 MPCore #0"}
stop
after 5000      # ← wait for NaxRiscv BIOS memtest to finish before dowing
dow -data <files>
```

Without this, BIOS memtest writes to main_ram race against dow writes for
the same region → kernel corrupted in flight → OpenSBI hangs at MEDELEG.

## Pinned versions

| Artifact | Path | md5 / commit |
|---|---|---|
| `jtag_boot.py` | `hardware/zynq/ps_loader/jtag_boot.py` | `4d2dc78f4dacd2478b4d2ee649f29424` |
| Patched bitstream | `hardware/zynq/litex-build-nax64-patched/gateware/hamgeek_rk7020f.bit` | `c3420e7a82fc0c1011d40c8371599f8f` |
| LiteX repo | `/home/mattrock/litex/litex` | `498d1fe8270b8456ed4615bf14847eb05ae38860` (+ local patch) |
| NaxRiscv data repo | `/home/mattrock/litex/pythondata-cpu-naxriscv` | `0b27d98a769a04c5291ea069cdc144ef10efa9aa` |

## LiteX patch (not strictly needed for this boot, but a real upstream bug)

`/home/mattrock/litex/litex/litex/soc/software/libbase/system.c`:
1. Alias `CONFIG_L2_SIZE = CONFIG_CPU_L2CACHE_SIZE` so `flush_l2_cache()`
   isn't a no-op on NaxRiscv builds.
2. Read from `MAIN_RAM_BASE + MEMTEST_DATA_SIZE + 64 KiB` (beyond BIOS
   memtest region) so reads actually miss L2 and trigger eviction instead
   of returning stale cached data.

## Image + address layout (NaxRiscv → PS DDR translation via LiteX wishbone remapper: PS = NaxRiscv − 0x3FF00000)

| File | NaxRiscv | PS DDR | Size |
|---|---|---|---|
| `Image_nax64` | 0x40000000 | 0x00100000 | 8,231,096 |
| `linux_nax64.dtb` | 0x40EF0000 | 0x00FF0000 | 2,444 |
| `fw_jump_nax64.bin` | 0x40F00000 | 0x01000000 | 133,632 |
| `ubuntu_rv64.cpio.gz` | 0x42000000 | 0x02100000 | 31,233,653 |
| `trampoline.bin` | 0x40A00000 | 0x00B00000 | 36 |

## Verified-working console

LiteX BIOS UART is on the external FT232R USB-UART adapter at
`/dev/ttyUSB2 @ 921600 baud`. FT2232H channel B (`/dev/ttyUSB1`) is NOT
wired to the PL UART on this board.

## Last-successful output signature

```
--============= Liftoff! ===============--
OpenSBI v1.3
...
Boot HART MEDELEG         : 0x000000000000b109
[    0.000000] Linux version 6.9.0 (mattrock@Jarvis) ...
...
[   60.915318] Run /init as init process

==========================================
 ATOMiK | Ubuntu 24.04 on RV64 NaxRiscv
 Board: ALINX AX7020 (XC7Z020-CLG484)
==========================================

root@atomik-rv64:/#
```

## Timing breakdown

| Phase | Time |
|---|---|
| xsdb init + ps7_init + fpga load + post_config | ~3 s |
| 5-second memtest wait | 5 s |
| 4 × `dow -data` + trampoline | ~87 s (dominated by 30 MB cpio.gz at JTAG rate) |
| BIOS + OpenSBI + kernel boot to initramfs | ~15 s |
| `/init` → user-space Ubuntu | ~60 s |
| **Total to root prompt** | **~155 s** |

## Do not touch

- The `after 5000` delay. It's the whole point.
- The trampoline at 0x40A00000 — belt-and-suspenders L2 flush, keep.
- The LiteX patched bitstream path in `jtag_boot.py`.
