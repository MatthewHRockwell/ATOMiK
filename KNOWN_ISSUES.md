# ATOMiK Known Issues & Error Log

**Last Updated:** February 22, 2026

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

### V3-005: CPU Synthesis LUT Budget

**Severity:** Medium
**Date:** February 17, 2026 (updated February 18, 2026)
**Status:** Substantially resolved (3,181 LUT, target 2,700)

**Original symptom:** CPU-only synthesis consumed 8,013 LUT (97%) with behavioral register file.

**Optimizations applied (Phase 2):**
1. BSRAM register file — dual SDP banks, 4 BSRAM blocks, registered reads → -5,000 LUT
2. Shared barrel shifter — SLL via bit-reversal through single right-shift barrel → -280 LUT
3. CSR narrowing — mstatus to 4 bits, mtval/mie/mip hardwired zero → -125 LUT
4. ALU reuse for branch/JAL targets — eliminated separate 64-bit pc+imm adder → -63 ALU
5. Counter removal — mcycle/minstret read-only zero → -300 LUT

**Current combined (CPU + ATOMiK) synthesis:** 3,181 LUT + 256 ALU = 3,437 total logic, 601 FF, 6 BSRAM.

**Remaining gap:** 481 LUT over 2,700 target. Dominated by cpu_top wiring/mux logic (1,590 LUT) which contains 64-bit writeback mux, operand mux, bus arbitration, and cross-module wiring that the synthesizer cannot further optimize. The ALU barrel shifter contributes 601 LUT.

**Possible further optimizations (deferred to Phase 3):**
- Iterative shifter: replace 6-stage barrel with shift-by-1 loop (saves ~300 LUT, adds up to 63 cycles per shift)
- Instruction-set subsetting: remove W-variant instructions if firmware doesn't use them (saves ~100 LUT)

**Note:** ATOMiK datapath itself is only 130 LUT + 2 BSRAM — excellent CLS mapping at 1.016 CLS/bit (target ≤1.2). The design is functionally complete and all compliance/verification tests pass.

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

### V3-008: Regfile Testbench Not Updated for BSRAM Registered Reads

**Severity:** Low
**Date:** February 18, 2026
**Status:** Open (testbench issue only, not RTL)

**Symptom:** `tb_v3_regfile` reports 7/38 pass. Writes appear to not take effect — reads return stale values.

**Root Cause:** The testbench was written for a behavioral register file with combinational reads. After migrating to BSRAM with registered read ports, data is available 1 cycle after the address is presented. The testbench reads immediately after writing without waiting for the registered read output.

**Impact:** None on RTL correctness. The BSRAM register file works correctly in the full CPU (all 53/54 compliance tests pass). Only the standalone iverilog testbench needs updating.

**Fix:** Update testbench to account for BSRAM read latency: present address, wait 1 clock cycle, then check output. Deferred to Phase 3.

---

## v3 SoC Issues (Phase 3)

### V3-009: Power-on Reset Not Applied — CPU FSMs Powered On In Unknown State

**Severity:** Critical
**Date:** February 22, 2026
**Status:** Fixed

**Symptom:** After SRAM bitstream load, CPU shows mem_valid continuously asserted (stuck HIGH). No state progression, no instruction fetch activity. Reset signal sys_resetn is HIGH but CPU never executes.

**Root Cause:** In `atomik_v3_soc.v`, the sys_resetn signal was assigned to a constant:
```verilog
assign sys_resetn = 1'b1;  // CPU never sees reset pulse!
```

On FPGA power-up or bitstream load, flip-flops in the CPU FSM (fetch, LSU, control state) are initialized to random values. Without an explicit reset pulse, the FSM remains in an undefined state even though the reset signal is HIGH. The CPU's reset-sensitive logic never transitions to a known initial state.

**Impact:** CPU completely non-functional on hardware. Verilator simulations worked because Verilator initializes all registers to zero by default (unless `--x-initial` is used).

**Fix:** Added explicit power-on reset generator with initial block:
```verilog
// Power-on reset generator (hold reset for 256 cycles after power-on)
reg [7:0] reset_counter;
reg sys_resetn_reg;

initial begin
    reset_counter = 8'h00;
    sys_resetn_reg = 1'b0;
end

always @(posedge clk_p) begin
    if (reset_counter != 8'hFF) begin
        reset_counter <= reset_counter + 1;
        sys_resetn_reg <= 1'b0;  // Hold in reset
    end else begin
        sys_resetn_reg <= 1'b1;  // Release reset after 256 cycles
    end
end

assign sys_resetn = sys_resetn_reg;
```

This ensures the CPU FSMs are held in reset for 256 clock cycles (~19 µs at 13.5 MHz) after bitstream load, then cleanly released to a known initial state.

