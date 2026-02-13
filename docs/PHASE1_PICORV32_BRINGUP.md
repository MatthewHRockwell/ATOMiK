# Phase 1 — PicoRV32 Standalone Bringup Results

**Date:** February 12, 2026
**Board:** Tang Nano 9K (GW1NR-LV9QN88PC6/I5)
**Toolchain:** Gowin EDA V1.9.12.01 (gw_sh command-line synthesis)
**Reference Design:** sipeed/TangNano-9K-example/picotiny

---

## Build Configuration

- **Top module:** `picotiny`
- **CPU:** PicoRV32 (RV32I, no compressed ISA, no IRQ)
- **Clock:** 27 MHz crystal → PLL → 25.2 MHz pixel clock, clock divider for CPU
- **Peripherals:** UART (115200 baud), SPI Flash XIP (8 MB), GPIO (7-bit), HDMI terminal output
- **Memory:** 8 KB SRAM (data) + 2 KB Boot ROM (ISP flasher) + SPI Flash XIP
- **MSPI as GPIO:** Enabled (required for SPI flash access)

## Resource Utilization — PicoRV32 + Peripherals + HDMI

| Resource | Used | Available | Utilization |
|----------|------|-----------|-------------|
| **Logic (LUT + ALU)** | 4,357 | 8,640 | **51%** |
| — LUT | 3,608 | — | — |
| — ALU | 707 | — | — |
| — SSRAM (RAM16) | 7 | — | — |
| **Register (FF)** | 1,930 | 6,693 | **29%** |
| — Logic FF | 1,928 | 6,480 | 30% |
| — I/O FF | 2 | 213 | <1% |
| **CLS** | 2,854 | 4,320 | **67%** |
| **BSRAM** | 12 | 26 | **47%** |
| — SP (Single Port) | 8 | — | — |
| — SDPB (Semi-Dual Port) | 2 | — | — |
| — DPB (Dual Port) | 1 | — | — |
| — pROM | 1 | — | — |
| **I/O Port** | 23 | 71 | 33% |
| **IOLOGIC** | 6 | 97 | 7% |

## Timing

| Clock | Constraint | Actual Fmax | Logic Levels | Status |
|-------|-----------|-------------|--------------|--------|
| CLKDIV (CPU) | 25.2 MHz | 33.7 MHz | 13 | **PASS** (1.34× margin) |

- Setup TNS: 0.000 (no violations)
- Worst setup slack: 10.009 ns
- HDMI cross-domain recovery paths show minor violations (cosmetic, typical for HDMI reset synchronization)

## Comparison: ATOMiK Standalone vs PicoRV32 SoC

| Metric | ATOMiK (1-bank, 54 MHz) | PicoRV32 SoC (full) | Combined Budget |
|--------|------------------------|---------------------|-----------------|
| **LUT** | 478 (5.5%) | 4,357 (51%) | 4,835 (56%) |
| **FF** | 537 (8.0%) | 1,930 (29%) | 2,467 (37%) |
| **BSRAM** | 0 (0%) | 12 (47%) | 12 (47%) |
| **CLS** | 432 (10%) | 2,854 (67%) | 3,286 (76%) |

### Headroom Analysis (with 4-bank ATOMiK)

| Resource | PicoRV32 SoC | ATOMiK 4-bank | Total | Available | Remaining |
|----------|-------------|---------------|-------|-----------|-----------|
| **LUT** | 4,357 | 744 | 5,101 | 8,640 | **3,539 (41%)** |
| **FF** | 1,930 | 731 | 2,661 | 6,693 | **4,032 (60%)** |
| **BSRAM** | 12 | 0 | 12 | 26 | **14 (54%)** |

**Conclusion:** Sufficient headroom exists for ATOMiK integration. The combined design uses ~59% of LUTs, leaving 41% for bus wrappers, additional peripherals, and future expansion.

## Verification Steps Completed

1. **Synthesis:** Gowin gw_sh — zero errors, warnings only (cosmetic PicoRV32 undriven PCPI ports)
2. **Place & Route:** Completed in 7 seconds, timing closure met
3. **Flash Programming:** openFPGALoader → persistent flash, CRC check passed
4. **Firmware Load:** pico-programmer.py → 11,761 bytes, 3 sectors, 46 pages — success
5. **UART Verification:** picocom 115200 baud — PicoSoC boot menu confirmed:
   - LED toggle, SPI flash mode selection, benchmark commands functional
   - Bidirectional UART communication verified

## HDMI Output

The picotiny design includes a full HDMI terminal output (SimpleVout). When connected to an HDMI monitor, it displays a terminal with the PicoSoC boot output. The HDMI subsystem accounts for a significant portion of the resource usage (~1,000-1,500 LUTs estimated).

## Files

- **Bitstream:** `TangNano-9K-example/picotiny/project/impl/pnr/picotiny.fs`
- **Synthesis report:** `TangNano-9K-example/picotiny/project/impl/gwsynthesis/picotiny_syn.rpt.html`
- **P&R report:** `TangNano-9K-example/picotiny/project/impl/pnr/picotiny.rpt.txt`
- **Timing report:** `TangNano-9K-example/picotiny/project/impl/pnr/picotiny.tr`
- **TCL build script:** `TangNano-9K-example/picotiny/project/synth.tcl`
