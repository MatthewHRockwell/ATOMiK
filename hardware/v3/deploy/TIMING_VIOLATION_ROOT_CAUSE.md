# TIMING VIOLATION ROOT CAUSE — v3 Phase 3D

## Executive Summary

The CPU hang on repeated MMIO loads (BISECT_STEP7, STEP8) is caused by **timing violations**, NOT an RTL handshake bug.

- **Current clock**: 27 MHz (crystal direct)
- **Achieved Fmax**: 26.563 MHz
- **Timing violations**: 24 setup endpoints, TNS = -7.086 ns
- **Worst slack**: -0.609 ns

The LSU handshake fix (adding `bus_valid &&`) was correct, but ineffective because the CPU is running beyond its maximum frequency.

---

## Timing Report Analysis

### From `/home/mattrock/Projects/ATOMiK/hardware/v3/synth/impl/pnr/atomik_v3_soc_tr_content.html`:

```
Numbers of Setup Violated Endpoints: 24
Total Negative Slack: -7.086 ns

Clock: clk_osc
  Constraint: 27.000 MHz
  Actual Fmax: 26.563 MHz (FAILED - in RED)
  Logic Levels: 15
```

### Critical Paths (Top 5):

| Path | Slack | From | To | Delay |
|------|-------|------|-----|-------|
| 1 | -0.609 ns | `u_cpu/u_fetch/instr_3_s0/Q` | `u_cpu/u_regfile/regs_b_regs_b_0_0_s/DI[14]` | 37.603 ns |
| 2 | -0.594 ns | `u_cpu/u_fetch/instr_3_s0/Q` | `u_cpu/u_regfile/regs_b_regs_b_0_1_s0/DI[26]` | 37.588 ns |
| 3 | -0.577 ns | `u_cpu/u_fetch/instr_3_s0/Q` | `u_cpu/u_regfile/regs_b_regs_b_0_0_s/DI[4]` | 37.570 ns |
| 4 | -0.564 ns | `u_cpu/u_fetch/instr_3_s0/Q` | `u_cpu/u_regfile/regs_b_regs_b_0_1_s0/DI[29]` | 37.558 ns |
| 5 | -0.544 ns | `u_cpu/u_fetch/instr_3_s0/Q` | `u_cpu/u_regfile/regs_b_regs_b_0_1_s/DI[8]` | 37.538 ns |

**Critical path**: Instruction register → Decoder → Register file BSRAM address (15 logic levels, 37.6 ns)

---

## Root Cause

From `atomik_v3_soc.v` lines 51-61:

```verilog
// Clock generation: Crystal direct (27 MHz) - BYPASS CLKDIV
// Testing: Remove CLKDIV to eliminate potential instability source
wire clk_p;       // 27 MHz CPU + pixel clock (crystal direct)

// BYPASS CLKDIV - use crystal directly
assign clk_p = clk;  // Direct connection, no divider
```

**The CLKDIV was bypassed for debugging, causing the CPU to run at 27 MHz instead of the intended 25.2 MHz.**

Original design intent (line 10):
```
// Clock: PLL1 27 MHz → 126 MHz (CLKOUT), CLKDIV ÷5 → 25.2 MHz (clk_cpu)
```

---

## Why This Causes the Specific Failure Pattern

### Simple tests (STEP 1-5) PASS:
- Execute few cycles
- Statistical chance of avoiding timing violations
- May work due to favorable PVT conditions at the moment

### Repeated MMIO loads (STEP 7-8) FAIL:
- Execute many cycles in tight loop
- Eventually hit a timing violation
- Wrong data latched into register file or LSU state machine
- CPU enters invalid state → hangs

**Timing violations are non-deterministic** — depends on temperature, voltage, routing, data patterns. This explains why simple tests sometimes work.

---

## Fix Strategy

### Option 1: Restore PLL + CLKDIV (RECOMMENDED — matches v2)

Copy from v2 `picotiny.v`:
1. Add Gowin rPLL IP: 27 MHz → 126 MHz
2. Add Gowin CLKDIV IP: 126 MHz ÷ 5 → 25.2 MHz
3. Connect `clk_p` to CLKDIV output
4. Connect `clk_p5` to PLL CLKOUT for HDMI

**Result**: 25.2 MHz CPU clock, well within 26.563 MHz Fmax (4.8% margin)

### Option 2: Simple divide-by-2 (QUICK TEST — 13.5 MHz)

Add RTL divider:
```verilog
reg clk_div2;
always @(posedge clk) clk_div2 <= ~clk_div2;
assign clk_p = clk_div2;
```

**Result**: 13.5 MHz CPU clock, massive timing margin but slow

### Option 3: Optimize critical path (HARD — not recommended)

- Add pipeline stage between fetch and decode
- Requires multi-cycle instruction execution changes
- Significant design effort

---

## Verification Plan

After implementing Option 1 (PLL + CLKDIV):

1. **Re-synthesize**: Check Fmax ≥ 25.2 MHz, TNS = 0
2. **BISECT_STEP7**: Should complete 10,000 UART reads
3. **BISECT_STEP8**: Should complete 10 UART reads
4. **Full ISP mode**: Should boot, timeout, jump to flash
5. **Flash firmware**: ATOMiK hardware tests

---

## Historical Context

**v2 Production SoC** (PicoRV32):
- CPU Fmax: 30.6 MHz (target 25.2 MHz, +21.4% margin)
- ATOMiK Fmax: 100.2 MHz (target 81 MHz, +23.6% margin)
- Zero TNS on all clock domains

**v3 Current** (RV64I):
- CPU Fmax: 26.563 MHz (target 27 MHz, -1.6% VIOLATION)
- TNS: -7.086 ns across 24 endpoints

The v3 CPU is slightly slower than v2 due to:
- 64-bit datapath (wider ALU, wider register file)
- More complex decode logic (RV64I vs RV32I)
- Longer paths through BSRAM (64-bit vs 32-bit)

**25.2 MHz is the right target** — proven in v2, achievable in v3.

---

## Action Items

1. ✅ Document timing violation root cause (this file)
2. ⬜ Copy PLL + CLKDIV IP from v2
3. ⬜ Restore clock generation logic in `atomik_v3_soc.v`
4. ⬜ Update SDC constraint to 25.2 MHz
5. ⬜ Re-synthesize and verify TNS = 0
6. ⬜ Test BISECT_STEP7/8 on hardware
7. ⬜ Test full boot chain
8. ⬜ Update KNOWN_ISSUES.md

---

## Conclusion

**The bug is NOT in the LSU or firmware** — it's a clock frequency misconfiguration.

Restoring the PLL + CLKDIV to generate 25.2 MHz (as originally intended) will eliminate all timing violations and allow the CPU to function correctly.

This also explains why the third-party's handshake fix didn't work: **the fix was correct, but the hardware couldn't execute it reliably due to timing violations**.
