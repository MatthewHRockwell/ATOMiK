# Phase 1 — ATOMiK + PicoRV32 Integration Results

**Date:** February 12, 2026
**Board:** Tang Nano 9K (GW1NR-LV9QN88PC6/I5)
**Toolchain:** Gowin EDA V1.9.12.01 (gw_sh command-line synthesis)
**CPU:** PicoRV32 (RV32I) @ 25.2 MHz
**SoC Base:** sipeed/TangNano-9K-example/picotiny

---

## Integration Summary

ATOMiK delta accumulator (32-bit, single-bank) integrated into the picotiny SoC via the previously unused S3 Wishbone port. The ATOMiK core is memory-mapped at `0xC000_0000` and accessed through a bus wrapper that adapts the PicoRV32 valid/ready protocol.

### Architecture

```
PicoRV32 CPU
    |
    v
PicoMem_Mux_1_4 (address decoder)
    |-- S0: SPI Flash XIP   (0x0000_0000)
    |-- S1: SRAM 8KB         (0x4000_0000)
    |-- S2: Peripherals       (0x8000_0000)
    |-- S3: ATOMiK            (0xC000_0000)  <-- NEW
```

### Files Created/Modified

| File | Description |
|------|-------------|
| `hardware/picorv32/atomik_bus_wrapper.v` | Bus wrapper: PicoRV32 valid/ready → ATOMiK core |
| `hardware/picorv32/memory_map.md` | Register map and C header definitions |
| `hardware/picorv32/firmware/firmware.c` | Test firmware with ATOMiK test suite |
| `picotiny/hw/picotiny.v` | Modified: replaced `wbp_ready = 1'b1` stub with ATOMiK instance |
| `picotiny/project/picotiny.gprj` | Updated: added 4 ATOMiK RTL files |

---

## Resource Utilization

### Combined Design (PicoRV32 + HDMI + ATOMiK)

| Resource | Used | Available | Utilization |
|----------|------|-----------|-------------|
| **Logic (LUT + ALU)** | 4,578 | 8,640 | **53%** |
| — LUT | 3,829 | — | — |
| — ALU | 707 | — | — |
| — SSRAM (RAM16) | 7 | — | — |
| **Register (FF)** | 2,062 | 6,693 | **31%** |
| — Logic FF | 2,060 | 6,480 | 32% |
| — I/O FF | 2 | 213 | <1% |
| **CLS** | 3,025 | 4,320 | **71%** |
| **BSRAM** | 12 | 26 | **47%** |
| **I/O Port** | 23 | 71 | 33% |

### ATOMiK Overhead (delta from PicoRV32-only baseline)

| Resource | PicoRV32 Only | With ATOMiK | Delta | % of Total |
|----------|--------------|-------------|-------|------------|
| **LUT** | 3,608 | 3,829 | **+221** | 2.6% |
| **FF** | 1,930 | 2,062 | **+132** | 2.0% |
| **CLS** | 2,854 | 3,025 | **+171** | 4.0% |
| **BSRAM** | 12 | 12 | **+0** | 0% |

### Remaining Headroom

| Resource | Used | Available | Remaining | Headroom |
|----------|------|-----------|-----------|----------|
| **LUT** | 3,829 | 8,640 | 4,811 | **56%** |
| **FF** | 2,062 | 6,693 | 4,631 | **69%** |
| **BSRAM** | 12 | 26 | 14 | **54%** |

---

## Timing

| Clock | Constraint | Actual Fmax | Logic Levels | Status |
|-------|-----------|-------------|--------------|--------|
| CLKDIV (CPU) | 25.2 MHz | 31.032 MHz | 9 | **PASS** (1.23x margin) |

- Setup TNS: 0.000 (no violations)
- Hold TNS: 0.000 (no violations)
- 3 setup-violated endpoints (HDMI cross-domain recovery paths — cosmetic, same as baseline)

---

## Functional Verification (On-Hardware)

All 11 tests executed on real hardware via UART at 115200 baud.

| Test | Description | Result |
|------|-------------|--------|
| T1 | Soft reset clears accumulator | **PASS** |
| T2 | Load initial state (0xDEADBEEF) | **PASS** |
| T3 | Accumulator zero after load | **PASS** |
| T4 | State equals initial when acc=0 | **PASS** |
| T5 | Accumulate delta (0xFF) | **PASS** |
| T6 | Accumulator non-zero after delta | **PASS** |
| T7 | State = initial XOR delta (0xDEADBE10) | **PASS** |
| T8 | XOR cancellation (apply same delta twice) | **PASS** |
| T9 | Accumulator zero after cancellation | **PASS** |
| T10 | Multi-delta: 0x11..^0x22..^0x44.. = 0x77777777 | **PASS** |
| T11 | Performance cycle count | **224 cycles** |

**Result: 11/11 PASS**

### Performance Analysis

The T11 measurement (224 cycles = 0xE0) covers the full round-trip:
- `ATOMIK_LOAD = 0` (store to MMIO)
- `rdcycle` (read cycle counter)
- `ATOMIK_ACCUM = 0xAAAAAAAA` (store to MMIO — the actual accumulate operation)
- `val = ATOMIK_STATE` (load from MMIO — state reconstruction)
- `rdcycle` (read cycle counter)

At 25.2 MHz: **224 cycles = 8.9 µs** total round-trip.

The ATOMiK hardware itself completes each operation in 1 clock cycle (39.7 ns). The overhead is dominated by PicoRV32 instruction fetch from SPI flash XIP and bus transaction protocol.

---

## Key Observations

1. **Minimal footprint**: ATOMiK adds only 221 LUTs (2.6% of total) — negligible overhead for a full delta accumulation engine.

2. **Zero BSRAM impact**: ATOMiK uses no block RAM, preserving all 14 remaining BSRAM blocks for future expansion.

3. **Timing margin maintained**: Fmax dropped from 33.7 MHz (baseline) to 31.032 MHz (integrated), still well above the 25.2 MHz constraint. The 9 logic levels (vs 13 baseline) suggest ATOMiK is not on the critical path.

4. **XOR cancellation verified**: Test T8/T9 confirm the fundamental ATOMiK property — applying a delta twice returns to the original state, proving the algebraic identity `a ⊕ d ⊕ d = a`.

5. **Multi-delta composition verified**: Test T10 confirms that deltas compose correctly via XOR, proving `0 ⊕ 0x11111111 ⊕ 0x22222222 ⊕ 0x44444444 = 0x77777777`.
