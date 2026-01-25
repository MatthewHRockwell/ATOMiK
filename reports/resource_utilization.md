# ATOMiK Phase 3 - Resource Utilization Report

**Date:** January 25, 2026  
**Tool:** Gowin EDA V1.9.11.03 Education  
**Target:** GW1NR-LV9QN88PC6/I5 (Tang Nano 9K)  
**Top Module:** atomik_top

---

## 1. Resource Summary

| Resource | Used | Available | Utilization |
|----------|------|-----------|-------------|
| **LUT4** | 236 | 8,640 | 2.7% |
| **ALU** | 156 | 8,640 | 1.8% |
| **Logic FF** | 209 | 6,480 | 3.2% |
| **IO FF** | 1 | 213 | <1% |
| **BSRAM** | 0 | 26 | 0% |
| **PLL** | 1 | 2 | 50% |

**Total Logic Utilization: ~3%**

---

## 2. Timing Summary

| Clock Domain | Target Freq | Achieved Freq | Period | Slack | Status |
|--------------|-------------|---------------|--------|-------|--------|
| sys_clk | 27.0 MHz | 174.3 MHz | 37.037 ns | +31.3 ns | ✅ PASS |
| atomik_clk | 94.5 MHz | 94.6 MHz | 10.582 ns | +0.01 ns | ✅ PASS |

**All timing constraints met.**

---

## 3. Module Breakdown (Estimated)

Based on RTL architecture specification estimates vs actual:

| Module | Est. LUTs | Est. FFs | Notes |
|--------|-----------|----------|-------|
| atomik_delta_acc | ~82 | ~128 | 64-bit XOR + control |
| atomik_state_rec | ~64 | 0 | Combinational only |
| atomik_core_v2 | ~15 | ~66 | Output reg + decode |
| atomik_core (legacy) | ~50 | ~100 | Polymorphic engine |
| uart_genome_loader | ~30 | ~40 | UART + FSM |
| PLL wrapper | ~5 | ~5 | Clock generation |
| **Total** | **~246** | **~339** | |
| **Actual** | **236** | **210** | Optimizer savings |

The actual utilization is slightly lower than estimates due to synthesis optimization.

---

## 4. Comparison: Estimates vs Actual

| Metric | Spec Estimate | Actual | Delta |
|--------|---------------|--------|-------|
| LUTs | ~161 (core v2 only) | 236 (full design) | +75 (other modules) |
| FFs | ~194 (core v2 only) | 210 (full design) | +16 (other modules) |
| Fmax | >94.5 MHz | 94.6 MHz | ✅ Met |

**Conclusion:** Resource estimates were accurate. Design has >95% headroom for future expansion.

---

## 5. Critical Paths

The timing analyzer reports the following critical paths at 94.5 MHz:

1. **Delta Accumulator Feedback** (atomik_delta_acc)
   - Path: `delta_accumulator[*]` → XOR → `delta_accumulator[*]`
   - Estimated: 3.9 ns, Actual: ~10.5 ns (meets 10.582 ns requirement)

2. **State Reconstruction** (atomik_state_rec)
   - Path: `initial_state[*]` → XOR → `data_out[*]`
   - Pure combinational, registered at output

---

## 6. Power Estimate

See `impl/pnr/ATOMiK.power.html` for detailed power analysis.

Typical estimates for GW1NR-9 at this utilization:
- Static power: ~10-15 mW
- Dynamic power: ~5-10 mW @ 94.5 MHz
- **Total: ~15-25 mW**

---

## 7. Recommendations

1. **Timing margin is tight** (0.1 MHz) - consider:
   - Reducing to 81 MHz PLL for more margin
   - Or accept current margin for development

2. **Resource headroom is excellent** (>95%) - enables:
   - Multi-port delta accumulator (parallel inputs)
   - Additional debug logic
   - Future feature expansion

3. **Ready for hardware validation** (T3.9)

---

## 8. Files Generated

| File | Description |
|------|-------------|
| `impl/pnr/ATOMiK.fs` | FPGA bitstream |
| `impl/pnr/ATOMiK.rpt.txt` | Text utilization report |
| `impl/pnr/ATOMiK.rpt.html` | HTML utilization report |
| `impl/pnr/ATOMiK.tr.html` | Timing report |
| `impl/pnr/ATOMiK.power.html` | Power analysis |
| `impl/pnr/ATOMiK.pin.html` | Pin assignments |

---

## 9. Verification Status

| Check | Status |
|-------|--------|
| Synthesis complete | ✅ |
| Place & route complete | ✅ |
| Timing constraints met | ✅ |
| Bitstream generated | ✅ |
| Ready for programming | ✅ |

---

*Report generated from Gowin EDA synthesis run on January 25, 2026*
