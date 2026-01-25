# Gowin rPLL Clock Reference

Reference documentation for PLL configuration on GW1NR-9 FPGA.

**Source**: UG286-1.9.1E (Gowin Clock User Guide), DS117-2.9.3E (GW1NR Datasheet)

---

## Frequency Calculation Formulas

From UG286-1.9.1E Section 5.1:

```
fCLKOUT  = (fCLKIN × FBDIV) / IDIV
fVCO     = fCLKOUT × ODIV
fCLKOUTD = fCLKOUT / SDIV
fPFD     = fCLKIN / IDIV = fCLKOUT / FBDIV
```

Where:
- **fCLKIN**: Input clock frequency
- **fCLKOUT**: Primary output clock frequency (also CLKOUTP)
- **fCLKOUTD**: Divided output clock frequency
- **fVCO**: Voltage-Controlled Oscillator frequency (must be within spec)
- **fPFD**: Phase Frequency Detector frequency
- **IDIV**: Input divider (IDIV_SEL + 1, range 1-64)
- **FBDIV**: Feedback divider (FBDIV_SEL + 1, range 1-64)
- **ODIV**: Output divider (2, 4, 8, 16, 32, 48, 64, 80, 96, 112, 128)
- **SDIV**: Secondary divider for CLKOUTD (DYN_SDIV_SEL, range 2-128)

---

## GW1NR-9 PLL Specifications

From DS117-2.9.3E Table 4-21:

| Parameter | Speed C7/I6 | Speed C6/I5 | Speed C5/I4 | Unit |
|-----------|-------------|-------------|-------------|------|
| **CLKIN** | 3 - 400 | 3 - 400 | 3 - 320 | MHz |
| **PFD** | 3 - 400 | 3 - 400 | 3 - 320 | MHz |
| **VCO** | 400 - 1200 | 400 - 1200 | 320 - 960 | MHz |
| **CLKOUT** | 3.125 - 600 | 3.125 - 600 | 2.5 - 480 | MHz |

**Important**: The VCO frequency must stay within the specified range. If fVCO is outside this range, the PLL will not lock properly.

---

## ATOMiK PLL Configurations

### atomik_pll_94p5m (Current - Used by atomik_top.v)

**Location**: `rtl/pll/atomik_pll_94p5m.v`

| Parameter | Value | Calculation |
|-----------|-------|-------------|
| FCLKIN | 27 MHz | Tang Nano 9K crystal |
| IDIV_SEL | 1 | IDIV = 2 |
| FBDIV_SEL | 6 | FBDIV = 7 |
| ODIV_SEL | 8 | ODIV = 8 |
| **fCLKOUT** | **94.5 MHz** | 27 × 7 / 2 = 94.5 |
| fVCO | 756 MHz | 94.5 × 8 = 756 ✓ (within 400-1200) |

### atomik_pll_81m (Alternative)

**Location**: `rtl/pll/atomik_pll_81m.v`

| Parameter | Value | Calculation |
|-----------|-------|-------------|
| FCLKIN | 27 MHz | Tang Nano 9K crystal |
| IDIV_SEL | 0 | IDIV = 1 |
| FBDIV_SEL | 2 | FBDIV = 3 |
| ODIV_SEL | 8 | ODIV = 8 |
| **fCLKOUT** | **81 MHz** | 27 × 3 / 1 = 81 |
| fVCO | 648 MHz | 81 × 8 = 648 ✓ (within 400-1200) |

### Gowin_rPLL (Dynamic Frequency)

**Location**: `rtl/pll/gowin_rpll/gowin_rpll.v`

| Parameter | Value | Notes |
|-----------|-------|-------|
| FCLKIN | 27 MHz | Tang Nano 9K crystal |
| DYN_IDIV_SEL | true | **Runtime selectable** |
| IDIV_SEL | 0 | Default IDIV = 1 |
| FBDIV_SEL | 2 | FBDIV = 3 |
| ODIV_SEL | 8 | ODIV = 8 |

