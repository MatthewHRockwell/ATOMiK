# ALINX AX7020 Board Reference

Board-specific reference for the ALINX AX7020 Zynq-7020 FPGA development board.

**Board**: ALINX AX7020
**FPGA**: XC7Z020-2CLG400I (Zynq-7000, industrial grade, -2 speed)
**Price**: $195
**Date**: March 7, 2026
**Status**: Pre-board (from ALINX documentation and Xilinx datasheets)

**Sources**: ALINX AX7020 User Manual, Xilinx DS187 (Zynq-7000 Data Sheet: Overview), UG585 (Zynq-7000 TRM), component datasheets (ADV7511, ADV7611, KSZ9031, CP2102, USB3320, DS1302)

---

## Table of Contents

1. [Board Overview](#1-board-overview)
2. [On-Board Resources](#2-on-board-resources)
3. [Processing System (PS) Resources](#3-processing-system-ps-resources)
4. [Programmable Logic (PL) Resources](#4-programmable-logic-pl-resources)
5. [PS-Side I/O (MIO Pins)](#5-ps-side-io-mio-pins)
6. [PL-Side I/O](#6-pl-side-io)
7. [Power](#7-power)
8. [JTAG Programming](#8-jtag-programming)
9. [Comparison: AX7020 vs Tang Nano 9K](#9-comparison-ax7020-vs-tang-nano-9k)
10. [Notes and Caveats](#10-notes-and-caveats)

---

## 1. Board Overview

The ALINX AX7020 is a mid-range Zynq-7000 development board built around the XC7Z020-2CLG400I. It pairs a dual-core ARM Cortex-A9 processing system (PS) with Artix-7-class programmable logic (PL), providing enough resources to run Linux on the PS while implementing ATOMiK multi-bank acceleration in the PL fabric.

```
    ┌──────────────────────────────────────────────────────────────────┐
    │                          ALINX AX7020                          │
    │                                                                │
    │   [USB-JTAG]    [USB-UART]    [USB 2.0]    [Ethernet RJ45]    │
    │                                                                │
    │   ┌────────────────────────────────────┐                       │
    │   │         XC7Z020-2CLG400I           │                       │
    │   │                                    │                       │
    │   │   PS: Dual Cortex-A9 @ 667 MHz    │    [DDR3 1 GB]        │
    │   │   PL: 53,200 LUT6 / 220 DSP       │                       │
    │   │                                    │                       │
    │   └────────────────────────────────────┘                       │
    │                                                                │
    │   [HDMI IN]    [HDMI OUT]    [MicroSD]    [QSPI Flash]        │
    │                                                                │
    │   [40-pin J1]                              [40-pin J2]         │
    │   ○○○○○○○○○○○○○○○○○○○○          ○○○○○○○○○○○○○○○○○○○○         │
    │   ○○○○○○○○○○○○○○○○○○○○          ○○○○○○○○○○○○○○○○○○○○         │
    │                                                                │
    │   [LED x4] [BTN x4]    [RTC]    [5V Barrel Jack]              │
    └──────────────────────────────────────────────────────────────────┘
```

**Board Dimensions**: ~130 x 90 mm

---

## 2. On-Board Resources

| Resource | Specification | Notes |
|----------|---------------|-------|
| **FPGA** | XC7Z020-2CLG400I | Zynq-7000, CLG400 BGA, -2 speed, industrial temp |
| **DDR3 SDRAM** | 2x MT41K256M16TW-107 (1 GB total) | 32-bit bus, 1066 MT/s, connected to PS DDR controller |
| **QSPI Flash** | 32 MB (256 Mbit) | Bitstream storage, boot media, quad-SPI |
| **MicroSD Slot** | 1x | Boot media, Linux rootfs, 4-bit SD mode via PS MIO |
| **HDMI Input** | Analog Devices ADV7611 | HDMI 1.4a receiver, I2C config required |
| **HDMI Output** | Analog Devices ADV7511 | HDMI 1.4a transmitter, I2C config required |
| **Gigabit Ethernet** | Micrel KSZ9031 PHY | RGMII, connected to PS GEM0 via MIO |
| **USB 2.0 Host** | SMSC USB3320 PHY | Connected to PS USB0 via MIO, ULPI interface |
| **USB 2.0 OTG** | Via USB3320 | Device mode support |
| **USB-UART** | Silicon Labs CP2102 | PS MIO UART0, debug console, 115200 baud default |
| **USB-JTAG** | On-board Xilinx-compatible | No external Platform Cable needed |
| **RTC** | Dallas DS1302 | Battery backup, SPI interface |
| **EEPROM** | 2x | Board configuration storage |
| **User LEDs** | 4 | Connected to PL pins, active-high |
| **User Buttons** | 4 | Connected to PL pins, active-low with pull-ups |
| **Expansion** | 2x 40-pin headers (J1, J2) | 3.3V LVCMOS, active-low accent per-pin level |
| **Power** | 5V DC barrel jack | On-board regulators for all rails |
| **Crystal** | 33.333 MHz (PS_CLK) | PS processing system reference clock |

---

## 3. Processing System (PS) Resources

The Zynq-7000 PS is a hardened ARM subsystem, not soft logic. It boots independently of the PL.

| PS Resource | Specification |
|-------------|---------------|
| **CPU** | Dual ARM Cortex-A9 @ 667 MHz |
| **L1 Cache** | 32 KB I-cache + 32 KB D-cache per core |
| **L2 Cache** | 512 KB shared |
| **On-Chip RAM** | 256 KB (OCM) |
| **DDR Controller** | DDR3/DDR3L/DDR2/LPDDR2, 32-bit, up to 1066 MT/s |
| **DMA** | 8-channel DMA controller |
| **GigE** | 2x Ethernet MAC (GEM0 used on AX7020) |
| **USB** | 2x USB 2.0 OTG (USB0 used on AX7020) |
| **UART** | 2x (UART0 routed to CP2102 via MIO) |
| **SPI** | 2x SPI controller |
| **I2C** | 2x I2C controller |
| **SD/SDIO** | 2x (SD0 used for MicroSD via MIO) |
| **GPIO** | 54 MIO pins + 64 EMIO pins (EMIO bridges to PL) |
| **PS-PL Interfaces** | 2x GP AXI master, 2x GP AXI slave, 4x HP AXI slave |

---

## 4. Programmable Logic (PL) Resources

The PL fabric is Artix-7 class, integrated with the PS via AXI interconnect.

| PL Resource | Count | Notes |
|-------------|------:|-------|
| **LUT6** | 53,200 | 6-input lookup tables (~2x capability vs LUT4) |
| **Flip-Flops** | 106,400 | D-type registers |
| **Slices** | 13,300 | Each slice: 4 LUT6 + 8 FF |
| **CLB** | 3,325 | Each CLB: 2 slices |
| **BRAM36** | 140 | 36 Kbit blocks (4.9 Mb total), configurable as 2x BRAM18 |
| **DSP48E1** | 220 | 25x18 multiply-accumulate, 48-bit accumulator |
| **MMCM** | 4 | Mixed-mode clock manager (7 outputs each) |
| **PLL** | 2 | Phase-locked loop (simpler than MMCM, fewer outputs) |
| **BUFG** | 32 | Global clock buffers |
| **GTP** | 0 | No transceivers on XC7Z020 (available on XC7Z030+) |
| **XADC** | 1 | Dual 12-bit ADC, on-die temperature/voltage monitor |
| **IOB** | ~200 | Available PL I/O pins (package-dependent) |

---

## 5. PS-Side I/O (MIO Pins)

The PS has 54 multiplexed I/O (MIO) pins directly connected to the ARM subsystem, independent of PL configuration. These are hardwired on the AX7020 board.

| MIO Range | Function | Peripheral | Interface | Notes |
|-----------|----------|------------|-----------|-------|
| MIO[0:15] | QSPI Flash | 32 MB quad-SPI | Quad SPI | Boot bitstream + configuration |
| MIO[16:27] | Gigabit Ethernet | KSZ9031 PHY | RGMII | PS GEM0, 1000BASE-T |
| MIO[28:39] | USB 2.0 | USB3320 PHY | ULPI | PS USB0, host/OTG mode |
| MIO[40:45] | SD Card | MicroSD slot | 4-bit SD | PS SD0, boot media / rootfs |
| MIO[46:47] | UART0 | CP2102 USB-UART | TX/RX | Debug console (115200 baud) |
| MIO[48:51] | UART1 / GPIO | Optional | TX/RX or GPIO | Available for user assignment |
| MIO[52:53] | I2C | Optional | SDA/SCL | Available for user assignment |

### MIO Voltage Banks

| Bank | MIO Range | Voltage | Connected To |
|------|-----------|---------|--------------|
| MIO Bank 0 | MIO[0:15] | 3.3V | QSPI Flash |
| MIO Bank 1 | MIO[16:53] | 1.8V | Ethernet, USB, SD, UART |

**EMIO**: Up to 64 additional EMIO pins bridge PS peripherals (UART, SPI, I2C, GPIO) into the PL fabric. EMIO is configured in Vivado IP Integrator and does not consume MIO pins.

---

## 6. PL-Side I/O

### 6.1 User LEDs

4 LEDs directly connected to PL pins.

| Signal | Schematic Net | I/O Standard | Notes |
|--------|---------------|--------------|-------|
| LED[0] | PL_LED0 | LVCMOS33 | Active-high (to be validated) |
| LED[1] | PL_LED1 | LVCMOS33 | Active-high (to be validated) |
| LED[2] | PL_LED2 | LVCMOS33 | Active-high (to be validated) |
| LED[3] | PL_LED3 | LVCMOS33 | Active-high (to be validated) |

**XDC Constraint Template** (pin numbers TBD -- will be populated from schematics when board arrives):

```tcl
# PL LEDs (pin assignments TBD)
# set_property PACKAGE_PIN <pin> [get_ports {led[0]}]
# set_property IOSTANDARD LVCMOS33 [get_ports {led[0]}]
```

### 6.2 User Push Buttons

4 push buttons connected to PL pins, active-low with external pull-ups.

| Signal | Schematic Net | I/O Standard | Notes |
|--------|---------------|--------------|-------|
| BTN[0] | PL_KEY0 | LVCMOS33 | Active-low, pressed = LOW |
| BTN[1] | PL_KEY1 | LVCMOS33 | Active-low, pressed = LOW |
| BTN[2] | PL_KEY2 | LVCMOS33 | Active-low, pressed = LOW |
| BTN[3] | PL_KEY3 | LVCMOS33 | Active-low, pressed = LOW |

### 6.3 Expansion Headers

Two 40-pin dual-row headers provide user I/O from PL fabric.

| Header | Total Pins | I/O Pins | Power Pins | Ground Pins | I/O Standard |
|--------|:----------:|:--------:|:----------:|:-----------:|:------------:|
| J1 | 40 | 34 | 3 (3.3V) | 3 | LVCMOS33 |
| J2 | 40 | 34 | 3 (3.3V) | 3 | LVCMOS33 |
| **Total** | **80** | **68** | **6** | **6** | |

Each header provides:
- 34 user I/O pins directly connected to PL bank I/O
- 3.3V power pins for external peripheral supply
- Ground reference pins
- All I/O are 3.3V LVCMOS tolerant (active-low accent per-pin level)

**Pin assignments for expansion headers will be populated from the ALINX schematic when the board arrives.**

### 6.4 HDMI I/O (PL)

Both HDMI ports are routed through the PL fabric via dedicated Analog Devices transceiver chips.

| Port | Chip | Data Path | Control Path | Notes |
|------|------|-----------|--------------|-------|
| HDMI Input | ADV7611 | PL pins (24-bit video data) | I2C (from PS or PL) | HDMI 1.4a receiver |
| HDMI Output | ADV7511 | PL pins (24-bit video data) | I2C (from PS or PL) | HDMI 1.4a transmitter |

The ADV7611 (receiver) and ADV7511 (transmitter) both require I2C configuration to set resolution, color space, and operating mode. The I2C bus can be driven from either the PS I2C controller (via EMIO) or a soft I2C master in the PL.

### 6.5 PMOD

The AX7020 does not have dedicated PMOD connectors. PMOD modules can be connected via the 40-pin expansion headers with an adapter or breakout board.

---

## 7. Power

### 7.1 Power Input

| Input | Specification | Notes |
|-------|---------------|-------|
| **Connector** | 5V DC barrel jack | Center-positive |
| **Voltage** | 5V DC | Regulated 5V required |
| **Current** | ~2-3A typical (estimated) | Depends on PL utilization and peripherals |
| **Power** | ~10-15W estimated | Higher with DDR3 active + full PL utilization |

### 7.2 On-Board Voltage Regulators

| Rail | Voltage | Supplies | Notes |
|------|---------|----------|-------|
| 3.3V | 3.3V | PS MIO Bank 0, PL I/O banks, expansion headers | Main I/O voltage |
| 1.8V | 1.8V | PS MIO Bank 1, DDR3 I/O | Ethernet, USB, SD I/O |
| 1.5V | 1.5V | DDR3 SDRAM | DDR3 memory voltage (VDDQ) |
| 1.0V | 1.0V | PS + PL core logic | Zynq internal core voltage |

### 7.3 Power Monitoring

- A USB-C inline power meter between the 5V supply and the board barrel jack can measure actual power consumption
- The Zynq XADC can monitor on-die temperature and core voltage in real time (accessible from PS via `/sys/bus/iio/` under Linux)

---

## 8. JTAG Programming

### 8.1 On-Board USB-JTAG

The AX7020 includes an on-board USB-JTAG adapter. No external Xilinx Platform Cable is required.

| Feature | Specification |
|---------|---------------|
| **Connector** | USB Type-B or Micro-USB (board-revision dependent) |
| **Interface** | Xilinx-compatible JTAG |
| **Auto-detection** | Vivado HW Manager detects automatically |
| **Cable drivers** | Installed with Vivado (Linux: may require udev rules) |

### 8.2 Supported Operations

| Operation | Tool | Notes |
|-----------|------|-------|
| **FPGA Configuration (volatile)** | Vivado HW Manager | Loads bitstream to PL SRAM; lost on power cycle |
| **QSPI Flash Programming** | Vivado HW Manager | Persistent bitstream + FSBL + boot image |
| **PS Debug** | Vivado SDK / Vitis | JTAG debug of ARM Cortex-A9 software |
| **PL Debug** | Vivado ILA/VIO | Integrated logic analyzer, virtual I/O |
| **Linux Boot via JTAG** | Vivado/XSDB | Download FSBL + U-Boot + kernel via JTAG for development |

### 8.3 Programming Flow

```
1. Connect USB-JTAG cable to board and host PC
2. Power on board (5V barrel jack)
3. Open Vivado HW Manager
4. Auto-detect target (should show xc7z020)
5. Program device:
   - Volatile:  right-click device -> Program Device -> select .bit file
   - Persistent: right-click device -> Add Configuration Memory Device -> select QSPI flash type -> program .bin/.mcs
```

### 8.4 Linux udev Rule

To allow non-root Vivado JTAG access on Linux:

```bash
# /etc/udev/rules.d/52-xilinx-digilent-usb.rules
ATTR{idVendor}=="0403", ATTR{idProduct}=="6010", MODE="0666"
ATTR{idVendor}=="0403", ATTR{idProduct}=="6014", MODE="0666"
```

Then reload: `sudo udevadm control --reload-rules && sudo udevadm trigger`

---

## 9. Comparison: AX7020 vs Tang Nano 9K

### 9.1 FPGA Resources

| Feature | Tang Nano 9K (GW1NR-9) | ALINX AX7020 (XC7Z020) | Ratio |
|---------|:----------------------:|:----------------------:|:-----:|
| **FPGA Family** | Gowin LittleBee (GW1NR) | Xilinx Zynq-7000 (Artix-7 PL) | -- |
| **Device** | GW1NR-LV9QN88PC6/I5 | XC7Z020-2CLG400I | -- |
| **Package** | QN88P (88-pin QFP) | CLG400 (400-ball BGA) | -- |
| **Speed Grade** | C6/I5 | -2 (industrial) | -- |
| **LUT** | 8,640 (LUT4) | 53,200 (LUT6) | **6.2x count, ~12x effective** |
| **Flip-Flops** | 6,480 | 106,400 | **16.4x** |
| **BRAM** | 26 x 18Kb (468 Kb) | 140 x 36Kb (4,900 Kb) | **10.5x** |
| **DSP** | 20 (pMAC18, 18x18) | 220 (DSP48E1, 25x18) | **11x** |
| **PLL/MMCM** | 2 PLL | 4 MMCM + 2 PLL | **3x** |

### 9.2 System Resources

| Feature | Tang Nano 9K | ALINX AX7020 | Notes |
|---------|:------------:|:------------:|-------|
| **CPU** | None (soft PicoRV32 @ 25.2 MHz) | Dual Cortex-A9 @ 667 MHz (hard) | 26x clock, superscalar, caches |
| **Memory** | 8 KB SRAM (BSRAM) | 1 GB DDR3 (+ 256 KB OCM) | 131,072x main memory |
| **On-die RAM** | 64 Mbit PSRAM (embedded) | 256 KB OCM | PSRAM needs controller IP |
| **Boot Flash** | 32 Mbit SPI NOR | 256 Mbit QSPI NOR | 8x capacity |
| **OS Support** | Bare-metal only | Linux (Ubuntu, PetaLinux) | Full OS with networking |
| **Bus Architecture** | Wishbone (custom) | AXI4 / AXI4-Lite | Industry standard |

### 9.3 Peripherals

| Peripheral | Tang Nano 9K | ALINX AX7020 |
|------------|:------------:|:------------:|
| **HDMI Output** | 1x (TMDS via PL pins) | 1x (ADV7511 transmitter via PL) |
| **HDMI Input** | None | 1x (ADV7611 receiver via PL) |
| **Gigabit Ethernet** | None | 1x (KSZ9031 PHY, PS MIO) |
| **USB 2.0** | None (USB-C for power/JTAG only) | 1x Host/OTG (USB3320 PHY, PS MIO) |
| **USB-UART** | 1x (BL702 via USB-C) | 1x (CP2102, separate USB port) |
| **MicroSD** | 1x (SPI mode via PL) | 1x (4-bit SD mode via PS MIO) |
| **User LEDs** | 6 (PL, active-low) | 4 (PL) |
| **User Buttons** | 2 (PL, active-low) | 4 (PL, active-low) |
| **RTC** | None | 1x (DS1302, battery backup) |
| **EEPROM** | None | 2x |

### 9.4 Expansion and Connectivity

| Feature | Tang Nano 9K | ALINX AX7020 |
|---------|:------------:|:------------:|
| **Expansion Headers** | 2x single-row (24 I/O) | 2x 40-pin dual-row (68 I/O) |
| **I/O Voltage** | 3.3V / 1.8V (bank-dependent) | 3.3V LVCMOS (headers) |
| **PMOD** | Compatible via header | No dedicated connector (adapter needed) |
| **Total User I/O** | ~24 pins | ~68 pins |

### 9.5 Physical and Practical

| Feature | Tang Nano 9K | ALINX AX7020 |
|---------|:------------:|:------------:|
| **Dimensions** | 70 x 26 mm | ~130 x 90 mm |
| **Power Input** | USB-C (5V, 500 mA) | 5V DC barrel jack (2-3A) |
| **Power Draw** | ~0.5W | ~10-15W estimated |
| **Programming** | openFPGALoader (CLI) | Vivado HW Manager (GUI/TCL) |
| **JTAG** | Via BL702 (USB-C) | On-board USB-JTAG (dedicated port) |
| **Toolchain Cost** | Free (Gowin EDA Education) | Free (Vivado WebPACK for XC7Z020) |
| **Price** | $13.50 | $195 |
| **Form Factor** | Breadboard-friendly | Desktop development board |

### 9.6 ATOMiK Implications

| Capability | Tang Nano 9K (Current) | ALINX AX7020 (Planned) |
|------------|----------------------|----------------------|
| **ATOMiK Banks** | N=1 (practical limit N=4-8) | N=64+ feasible |
| **ATOMiK Clock** | 81 MHz (PLL-limited) | 200+ MHz (MMCM-derived) |
| **Throughput** | 94.5 Mops/s (N=1) | 12,800+ Mops/s est. (N=64 @ 200 MHz) |
| **CPU Access** | MMIO via Wishbone | MMIO via AXI4-Lite (or DMA via HP port) |
| **Software Stack** | Bare-metal C | Linux userspace (UIO driver) |
| **Multi-board** | SPI (custom protocol) | Ethernet (standard networking) |

---

## 10. Notes and Caveats

### 10.1 Pre-Board Validation Status

This document was written before the board arrived. The following items require validation when the board is in hand:

- [ ] PL LED pin assignments (PACKAGE_PIN values for XDC)
- [ ] PL button pin assignments
- [ ] Expansion header pin mapping (J1, J2 PACKAGE_PIN values)
- [ ] HDMI I/O pin assignments (ADV7611 data bus, ADV7511 data bus)
- [ ] I2C bus assignments for ADV7511/ADV7611 configuration
- [ ] LED polarity (active-high vs active-low -- assumed active-high)
- [ ] Button polarity and pull-up configuration
- [ ] Actual power consumption measurement
- [ ] JTAG USB connector type (Type-B vs Micro-USB, board-revision dependent)
- [ ] UART baud rate default in ALINX BSP

### 10.2 ALINX Resources

ALINX provides the following for the AX7020 (download from their website or included SD card):

| Resource | Contents |
|----------|----------|
| **Schematics** | Full board schematic in PDF (pin assignments, power tree) |
| **User Manual** | Board overview, jumper settings, boot mode selection |
| **BSP** | Vivado project, block design, PetaLinux BSP |
| **Example Designs** | LED blink, HDMI loopback, Ethernet, USB, SD card |
| **Constraint File** | Master XDC with all pin assignments |

**The ALINX master XDC file is the definitive source for pin assignments.** All pin numbers in this document should be cross-referenced against it.

### 10.3 HDMI Configuration

Both HDMI chips require I2C initialization before they pass video data:

| Chip | I2C Address | Function | Driver |
|------|-------------|----------|--------|
| ADV7611 (receiver) | 0x4C (default) | Configure input format, EDID, color space | Linux: `adv7604` driver |
| ADV7511 (transmitter) | 0x39 (default) | Configure output resolution, audio, HDCP | Linux: `adv7511` driver |

Under Linux, these are typically configured via device tree entries and kernel DRM/KMS drivers. For bare-metal PL-only usage, an I2C master (soft or hard) must write the configuration register sequence at boot.

### 10.4 Boot Mode Selection

The Zynq boot mode is selected by MIO[5:2] pin straps (active at power-on reset):

| Mode | MIO[5:2] | Source | Use Case |
|------|----------|--------|----------|
| JTAG | 0x0 | JTAG cable | Development, debugging |
| QSPI | 0x1 | On-board 32 MB QSPI flash | Production boot |
| SD Card | 0x5 | MicroSD slot | Linux development |

The AX7020 typically has a DIP switch or jumper block to select boot mode. Consult the ALINX user manual for the exact switch positions.

---

## Appendix A: Revision History

| Date | Change |
|------|--------|
| 2026-03-07 | Initial version. Pre-board reference from ALINX documentation and Xilinx datasheets. |

---

*References: ALINX AX7020 User Manual, Xilinx DS187 (Zynq-7000 Data Sheet: Overview), UG585 (Zynq-7000 TRM), Analog Devices ADV7511/ADV7611 datasheets, Micrel KSZ9031 datasheet, Silicon Labs CP2102 datasheet, SMSC USB3320 datasheet, Dallas DS1302 datasheet*
