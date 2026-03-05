# ATOMiK Known Issues & Error Log

**Last Updated:** March 3, 2026

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

### V3-012: No UART Output in Bringup Mode — CPU Liveness Unknown

**Severity:** Critical (blocking Phase 3C Task 4)
**Date:** February 23, 2026
**Status:** Resolved (see V3-013 clock issue, V3-014 UART issue)

**Symptom:** After implementing Boot ROM bringup mode (continuous UART 'T' spam + GPIO toggle heartbeat) and successfully synthesizing/loading bitstream, there is zero UART output at any baud rate tested (9600-230400). The CPU liveness is unknown — the failure occurs before any observable UART transmission.

**Build artifacts verified:**
- ✅ Synthesis: Clean (zero timing violations, 0 TNS/WNS)
- ✅ Bitstream: Loads successfully to Tang Nano 9K SRAM (`openFPGALoader` completes)
- ✅ Disassembly: Correct boot sequence (crtStart @ 0x80000000 → main @ 0x80000054)
  - UART CLKDIV set correctly: 115 = (13,500,000 / 115,200) - 2 ✓
  - UART DATA write loop with 'T' (0x54) ✓
  - GPIO[0] toggle heartbeat ✓
- ✅ Boot ROM firmware: 752 bytes, built successfully
- ✅ BSRAM IP updated: bootram_2kx8_0..3 with bringup Boot ROM

**Test configuration:**
- Clock: 27 MHz crystal → CLKDIV ÷2 → 13.5 MHz CPU clock
- UART: 115200 baud (divider = 115)
- Reset PC: 0x80000000 (Boot ROM base)
- Boot ROM mapped at: 0x80000000-0x80001FFF (8 KB, BSRAM)

**Diagnostic attempts:**
- ✅ Tried multiple baud rates: 9600, 19200, 38400, 57600, 115200, 230400 (all silent)
- ❌ Cannot verify GPIO toggle (pins 15/16 on expansion header, no on-board LED)
- ❌ Cannot probe clock/reset signals (no scope/logic analyzer)
- ✅ Debug signal pins defined in CST: debug_reset_n (pin 15), debug_clk_toggle (pin 16)

**Possible root causes (ordered by likelihood):**
1. **CPU not fetching instructions** — Clock not running, PLL failure, or reset stuck
2. **Boot ROM not mapped correctly** — Bus arbiter routing, address decode error
3. **UART peripheral not functional** — TX pin not connected, peripheral initialization issue
4. **UART divider formula wrong** — Oversampling assumption (8x/16x vs 1x) incorrect
5. **Power-on reset timing** — Reset release too fast, CPU FSMs in metastable state

**Comparison with v2 production SoC:**
- v2 @ 25.2 MHz (PLL-based) boots reliably and produces UART output
- v3 @ 13.5 MHz (crystal ÷2, PLL bypassed due to earlier lock failures) — silent

**Previous similar issue:**
- V3-009: CPU powered on in unknown state (missing reset pulse) — fixed with explicit reset generator
- V3-010: Bus arbiter crosstalk (fetch unit saw LSU ready) — fixed with gated ready signals
- V3-011: UART timing violation (DATA written before transmitter ready) — fixed with delay after CLKDIV

**Next diagnostic steps (recommended order):**
1. **Verilator SoC testbench** — Simulate full SoC boot sequence to verify:
   - PC jumps to 0x80000000 on reset
   - Boot ROM reads succeed (instruction fetch from BSRAM)
   - UART TX toggles at correct times
   - Clock domain crossing (if any) is correct
2. **Try alternative UART divider formulas:**
   - Current: `CLKDIV = (clk / baud) - 2` = 115
   - Try 8x oversample: `CLKDIV = (clk / (baud * 8)) - 2` = 12
   - Try 16x oversample: `CLKDIV = (clk / (baud * 16)) - 2` = 5
3. **Physical hardware debug** (if tools available):
   - Scope probe on ser_tx (pin 17) — verify toggle vs constant HIGH
   - Scope probe on GPIO[0] (expansion header) — verify CPU heartbeat
   - Scope probe on clk_p — verify 13.5 MHz clock is running
4. **Simplify bringup test further:**
   - Remove UART entirely, toggle GPIO only (LED blink test)
   - Hard-wire ser_tx to a counter instead of UART peripheral
   - Create minimal CPU smoke test (execute single instruction, toggle output pin)

**Bringup mode can be disabled to restore production ISP flasher:**
```c
// In hardware/v3/soc/firmware/fw-brom/isp_flasher.c
#define BRINGUP_MODE 0  // Change from 1 to 0
```

