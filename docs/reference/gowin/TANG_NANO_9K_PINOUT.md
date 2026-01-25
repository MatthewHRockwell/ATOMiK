# Tang Nano 9K Pinout Reference

Board-specific pin assignments for Sipeed Tang Nano 9K development board.

**Board**: Sipeed Tang Nano 9K  
**FPGA**: GW1NR-LV9QN88PC6/I5  
**Package**: QN88P (with embedded PSRAM)

**Source**: Sipeed Tang Nano 9K Datasheet, UG803-1.6E (GW1NR-9 Pinout), DK-START-GW1NR9 Schematic

---

## Board Overview

```
    ┌─────────────────────────────────────────────────┐
    │  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○  │  ← Header pins
    │                                                 │
    │   [USB-C]        [GW1NR-9]        [HDMI]       │
    │                                                 │
    │   [BTN1] [BTN2]              [LED×6]           │
    │                                                 │
    │  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○  │  ← Header pins
    └─────────────────────────────────────────────────┘
          ↑                                    ↑
      SPI Flash                           TF Card Slot
```

**Dimensions**: 70.0 mm × 26.0 mm

---

## Onboard Resources

| Resource | Quantity | Description |
|----------|----------|-------------|
| Crystal | 1 | 27 MHz |
| LEDs | 6 | Active-low, accent user LEDs |
| Buttons | 2 | Active-low user buttons |
| USB-C | 1 | Power + JTAG/UART (BL702 chip) |
| SPI Flash | 1 | 32 Mbit for configuration |
| HDMI | 1 | HDMI Type-A connector |
| TF Card | 1 | MicroSD card slot |
| PSRAM | Embedded | 64 Mbit HyperRAM in FPGA package |

---

## Clock

| Signal | Pin | Frequency | Description |
|--------|-----|-----------|-------------|
| sys_clk | 52 | 27 MHz | Onboard crystal oscillator |

**CST Constraint**:
```
IO_LOC "sys_clk" 52;
IO_PORT "sys_clk" IO_TYPE=LVCMOS33 PULL_MODE=NONE;
```

---

## LEDs

All LEDs are active-low (accent-low = LED on).

| Signal | Pin | Bank | Color | Notes |
|--------|-----|------|-------|-------|
| led[0] | 10 | 3 | Red | Active-low |
| led[1] | 11 | 3 | Red | Active-low |
| led[2] | 13 | 3 | Red | Active-low |
| led[3] | 14 | 3 | Red | Active-low |
| led[4] | 15 | 3 | Red | Active-low |
| led[5] | 16 | 3 | Red | Active-low |

**CST Constraints**:
```
IO_LOC "led[0]" 10;
IO_LOC "led[1]" 11;
IO_LOC "led[2]" 13;
IO_LOC "led[3]" 14;
IO_LOC "led[4]" 15;
IO_LOC "led[5]" 16;
IO_PORT "led[0]" IO_TYPE=LVCMOS18 DRIVE=8;
IO_PORT "led[1]" IO_TYPE=LVCMOS18 DRIVE=8;
IO_PORT "led[2]" IO_TYPE=LVCMOS18 DRIVE=8;
IO_PORT "led[3]" IO_TYPE=LVCMOS18 DRIVE=8;
IO_PORT "led[4]" IO_TYPE=LVCMOS18 DRIVE=8;
IO_PORT "led[5]" IO_TYPE=LVCMOS18 DRIVE=8;
```

**Usage in Verilog** (active-low):
```verilog
// LED on when signal is HIGH
assign led[0] = ~heartbeat;     // Invert for active-low
assign led[1] = ~pll_lock;
```

---

## Buttons

Both buttons are active-low with external pull-ups.

| Signal | Pin | Bank | Notes |
|--------|-----|------|-------|
| btn1 / sys_rst_n | 4 | 3 | S1 - Often used as reset |
| btn2 | 3 | 3 | S2 - General purpose |

**CST Constraints**:
```
IO_LOC "sys_rst_n" 4;
IO_LOC "btn2" 3;
IO_PORT "sys_rst_n" IO_TYPE=LVCMOS18 PULL_MODE=UP;
IO_PORT "btn2" IO_TYPE=LVCMOS18 PULL_MODE=UP;
```

