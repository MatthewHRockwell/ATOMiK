# Gowin FPGA Reference Documentation

Quick reference for ATOMiK hardware development targeting Tang Nano 9K (GW1NR-9).

## Target Hardware

| Specification | Value |
|---------------|-------|
| **Device** | GW1NR-LV9QN88PC6/I5 |
| **Family** | GW1NR-9 (LittleBee) |
| **Package** | QN88P (with embedded PSRAM) |
| **Speed Grade** | C6/I5 |
| **LUT4** | 8,640 |
| **Flip-Flops** | 6,480 |
| **BRAM** | 26 × 18Kbit |
| **PLL** | 2 |
| **Board** | Sipeed Tang Nano 9K |
| **Crystal** | 27 MHz |

---

## Quick Reference Files

| File | Contents |
|------|----------|
| [CLOCK_REFERENCE.md](CLOCK_REFERENCE.md) | PLL formulas, frequency specs, instantiation templates |
| [GPIO_REFERENCE.md](GPIO_REFERENCE.md) | IO standards, CST syntax, bank configuration |
| [TIMING_REFERENCE.md](TIMING_REFERENCE.md) | SDC syntax, timing closure strategies |
| [TANG_NANO_9K_PINOUT.md](TANG_NANO_9K_PINOUT.md) | Board-specific pin assignments |

---

## Official Gowin Documentation Sources

These references are derived from official Gowin documentation. For complete details, consult the original documents.

| Topic | Document ID | Title | Key Sections |
|-------|-------------|-------|--------------|
| Clock/PLL | UG286-1.9.1E | Gowin Clock User Guide | Section 5.1 rPLL |
| IP Core Generator | SUG284-2.1E | Gowin IP Core Generator User Guide | Section 3.4 CLOCK |
| Device Specifications | DS117-2.9.3E | GW1NR Series Datasheet | Section 4.4.7 PLL Switching |
| Pinout | UG803-1.6E | GW1NR-9 Pinout | Full pin list for QN88P |
| GPIO | UG289-1.9.2E | Gowin Programmable IO User Guide | Sections 3.1-3.6 |
| Timing Constraints | SUG940-1.3E | Gowin Design Timing Constraints User Guide | Appendix A (SDC syntax) |
| Physical Constraints | SUG935-1.3E | Gowin Design Physical Constraints User Guide | CST file syntax |
| Design Guide | SUG113-1.1E | Gowin FPGA Design Guide | Timing closure strategies |
| Software | SUG100-2.6E | Gowin Software User Guide | IDE workflow |
| Schematic Reference | UG284-1.8E | GW1NR Series Schematic Manual | Reference circuits |

---

## ATOMiK-Specific Configuration

### Current PLL Setup

The project uses `atomik_pll_94p5m` (94.5 MHz) as configured in `atomik_top.v`:

```
Input:  27 MHz (Tang Nano 9K crystal)
Output: 94.5 MHz (SYS_CLK_HZ parameter)
```

### Available PLL Modules

| Module | Location | Output | Status |
|--------|----------|--------|--------|
| `atomik_pll_94p5m` | `rtl/pll/atomik_pll_94p5m.v` | 94.5 MHz | **Active** |
| `atomik_pll_81m` | `rtl/pll/atomik_pll_81m.v` | 81.0 MHz | Available |
| `Gowin_rPLL` | `rtl/pll/gowin_rpll/gowin_rpll.v` | Dynamic | Available (has idsel input) |

### Constraints Files

| File | Purpose |
|------|---------|
| `constraints/atomik_constraints.cst` | Physical pin assignments |
| `constraints/atomik_timing.sdc` | Timing constraints |

---

## Tool Requirements

| Tool | Version | Purpose |
|------|---------|---------|
| Gowin EDA | V1.9.11.03+ | Synthesis, P&R, Programming |
| Icarus Verilog | Any recent | Simulation |
| Verilator | Any recent | Linting |
| GTKWave | Any recent | Waveform viewing |

### Gowin EDA Installation

1. Download from [Gowin Website](http://www.gowinsemi.com.cn/faq.aspx)
2. Install and obtain Education license
3. Add to PATH: `export PATH=$PATH:/path/to/gowin/IDE/bin`
4. Verify: `gw_sh --version`

---

## Related Project Documentation

| Document | Location |
|----------|----------|
| RTL Architecture Spec | `specs/rtl_architecture.md` |
| PLL Module README | `rtl/pll/README.md` |
| Synthesis Scripts | `synth/README.md` |
| Phase 3 Execution Guide | `agents/phase3/PHASE_3_EXECUTION_GUIDE.md` |

---

*Last Updated: January 25, 2026*