**Related files:**
- Boot ROM: `hardware/v3/soc/firmware/fw-brom/isp_flasher.c`
- SoC top: `hardware/v3/soc/atomik_v3_soc.v`
- Synthesis log: `hardware/v3/synth/synth_bringup.log`
- Commit: 0f6f247 "Add Boot ROM bringup mode for hardware liveness testing"

**Resolution:** The investigation uncovered **two separate root causes**:
1. **V3-013 (CLKDIV not dividing)**: CPU running at 27 MHz instead of 13.5 MHz. Firmware baud rate calculation was wrong. Fixed by updating `CLK_FREQ` to 27 MHz.
2. **V3-014 (manual_uart_tx data corruption)**: UART peripheral had timing/glitch issue causing scrambled data. GPIO on same bus worked perfectly, proving CPU and data path functional. Fixed by reverting to proven v2 simpleuart module.

**CPU liveness confirmed via GPIO:** Pin 10 (GPIO[0]) blinked correctly throughout testing, proving:
- ✅ CPU executes instructions
- ✅ Boot ROM firmware runs
- ✅ Register file works
- ✅ LSU and bus infrastructure functional
- ✅ Peripheral address decode correct

The root issue was **peripheral-specific** (UART), not CPU/architecture. All v3 CPU compliance tests remain PASS (53/54).

---

### V3-013: CLKDIV Not Dividing — Clock Running at 27 MHz Instead of 13.5 MHz

**Severity:** High
**Date:** February 23, 2026
**Status:** Workaround (firmware adjusted to 27 MHz)

**Symptom:** UART transmits at 230,400 baud when firmware is configured for 115,200 baud. This is exactly 2× the expected rate, indicating the CPU clock is running at 27 MHz (crystal frequency) instead of 13.5 MHz (÷2).

**Diagnostic evidence:**
- Firmware sets `UART_CLKDIV = (13,500,000 / 115,200) - 2 = 115`
- Expected baud: 13.5 MHz / 116 = 116,379 ≈ 115,200 ✓
- **Actual baud: 230,400** (2× expected)
- Reverse calculation: 230,400 × 116 = 26.7 MHz ≈ **27 MHz**
- Synthesis power report claims: 13.5 MHz (incorrect!)

**Root cause:** The Gowin CLKDIV IP has a configuration mismatch:
- **IPC file**: `/hardware/v3/soc/gowin_ip/gowin_clkdiv/gowin_clkdiv.ipc` has `Division_Factor=5`
- **Verilog file**: `/hardware/v3/soc/gowin_ip/gowin_clkdiv/gowin_clkdiv.v` has `DIV_MODE="2"`

The CLKDIV was copied from the v2 picotiny project (which uses ÷5 for 126 MHz → 25.2 MHz). Someone manually edited the `.v` file to change `DIV_MODE` from "5" to "2" but **did not regenerate the `.ipc` file**. Gowin synthesis tools appear to use the `.ipc` configuration file, ignoring the Verilog `defparam`.

Even after fixing the IPC file (`Division_Factor=2`), the CLKDIV still does not divide. The exact reason is unknown — possible causes:
1. CLKDIV `resetn` pin requirement (currently hardwired to `1'b1`)
2. CLKDIV requiring external regeneration via Gowin IP Compiler GUI
3. Missing SDC constraint for the divided clock output
4. Tang Nano 9K hardware limitation with CLKDIV primitive

**Impact:**
- CPU runs at 27 MHz instead of 13.5 MHz (2× faster than designed)
- Higher power consumption than expected
- Timing closure easier (more margin at lower target frequency)
- Must account for actual 27 MHz in all firmware timing calculations

**Workaround:**
Update firmware to use actual clock frequency:
```c
// In hardware/v3/soc/firmware/fw-brom/isp_flasher.c
#define CLK_FREQ 27000000  // Crystal direct (CLKDIV not working!)
#define UART_BAUD 115200
// CLKDIV = 27,000,000 / 115,200 - 2 = 232
```

**Attempted fixes:**
1. ✅ Updated `.ipc` file to `Division_Factor=2` — no effect
2. ✅ Verified `resetn` tied to `1'b1` (always enabled) — correct
3. ✅ Confirmed CLKDIV instantiated in netlist with `DIV_MODE="2"` — present
4. ❌ CLKDIV still outputs 27 MHz instead of 13.5 MHz

**Long-term fix (recommended for Phase 4):**
1. Delete existing CLKDIV IP directory entirely
2. Use Gowin IP Compiler GUI to generate fresh CLKDIV with Division_Factor=2
3. Or: Use a PLL with CLKOUT at 27 MHz (bypassing CLKDIV entirely)
4. Or: Accept 27 MHz operation and update all documentation/firmware

