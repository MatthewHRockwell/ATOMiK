# ATOMiK v3 SoC - Hardware Deployment Guide

## Build Summary

**Date:** 2026-02-23
**Bitstream:** `atomik_v3_soc.fs`

### Synthesis Results

**Timing:**
- Fmax: 17.585 MHz (target 13.5 MHz, **+30% margin**)
- TNS: 0.000 (zero timing violations)
- All setup/hold paths met

**Resources:**
- LUT: 4,234 / 8,640 (49%)
- Registers: 997 / 6,693 (15%)
- BSRAM: 14 / 26 (54%)

### Firmware Configuration

**Boot ROM (BRINGUP_MODE enabled):**
- Continuous UART 'T' spam at 115200 baud
- GPIO LED heartbeat on pin 10
- Clock: 27 MHz (CLKDIV bypass - see KNOWN_ISSUES V3-013)
- UART module: simpleuart (V3-014 workaround)

---

## Hardware Validation Steps

### Step 1: Flash Bitstream to SRAM

```bash
# From ATOMiK project root
cd hardware/v3/deploy

# Load bitstream to FPGA SRAM (volatile, lost on power cycle)
openFPGALoader -b tangnano9k atomik_v3_soc.fs

# Wait 5 seconds for ISP bootloader timeout
sleep 5
```

**Expected behavior:**
- Status LED on (solid)
- Pin 15 on (power indicator)
- Pin 10 blinking (~3.7 ms period) - CPU heartbeat
- Pin 17 (UART TX) active with 'T' characters at 115200 baud

### Step 2: Monitor UART Output

```bash
# Connect to UART (115200 baud, 8N1)
minicom -D /dev/ttyUSB1 -b 115200

# Or use screen
screen /dev/ttyUSB1 115200

# Or use Python
python3 -c "import serial; s=serial.Serial('/dev/ttyUSB1', 115200); print(s.read(100))"
```

**Expected output:**
```
TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT...
```

Continuous 'T' characters confirms:
1. ✅ CPU executing instructions (fetching from Boot ROM @ 0x80000000)
2. ✅ UART TX functional at correct baud rate
3. ✅ Clock divider stable (27 MHz direct crystal, CLKDIV bypassed)

### Step 3: Verify GPIO Heartbeat

Use oscilloscope or logic analyzer on pin 10:
- **Frequency:** ~270 Hz (100k cycles @ 27 MHz ≈ 3.7 ms period)
- **Duty cycle:** 50%
- **Voltage:** 3.3V CMOS logic levels

This confirms CPU is running independently of UART (isolation test from Phase 3C debugging).

---

## Next Steps (Phase 3D)

Once basic bringup is confirmed:

1. **Disable BRINGUP_MODE** in `isp_flasher.c`
2. **Test ISP flasher protocol** (0x55 handshake, firmware upload)
3. **Port main firmware** (`fw-flash/`) to RV64I
4. **Test ATOMiK custom instructions** via UART menu
5. **Validate HDMI output** (test pattern)

---

## Troubleshooting

### No UART output

**Symptom:** Pin 17 is silent (no transitions)

**Check:**
1. Pin 10 blinking? → If yes, CPU is alive, UART module issue
2. Pin 10 not blinking? → CPU not executing, check bitstream/power
3. Verify baud rate with logic analyzer (should see 8.68 µs bit period)

**Resolution:**
- Re-flash bitstream
- Check USB-UART adapter (CH340, FT232, etc.)
- Verify correct /dev/ttyUSB* device

### GPIO not toggling

**Symptom:** Pin 10 stuck high/low

**Check:**
1. UART active? → If yes, CPU is running but GPIO issue
2. No UART, no GPIO? → CPU not booting from 0x80000000
3. Check Boot ROM BSRAM initialization

**Resolution:**
- Verify firmware embedded in BSRAM (check bootram_2kx8_*.v INIT_RAM_XX)
- Re-run `make -C ../soc/firmware/fw-brom` and update BSRAM

### Wrong baud rate

**Symptom:** Garbled UART output, oscilloscope shows wrong bit period

**Expected:** 8.68 µs/bit (115200 baud)

**If different:**
- 17.36 µs → 57600 baud (CLKDIV working, firmware misconfigured)
- 4.34 µs → 230400 baud (clock doubled, check CLK_FREQ)

**Resolution:** Update `CLK_FREQ` in `isp_flasher.c` to match actual clock

---

## Known Issues

See `/docs/KNOWN_ISSUES.md` for full details:

- **V3-013:** CLKDIV not dividing (firmware compensates with CLK_FREQ=27MHz)
- **V3-014:** manual_uart_tx data corruption (resolved by using simpleuart)

---

## Pin Mapping (Tang Nano 9K)

| Function | Pin # | Net Name |
|----------|-------|----------|
| Crystal  | 4     | clk (27 MHz) |
| Status LED | 10 | led[0] (GPIO heartbeat) |
| Power LED | 15  | led[5] (on when active) |
| User BTN | 3    | btn[0] (reset) |
| UART TX  | 17   | ser_tx |
| UART RX  | 18   | ser_rx |
| SPI CLK  | 60   | flash_clk |
| SPI CS#  | 61   | flash_csb |
| SPI MOSI | 62   | flash_mosi |
| SPI MISO | 63   | flash_miso |