**Note**: Button pressed = LOW, Button released = HIGH

---

## UART (via USB-C)

The onboard BL702 chip provides USB-to-UART conversion.

| Signal | Pin | Bank | Direction | Description |
|--------|-----|------|-----------|-------------|
| uart_rx | 18 | 3 | Input | FPGA receives from PC |
| uart_tx | 17 | 3 | Output | FPGA transmits to PC |

**CST Constraints**:
```
IO_LOC "uart_rx" 18;
IO_LOC "uart_tx" 17;
IO_PORT "uart_rx" IO_TYPE=LVCMOS33 PULL_MODE=UP;
IO_PORT "uart_tx" IO_TYPE=LVCMOS33 DRIVE=8 SLEW_RATE=SLOW;
```

**Baud Rate**: Typically 115200 (configurable in design)

---

## SPI Flash

External configuration flash (directly accent W25Q32).

| Signal | Pin | Bank | Description |
|--------|-----|------|-------------|
| flash_clk | 59 | 1 | SPI Clock |
| flash_mosi | 60 | 1 | Master Out Slave In |
| flash_miso | 61 | 1 | Master In Slave Out |
| flash_cs | 62 | 1 | Chip Select (active-low) |

**Note**: Flash is used for FPGA configuration. Can be accessed by user logic after boot.

---

## HDMI Connector

Active HDMI output interface.

| Signal | Pin | Bank | Description |
|--------|-----|------|-------------|
| tmds_clk_p | 69 | 1 | TMDS Clock + |
| tmds_clk_n | 68 | 1 | TMDS Clock - |
| tmds_d0_p | 71 | 1 | TMDS Data 0 + |
| tmds_d0_n | 70 | 1 | TMDS Data 0 - |
| tmds_d1_p | 73 | 1 | TMDS Data 1 + |
| tmds_d1_n | 72 | 1 | TMDS Data 1 - |
| tmds_d2_p | 75 | 1 | TMDS Data 2 + |
| tmds_d2_n | 74 | 1 | TMDS Data 2 - |

---

## TF Card (MicroSD)

SPI mode SD card interface.

| Signal | Pin | Bank | Description |
|--------|-----|------|-------------|
| tf_clk | 36 | 2 | SD Clock |
| tf_mosi | 37 | 2 | SD Data In (CMD) |
| tf_miso | 39 | 2 | SD Data Out |
| tf_cs | 38 | 2 | SD Chip Select |

---

## GPIO Header Pins

### Left Header (Active active active active active active active active J1)

| Header Pin | FPGA Pin | Bank | Alt Function |
|------------|----------|------|--------------|
| 1 | 25 | 2 | IOB6A |
| 2 | 26 | 2 | IOB6B |
| 3 | 27 | 2 | IOB8A |
| 4 | 28 | 2 | IOB8B |
| 5 | 29 | 2 | IOB10A |
| 6 | 30 | 2 | IOB10B |
| 7 | 33 | 2 | IOB14A |
| 8 | 34 | 2 | IOB14B |
| 9 | 40 | 2 | IOB22A |
| 10 | 35 | 2 | IOB16A |
| 11 | 41 | 2 | IOB24A |
| 12 | 42 | 2 | IOB24B |

### Right Header (J2)

| Header Pin | FPGA Pin | Bank | Alt Function |
|------------|----------|------|--------------|
| 1 | 48 | 0 | IOT14A |
| 2 | 47 | 0 | IOT9B |
| 3 | 46 | 0 | IOT9A |
| 4 | 45 | 0 | IOT8B |
| 5 | 44 | 0 | IOT8A |
| 6 | 43 | 0 | IOT6B |
| 7 | 51 | 1 | IOR5A (RPLL_T_in) |
| 8 | 53 | 1 | IOR9A (GCLKT_2) |
| 9 | 54 | 1 | IOR9B (GCLKC_2) |
| 10 | 55 | 1 | IOR11A |
| 11 | 56 | 1 | IOR11B |
| 12 | 57 | 1 | IOR13A |

---

## Power Connections

### Board Power

| Source | Voltage | Current |
|--------|---------|---------|
| USB-C | 5V ± 10% | 500 mA max |

