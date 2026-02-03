# ATOMiK Synthesis Directory

This directory contains synthesis scripts and build infrastructure for Phase 3 hardware implementation.

## Structure

```
hardware/synth/
├── gowin_synth.tcl       # Gowin EDA TCL synthesis script
├── run_synthesis.sh      # Synthesis runner (Linux/macOS)
├── run_synthesis.ps1     # Synthesis runner (Windows PowerShell)
└── README.md             # This file
```

## Prerequisites

### Gowin EDA (Required)
- **Gowin EDA V1.9.11.03** or later
- Valid license (Education or Commercial)
- Device support for GW1NR-9C

Download: https://www.gowinsemi.com/en/support/download_eda/

### Environment Setup

**Windows:**
```powershell
# Option 1: Set environment variable
$env:GOWIN_HOME = "C:\Gowin\Gowin_V1.9.11.03_Education_x64\IDE"

# Option 2: Script auto-detects common installation paths
```

**Linux/macOS:**
```bash
export PATH=$PATH:/path/to/gowin/IDE/bin
```

## Build Instructions

### Windows (PowerShell)

```powershell
# Command-line synthesis
.\hardware\synth\run_synthesis.ps1

# Open GUI
.\hardware\synth\run_synthesis.ps1 -GUI
```

### Linux/macOS

```bash
# Command-line synthesis
./hardware/synth/run_synthesis.sh

# Open GUI
./hardware/synth/run_synthesis.sh --gui
```

### Manual (Gowin EDA GUI)

1. Open Gowin EDA
2. Open project: `ATOMiK.gprj`
3. Run: Process → Synthesize
4. Run: Process → Place & Route
5. Generate bitstream: Process → Generate Bitstream

## Output Files

After successful synthesis:

| File | Description |
|------|-------------|
| `impl/gwsynthesis/ATOMiK.vg` | Synthesized netlist |
| `impl/pnr/ATOMiK.fs` | FPGA bitstream |
| `impl/pnr/ATOMiK_tr.html` | Timing report |
| `impl/pnr/ATOMiK_pr.html` | Place & route report |

## Target Device

| Parameter | Value |
|-----------|-------|
| **Device** | GW1NR-LV9QN88PC6/I5 |
| **Family** | GW1NR-9C |
| **Package** | QN88P |
| **Speed Grade** | C6 (commercial) / I5 (industrial) |
| **LUT4** | 8,640 |
| **FF** | 6,480 |
| **BSRAM** | 26 × 18Kbit |
| **PLL** | 2 |

## Clock Configuration

| Clock | Frequency | Source |
|-------|-----------|--------|
| sys_clk | 27 MHz | On-board crystal (pin 52) |
| atomik_clk | 94.5 MHz | PLL output (`atomik_pll_94p5m`) |

## Resource Budget (ATOMiK Core v2)

| Resource | Estimated | Available | Utilization |
|----------|-----------|-----------|-------------|
| LUT4 | ~161 | 8,640 | 1.9% |
| FF | ~194 | 6,480 | 3.0% |
| BSRAM | 0 | 26 | 0% |
| PLL | 1 | 2 | 50% |

## RTL Modules

### Core v2 (Delta Architecture)
- `hardware/rtl/atomik_delta_acc.v` - Delta accumulator (XOR composition)
- `hardware/rtl/atomik_state_rec.v` - State reconstructor (combinational)
- `hardware/rtl/atomik_core_v2.v` - Top-level integration

### Support Modules
- `hardware/rtl/pll/atomik_pll_94p5m.v` - PLL configuration (94.5 MHz)
- `hardware/rtl/atomik_top.v` - Top-level with IO
- `hardware/rtl/atomik_bios.v` - Boot/configuration
- `hardware/rtl/atomik_uart_rx.v` - UART receiver
- `hardware/rtl/atomik_uart_tx.v` - UART transmitter

## Programming the FPGA

1. Connect Tang Nano 9K via USB
2. Open **Gowin Programmer** (separate application)
3. Click **Read Device Info** to verify connection
4. Load bitstream: `impl/pnr/ATOMiK.fs`
5. Select **SRAM Program** for volatile (development)
6. Select **embFlash** for persistent (deployment)
7. Click **Program/Configure**

### Alternative: openFPGALoader

```bash
# Install openFPGALoader
# https://github.com/trabucayre/openFPGALoader

# Program SRAM (volatile)
openFPGALoader -b tangnano9k impl/pnr/ATOMiK.fs

# Program Flash (persistent)
openFPGALoader -b tangnano9k -f impl/pnr/ATOMiK.fs
```

## Troubleshooting

### "Device not found"
- Ensure USB cable supports data (not charge-only)
- Install Gowin USB drivers
- Check Device Manager for "JTAG Debugger"

### Timing violations
- Check `impl/pnr/ATOMiK_tr.html` for failing paths
- Reduce target frequency in `gowin_synth.tcl`
- Review `hardware/constraints/timing_constraints.sdc`

### Synthesis errors
- Verify all RTL files are syntax-correct: `.\hardware\scripts\test_phase3_rtl.ps1`
- Check for missing module definitions
- Review Gowin EDA console output