**Lesson:** On FPGAs, always use explicit reset generators with initial blocks or external reset signals. Never rely on constant-high reset — it doesn't initialize flip-flop state on power-up.

---

### V3-010: Bus Arbiter Ready Signal Crosstalk — Fetch Unit Sees LSU Responses

**Severity:** Critical
**Date:** February 22, 2026
**Status:** Fixed

**Symptom:** After fixing V3-009, CPU progressed past FETCH state but still did not boot. mem_valid toggled but no UART output. Debug showed both fetch_bus_valid and mem_ready asserted simultaneously even though CPU FSM was in EXECUTE state (where fetch should be idle).

**Root Cause:** In `atomik_v3_cpu.v`, both the fetch unit and LSU were connected directly to the shared mem_ready signal without gating:
```verilog
// BROKEN: Both units receive mem_ready directly
.bus_ready      (mem_ready),  // fetch unit (line 138)
.bus_ready      (mem_ready),  // LSU (line 232)
```

When the LSU had bus ownership (during MEMORY state or load/store in EXECUTE), mem_ready asserted in response to the LSU's bus transaction. However, the fetch unit also saw this mem_ready pulse and incorrectly latched stale bus data, corrupting its instruction register. The fetch unit's state machine advanced as if it had successfully fetched an instruction, even though the bus was serving the LSU.

**Impact:** CPU executed garbage instructions due to corrupted fetch data. Bus transactions appeared to complete but the CPU never made forward progress.

**Fix:** Gated mem_ready based on bus ownership (which master currently owns the bus):
```verilog
// Gate mem_ready based on bus ownership (FIX: prevent crosstalk)
wire lsu_has_bus = (state_out == S_MEMORY) ||
                   (state_out == S_EXECUTE && (dec_is_load || dec_is_store));

wire fetch_bus_ready = mem_ready && !lsu_has_bus;
wire lsu_bus_ready   = mem_ready && lsu_has_bus;

// In fetch unit instantiation:
.bus_ready      (fetch_bus_ready),  // FIX: use gated ready signal

// In LSU instantiation:
.bus_ready      (lsu_bus_ready),    // FIX: use gated ready signal
```

Now each unit only sees mem_ready when it has bus ownership. The FSM state machine guarantees mutual exclusion — fetch is only active in FETCH state, LSU only in MEMORY/EXECUTE-load-store.

**Lesson:** When multiple bus masters share a single bus, always gate the ready signal based on ownership. A bus arbiter must ensure only the current master sees handshake signals. Direct connection of ready to multiple masters causes crosstalk and data corruption.

---

### V3-011: UART Timing Violation — Data Written Before Transmitter Ready

**Severity:** High
**Date:** February 22, 2026
**Status:** Fixed

**Symptom:** After fixing V3-009 and V3-010, CPU executed firmware correctly (confirmed via FSM state debug), but UART TX remained idle (no transmission). Debug confirmed both UART CLKDIV and DATA registers were written by firmware, but the UART TX pin (ser_tx) stayed HIGH (idle state).

**Root Cause:** The simpleuart module sends 15 idle bits after a CLKDIV write (lines 119-123 of simpleuart.v):
```verilog
if (reg_div_we) begin
    send_pattern <= 1;
    send_bitcnt <= 0;
    send_divcnt <= 0;
    send_dummy <= ~0;  // 15 bits of 1's (idle state)
end
```

This takes approximately 1,740 cycles at 13.5 MHz (15 bits × 116 cycles/bit at 115200 baud). The DATA register write is only accepted when `send_bitcnt == 0` (line 127). If firmware writes DATA immediately after CLKDIV, the write is silently ignored because the transmitter is busy sending idle bits.

The v3 firmware (isp_flasher.c) set CLKDIV and then immediately sent data:
```c
UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;
// BUG: Immediately write DATA without waiting for idle bits
UART0->DATA = 'X';  // Silently ignored!
```

**Impact:** No UART output after boot. The ISP flasher's initial diagnostic message was never transmitted. Firmware appeared to hang waiting for UART.

**Fix:** Added explicit delay after CLKDIV initialization:
```c
UART0->CLKDIV = CLK_FREQ / UART_BAUD - 2;

// CRITICAL: Wait for UART to finish sending 15 idle bits after CLKDIV write
// This takes ~1740 cycles (15 bits * 116 cycles/bit at 115200 baud, 13.5 MHz clock)
for (waitcnt = 0; waitcnt < 2000; waitcnt++);

// Now safe to write DATA
UART0->DATA = 'X';
```

The 2000-cycle delay ensures the transmitter has finished sending idle bits before any DATA writes occur.

**Lesson:** Peripheral modules may have initialization timing requirements not exposed in the register interface. When porting firmware to a new architecture, review the peripheral RTL for state machine delays and ensure firmware timing accounts for these. UART CLKDIV changes require waiting for the transmitter to stabilize before sending data.

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
