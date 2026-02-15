# ATOMiK Known Issues & Error Log

**Last Updated:** February 15, 2026

This document tracks hardware and software issues encountered during development, their root causes, and resolutions. It serves as a troubleshooting reference for future work.

---

## Hardware Issues

### HW-001: `openFPGALoader --external-flash` Corrupts Firmware

**Severity:** Critical
**Date:** February 14, 2026
**Status:** Resolved (workaround documented)

**Symptom:** After using `openFPGALoader --external-flash` to write firmware directly to the MSPI NOR flash via JTAG, the firmware appears partially corrupted. The CPU boots, prints the menu, and responds to simple commands (F, X), but crashes when executing ATOMiK MMIO operations in the performance benchmark suite. Specifically, `bench_atomik_ops()` hangs after ~2 iterations (12 `##PERF` lines), and the CPU becomes permanently unresponsive.

**Root Cause:** The GW1NR-9 has two separate flash memories:
1. **Embedded NAND flash** — stores the FPGA bitstream (written by `openFPGALoader -f`)
2. **External MSPI NOR flash** (Puya P25Q32SH, Jedec 0x856016) — stores user firmware (written by ISP programmer via UART)

`openFPGALoader --external-flash` writes to the MSPI NOR flash via JTAG SPI bitbang. While the write and verify both succeed, the data format and/or flash state left by the JTAG bitbang is incompatible with what PicoRV32's `spimemio` XIP controller expects. This results in partially readable but corrupted firmware — early code pages work, later pages cause CPU hangs on MMIO transactions.

**Resolution:** Never use `openFPGALoader --external-flash` for firmware. Always use the ISP programmer:

```bash
# CORRECT: Flash firmware via ISP programmer
cd /path/to/TangNano-9K-example/picotiny
python3 sw/pico-programmer.py /path/to/atomik-fw.v /dev/ttyUSB1 &
sleep 2
openFPGALoader -b tangnano9k picotiny.fs
# Programmer catches ISP boot window, flashes correctly
```

**Prevention:** Added to MEMORY.md and FIRMWARE_BUILD_GUIDE.md as a critical warning.

---

### HW-002: ISP Programmer Requires Boot Window Timing

**Severity:** Medium
**Date:** February 13, 2026
**Status:** Resolved (ISP flash trick documented)

**Symptom:** `pico-programmer.py` times out with "Picorv32-isp not detected or not in isp mode" when the board has already booted past the ISP bootloader window.

**Root Cause:** The boot ROM ISP flasher waits for a 0x55 sync byte over UART. If no sync is received within ~500K loop iterations (~2 seconds at 25 MHz), it jumps to the main firmware at 0x00000000. The programmer must connect during this brief window.

**Resolution:** Use the ISP flash trick — start the programmer first, then reload the bitstream to trigger a CPU reboot:

```bash
python3 pico-programmer.py firmware.v /dev/ttyUSB1 &
sleep 2
openFPGALoader -b tangnano9k picotiny.fs
```

The SRAM bitstream reload resets the CPU, which enters the ISP bootloader. The already-running programmer catches the sync window.

---

### HW-003: Newer Bitstream Build Non-functional

**Severity:** Low
**Date:** February 14, 2026
**Status:** Open (using known-good backup)

**Symptom:** The bitstream at `project/impl/pnr/picotiny.fs` (Feb 14 build) produces no UART output when loaded. The older backup at `picotiny.fs` (Feb 13 build, project root) works correctly.

**Root Cause:** Unknown. Both bitstreams are from the same project source. The newer build may have a Gowin EDA configuration difference or synthesis option change that affects UART initialization.

**Workaround:** Use the known-good backup bitstream:
```bash
openFPGALoader -b tangnano9k /path/to/TangNano-9K-example/picotiny/picotiny.fs
```

---

### HW-004: Serial Port Unresponsive After SRAM Bitstream Load

**Severity:** Medium
**Date:** February 14, 2026
**Status:** Resolved

**Symptom:** After loading a bitstream to SRAM via `openFPGALoader -b tangnano9k`, connecting to the serial port immediately returns 0 bytes. The firmware appears unresponsive.

**Root Cause:** The ISP bootloader runs for ~2-5 seconds after bitstream load, consuming any UART data sent during this window. The bootloader looks for 0x55 sync; any other bytes are discarded. Only after the bootloader times out does the main firmware start accepting commands.

**Resolution:** Wait at least 5 seconds after SRAM bitstream load before connecting to the serial port. The `perf_runner.py --reset` flag handles this automatically.

---

## Software Issues

### SW-001: `perf_runner.py` Key Parsing ValueError

**Severity:** Medium
**Date:** February 14, 2026
**Status:** Fixed

**Symptom:** `build_benchmarks()` crashes with `ValueError: invalid literal for int() with base 10: 'load'` when processing measurement groups.

**Root Cause:** The key parsing used `split("_")` which split compound test names like `"atomik_load"` into `["atomik", "load"]` and attempted `int("load")`.

**Fix:** Changed to `rsplit("_", 1)` with `.isdigit()` check before integer conversion. Only the trailing numeric suffix (e.g., `_32`, `_100`) is parsed as a size/count parameter.

---

## Build Lessons (Permanent Reference)

These are not bugs but important lessons learned during development:

| Lesson | Details |
|--------|---------|
| **`-fno-builtin` is mandatory** | GCC -O3 recognizes memset/memcpy loop patterns and replaces them with recursive calls to themselves. Causes immediate stack overflow on boot. |
| **RV32I has no multiply/divide** | Use powers-of-10 table with repeated subtraction for printf decimal conversion. Avoid `*` and `/` in hot loops. |
| **Do NOT replace PicoRV32 ALU carry chains** | Gowin ALU carry chains are hardware-optimized. LUT-based ripple carry creates 65 logic levels, Fmax drops to 12.9 MHz. CPU won't boot. |
| **Do NOT gate reset on ATOMiK PLL lock** | If `atomik_pll_lock` gates `Reset_Sync`, a PLL failure prevents CPU boot entirely. Keep ATOMiK PLL independent. |
| **Persistent flash bitstream for reliable PLL** | SRAM bitstream loads may have inconsistent PLL lock timing. For production, use `openFPGALoader -b tangnano9k -f` to write to persistent embedded flash. |

---

*This document is updated as new issues are discovered and resolved.*
