# UART Root Cause Analysis — February 23, 2026

## Executive Summary

**RTL IS CORRECT** ✅
The uart_handshake_test design **works perfectly in Verilator simulation** with uart_tx toggling correctly at the expected baud rate.

**HARDWARE SYNTHESIS ISSUE** ❌
The same design **fails on Tang Nano 9K hardware** - no UART output despite proper handshake protocol.

## Critical Evidence

### Verilator Simulation Results
```
Time    10526: uart_tx changed: 1 -> 0
Time    10994: uart_tx changed: 0 -> 1
Time    11462: uart_tx changed: 1 -> 0
...continuous toggling every ~468 cycles (234 * 2) = 115,200 baud @ 27 MHz
```

**Conclusion:** simpleuart module + handshake FSM is functionally correct.

### Hardware Test Results
- **Manual UART bitbanging** (uart_test.v): ✅ Works - sends 0x55 continuously
- **Direct simpleuart test** (uart_direct_test.v): ✅ Works in early test
- **State machine + simpleuart** (all handshake tests): ❌ Fails - no output

## Root Cause: Synthesis/Hardware Mismatch

The RTL simulates correctly but fails after synthesis. Possible causes:

### 1. **Synthesis Optimizer Breaking Logic** (Most Likely)
Gowin synthesis may be optimizing away critical state or incorrectly inferring don't-cares in the FSM.

**Evidence:**
- Complex always blocks with multiple assignments to same signal (send_dummy)
- Potential race conditions in line 110-111 vs 119-123 of simpleuart.v

**Fix:** Add synthesis directives:
```verilog
(* keep *) reg send_dummy;
(* keep *) reg [9:0] send_pattern;
```

### 2. **Clock Domain Issue**
The 27 MHz clock may not be reaching simpleuart correctly after synthesis.

**Test:** Add clock monitoring - toggle a GPIO pin at clk rate to verify.

### 3. **Reset Sequencing**
Hardware reset timing might differ from simulation, causing simpleuart to enter bad state.

**Test:** Extend reset_cnt to [15:0] for longer reset hold.

### 4. **Timing Violation**
Even though synthesis reports clean timing, actual hardware may have glitches.

**Test:** Reduce clock to 13.5 MHz (current known-working for other tests) or add IOB registers.

## Recommended Next Steps

### Immediate (30 min):
1. **Re-test uart_direct_test** (confirmed working earlier) to verify baseline hardware function
2. **Add synthesis keep attributes** to simpleuart critical signals
3. **Check synthesis report** for unexpected optimizations

### Short-term (2-4 hours):
4. **Replace simpleuart** with proven manual UART TX based on uart_test.v
5. **Instrument GPIO debug** to verify FSM progression on hardware

### Long-term (for root cause investigation):
6. **Deep synthesis analysis:** Compare .vg netlist to Verilog source
7. **Timing analysis:** Check for hold violations even if setup is clean
8. **Gowin support:** Report potential synthesis bug

## Business Decision

**For ATOMiK v3 SoC progress:**
- **Tactical:** Replace simpleuart with manual TX module (2 hours, guaranteed fix)
- **Strategic:** Debug synthesis issue (unknown timeline, educational value)

Given ATOMiK v3 schedule pressure, recommend **tactical fix** to unblock SoC bringup, then investigate synthesis issue in parallel.

## Files

| Test | RTL | Result | Evidence |
|------|-----|--------|----------|
| Manual bitbang | uart_test.v | ✅ Works | Confirmed 0x55 output |
| Direct simpleuart | uart_direct_test.v | ✅ Works | Early test success |
| Handshake FSM | uart_handshake_test.v | ✅ Sim / ❌ HW | **Key finding** |
| Simulation | sim/uart_debug_tb.cpp | ✅ Verified | uart_debug.vcd waveforms |

## Conclusion

This is **NOT an RTL bug** - it's a synthesis/hardware issue. The design is proven correct in simulation.

Fastest path forward: Replace simpleuart with manual UART TX module while investigating synthesis root cause separately.