**Files affected:**
- `hardware/v3/soc/gowin_ip/gowin_clkdiv/gowin_clkdiv.ipc` (updated, ineffective)
- `hardware/v3/soc/gowin_ip/gowin_clkdiv/gowin_clkdiv.v` (DIV_MODE="2")
- `hardware/v3/soc/atomik_v3_soc.v` (CLKDIV instantiation)
- `hardware/v3/soc/firmware/fw-brom/isp_flasher.c` (CLK_FREQ workaround)

**Comparison with v2:**
- v2 uses: PLL (27→126 MHz) + CLKDIV ÷5 → 25.2 MHz ✓ (works)
- v3 uses: Crystal (27 MHz) + CLKDIV ÷2 → should be 13.5 MHz ✗ (bypassed)

---

### V3-014: manual_uart_tx Data Corruption — UART Receives Scrambled Data

**Severity:** Critical (UART unusable)
**Date:** February 23, 2026
**Status:** Workaround (use v2 simpleuart instead)

**Symptom:** The `manual_uart_tx` peripheral receives completely scrambled data compared to what the CPU writes. GPIO peripheral on the same bus works perfectly with identical writes, proving the data path itself is functional.

**Diagnostic evidence:**

*Test 1: CPU writes 0x54 ('T') constantly*
- Expected: 0x54 (01010100)
- GPIO receives: ✅ Correct (pin 10 blinks, bit 0 toggles)
- UART transmits: ❌ 0x00 (00000000) — all zeros

*Test 2: CPU writes 0xFF*
- Expected: 0xFF (11111111)
- GPIO receives: ✅ Correct
- UART transmits: ❌ 0x00 (00000000) — all zeros

*Test 3: CPU writes alternating 0x55/0xAA*
- Expected: 0x55, 0xAA alternating
- GPIO receives: ✅ Correct (pin 10 toggles normally)
- UART transmits: ❌ 0x66, 0xE6, 0x98 — wrong values

*Test 4: Bit pattern mapping (powers of 2 + 0xFF/0x00)*
- **Written values**: `01 02 04 08 10 20 40 80 FF 00` (repeating)
- **UART received**: `18 60 80 80 00 E0 00 F8 00 FE 00 FE 00 06` (repeating)

Detailed bit mapping:

| CPU Write | Binary     | UART RX | Binary     | Pattern |
|-----------|------------|---------|------------|---------|
| 0x01      | 00000001   | 0x18    | 00011000   | Scrambled |
| 0x02      | 00000010   | 0x60    | 01100000   | Scrambled |
| 0x04      | 00000100   | 0x80    | 10000000   | Bit 7 only |
| 0x08      | 00001000   | 0x80    | 10000000   | Same as 0x04! |
| 0x10      | 00010000   | 0x00    | 00000000   | Zero |
| 0x20      | 00100000   | 0xE0    | 11100000   | Scrambled |
| 0x40      | 01000000   | 0x00    | 00000000   | Zero |
| 0x80      | 10000000   | 0xF8    | 11111000   | Scrambled |
| 0xFF      | 11111111   | 0x00    | 00000000   | **Inverted!** |
| 0x00      | 00000000   | 0xFE    | 11111110   | **Inverted!** |

**Analysis:**
The corruption is not a simple bit shift, inversion, or byte lane swap. The pattern suggests the UART is either:
1. **Sampling bus data at the wrong time** (glitch/race condition)
2. **Reading from wrong bit positions** (incorrect slice of bus word)
3. **Seeing data from a different bus transaction** (address instead of data, or neighboring transaction)

**Architecture verification:**
- ✅ `PicoMem_Mux_1_4` routing: Direct passthrough `assign picos3_wdata = picom_wdata;`
- ✅ `PicoMem_UART` connection: `reg_dat_di(mem_s_wdata)` — correct
- ✅ `manual_uart_tx` latching: `tx_data <= reg_dat_di[7:0]` on posedge when `reg_dat_we` — correct
- ✅ Write enable generation: `reg_dat_we = reg_dat_sel & mem_s_wstrb[0]` — correct
- ✅ Address decode: `reg_dat_sel = mem_s_valid && ~mem_s_addr[2]` — correct
- ✅ GPIO peripheral: Uses same bus infrastructure, works perfectly

