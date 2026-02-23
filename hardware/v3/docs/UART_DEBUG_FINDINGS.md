# UART Debugging Findings — February 23, 2026

## Problem Statement
ATOMiK v3 SoC has no UART output on Tang Nano 9K hardware. Pin 15 LED indicates CPU is running and writing to UART registers, but no data appears on /dev/ttyUSB1.

## Hardware Validation Tests

### ✅ Test 1: Manual UART Bitbanging (27 MHz)
**File:** `hardware/v3/test/uart_test.v`
**Result:** **SUCCESS** - Continuously sends 0x55 bytes
**Proves:**
- Hardware connectivity (pin 17 → FT2232H → /dev/ttyUSB1) is perfect
- Baud rate timing is correct (27 MHz / 234 = 115,385 baud)
- FPGA can successfully output UART signals

### ✅ Test 2: Direct simpleuart (13.5 MHz via CLKDIV)
**File:** `hardware/v3/test/uart_direct_test.v`
**Result:** **SUCCESS** - Continuously sends 0x55 bytes
**Proves:**
- simpleuart module works when driven correctly
- CLKDIV ÷2 clock generation is functional
- Baud rate calculation is correct (13.5 MHz / 115 = 117,187 baud)

### ❌ Test 3: simpleuart + PicoMem_UART Wrapper
**File:** `hardware/v3/test/soc_uart_test.v`
**Result:** **FAILURE** - No UART output
**Configuration:** State machine drives PicoMem_UART wrapper at 13.5 MHz
**Conclusion:** Issue is in the PicoMem_UART wrapper integration

### ❌ Test 4: simpleuart without wrapper (13.5 MHz)
**File:** `hardware/v3/test/uart_nowrapper_test.v`
**Result:** **FAILURE** - No UART output
**Configuration:** State machine drives simpleuart directly at 13.5 MHz
**Conclusion:** Issue is NOT in PicoMem_UART wrapper!

### ❌ Test 5: simpleuart without wrapper (27 MHz)
**File:** `hardware/v3/test/uart_27mhz_test.v`
**Result:** **FAILURE** - No UART output
**Configuration:** State machine drives simpleuart directly at 27 MHz
**Conclusion:** Issue is in how the state machine drives simpleuart

## Root Cause Analysis

### Key Observation
The **ONLY** difference between working and non-working tests:

**WORKING (uart_direct_test):**
```verilog
always @(posedge clk) begin
    if (bit_counter >= 234) begin
        bit_counter <= 0;
        case (bit_index)
            0: uart_tx <= 1'b0;  // Direct assignment
            1: uart_tx <= data_byte[0];
            // ... etc
        endcase
    end else begin
        bit_counter <= bit_counter + 1;
    end
end
```

**NOT WORKING (uart_27mhz_test):**
```verilog
always @(posedge clk) begin
    // ... state machine ...
    3: begin
        if (!reg_dat_wait) begin
            reg_dat_we_r <= 1;  // Register write to simpleuart
            reg_dat_di_r <= 8'h55;
        end
    end
end
```

### Hypothesis
There is a **timing or sequencing issue** in how simpleuart responds to `reg_dat_we` and `reg_div_we` signals. The state machine approach may be:
1. Asserting signals at the wrong time
2. Not meeting setup/hold requirements
3. Triggering a corner case in simpleuart's FSM

### Synthesis Analysis
- simpleuart module IS being synthesized (not optimized away)
- `send_pattern[0]` IS connected to `uart_tx`
- `send_pattern[0]` flip-flop has unusual SET signal (`n1006_8`)
- This SET signal may be holding uart_tx HIGH (idle state)

## Next Steps

### Option 1: Fix simpleuart State Machine Interaction
Investigate why simpleuart FSM doesn't respond correctly to register writes from a state machine but works fine with direct bitbanging.

**Specific areas to check:**
1. `send_dummy` logic (lines 110-123) - does it get stuck?
2. `cfg_divider` initialization (line 57: defaults to 1) - timing issue?
3. Reset behavior - is simpleuart properly reset before first use?

### Option 2: Replace simpleuart with Known-Good UART
Since manual bitbanging works, implement a proper UART TX module based on the working uart_test.v pattern.

### Option 3: Add Simulation
Create a Verilator testbench for uart_27mhz_test to observe internal simpleuart signals:
- `send_pattern`
- `send_bitcnt`
- `send_divcnt`
- `cfg_divider`
- `send_dummy`

This will show exactly where the FSM gets stuck.

### Option 4: Hardware Debug with Logic Analyzer
If available, probe:
- `uart_tx` (pin 17)
- `reg_dat_we` / `reg_div_we` signals
- `send_pattern[0]` internal signal

## Files Created During Debug

| File | Purpose | Result |
|------|---------|--------|
| `test/uart_test.v` | Manual UART bitbanging | ✅ Works |
| `test/uart_direct_test.v` | Direct simpleuart test | ✅ Works |
| `test/soc_uart_test.v` | PicoMem_UART wrapper test | ❌ Fails |
| `test/uart_nowrapper_test.v` | simpleuart without wrapper | ❌ Fails |
| `test/uart_27mhz_test.v` | 27 MHz simpleuart test | ❌ Fails |

## Conclusion

The issue is **NOT**:
- Hardware connectivity ✅
- Clock generation ✅
- Baud rate calculation ✅
- simpleuart module (when driven correctly) ✅
- PicoMem_UART wrapper ✅

The issue **IS**:
- How the state machine drives simpleuart register interface ❌
- Likely a timing/sequencing bug in simpleuart FSM when used with pulsed register writes

**Recommendation:** Simulate uart_27mhz_test in Verilator to observe internal simpleuart state and identify where the TX FSM gets stuck.
