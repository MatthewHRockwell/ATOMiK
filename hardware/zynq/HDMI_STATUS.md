# HDMI Output — Current Status and Blockers

## The Problem
We need the correct FPGA pin-to-HDMI-connector mapping for the **RK-ZYNQ7020-F** board (XC7Z020-**CLG484**-2). Without it, we cannot produce HDMI output.

## What Is Proven
- **Factory QSPI image produces working HDMI** — confirmed on monitor. The hardware works.
- **Ubuntu boots cleanly on the same SoC** (NaxRiscv RV64GC + ATOMiK) — Stage A complete.
- **LiteX VideoS7HDMIPHY works** when given correct CLG484 differential pairs — builds are clean, 0 DRC errors, all pins FIXED. The software/gateware is ready.

## What Failed and Why
All HDMI pin data was sourced from the **ALINX AX7020** which uses **CLG400** (400-ball BGA). Our board uses **CLG484** (484-ball BGA). These are different packages with different ball-to-IO mappings:
- CLG400 pin T20 = valid IO → CLG484 pin T20 = **GND**
- CLG400 pin P19 = valid IO → CLG484 pin P19 = **VCCO_34 (power)**

This means ALL HDMI pin data from the ALINX reference (vendor XDC, pinout xlsx, schematic) is **invalid** for our board.

## What Was Tried
| Attempt | Bank | Pins (clk_p/d0_p/d1_p/d2_p) | Result |
|---------|------|-----|--------|
| ALINX CLG400 data | 34 | N18/V20/T20/N20 | CRITICAL WARNING: T20=GND, P19=VCCO. No signal. |
| OCR Bank 13 L13-L16 | 13 | Y6/AB5/AB2/AA7 | Clean build, 0 DRC. No signal. BIOS on UART works. |
| OCR Bank 33 L13-L16 | 33 | W17/U17/U15/W16 | Clean build, 0 DRC. No signal. UART dead (unknown why). |
| OCR Bank 34 L13-L16 | 34 | M19/N22/M21/N19 | Clean build, 0 DRC. No signal. |

## The Core Blocker
The **RK-ZYNQ7020-F schematic PDF** (`Schematics.pdf`) is a low-resolution web browser capture (screenshot from Baidu cloud drive preview). The text is embedded as images, not selectable text. OCR produces unreliable results — bank numbers and pair designators cannot be read with confidence.

The HDMI schematic is on **page 11** of Schematics.pdf. It shows TMDS differential pairs going from an HDMI connector through 100Ω series resistors and 100nF AC coupling capacitors to the FPGA. The signal names appear to be `HDMI1_CC_CLK_P/N`, `HDMI1_CC_DATA0_P/N`, `HDMI1_CC_DATA1_P/N`, `HDMI1_CC_DATA2_P/N`.

The PL bank pin list is on **page 5** of Schematics.pdf. It shows 4 banks (13, 33, 34, 35) with IO function names, CLG484 ball names, and board signal names. The HDMI signals should appear in the signal name column of one of these banks.

## What Would Solve This Immediately
**Any ONE of these:**

1. **The actual Orcad/Altium schematic** (not the browser screenshot) — it would have selectable text with exact pin assignments.

2. **An XDC constraints file** from any RIGUKE/HamGeek Vivado project for this board — even a simple LED blink project would confirm the HDMI pins if it includes HDMI constraints.

3. **The factory Vivado project** from the Baidu cloud drive (path: `RIGUKE网盘资料/6.RK-ZYNQ7020-F开发板网盘资料/`) — it would have the definitive XDC.

4. **Someone who can read Chinese schematics** looking at page 5 and page 11 of Schematics.pdf and reporting the exact CLG484 ball names for the HDMI TMDS signals.

5. **Probing the HDMI connector** with a multimeter for continuity to known FPGA balls (the CLG484 has 4 HR banks with ~50 IO pairs each — systematic probing could identify the HDMI pairs).

## Files Available
- `reference-documents-tmp/Schematics.pdf` — 15-page schematic (browser capture, low-res images)
- `reference-documents-tmp/PinLayout.pdf` — FMC + 40-pin header pins only (no HDMI)
- `reference-documents-tmp/RK-ZYNQ7020-F_English_Manual.pdf` — translated manual, HDMI section §1.12 on pages 10-11 (schematic images too small to read pin numbers)
- `/tmp/factory.rbd` + `/tmp/factory.bin` — FPGA readback from factory QSPI image (32MB raw readback data)
- Baidu cloud drive: `https://pan.baidu.com/s/1uBd0RYMa9Vi8g7eGg2zS1A?pwd=xixi` (full documentation, ~30GB, requires verification code for bulk download)

## CLG484 Bank Reference
For XC7Z020-CLG484, the HR (High Range) PL IO banks that support TMDS_33 are:
- **Bank 13**: Y/AA/AB column pins (right side of die)
- **Bank 33**: U/V/W/Y/AA/AB column pins (upper right)
- **Bank 34**: L/M/N/P column pins (upper left)
- **Bank 35**: A/B/C/D/E/F/G column pins (lower left)

ALL FOUR banks have been tried with pairs L13-L16 — none produced HDMI output:
- Bank 13 L13-L16: Y6/AB5/AB2/AA7 — clean build, no signal, UART works
- Bank 33 L13-L16: W17/U17/U15/W16 — clean build, no signal, UART dead
- Bank 34 L13-L16: M19/N22/M21/N19 — clean build, no signal
- Bank 35 L13-L16: B19/D22/A21/D20 — clean build, no signal

The OCR-extracted pair numbers (L13-L16) are wrong, or the bank-to-signal mapping is wrong. The schematic PDF resolution is too low for reliable OCR — the pair numbers could be L1-L6, L7-L12, or any other set.

Additionally tried: Bank 33 with swapped P/N + DRC override (rejected LOCs, auto-placed to wrong pins), LVCMOS33 complementary drive (DRC errors on IOBS pins + ODDR load violation), rgb2dvi vendor IP (same LOC issues + PLL VCO error).

The factory QSPI image's FPGA readback was captured (`/tmp/factory.rbd`, `/tmp/factory.bin`) but could not be analyzed to extract pin assignments — Vivado's hardware manager doesn't support IO reporting from readback without a reference design.

## What The Build Needs
Once the correct 4 differential pairs are known (clk, data0, data1, data2):
```python
("hdmi_out", 0,
    Subsignal("clk_p",   Pins("XX"), IOStandard("TMDS_33")),
    Subsignal("clk_n",   Pins("XX"), IOStandard("TMDS_33")),
    Subsignal("data0_p", Pins("XX"), IOStandard("TMDS_33")),
    Subsignal("data0_n", Pins("XX"), IOStandard("TMDS_33")),
    Subsignal("data1_p", Pins("XX"), IOStandard("TMDS_33")),
    Subsignal("data1_n", Pins("XX"), IOStandard("TMDS_33")),
    Subsignal("data2_p", Pins("XX"), IOStandard("TMDS_33")),
    Subsignal("data2_n", Pins("XX"), IOStandard("TMDS_33")),
),
```
The SoC, video terminal, TMDS serializer, and build infrastructure are all working. It's purely a pin assignment problem.