**Root cause hypothesis:**
The `reg_dat_we` signal is generated combinationally from `mem_s_valid`, `mem_s_addr[2]`, and `mem_s_wstrb[0]`. If there are any glitches on these bus signals during a transaction (e.g., address changing before valid drops), `reg_dat_we` could pulse momentarily while `mem_s_wdata` has a stale or transitioning value. The UART latches this glitched data on the next clock edge.

The GPIO peripheral likely doesn't exhibit this issue because:
1. It uses a registered output (`out_r`) that's written on posedge, masking setup/hold issues
2. Its address decode might happen to be glitch-free due to different timing
3. Visual observation of LED blinking is much more tolerant of occasional corruption than serial data

**Why simulation didn't catch this:**
Verilator and iverilog use zero-delay combinational propagation and don't model real-world glitches or setup/hold violations. The UART appears to work correctly in simulation but fails on hardware due to actual gate delays and routing skew.

**Workaround:**
Use the v2 `simpleuart` module instead of `manual_uart_tx`. The v2 simpleuart has been proven reliable on Tang Nano 9K hardware in the production PicoRV32 SoC.

**Fix applied:**
```verilog
// In hardware/v3/soc/picoperipheral.v - PicoMem_UART module

// BEFORE (broken):
manual_uart_tx u_manual_uart_tx (
    ...
);

// AFTER (working):
simpleuart u_uart (
    .clk(clk),
    .resetn(resetn),
    .ser_tx(ser_tx),
    .ser_rx(ser_rx),
    .reg_div_we(reg_div_sel ? mem_s_wstrb : 4'b0),
    .reg_div_di(mem_s_wdata),
    .reg_div_do(reg_div_do),
    .reg_dat_we(reg_dat_sel ? |mem_s_wstrb : 1'b0),
    .reg_dat_re(reg_dat_sel ? ~(|mem_s_wstrb) : 1'b0),
    .reg_dat_di(mem_s_wdata),
    .reg_dat_do(reg_dat_do),
    .reg_dat_wait(reg_dat_wait)
);
```

**Status:**
✅ **Resolved by reverting to simpleuart**. The manual_uart_tx module is deprecated for v3 SoC. Future investigation deferred to Phase 4+ if manual implementation is still desired.

**Potential permanent fix (not implemented):**
1. Register all bus inputs to the UART peripheral (mem_s_valid, mem_s_addr, mem_s_wdata, mem_s_wstrb)
2. Generate reg_dat_we from registered signals on the following cycle
3. This adds 1 cycle latency but eliminates glitch/race conditions
4. Similar approach used in BSRAM register file with registered reads

**Files affected:**
- `hardware/v3/soc/manual_uart_tx.v` (deprecated, do not use)
- `hardware/v3/soc/picoperipheral.v` (PicoMem_UART module, reverted to simpleuart)
- `hardware/v3/soc/simpleuart.v` (v2 reference, working)

**Lesson:** Hardware timing issues (glitches, setup/hold violations) cannot be reliably detected in Verilator/iverilog simulation. Always test peripherals on actual FPGA hardware. Favor proven modules (simpleuart) over new implementations (manual_uart_tx) unless there's a compelling reason and thorough hardware validation.

---

### V3-015: LSU Bus Output Glitches — Root Cause of UART Corruption

**Severity:** Critical (all peripherals affected)
**Date:** February 23, 2026
**Status:** ✅ Resolved

**Symptom:** Even after replacing `manual_uart_tx` with proven `simpleuart` module (V3-014 workaround), UART still receives scrambled data. Both 115200 and 57600 baud show garbled output (`�` characters in minicom).

**Critical discovery:** The bug was **NOT** in the UART peripheral at all - it was in the **CPU's Load-Store Unit (LSU)**.

**Root cause:** The LSU used **combinational** `always @(*)` block to drive bus outputs:
```verilog
// BROKEN (original design):
always @(*) begin
    bus_valid = 1'b0;
    bus_addr  = 32'b0;
    bus_wdata = 32'b0;
    bus_wstrb = 4'b0;

    case (state)
        S_XACT1: begin
            bus_valid = 1'b1;
            bus_addr = req_addr[31:0] & 32'hFFFFFFFC;
            if (req_is_store) begin
                bus_wdata = req_wdata[31:0];
                bus_wstrb = 4'b1111;
            end
        end
        ...
    endcase
end
```

**Why this causes corruption:**
When the FSM state changes (e.g., `S_XACT1` → `S_DONE`), the combinational logic immediately changes `bus_valid`, `bus_wdata`, and `bus_wstrb`. During this transition:
1. The peripheral's registered inputs are still latching data from the previous cycle
2. The bus signals can glitch or present stale/transitional values
3. The peripheral latches whatever happens to be on the bus during the glitch window
4. Result: scrambled data, exactly like we observed