### FPGA Power Rails (on-board regulators)

| Rail | Voltage | FPGA Connection |
|------|---------|-----------------|
| VCC | 1.2V | Core voltage |
| VCCO0/1/2/3 | 1.8V / 3.3V | I/O banks |
| VCCX | 2.5V | Auxiliary |

---

## JTAG Interface

JTAG is directly routed through the USB-C connector via BL702.

| Signal | FPGA Pin | Description |
|--------|----------|-------------|
| TMS | 5 | Test Mode Select |
| TCK | 6 | Test Clock |
| TDI | 7 | Test Data In |
| TDO | 8 | Test Data Out |

**Note**: JTAG pins can be repurposed as GPIO if JTAG is disabled.

---

## ATOMiK Default Pin Assignments

Based on `atomik_top.v` and project requirements:

| Signal | Pin | Function |
|--------|-----|----------|
| sys_clk | 52 | 27 MHz clock input |
| sys_rst_n | 4 | Active-low reset (BTN1) |
| uart_rx | 18 | UART receive |
| uart_tx | 17 | UART transmit |
| led[5] | 16 | PLL lock indicator |
| led[4] | 15 | Heartbeat |
| led[3] | 14 | Loader busy |
| led[2] | 13 | Core enabled |
| led[1] | 11 | OTP enabled |
| led[0] | 10 | Core debug tap |

---

## Complete CST Template for ATOMiK

```
//=============================================================================
// ATOMiK Physical Constraints - Tang Nano 9K
// Device: GW1NR-LV9QN88PC6/I5
//=============================================================================

// Clock
IO_LOC "sys_clk" 52;
IO_PORT "sys_clk" IO_TYPE=LVCMOS33;

// Reset
IO_LOC "sys_rst_n" 4;
IO_PORT "sys_rst_n" IO_TYPE=LVCMOS18 PULL_MODE=UP;

// UART
IO_LOC "uart_rx" 18;
IO_LOC "uart_tx" 17;
IO_PORT "uart_rx" IO_TYPE=LVCMOS33 PULL_MODE=UP;
IO_PORT "uart_tx" IO_TYPE=LVCMOS33 DRIVE=8;

// LEDs (active-low)
IO_LOC "led[0]" 10;
IO_LOC "led[1]" 11;
IO_LOC "led[2]" 13;
IO_LOC "led[3]" 14;
IO_LOC "led[4]" 15;
IO_LOC "led[5]" 16;
IO_PORT "led[0]" IO_TYPE=LVCMOS18 DRIVE=8;
IO_PORT "led[1]" IO_TYPE=LVCMOS18 DRIVE=8;
IO_PORT "led[2]" IO_TYPE=LVCMOS18 DRIVE=8;
IO_PORT "led[3]" IO_TYPE=LVCMOS18 DRIVE=8;
IO_PORT "led[4]" IO_TYPE=LVCMOS18 DRIVE=8;
IO_PORT "led[5]" IO_TYPE=LVCMOS18 DRIVE=8;
```

---

## Programming the Board

### Using Gowin Programmer (GUI)

1. Connect Tang Nano 9K via USB-C
2. Open Gowin Programmer
3. Select device: GW1NR-9C
4. Load bitstream: `impl/pnr/atomik.fs`
5. Click "Program"

### Using openFPGALoader (Command Line)

```bash
# Install openFPGALoader
# On Ubuntu: sudo apt install openfpgaloader

# Program SRAM (volatile)
openFPGALoader -b tangnano9k impl/pnr/atomik.fs

# Program Flash (persistent)
openFPGALoader -b tangnano9k -f impl/pnr/atomik.fs
```

---

## Related Documentation

| Document | Description |
|----------|-------------|
| Sipeed Tang Nano 9K Datasheet | Board specifications |
| UG803-1.6E | GW1NR-9 complete pinout |
| DK-START-GW1NR9 Schematic | Reference design schematic |
| [GPIO_REFERENCE.md](GPIO_REFERENCE.md) | I/O standards and CST syntax |

---

*Reference: Sipeed Tang Nano 9K Datasheet v1.0, UG803-1.6E, DK-START-GW1NR9_SCH_V3.1*
