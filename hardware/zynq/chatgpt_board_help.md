# Help Needed: Pin Assignments for HamGeek RK-ZYNQ7020-F Development Board

## The Board

I have a **HamGeek RK-ZYNQ7020-F** FPGA development board. The FPGA is **XC7Z020-2CLG484I** (Zynq-7000, 484-pin BGA, industrial grade, speed grade -2).

The board's product listing describes these features:
- DDR3 1GB (32-bit)
- eMMC, QSPI flash, SD card slot
- Onboard JTAG+UART integration via USB-C (labeled "Debug")
- USB-C Slave (OTG)
- USB-A Host
- PS Gigabit Ethernet + PL Gigabit Ethernet
- RS485 x2 (on green 4-terminal connectors)
- CAN x2
- FMC (LPC) connector
- HDMI output (mini HDMI connector)
- MIPI CSI interface
- EEPROM, RTC, SPI LCD interface
- 40-pin GPIO expansion header
- PS LEDs + PS buttons
- PL LEDs + PL buttons
- 50 MHz PL oscillator
- 33.333 MHz PS oscillator
- 12V power input with power switch
- Boot mode: 2-position DIP switch (JTAG=00, QSPI=10, SD=11)

Product page: https://www.hgeek.com/products/hamgeek-rk-zynq7020-f-zynq7020-development-board-for-fpga-fmc-40pin-io-support-for-ad9361-sub-card

The board is also sold as "ZYNQ7020-DEV" on AliExpress by TZT.

## What I Need

I need the **exact FPGA pin assignments** (PACKAGE_PIN in Vivado XDC format) for this specific board. The manufacturer (HamGeek) does not publish schematics or XDC files publicly. The included SD card was blank.

Specifically I need:

### 1. PL LEDs
- How many PL LEDs are there?
- What CLG484 package pins are they connected to?
- Are they active-high or active-low?
- What I/O bank are they in?
- What IOSTANDARD (LVCMOS33, LVCMOS25, LVCMOS18)?

### 2. PL Clock (50 MHz oscillator)
- What CLG484 package pin is the 50 MHz PL oscillator connected to?
- Is it single-ended or differential (MRCC pin)?

### 3. HDMI Output
- What CLG484 pins are TMDS_CLK_P/N, TMDS_DATA0_P/N, TMDS_DATA1_P/N, TMDS_DATA2_P/N?
- Is there an HDMI output enable pin?
- Is there an I2C (DDC) interface? What pins?
- Is HDMI direct TMDS from PL, or does it go through an encoder chip (ADV7511, SiI9022, etc.)?

### 4. UART / Serial Console
- The USB-C "Debug" port uses an FTDI FT2232H (USB VID:PID 0403:6010)
- Channel A is confirmed as JTAG (Vivado hw_server uses it)
- Channel B appears as /dev/ttyUSB1 on Linux but produces NO serial data
- I have confirmed via xsdb that PS UART1 on MIO48/49 transmits data (TX FIFO empties), but nothing appears on ttyUSB1
- **What is the FTDI channel B actually connected to?**
- Is the UART routed through EMIO to PL pins instead of directly to MIO?
- Does the UART need a PL bitstream loaded to function (EMIO routing)?

### 5. PL Buttons/Keys
- How many PL buttons?
- What pins?

### 6. 40-Pin GPIO Header
- Pin-to-ball mapping for the 40-pin expansion header

### 7. PS MIO Configuration
From a GitHub repo (FernandesKA/buildroot_custom) that has the ps7_init for this board, I know:
- MIO 0: GPIO (3.3V)
- MIO 1-6: QSPI
- MIO 7-15: GPIO (3.3V)
- MIO 16-27: Ethernet GEM0 (1.8V)
- MIO 28-39: USB0 (1.8V)
- MIO 40-45: SD0 (1.8V)
- MIO 46: GPIO (USB reset)
- MIO 47: GPIO (SD card detect)
- MIO 48-49: UART1 (1.8V, L3_SEL=7)
- MIO 50-51: GPIO
- MIO 52-53: MDIO (Ethernet)

## What I've Already Tried

1. **JTAG works** — Vivado detects `xc7z020_1` and `arm_dap_0`. Programming bitstreams reports "startup status: HIGH".

2. **In QSPI boot mode** (factory image), PS LED1, PL LED1, and PL LED2 turn ON. The DONE LED is OFF (PL unconfigured). This means the PL LEDs are ON due to default pin state (pull-downs on the LED circuit), not from a programmed design.

3. **After programming ANY bitstream** (CLG400 or CLG484), all PL LEDs turn OFF. This is because the bitstream enables internal pull-ups on all pins, overriding the board pull-downs. The LEDs need to be actively driven LOW by the correct pins.

4. **I built and programmed a static LED test** for CLG484 (`assign led = 4'b0000`) targeting pins T22/T21/U22/U21 (ZC702 reference pins) — LEDs stayed off. Those are the wrong pins for this board.

5. **UART1 transmits data** (confirmed via xsdb register reads showing TX FIFO empty) but nothing appears on the FTDI channel B (ttyUSB1). The FTDI channel B does not appear to be connected to MIO48/49.

6. **Level shifters have been enabled** (SLCR LVL_SHFTR_EN = 0xF) and **FPGA resets cleared** (FPGA_RST_CTRL = 0x0) via xsdb. No effect on PL LEDs.

7. **ps7_init and ps7_post_config** have been run via xsdb in the correct sequence (system reset → ps7_init → PL program → ps7_post_config). Still no PL LED activity.

## Board Photos / Physical Description

The board has silkscreen labels visible:
- "PL LED1", "PL LED2", "PL LED3", "PL LED4" near 4 small SMD LEDs
- "PS LED1", "PS LED2" near 2 other SMD LEDs
- "PL KEY1" through "PL KEY4" near pushbuttons
- "PS KEY1", "PS KEY2" near pushbuttons
- "JTAG+UART" or "Debug" near the USB-C debug port
- "HDMI" near the mini-HDMI connector
- "J10" or similar near the 40-pin header
- Boot mode DIP switch with label: "BOOT 1:0, JTAG 00, QSPI 10, SD 11"

## Similar Boards for Reference

The closest reference board with CLG484 is the **Xilinx ZC702** (XC7Z020CLG484-1), but its LED/HDMI pins won't match because the PCB routing is completely different.

The **ALINX AX7Z020B** has similar peripherals (RS485, CAN, HDMI, FMC) but uses CLG400, so its pin assignments are also wrong.

## What Would Help

1. **The schematic PDF** for the RK-ZYNQ7020-F
2. **A Vivado XDC constraint file** for this board
3. **A Vivado .xpr or .xsa project** that was built for this board
4. **Any pin mapping table** showing CLG484 ball → board function
5. **Knowledge of what the FTDI channel B connects to** on this board
6. **Contact info for someone who has successfully used this board** with custom PL designs

If you have this board, have seen documentation for it, or can identify the pin assignments from the product photos, please help!

## Technical Context

This is for the ATOMiK project — we're building a 64-bit RISC-V Linux system with hardware-accelerated delta-state computing on this Zynq board. Everything is ready (VexiiRiscv SoC, Linux kernel, OpenSBI all built for CLG484) but we cannot proceed without knowing the physical pin assignments for this specific board.
