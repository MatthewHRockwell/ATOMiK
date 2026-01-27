# ATOMiK Phase 3 Resource Utilization Report

**Date**: January 25, 2026  
**Target**: Gowin GW1NR-LV9QN88PC6/I5 (Tang Nano 9K)  
**Tool**: Gowin EDA V1.9.11.03 Education  
**Status**: ✅ All constraints met

---

## Executive Summary

The ATOMiK Core v2 delta architecture has been successfully synthesized on the Gowin GW1NR-9 FPGA. The design achieves timing closure at 94.5 MHz with excellent resource efficiency, using only 7% of available logic resources.

---

## Resource Utilization

### Summary Table

| Resource | Used | Available | Utilization |
|----------|------|-----------|-------------|
| **Logic (LUT/ALU/ROM16)** | 579 | 8,640 | **7%** |
| **Registers (FF)** | 537 | 6,693 | **9%** |
| CLS | 417 | 4,320 | 10% |
| I/O Port | 10 | 71 | 15% |
| PLL (rPLL) | 1 | 2 | 50% |
| BSRAM | 0 | 26 | 0% |

### Logic Breakdown

| Type | Count | Description |
|------|-------|-------------|
| LUT | 539 | Pure lookup tables |
| ALU | 40 | Arithmetic/logic units (counters) |
| ROM16 | 0 | Not used |

### Register Breakdown

| Type | Count | Description |
|------|-------|-------------|
| Logic FF | 535 | Standard flip-flops |
| I/O FF | 2 | I/O registers |
| Latches | 0 | No latches (good!) |

---

## Clock Resources

### Clock Summary

| Resource | Used | Available | Utilization |
|----------|------|-----------|-------------|
| PRIMARY | 2 | 8 | 25% |
| LW | 1 | 8 | 13% |
| GCLK_PIN | 2 | 3 | 67% |
| rPLL | 1 | 2 | 50% |

### Global Clock Signals

| Signal | Type | Source | Frequency |
|--------|------|--------|-----------|
| sys_clk_d | PRIMARY | External crystal | 27 MHz |
| clk_int | PRIMARY | PLL output | 94.5 MHz |
| rst_sync[2] | LW | Reset synchronizer | - |

---

## Timing Analysis

### Clock Frequency Summary

| Clock | Constraint | Achieved Fmax | Slack | Margin |
|-------|------------|---------------|-------|--------|
| sys_clk | 27.0 MHz | 174.5 MHz | +31.3 ns | **+547%** |
| atomik_clk | 94.5 MHz | 94.9 MHz | +0.049 ns | **+0.5%** |

### Critical Path

| Parameter | Value |
|-----------|-------|
| From | rx_data_6_s0/Q |
| To | byte_cnt_0_s0/CE |
| Clock | atomik_clk (94.5 MHz) |
| Data Delay | 10.490 ns |
| Clock Period | 10.582 ns |
| Slack | 0.049 ns |
| Logic Levels | 5 |

**Analysis**: The critical path is in the UART RX state machine (command parsing), not in the delta accumulator core. The core XOR operations have substantial timing margin.

### Timing Summary

| Analysis | Violated Endpoints | TNS |
|----------|-------------------|-----|
| Setup (sys_clk) | 0 | 0.000 |
| Hold (sys_clk) | 0 | 0.000 |
| Setup (atomik_clk) | 0 | 0.000 |
| Hold (atomik_clk) | 0 | 0.000 |

- **Paths Analyzed**: 1,521
- **Endpoints Analyzed**: 1,513
- **Setup Violations**: 0
- **Hold Violations**: 0

---

## Module Resource Breakdown (Estimated)

| Module | LUTs | FFs | Description |
|--------|------|-----|-------------|
| atomik_delta_acc | ~80 | 128 | Delta accumulator (64-bit XOR + registers) |
| atomik_state_rec | ~64 | 0 | State reconstructor (combinational XOR) |
| atomik_core_v2 | ~15 | 65 | Control logic + data_out register |
| UART RX | ~100 | 50 | Receiver state machine |
| UART TX | ~80 | 40 | Transmitter state machine |
| Command FSM | ~150 | 180 | Protocol handler + buffers |
| Misc (reset, LED) | ~90 | 74 | Support logic |
| **Total** | **~579** | **~537** | |

---

## I/O Bank Usage

| Bank | Used | Available | Utilization |
|------|------|-----------|-------------|
| Bank 1 | 1 | 25 | 4% |
| Bank 2 | 2 | 23 | 9% |
| Bank 3 | 7 | 23 | 31% |

---

## Power Estimate

Based on resource utilization and clock frequency:

| Parameter | Estimate |
|-----------|----------|
| Static Power | ~5 mW |
| Dynamic Power | ~10-15 mW |
| **Total** | **~15-20 mW** |

*Note: Actual power consumption may vary based on switching activity.*

---

## Recommendations

### Timing Margin

The atomik_clk has tight timing margin (0.049 ns slack). For production:
- Consider reducing PLL frequency to 81 MHz for ~20% margin
- Or add pipeline stages to UART command parsing

### Resource Headroom

With 93% logic available, the design supports significant expansion:
- Multiple delta channels (parallel processing)
- Wider data paths (128-bit or 256-bit)
- Additional debug/monitoring logic
- Application-specific processing

### Optimization Opportunities

1. **BSRAM**: Currently unused - could store lookup tables or configuration
2. **Additional PLLs**: One PLL remaining for separate clock domains
3. **I/O**: 61 pins remaining for external interfaces

---

## Build Information

| Parameter | Value |
|-----------|-------|
| Build Time | ~3 seconds |
| Placement | 0.975s |
| Routing | 0.589s |
| Output Gen | 0.938s |
| Peak Memory | 232 MB |

---

## Output Files

| File | Size | Description |
|------|------|-------------|
| ATOMiK.fs | ~290 KB | Bitstream for programming |
| ATOMiK.rpt.txt | ~50 KB | Resource report |
| ATOMiK.tr.html | ~100 KB | Timing report |
| ATOMiK.power.html | ~30 KB | Power analysis |

---

## Comparison: Target vs Achieved

| Metric | Target | Achieved | Status |
|--------|--------|----------|--------|
| Frequency | ≥50 MHz | 94.5 MHz | ✅ +89% |
| LUT Usage | ≤80% | 7% | ✅ Excellent |
| FF Usage | ≤80% | 9% | ✅ Excellent |
| Timing Closure | Pass | Pass | ✅ |
| ACCUMULATE Latency | 1 cycle | 1 cycle | ✅ |
| READ Latency | 1 cycle | 0 cycles* | ✅ |

*State reconstruction is combinational (zero additional latency)

---

## Hardware Validation Summary

All 10 hardware tests passing:
- ✅ Load/Read roundtrip
- ✅ Accumulator zero detection
- ✅ Single delta accumulation
- ✅ Self-inverse property (δ ⊕ δ = 0)
- ✅ Identity property (S ⊕ 0 = S)
- ✅ Multiple delta accumulation
- ✅ State reconstruction (S ⊕ δ)

---

*Report generated: January 25, 2026*
*ATOMiK Phase 3 - Hardware Synthesis Complete*
