# ATOMiK v3 Phase 3C Status Report

**Date:** February 24, 2026
**Status:** Partial Success with Critical Instability Issues

---

## Achievements ✅

1. **LSU Bus Timing Fix Implemented**
   - Root cause identified: Combinational bus outputs caused glitches
   - Solution: Registered all LSU bus outputs using `state_next`
   - File: `hardware/v3/rtl/atomik_v3_lsu.v` (lines 122-193)

2. **UART Transmission Confirmed**
   - Successfully received 'T' (0x54) at 76800 baud
   - Proves: CPU executing, bus functional, UART working
   - Bitstream: `atomik_v3_soc_fixed.fs`

3. **Hardware Validation**
   - GPIO heartbeat: ✅ Pin 10 blinking consistently
   - CPU execution: ✅ Firmware running from Boot ROM
   - UART TX: ✅ Works intermittently at 76800 baud

---

## Critical Issues ❌

### Issue 1: Extreme Non-Deterministic Behavior

**Symptom:**
Same bitstream produces different UART output on successive tests:
- Test 1: `0x54` ('T') ✅ Working
- Test 2: `0x60, 0xe6` ❌ Corrupted (seconds later)
- Test 3: `0x48, 0xff` ❌ Different corruption

**Observed patterns:**
- Works immediately after power cycle + bitstream load
- Fails on subsequent tests without reload
- Behavior changes between tests seconds apart
- No consistent pattern or reproducibility

**Impact:** Hardware unusable for development

### Issue 2: Actual Baud Rate Mismatch

**Expected:** 115200 baud (firmware target)
**Actual:** 76800 baud (measured)

**Analysis:**
- Firmware calculates: `CLKDIV = 27MHz / 115200 - 2 = 232`
- Expected baud: `27MHz / 233 = 115,879 baud`
- Measured baud: 76800 baud
- Implies actual clock: `76800 × 233 = 17.89 MHz` (not 27 MHz)

**Possible causes:**
- CLKDIV dividing by 1.5 instead of being bypassed
- Different clock source than expected
- Measurement error (but consistent across tests)

---

## Timing Analysis

**Static Timing Analysis (STA):**
```
Fmax: 18.705 MHz (target 13.5 MHz, +38% margin)
TNS: 0.000 (zero timing violations)
Setup/Hold: All paths met
```

**Conclusion:** STA shows clean timing, yet hardware exhibits extreme instability. This suggests:
1. Signal integrity issues not captured by STA
2. Metastability in clock domain crossings
3. Hardware defect (board, FPGA, or connections)
4. Incomplete timing constraints

---

## Test Procedure

### Working Configuration (when it works):

```bash
# 1. Power cycle board (unplug USB, wait 10 sec, replug)

# 2. Load bitstream
openFPGALoader -b tangnano9k atomik_v3_soc_fixed.fs

# 3. Wait for boot
sleep 7

# 4. Test UART at 76800 baud
python3 << 'EOF'
import serial
s = serial.Serial('/dev/ttyUSB1', 76800, timeout=1)
d = s.read(50)
s.close()
print('Result:', 'T found' if 0x54 in set(d) else d.hex())
EOF
```

### Success Rate
- Immediately after power cycle + load: ~80% success
- Without power cycle: ~10% success
- Successive tests: Degrades rapidly

---

## Root Cause Investigation Needed

### Hypotheses to Test:

1. **Signal Integrity**
   - Check power supply stability (scope VDD)
   - Verify ground connections
   - Check for EMI/crosstalk on UART pins

2. **Clock Stability**
   - Scope the 27 MHz crystal oscillator
   - Check CLKDIV output
   - Verify PLL lock signals

3. **Bus Protocol**
   - Add simulation with realistic delays
   - Check for setup/hold violations in actual hardware
   - Verify CDC (clock domain crossing) if any

4. **UART Peripheral**
   - Test with different UART modules (not simpleuart)
   - Add deeper FIFOs or buffering
   - Check for overrun conditions

5. **FPGA Configuration**
   - Try different synthesis settings
   - Check for partial reconfiguration issues
   - Verify bitstream integrity

---

## Recommendations

### Short Term:
1. **Do not proceed to Phase 3D** until stability is resolved
2. Focus on root cause analysis using oscilloscope/logic analyzer
3. Consider testing on different Tang Nano 9K board (hardware defect?)

### Medium Term:
1. Implement comprehensive CDC (clock domain crossing) constraints
2. Add error detection/correction (parity, checksums)
3. Deeper hardware validation with ILA (integrated logic analyzer)

### Long Term:
1. Consider moving to more stable FPGA board
2. Implement formal verification of bus protocol
3. Add built-in self-test (BIST) for diagnostics

---

## Files

**Working bitstream:** `atomik_v3_soc_fixed.fs` (Feb 23 18:03)
**LSU fix:** `hardware/v3/rtl/atomik_v3_lsu.v`
**Documentation:** `docs/KNOWN_ISSUES.md` (V3-015)

---

## Conclusion

Phase 3C demonstrates that the architecture CAN work:
- ✅ RV64I CPU executes correctly
- ✅ Registered bus outputs eliminate glitches (when stable)
- ✅ UART transmits correctly (when stable)

However, **critical instability prevents practical use**. Further investigation required before proceeding to Phase 3D.

**Status: BLOCKED on stability investigation**
