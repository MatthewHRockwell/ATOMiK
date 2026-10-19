# RK-ZYNQ7020-F Board Debug Plan
## Systematic, No-Guessing Approach

### Root Causes Identified

**1. Wrong pin assignments (CERTAIN)**
Every bitstream we tested uses pins from CLG400 (ALINX AX7020) or guessed CLG484 pins (ZC702). This board's actual LED pins are unknown. Confirmed: M14 doesn't even exist as a PL I/O on CLG484.

**2. ps7_init_rk7020f.tcl had EMPTY procedure bodies (CERTAIN)**
The auto-generated TCL from the C init file had 0 register writes due to a parser bug. The REAL ps7_init is at `/tmp/xsa_extract/ps7_init.tcl` (35KB with actual register writes). This must be fixed first.

**3. Level shifters + FPGA resets not configured after JTAG programming (CONFIRMED)**
Vivado's `program_hw_devices` loads the bitstream but does NOT enable PS-PL level shifters (0xF8000900) or deassert FPGA resets (0xF8000240). These are PS-side SLCR registers that must be set via ps7_post_config after programming. Without this, PL outputs may not propagate to physical pins.

### Pre-Built Bitstreams Ready

All in `hardware/zynq/test/`:
- `all_low.bit` — ALL 167 CLG484 PL I/O pins driven LOW (LVCMOS33)
- `all_low_400.bit` — ALL 125 CLG400 PL I/O pins driven LOW (LVCMOS33)
- `group0.bit` through `group3.bit` — CLG484 pins split into 4 groups (~42 pins each)
- `blink_fclk_full.bit` + `blink_fclk_full.xsa` — PS7 FCLK blink with correct ps7_init

### Debug Sequence

#### Step 0: Fix ps7_init (MUST DO FIRST)
```bash
cp /tmp/xsa_extract/ps7_init.tcl hardware/zynq/scripts/ps7_init.tcl
```
This restores the REAL ps7_init with actual register writes (PLLs, MIO, DDR, clocks).

#### Step 1: Determine package with ALL-LOW test

Boot mode: **JTAG (switches 00)**. Power cycle.

**Test A — CLG484 (167 pins all LOW):**
```
xsdb program_zynq_proper.tcl hardware/zynq/test/all_low.bit
```
→ If ANY PL LEDs turn on → board is CLG484, go to Step 2

**Test B — CLG400 (125 pins all LOW):**
```
xsdb program_zynq_proper.tcl hardware/zynq/test/all_low_400.bit
```
→ If ANY PL LEDs turn on → board is CLG400, product listing was wrong

**Test C — Neither works:**
Read GTS status to check if PL I/O is released:
```
xsdb> mrd 0xF8007014
```
Decode STATUS register:
- Bit [11] PSS_GTS_USR_B: 1 = GTS released (I/O active), 0 = I/O still three-stated
- Bit [8] PSS_GTS_CFG_B: 1 = config GTS released
- Bit [4] PCFG_INIT: 1 = PL initialized

If GTS not released → try the PCAP programming path (program via xsdb `fpga` command AFTER ps7_init) instead of Vivado.

Also check level shifters and FPGA resets:
```
xsdb> mrd 0xF8000900   # Must be 0x0000000F
xsdb> mrd 0xF8000240   # Must be 0x00000000
```

#### Step 2: Find LED pin group (binary search)

Program each group and check which turns on LEDs:
```
xsdb program_zynq_proper.tcl hardware/zynq/test/group0.bit
xsdb program_zynq_proper.tcl hardware/zynq/test/group1.bit
xsdb program_zynq_proper.tcl hardware/zynq/test/group2.bit
xsdb program_zynq_proper.tcl hardware/zynq/test/group3.bit
```
The group that lights LEDs contains the LED pins (~42 candidates).

#### Step 3: Bisect within winning group

Split the 42-pin group in half, build two bitstreams, test each. Repeat ~4 times to narrow to individual pins. Script can automate this.

#### Step 4: Verify with blink test

Build a proper blink test using the identified LED pins + PS7 FCLK clock. Confirm LEDs blink at expected rate.

#### Step 5: Find UART

**Option A — Read factory config (QSPI boot):**
Set QSPI mode (10), power cycle, then via xsdb:
```
targets -set -filter {name =~ "ARM*#0"}
stop
mrd 0xF8000154    # UART_CLK_CTRL — which UART clocks active
mrd 0xF8000728    # MIO10 — UART0 RX?
mrd 0xF800072C    # MIO11 — UART0 TX?
mrd 0xF80007C0    # MIO48 — UART1 TX?
mrd 0xF80007C4    # MIO49 — UART1 RX?
mrd 0xE0000000    # UART0 CR
mrd 0xE0001000    # UART1 CR
```

**Option B — Check FTDI EEPROM:**
```bash
sudo apt install ftdi-eeprom libftdi1-dev
sudo rmmod ftdi_sio
pip install pyftdi
python -m pyftdi.bin.ftconf ftdi:///1 -o ftdi_config.ini
cat ftdi_config.ini  # Shows channel_a_type, channel_b_type
```

**Option C — UART may be EMIO (through PL):**
If FTDI channel B connects to PL pins (not PS MIO), the UART requires a PL bitstream with EMIO routing. The factory QSPI image may include a bitstream that does this. Check DONE LED in QSPI mode — if DONE is ON, there's a factory bitstream with EMIO routing.

#### Step 6: Find HDMI pins

After LEDs work, HDMI can be found by:
1. Same binary search method (drive groups of pins, check HDMI monitor)
2. Or cross-reference CLG484 pinout — TMDS pairs are on MRCC/SRCC pins in banks 34/35

#### Step 7: Find 50 MHz PL clock pin

Once UART works, build a design that:
1. Uses PS7 FCLK as a known working clock
2. Reads each potential clock input pin and reports frequency via UART
3. The pin reading ~50 MHz is the oscillator

Or: check CLG484 MRCC pins (there are only ~8 on the whole device). The oscillator is on one of them.

### Key Diagnostic Registers

| Register | Address | Expected | Meaning |
|----------|---------|----------|---------|
| DEVCFG STATUS | 0xF8007014 | Bit[11]=1, Bit[8]=1 | GTS released, PL I/O active |
| DEVCFG INT_STS | 0xF800700C | Bit[2]=1 | PCFG_DONE (PL configured) |
| LVL_SHFTR_EN | 0xF8000900 | 0x0000000F | All level shifters enabled |
| FPGA_RST_CTRL | 0xF8000240 | 0x00000000 | All FPGA resets deasserted |
| PLL_STATUS | 0xF800010C | 0x0000003F | All PLLs locked |
| UART_CLK_CTRL | 0xF8000154 | Bit[0] or Bit[1] | Which UART clock is active |