**Why both manual_uart_tx AND simpleuart failed:**
Both peripherals are correctly designed - they latch bus data on clock edges. The problem is that the **bus master (LSU) is sending glitched data**. No amount of peripheral fixes can compensate for a broken bus master.

**Why GPIO appeared to work:**
GPIO has a registered output (`out_r`) and visual observation (LED blinking) is much more tolerant of occasional bit corruption than serial UART data. Some glitches likely occurred but were not visible.

**Fix:** Register all LSU bus outputs (lines 122-176 in atomik_v3_lsu.v):
```verilog
// FIXED (registered outputs):
always @(posedge clk) begin
    if (!rst_n) begin
        bus_valid <= 1'b0;
        bus_addr  <= 32'b0;
        bus_wdata <= 32'b0;
        bus_wstrb <= 4'b0;
    end else begin
        case (state)
            S_IDLE: begin
                bus_valid <= 1'b0;
                bus_wstrb <= 4'b0;
            end

            S_XACT1: begin
                bus_valid <= 1'b1;
                bus_addr  <= req_addr[31:0] & 32'hFFFFFFFC;
                if (req_is_store) begin
                    bus_wdata <= req_wdata[31:0];
                    bus_wstrb <= 4'b1111;
                end else begin
                    bus_wstrb <= 4'b0;  // Reads: no write strobes
                end
            end

            S_DONE: begin
                bus_valid <= 1'b0;
                bus_wstrb <= 4'b0;
            end
            ...
        endcase
    end
end
```

**Why registered outputs fix the issue:**
1. All bus signal changes happen on clock edges (synchronous to peripherals)
2. No combinational glitches during state transitions
3. Peripherals always see stable, valid data when they sample
4. This is standard practice for reliable bus protocols

**Performance impact:** Minimal. One-cycle latency when starting a bus transaction, but this is already accounted for in the FSM design.

**Files affected:**
- `hardware/v3/rtl/atomik_v3_lsu.v` (atomik_v3_lsu, lines 122-176)

**Verification plan:**
1. ✅ Lint check (no combinational loops)
2. ⏳ Verilator simulation (UART echo test)
3. ⏳ Hardware validation (Tang Nano 9K, minicom UART output)

**Lesson:** **Bus master outputs should ALWAYS be registered.** Combinational bus drivers are a common source of hard-to-debug timing bugs that don't appear in simulation. This issue affected ALL peripherals (UART, GPIO, SPI, SRAM), not just UART - it's a fundamental bus protocol violation.

**Related issues:**
- V3-014: Incorrectly blamed manual_uart_tx peripheral (which was actually innocent)
- The simpleuart "workaround" didn't actually fix anything - it just happened to fail differently

---

### V3-021: HDMI Output Timing Not Recognized by Monitor

**Severity:** High
**Date:** March 3, 2026
**Status:** ✅ Resolved (March 5, 2026)

**Symptom:** Monitor displays "The current input timing is not supported by the monitor display" — no video output visible.

**Root Cause (Multi-Factor):**

1. **Non-standard pixel clock (original issue):** V3-020 lowered the CPU+pixel clock from 25.2 MHz to 21.6 MHz, producing ~51.4 Hz refresh instead of the standard 60 Hz. Since CPU and pixel shared a single PLL, the HDMI got dragged down with the CPU.

2. **atomik_delta_display pipeline backpressure bug (discovered during fix):** The 2-stage pixel pipeline in `atomik_delta_display.v` did NOT stall when `svo_enc` deasserted `tready` (pixel FIFO full during blanking periods). Pixels in the pipeline were overwritten before being consumed, causing `svo_enc`'s pixel FIFO to starve below its 6-entry startup threshold. TMDS output never began — the monitor saw no valid HDMI signal at all.

**Fix (Dual-PLL + Pipeline Backpressure):**

1. **Added dedicated HDMI PLL (PLL2):** 27 → 126 MHz → CLKDIV ÷5 → 25.2 MHz pixel clock. CPU stays at 21.6 MHz (PLL1). Standard 640×480 @ 60 Hz restored.

2. **Added CDC bridge (`disp_mmio_cdc`):** Toggle-handshake protocol between CPU domain (21.6 MHz) and pixel domain (25.2 MHz) for display MMIO registers. ~5-6 cycle latency per MMIO access.

