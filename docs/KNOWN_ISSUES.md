# ATOMiK Known Issues & Error Log

**Last Updated:** February 17, 2026

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

## v3 CPU Issues (Phase 1)

### V3-001: LSU `lsu_rdata` Gated on `lsu_done` — Load Data Lost During Writeback

**Severity:** Critical
**Date:** February 17, 2026
**Status:** Fixed

**Symptom:** All load instructions (LB/LH/LW/LD/LBU/LHU/LWU) fail compliance. Bus traces confirm correct data arrives from memory, but register writeback always writes zero.

**Root Cause:** `lsu_done` is registered: `lsu_done <= (state == S_DONE)`. This means `lsu_done` goes high one cycle *after* the LSU enters S_DONE. The control FSM sees `lsu_done` and transitions from MEMORY to WRITEBACK at the same posedge that the LSU transitions from S_DONE to S_IDLE — causing `lsu_done` to drop back to 0 during the WRITEBACK cycle. The `lsu_rdata` output was gated with `if (lsu_done && !req_is_store)`, so it output 0 during the exact cycle the writeback mux sampled it.

**Fix:** Removed the `lsu_done` gate on `lsu_rdata`. The load result in `last_rdata` is already stable after the bus transaction completes, and the writeback mux only selects `lsu_rdata` when `wb_src == LOAD`. The data is always valid when needed.

**Lesson:** Never gate a data output on a registered done signal if the consumer samples data on the same cycle as the done assertion. Either make the done combinational or ensure data persists independently.

---

### V3-002: Memory Model Base Address Mismatch

**Severity:** High
**Date:** February 17, 2026
**Status:** Fixed

**Symptom:** First compliance test (LUI) times out. CPU appears to execute no instructions.

**Root Cause:** The `MemModel` class used `MEM_BASE = 0x00000000` but riscv-tests ELF binaries load at `0x80000000`. The ELF loader wrote program data to addresses starting at 0x80000000, which fell outside the model's 0x00000000–0x00FFFFFF range. All reads returned 0 (NOP-like), so the CPU spun fetching zeros.

**Fix:** Changed `MEM_BASE` to `0x80000000` and added a `translate()` method that converts absolute addresses to array indices. Set `RESET_PC` to `0x80000000` via Verilator `-G` parameter.

---

### V3-003: Regfile Testbench Timing — Read-After-Write Visibility

**Severity:** Medium
**Date:** February 17, 2026
**Status:** Fixed

**Symptom:** 19/38 regfile iverilog tests fail. Writes appear to not take effect.

**Root Cause:** Testbench setup writes on posedge, then immediately reads on the next negedge without accounting for synchronous write latency. The combinational read sees the old value because the write hasn't been clocked in yet.

**Fix:** Added `@(negedge clk)` setup before posedge writes and `#1` delays after posedge for reads, ensuring writes are committed before reads are sampled.

---

### V3-004: Gowin Synthesis Script Double-Adds Source Files

**Severity:** Low
**Date:** February 17, 2026
**Status:** Fixed

**Symptom:** Gowin EDA synthesis fails with "unable to add file, already in project" for every RTL source.

**Root Cause:** The `synth_v3.tcl` script wrote file paths into the `.gprj` XML AND then called `add_file` for each file via TCL. Gowin's `open_project` already loads the file list from the XML.

**Fix:** Removed the `foreach f $cpu_files { add_file $f }` loop. Files are listed only in the `.gprj` XML.

---

### V3-005: CPU-Only Synthesis Exceeds LUT Budget (8,013 vs 2,500)

**Severity:** High
**Date:** February 17, 2026
**Status:** Open (deferred to Phase 2)

**Symptom:** CPU-only synthesis consumes 8,013 LUT (97% of GW1NR-9's 8,640), far exceeding the 2,500 LUT budget. No room for ATOMiK or peripherals.

**Root Cause:** The behavioral register file (`reg [63:0] regs [1:31]`) with two combinational read ports synthesizes to massive distributed LUT mux trees. A 32:1 mux per bit, times 64 bits, times 2 read ports = ~4,000+ LUT for read muxes alone. Gowin attempted BSRAM extraction (`Extracting RAM for identifier 'regs'`) but failed because combinational reads are incompatible with BSRAM (which requires registered reads).

**Mitigation plan (Phase 2):**
1. BSRAM register file — 2-4 BSRAM blocks with registered reads, add sub-cycle to DECODE
2. Two-stage barrel shifter to reduce shift logic LUT
3. CSR read mux optimization

**Note:** Fmax target (25 MHz) IS met at 28.8 MHz. The CPU is functionally correct — this is purely a resource constraint.

---

### V3-006: `ma_data` Compliance Test Fails

**Severity:** Low
**Date:** February 17, 2026
**Status:** Accepted (by design)

**Symptom:** The `rv64ui-p-ma_data` test fails. All other 53 tests pass.

**Root Cause:** This test verifies misaligned memory access trap handling. The v3 CPU does not implement misaligned access exceptions — it simply performs the access. The test expects an exception to be raised on misaligned loads/stores, which requires `mcause` to be set to 4 (load address misaligned) or 6 (store address misaligned) and control to transfer to `mtvec`.

**Decision:** Accepted. Misaligned access traps are not required for correct RV64I operation in this embedded context. The hardware naturally handles aligned accesses, and firmware will not perform misaligned accesses. Adding trap logic would increase LUT usage with no practical benefit.

---

### V3-007: PnR Fails on CPU-Only Synthesis (Too Many I/O Ports)

**Severity:** Low
**Date:** February 17, 2026
**Status:** Expected (not a real issue)

**Symptom:** Gowin Place & Route fails with `PA2024: The number(104) of ports exceeds the resource limit 59 regular I/Os`.

**Root Cause:** The `atomik_v3_cpu` module exposes 104 I/O ports (bus interface, clock, reset, debug signals). The GW1NR-9 has only 59+10 dual-purpose I/Os. This is expected for a CPU-only synthesis — the CPU will be wrapped in a SoC top module in Phase 3.

**Note:** Synthesis (LUT/Fmax) results are still valid from the synthesis stage. PnR is only needed for the final SoC.

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
| **Behavioral regfile = LUT explosion on GW1NR-9** | A 32x64-bit register array with combinational reads cannot be inferred as BSRAM. Gowin synthesizes it as distributed LUT mux trees (~4,000+ LUT). Use BSRAM with registered reads for production. |
| **Registered `done` + combinational data = race** | If a `done` signal is registered (1-cycle delay) but the data output is gated on `done`, the consumer may see stale data. Either make data persist independently or use a combinational `done`. |
| **Gowin `.gprj` XML includes file list** | When using `open_project` with a `.gprj` file, do NOT also call `add_file` — files are already loaded from the XML. Double-adding causes errors. |
| **Verilator `-G` parameter width** | `-GRESET_PC=0x80000000` passes a 32-bit value to a 64-bit parameter. Suppress the WIDTHEXPAND warning with `/* verilator lint_off WIDTHEXPAND */` at the parameter declaration. |

---

*This document is updated as new issues are discovered and resolved.*