This module exposes an `idsel[5:0]` input port allowing runtime frequency changes:
- `idsel = 0`: fCLKOUT = 27 × 3 / 1 / 8 = 10.125 MHz (via CLKOUTD path)
- `idsel = 1`: fCLKOUT = 27 × 3 / 2 / 8 = 5.0625 MHz
- etc.

---

## rPLL Port Diagram

```
              ┌──────────────────┐
   CLKIN ────►│                  │────► CLKOUT    (Primary output)
   CLKFB ────►│                  │────► CLKOUTP   (Phase-shifted output)
   RESET ────►│                  │────► CLKOUTD   (Divided output)
  RESET_P────►│       rPLL       │────► CLKOUTD3  (Divide-by-3 output)
FBDSEL[5:0]──►│                  │────► LOCK      (PLL locked indicator)
 IDSEL[5:0]──►│                  │
 ODSEL[5:0]──►│                  │
  PSDA[3:0]──►│                  │
DUTYDA[3:0]──►│                  │
  FDLY[3:0]──►│                  │
              └──────────────────┘
```

### Port Descriptions

| Port | Direction | Width | Description |
|------|-----------|-------|-------------|
| CLKIN | Input | 1 | Reference clock input |
| CLKFB | Input | 1 | Feedback clock (external feedback mode) |
| RESET | Input | 1 | Async reset, **active-HIGH** |
| RESET_P | Input | 1 | Power down, active-HIGH |
| FBDSEL | Input | 6 | Dynamic feedback divider select |
| IDSEL | Input | 6 | Dynamic input divider select |
| ODSEL | Input | 6 | Dynamic output divider select |
| PSDA | Input | 4 | Phase shift adjustment |
| DUTYDA | Input | 4 | Duty cycle adjustment |
| FDLY | Input | 4 | Fine delay adjustment |
| CLKOUT | Output | 1 | Primary clock output |
| CLKOUTP | Output | 1 | Phase-shifted clock output |
| CLKOUTD | Output | 1 | Divided clock output |
| CLKOUTD3 | Output | 1 | Divide-by-3 clock output |
| LOCK | Output | 1 | PLL lock indicator (HIGH when locked) |

---

## Instantiation Templates

### Static PLL (No Dynamic Control)

```verilog
// From atomik_pll_94p5m.v pattern
atomik_pll_94p5m u_pll (
    .clkin  (sys_clk),      // 27 MHz input
    .reset  (~sys_rst_n),   // Active-HIGH reset (invert active-low)
    .clkout (clk_pll),      // 94.5 MHz output
    .lock   (pll_lock)      // Lock indicator
);
```

### Dynamic PLL (With IDSEL Control)

```verilog
// From gowin_rpll_tmp.v
Gowin_rPLL u_pll_dyn (
    .clkout (clk_pll),      // Variable output
    .lock   (pll_lock),     // Lock indicator
    .reset  (pll_reset),    // Active-HIGH reset
    .clkin  (sys_clk),      // 27 MHz input
    .idsel  (freq_select)   // 6-bit frequency select
);
```

---

## Common PLL Configurations for 27 MHz Input

| Target Freq | IDIV_SEL | FBDIV_SEL | ODIV_SEL | fVCO | Valid? |
|-------------|----------|-----------|----------|------|--------|
| 27 MHz | 0 | 0 | 8 | 216 | ❌ VCO too low |
| 50 MHz | 0 | 14 | 8 | 405 | ✓ |
| 54 MHz | 0 | 1 | 8 | 432 | ✓ |
| 81 MHz | 0 | 2 | 8 | 648 | ✓ |
| 94.5 MHz | 1 | 6 | 8 | 756 | ✓ |
| 100 MHz | 0 | 29 | 8 | 810 | ✓ |
| 108 MHz | 0 | 3 | 8 | 864 | ✓ |
| 135 MHz | 0 | 4 | 8 | 1080 | ✓ |
| 162 MHz | 0 | 5 | 8 | 1296 | ❌ VCO too high |