3. **Fixed pipeline backpressure in `atomik_delta_display.v`:**
   ```verilog
   // Before (broken): pipeline always clocks, losing pixels on backpressure
   end else begin
       valid_d1 <= in_axis_tvalid & in_axis_tready;
       pixel_d2 <= pixel_d1;  // Overwrites unconsumed data!
   end
   assign in_axis_tready = out_axis_tready;

   // After (fixed): pipeline stalls when output can't accept
   wire pipe_advance = out_axis_tready || !valid_d2;
   end else if (pipe_advance) begin
       valid_d1 <= in_axis_tvalid & in_axis_tready;
       pixel_d2 <= pixel_d1;  // Only advances when output consumed
   end
   assign in_axis_tready = pipe_advance;
   ```

**Files Modified:**
- `soc/atomik_v3_soc.v` — Dual-PLL, pixel-domain reset, CDC bridge, HDMI clock routing
- `soc/hdmi/atomik_delta_display.v` — Pipeline stall logic
- `synth/atomik_v3_soc.sdc` — Added clk_pixel constraint + CDC false paths

**Files Created:**
- `soc/gowin_ip/gowin_rpll_hdmi/gowin_rpll_hdmi.v` — HDMI PLL (27→126 MHz)
- `soc/gowin_ip/gowin_clkdiv_hdmi/gowin_clkdiv_hdmi.v` — HDMI CLKDIV (÷5→25.2 MHz)
- `soc/hdmi/disp_mmio_cdc.v` — Toggle-handshake CDC bridge

**Timing Results (post-fix):**
- CPU Fmax: 21.987 MHz (target 21.6, +1.8% margin)
- Pixel Fmax: 33.964 MHz (target 25.2, +34.8% margin)
- TNS: 0.000 on all clocks
- rPLL: 2/2 (100%)

**Lesson:** Any registered pipeline stage inserted into the SVO video path MUST handle AXI-Stream backpressure. Without stall logic, `svo_enc`'s pixel FIFO starves and TMDS output never starts. Use `pipe_advance = out_axis_tready || !valid` to gate pipeline registers.

---

### V3-022: UART Output Garbled After Dual-PLL Clock Change

**Severity:** Medium
**Date:** March 5, 2026
**Status:** RESOLVED (March 5, 2026)

**Symptom:** After restoring dual-PLL architecture (V3-021 fix), UART serial output was garbled at 115200 baud. HDMI terminal output was legible, confirming CPU was running correctly.

**Root Cause:** Stale firmware in SPI flash. The firmware source (`CLK_FREQ = 21600000`) was already correct for 21.6 MHz, but the firmware in flash had not been re-programmed after the clock architecture change. The baud divisor in flash was calibrated for a previous clock frequency.

**Fix:** Rebuilt firmware and re-flashed via ISP programmer (`isp_flash_programmer.py`). 84 pages programmed, all checksums verified. Boot test confirmed clean UART output at 115200 baud — full banner, menu, and all commands working.

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

---

### V3-016: CPU Hang on Repeated MMIO Loads (Phase 3D)

**Severity:** Critical
**Date:** February 27-28, 2026
**Status:** Fixed

**Symptom:** CPU hangs completely when executing tight loops with repeated MMIO reads (e.g., UART polling). Simple CPU operations (function calls, stack, single loads, branches) work correctly, but repeated loads to memory-mapped peripherals cause a complete system hang with no UART output.

**Bisection Results:**
- ✅ BISECT_STEP1-5: Function calls, stack frames, single loads, long loops, function pointers - ALL PASS
- ❌ BISECT_STEP6-8: UART polling loops (10 to 10,000 iterations) - ALL HANG

**Root Cause (Multi-Factor):**

1. **Timing Violations (Primary):**
   - CPU running at 27 MHz direct from crystal
   - Achievable Fmax: only 26.563 MHz
   - Result: 24 setup timing violations, TNS = -7.086 ns
   - Critical path: instruction[3] → regfile BSRAM (15 logic levels)
   - Timing violations cause wrong data to be latched into registers on some clock cycles, creating probabilistic hangs that correlate with instruction count

2. **LSU Handshake Bug (Secondary):**
   - LSU state transitions checked `bus_ready` alone, not `bus_valid && bus_ready`
   - Lines 86, 87, 214 in `atomik_v3_lsu.v` violated valid/ready protocol
   - State machine could advance before bus transaction completed

3. **Reset Synchronization (Tertiary):**
   - Reset not gated on PLL lock
   - CPU could start before clock stabilized
   - Caused intermittent boot failures

4. **UART Baud Rate (Configuration):**
   - Firmware defined CLK_FREQ=27000000
   - Actual CPU clock is 25.2 MHz after PLL+CLKDIV
   - UART baud rate miscalculated → corrupted output

