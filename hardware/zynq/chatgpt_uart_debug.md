# FTDI UART Debug — Everything We Know

## Board: HamGeek RK-ZYNQ7020-F (XC7Z020CLG484-2)

## FTDI EEPROM Configuration (CONFIRMED via pyftdi)
```
Vendor ID:    0x0403
Product ID:   0x6010
Manufacturer: Xilinx
Product:      my product desc
Serial:       0ABC01
Channel A:    FIFO (used for JTAG)
Channel B:    UART (VCP driver)
Channel A driver: D2XX
Channel B driver: VCP
```

Channel B IS configured as UART with VCP. It should appear as /dev/ttyUSB1 (or ttyUSB0 depending on which channel hw_server claims).

## What We've Tested — All Failed

### PS UART on MIO pins (via xsdb register writes)
1. **UART1 on MIO48/49** (ALINX default config)
   - Configured via ps7_init from ALINX XSA
   - UART1 SR shows TEMPTY=1 (data sent)
   - Nothing on ttyUSB1

2. **UART0 on MIO10/11** (RK board's buildroot config shows L3_SEL=7)
   - Manually enabled UART0 clock (UART_CLK_CTRL bit 0)
   - Set MIO10=0x16E1 (UART0 RX), MIO11=0x16E0 (UART0 TX)
   - Configured UART0 at 115200 baud (CD=54, BDIV=15, 100MHz ref)
   - Nothing on ttyUSB1

### PL UART on header pins (via bitstream with PS7 FCLK)
3. **UART TX on all 31 valid 40-pin header pins** (LVCMOS33, CLG484)
   - 50 MHz FCLK, baud divider 434 → 115200 baud
   - Sends "OK\r\n" repeatedly on all pins simultaneously
   - Nothing on ttyUSB1

### PL UART on all non-header pins
4. **UART TX on 152 non-header PL I/O pins** (LVCMOS33)
   - Same FCLK-based UART TX
   - Nothing on ttyUSB1

### PS EMIO UART fanned to header pins
5. **PS UART0 and UART1 in EMIO mode**, TX signal fanned to all 32 header pins
   - Both UARTs configured at 115200
   - Sent "HELLO\r\n" on both UART0 and UART1
   - Nothing on ttyUSB1

### Baud rate scans
6. **Tried all baud rates** (9600 to 921600) on ttyUSB1 while board is in QSPI mode
   - Zero data at any baud rate

## What We've Confirmed Working
- JTAG: Vivado detects xc7z020_1 + arm_dap_0, programming works (startup HIGH)
- PL I/O: 40-pin header confirmed at 3.3V when driven HIGH (measured with multimeter)
- PS7 init: PLLs lock (0x3F), FCLK=50MHz, level shifters enabled (0xF), FPGA resets cleared (0x0)
- PS UART0 and UART1 both transmit (SR shows TEMPTY after sending data)

## Board Physical Description
- USB-C "Debug" port → FTDI FT2232H → Channel A=JTAG, Channel B=UART
- USB-C "Slave" port → PS USB OTG (no additional serial device appears)
- 40-pin header with silkscreen ball names (confirmed correct for CLG484)
- 2 PL LEDs (turn ON when PL unconfigured, OFF when any bitstream loaded)
- 2 PS LEDs
- Board silkscreen: "RK-ZYNQ7020-F V1.0"

## Key Question
The FTDI channel B is configured as UART but we cannot get data to appear on it. What is it connected to?

Possibilities:
1. PS UART on MIO pins we haven't tried (we tested MIO10/11 and MIO48/49)
2. PL pins not covered by our scan (we tested ~183 out of ~200)
3. An EMIO path that requires specific PL routing we haven't provided
4. A chip between the FTDI and the Zynq (level translator, mux) that needs enabling
5. The FTDI BCBUS pins (secondary channel pins) instead of the main data pins
6. The UART is actually connected but our baud rate / parity / stop bits are wrong
7. The FTDI channel B is physically disconnected (manufacturing issue)

## Register Dumps
```
FCLK0_CLK_CTRL (0xF8000170): 0x00400500  → FCLK=50MHz
PLL_STATUS     (0xF800010C): 0x0000003F  → all PLLs locked
UART_CLK_CTRL  (0xF8000154): 0x00000A03  → both UART clocks active
LVL_SHFTR_EN   (0xF8000900): 0x0000000F  → all level shifters on
FPGA_RST_CTRL  (0xF8000240): 0x00000000  → all resets off
DEVCFG_STATUS  (0xF8007014): 0x40000F30  → GTS released, PL active
```

## UPDATE: Board's REAL ps7_init (from buildroot repo)

Successfully converted the board's actual ps7_init_gpl.c to TCL (215 register operations).

### Correct MIO Configuration for THIS Board:
```
UART_CLK_CTRL (0xF8000154): 0x00000A01 → UART0 active, UART1 INACTIVE
MIO10 (0xF8000728): 0x000016E1 → UART0 RX (L3_SEL=7, TRI=1, LVCMOS33)
MIO11 (0xF800072C): 0x000016E0 → UART0 TX (L3_SEL=7, TRI=0, LVCMOS33)
MIO48 (0xF80007C0): 0x00001280 → SD CARD (NOT UART!)
MIO49 (0xF80007C4): 0x00001280 → SD CARD (NOT UART!)
FCLK0 (0xF8000170): 0x00200500 → 100 MHz (IO_PLL/5/2)
```

### Key Difference from ALINX AX7020:
- ALINX: UART1 on MIO48/49, FCLK=50MHz
- RK-ZYNQ7020-F: UART0 on MIO10/11, FCLK=100MHz, MIO48/49 are SD card

### What Still Doesn't Work:
Even with the correct board ps7_init, UART0 transmitting on MIO10/11, data does NOT appear on ttyUSB1. The FTDI channel B (confirmed as UART via EEPROM) is NOT receiving from MIO10/11.

### Current Theory:
The FTDI channel B RX/TX might be connected to PL pins through EMIO, requiring a PL bitstream to route PS UART through the fabric. However, we tested EMIO UART fanned to all 31 header PL pins — also no data. The FTDI might connect to non-header PL pins through EMIO, but we've also tested 152 non-header pins with PL-generated UART — no data.

The only untested possibilities:
1. ~16 PL pins that failed DRC (W8, W10, AB3, and AA/AB pins on restricted banks)
2. A chip between FTDI and Zynq that needs enabling (power enable, OE pin)
3. FTDI BCBUS pins (secondary I/O) instead of main UART pins
4. Manufacturing defect / not connected