**Note**: Always verify fVCO is within 400-1200 MHz for C6/I5 speed grade.

---

## Defparam Configuration Reference

Standard defparam block for rPLL instantiation:

```verilog
defparam rpll_inst.FCLKIN = "27";           // Input frequency (MHz)
defparam rpll_inst.DYN_IDIV_SEL = "false";  // Dynamic input divider
defparam rpll_inst.IDIV_SEL = 0;            // Input divider (0-63)
defparam rpll_inst.DYN_FBDIV_SEL = "false"; // Dynamic feedback divider
defparam rpll_inst.FBDIV_SEL = 2;           // Feedback divider (0-63)
defparam rpll_inst.DYN_ODIV_SEL = "false";  // Dynamic output divider
defparam rpll_inst.ODIV_SEL = 8;            // Output divider
defparam rpll_inst.PSDA_SEL = "0000";       // Phase shift
defparam rpll_inst.DYN_DA_EN = "false";     // Dynamic duty/phase adjust
defparam rpll_inst.DUTYDA_SEL = "1000";     // Duty cycle (50%)
defparam rpll_inst.CLKOUT_FT_DIR = 1'b1;    // Fine tune direction
defparam rpll_inst.CLKOUTP_FT_DIR = 1'b1;   // Fine tune direction
defparam rpll_inst.CLKOUT_DLY_STEP = 0;     // Output delay steps
defparam rpll_inst.CLKOUTP_DLY_STEP = 0;    // Output P delay steps
defparam rpll_inst.CLKFB_SEL = "internal";  // Feedback source
defparam rpll_inst.CLKOUT_BYPASS = "false"; // Bypass CLKOUT
defparam rpll_inst.CLKOUTP_BYPASS = "false";// Bypass CLKOUTP
defparam rpll_inst.CLKOUTD_BYPASS = "false";// Bypass CLKOUTD
defparam rpll_inst.DYN_SDIV_SEL = 2;        // CLKOUTD divider
defparam rpll_inst.CLKOUTD_SRC = "CLKOUT";  // CLKOUTD source
defparam rpll_inst.CLKOUTD3_SRC = "CLKOUT"; // CLKOUTD3 source
defparam rpll_inst.DEVICE = "GW1NR-9C";     // Target device
```

---

## Troubleshooting

### PLL Won't Lock

1. **Check VCO frequency**: Must be within 400-1200 MHz
2. **Verify input clock**: Ensure 27 MHz crystal is running
3. **Reset timing**: Hold reset for sufficient time after power-up
4. **Check RESET polarity**: rPLL RESET is **active-HIGH**

### Frequency Incorrect

1. **Verify divider values**: Remember IDIV = IDIV_SEL + 1
2. **Check ODIV_SEL**: Only specific values valid (2,4,8,16,32,48,64,80,96,112,128)
3. **Recalculate**: fCLKOUT = fCLKIN × (FBDIV_SEL + 1) / (IDIV_SEL + 1)

### Timing Closure Fails

1. Reduce target frequency
2. Add pipeline registers to critical paths
3. Check SDC constraints match actual PLL output

---

## Related Files

| File | Description |
|------|-------------|
| `rtl/pll/atomik_pll_94p5m.v` | Current production PLL (94.5 MHz) |
| `rtl/pll/atomik_pll_81m.v` | Alternative PLL (81 MHz) |
| `rtl/pll/gowin_rpll/gowin_rpll.v` | Dynamic PLL with idsel input |
| `rtl/pll/gowin_rpll/gowin_rpll_tmp.v` | Instantiation template |
| `rtl/pll/gowin_rpll/gowin_rpll.ipc` | IP Core configuration |
| `rtl/pll/README.md` | PLL module documentation |

---

*Reference: UG286-1.9.1E, DS117-2.9.3E, SUG284-2.1E*