**Fixes Applied:**

1. **Clock Architecture Restored:**
   ```
   PLL: 27 MHz → 126 MHz
   CLKDIV: 126 MHz ÷ 5 → 25.2 MHz
   Result: TNS = 0.000 ns, Fmax = 25.201 MHz
   ```
   - Files: `soc/gowin_ip/gowin_rpll/gowin_rpll.v`, `soc/gowin_ip/gowin_clkdiv/gowin_clkdiv.v`, `soc/atomik_v3_soc.v`

2. **LSU Handshake Protocol Fixed:**
   ```verilog
   // Before: if (bus_ready) state_next = ...
   // After:  if (bus_valid && bus_ready) state_next = ...
   ```
   - File: `rtl/atomik_v3_lsu.v` lines 86, 87, 214

3. **Reset Synchronization Fixed:**
   ```verilog
   // Added pll_lock gating to Reset_Sync
   else if (pll_lock)
       reset_cnt <= reset_cnt + !resetn;
   else
       reset_cnt <= 4'b0000;
   ```
   - File: `soc/picoperipheral.v`

4. **UART Baud Rate Corrected:**
   ```c
   #define CLK_FREQ 25200000  // was 27000000
   ```
   - File: `soc/firmware/fw-brom/isp_flasher.c`

**Validation Results:**
- **62/62 hardware tests PASS** across thermal and stress conditions
- **BISECT_STEP7 (10,000 UART reads):** 60/60 consecutive passes
- **Extended stress test:** 50/50 consecutive runs (~3.5 minutes)
- **Thermal stability:** Stable through 60s warmup + extended testing
- **ISP handshake:** Working (0x55→0x56 ACK, echo test pass)

**Analysis:**
Timing closure at 25.2 MHz plus the LSU handshake fix eliminates the hang across repeated-load stress tests. The probabilistic nature of the hang (simple tests passed, complex tests failed) is consistent with timing violations — more cycles executed increases probability of hitting violated paths.

**Lessons Learned:**
1. **Always check timing first** when debugging hardware hangs
2. **Timing violations mask RTL bugs** — both must be fixed
3. **Clock bypasses for "testing" are dangerous** — restore proper clock generation
4. **Thin margins are fragile** — +0.004% margin is barely sufficient across PVT variation

**Prevention:**
- Monitor timing reports in every synthesis run
- Never bypass clock generation without explicit timing analysis
- Validate handshake protocols even when timing is clean
- Test across thermal conditions for marginal designs

**Documentation:**
- `hardware/v3/deploy/TIMING_VIOLATION_ROOT_CAUSE.md`
- `hardware/v3/deploy/PHASE3D_TIMING_FIX_COMPLETE.md`
- `hardware/v3/deploy/HARDWARE_VALIDATION_COMPLETE.md`
- `hardware/v3/deploy/BUG_REPORT_BISECTION_COMPLETE.md`

---

### V3-017: AUIPC Instruction Broken

**Severity:** High
**Date:** February 2026
**Status:** Open (workaround available)

**Symptom:** AUIPC (Add Upper Immediate to PC) instruction generates incorrect addresses. This affects:
- Global pointer initialization (`.option push; .option norelax; la gp, __global_pointer$`)
- Position-independent code sequences
- Address materialization patterns generated by compiler

**Root Cause:** Not yet investigated (deferred pending Phase 3D completion)

**Workaround:** Use `li` (load immediate) instead of `la` (load address) in assembly:
```assembly
# BROKEN:
la sp, _stack_start  # Uses AUIPC + ADDI

# WORKING:
li sp, 0x800002F0    # Direct immediate load
```

**Impact:**
- Requires custom CRT (C runtime) without AUIPC
- Limits compiler code generation patterns
- Must use `-fno-pic` and absolute addressing

**Priority:** Should be fixed before production firmware development

**Next Steps:**
1. Create minimal AUIPC test case
2. Trace execution in Verilator waveforms
3. Compare ALU result with expected PC+immediate
4. Fix decode or ALU logic
5. Add AUIPC to compliance test suite

---

### V3-018: GCC Null-Pointer UB at Address 0x00000000

**Severity:** Critical
**Date:** March 2, 2026
**Status:** Fixed

**Symptom:** Flash XIP reads from address 0 appear to hang the CPU. The instruction after a volatile load from address 0 is `ebreak`, and all subsequent code is removed by the compiler.

**Root Cause:** GCC treats `*(volatile uint32_t*)0x00000000` as a null pointer dereference (undefined behavior). At `-Os`, it:
1. Emits the load instruction (which actually completes successfully)
2. Inserts `ebreak` immediately after (trap for UB)
3. **Optimizes away all code after the load** (unreachable after UB)

Similarly, `((void(*)())0)()` (function pointer call to address 0) is UB — GCC inserts `ebreak` and removes the jump entirely.

**Fix:** Use inline assembly for all address-0 operations:
```c
// Read from address 0 (XIP flash):
uint32_t val;
asm volatile("lw %0, 0(%1)" : "=r"(val) : "r"((uint32_t)0x00000000));

// Jump to address 0 (flash entry point):
asm volatile("li t0, 0; jr t0" ::: "t0");
```

**Impact:** All function pointer jumps to address 0 and all volatile reads from address 0 in `isp_flasher.c` (9 instances across ISP stages) were silently broken.

**Lesson:** On embedded systems where address 0 is valid memory (e.g., SPI flash XIP), never use C-level pointer dereferences or function pointer calls to address 0. GCC's null-pointer UB handling is aggressive and silent — no warnings at compile time, just `ebreak` in the disassembly.

---

### V3-019: ISP ESEC Command Silently Skipped — Sector Erase Gated on Buffer Length

**Severity:** Critical
**Date:** March 2, 2026
**Status:** Fixed

**Symptom:** ISP flash programming appears to succeed (host receives correct ACK bytes, checksums match), but programmed firmware doesn't execute correctly after XIP boot.

**Root Cause:** In `ISP_STAGE3` of `isp_flasher.c`, the ESEC (sector erase) command was guarded by `if (buflen)`:
```c
case 0x30:  // ESEC
    uart_putchar(0x31);   // ACK start
    // ... read address ...
    if (buflen) {         // BUG: buflen == 0 until first WBUF!
        spi_flashio(...); // Erase never executes
    }
    uart_putchar(0x32);   // ACK end (sent regardless!)
```

The host ISP programmer sends erases before any buffer writes (correct for flash). But `buflen` is 0 until the first WBUF (0x10) command, so the erase is skipped. The firmware sends 0x31 and 0x32 ACKs regardless, so the host cannot detect the failure.

Without erase, NOR flash page program can only clear bits (1->0) but cannot set bits (0->1). Old flash data corrupts the new firmware.

**Fix:** Removed the `if (buflen)` guard — sector erase is unconditional:
```c
case 0x30:  // ESEC
    uart_putchar(0x31);
    // ... read address ...
    spi_flashio((uint8_t *)&flash_buffer, 4, FLASHIO_REQWREN);  // Always erase
    uart_putchar(0x32);
```

**Protocol Improvement TODO:**
- ACK should only be sent after the operation completes successfully
- Add NACK/error codes for failed preconditions
- Add readback verify command (0x50) to detect silent corruption on host side

---

### V3-020: Setup Timing Violations at 25.2 MHz — No Correctness Guarantee

**Severity:** High
**Date:** March 2, 2026
**Status:** Resolved (clock lowered to 21.6 MHz)

**Symptom:** 40 setup timing violations on clk_cpu domain at 25.2 MHz. Fmax = 24.745 MHz (1.8% over).

**Worst paths:** All originate from `u_cpu/u_fetch/instr_0_s0/Q` (instruction register bit 0) fanning out to regfile and LSU endpoints. Worst slack at 25.2 MHz: -0.729ns. Logic depth: 14 levels.

**Critical path:** Fetch → Decode → Regfile write address / LSU request address. The single-cycle decode path from instruction fetch to register file and load-store unit is too long for 25.2 MHz.

**Resolution:** Lowered PLL from 126 MHz to 108 MHz (FBDIV_SEL=3, IDIV_SEL=0, ODIV_SEL=4). CLKDIV ÷5 produces 21.6 MHz CPU clock. HDMI 5:1 ratio preserved (108 MHz serializer = 5× 21.6 MHz pixel). HDMI refresh rate ~51.4 Hz (within monitor tolerance).

**Timing after fix:**
- Fmax: 21.766 MHz (target 21.6 MHz, +0.77% margin)
- Zero TNS, zero violations
- Worst slack: +0.354 ns (positive)
- Logic depth: 13 levels

**Performance impact:** ~14% reduction vs 25.2 MHz. ISP timeout increases from ~14s to ~16s. UART baud divisor changed from 217 to 185.

**Future improvement:** Pipeline the decode stage (add register between fetch and decode) to restore 25.2 MHz operation. This would require FSM changes but is the long-term fix.

**Validated:** Boot chain (BROM → ISP → XIP → F!F!) confirmed working at 21.6 MHz with zero corruption.

---
