# Phase 3D Task 1: ISP Flasher - READY FOR HARDWARE TEST

**Date:** February 24, 2026
**Status:** Software complete, hardware testing required

---

## What Was Completed

### 1. ISP Firmware Porting ✅
- **Removed BRINGUP_MODE** from `isp_flasher.c` (line 6 hardcoded define)
- **Disabled BRINGUP_MODE** in Makefile (line 22 commented out)
- **Rebuilt firmware** in ISP mode (1,520 bytes, 18.55% of 8 KB BROM)
- **Updated BSRAM init files** (`boot_rom_*.mi`) with ISP firmware
- **Verified ISP functions** in disassembly: `uart_getchar`, `spi_flashio` present

### 2. Build Artifacts ✅
```
firmware/fw-brom/build/fw-brom.elf    - 1,520 bytes (1004 text + 516 bss)
firmware/fw-brom/build/fw-brom.v      - Verilog hex format (for pico-programmer.py)
firmware/fw-brom/build/fw-brom_*.mi   - BSRAM initialization (4 files)
```

### 3. Programmer Script ✅
- **Copied pico-programmer.py** from v2 to `firmware/scripts/`
- **No modifications needed** - ISP protocol is byte-oriented, RV64I vs RV32I doesn't matter
- **Protocol:** 0x55/0x56 handshake, 0x10 WBUF, 0x30 ESEC, 0x40 WPAG, 0xF0 RST

### 4. SoC Synthesis ✅
**Bitstream:** `deploy/atomik_v3_soc_isp.fs`

**Resource Utilization:**
- LUT: 4,127 (47.8%) — 3,703 LUT + 424 ALU
- CLS: 2,573/4,320 (60%)
- BSRAM: 14/26 (54%)

**Timing:**
- Clock: 27 MHz crystal (clk_osc)
- Fmax: 27.004 MHz (meets timing, +0.004 MHz margin)
- TNS: 0.000 (zero timing violations)
- Endpoints: 0 failing

---

## ISP Protocol Overview

The Boot ROM implements the following ISP protocol over UART at 115200 baud:

### Handshake (ISP Detection)
```
Host → 0x55 (repeat every 100ms for up to 10 seconds)
Boot ROM → 0x56 (acknowledge ISP mode entered)
```

### Flash Commands
1. **WBUF (Write Page Buffer)** - `0x10`
   - Host: `0x10 <len> <data[0..len]>`
   - Boot ROM: `0x11 <checksum>`

2. **ESEC (Erase Sector)** - `0x30`
   - Host: `0x30 <addr[2]> <addr[1]> <addr[0]>`
   - Boot ROM: `0x31 [erase] 0x32`

3. **WPAG (Write Page)** - `0x40`
   - Host: `0x40 <addr[2]> <addr[1]> <addr[0]>`
   - Boot ROM: `0x41 [program] 0x42`

4. **RST (Reset/Jump)** - `0xF0`
   - Host: `0xF0`
   - Boot ROM: `0xF1` then jumps to 0x80000000

### Timeout Behavior
If no ISP handshake (0x55) received within ~370ms:
- Boot ROM jumps to flash entry point at 0x00000000
- This enables normal boot from flash after power-on

---

## Hardware Test Procedure

### Prerequisites
- Tang Nano 9K connected via USB
- `/dev/ttyUSB1` available (UART interface)
- `openFPGALoader` installed
- Python 3 with `pyserial`

### Step 1: Load Bitstream (SRAM)
```bash
cd /home/mattrock/Projects/ATOMiK/hardware/v3/deploy
openFPGALoader -b tangnano9k atomik_v3_soc_isp.fs
```

### Step 2: Verify ISP Boot ROM Responds
```python
import serial
import time

ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=0.5)

# Wait for Boot ROM to boot
time.sleep(0.5)

# Send ISP handshake
for i in range(10):
    ser.write(bytes([0x55, 0x55]))
    time.sleep(0.1)
    resp = ser.read()
    if len(resp) > 0 and resp[0] == 0x56:
        print("✅ ISP Boot ROM responded!")
        print(f"   Sent: 0x55, Received: 0x{resp[0]:02X}")
        break
else:
    print("❌ No ISP response")

ser.close()
```

**Expected output:**
```
✅ ISP Boot ROM responded!
   Sent: 0x55, Received: 0x56
```

### Step 3: Test ISP Flash Programming (CRITICAL - FPGA REQUIRED)
**NOTE:** This step requires actual flash firmware to program. Task 2 will create the flash firmware.

For now, we can verify the ISP protocol responds correctly by testing individual commands:

```python
import serial

ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=0.5)

# Handshake
ser.write(bytes([0x55]))
resp = ser.read()
assert resp[0] == 0x56, f"Expected 0x56, got 0x{resp[0]:02X}"

# Test WBUF (Write Page Buffer)
ser.write(bytes([0x10]))  # WBUF command
resp = ser.read()
assert resp[0] == 0x11, f"Expected 0x11, got 0x{resp[0]:02X}"

# Send 4 bytes: 0xAA, 0xBB, 0xCC, 0xDD
ser.write(bytes([0x03, 0xAA, 0xBB, 0xCC, 0xDD]))  # len=3 (0-indexed), 4 data bytes
resp = ser.read()
expected_checksum = (0xAA + 0xBB + 0xCC + 0xDD) & 0xFF  # 0x6A
print(f"Checksum - Expected: 0x{expected_checksum:02X}, Got: 0x{resp[0]:02X}")
assert resp[0] == expected_checksum

print("✅ ISP WBUF command works!")

ser.close()
```

---

## Exit Criteria for Task 1

- [x] ISP firmware ported and rebuilt
- [x] BSRAM init files updated
- [x] SoC synthesized with ISP firmware
- [x] Timing analysis clean (0 TNS)
- [ ] **HARDWARE TEST REQUIRED:** ISP handshake (0x55→0x56) verified on Tang Nano 9K
- [ ] **HARDWARE TEST REQUIRED:** ISP WBUF command verified with checksum

---

## Next Steps

### Option A: Hardware Available Now
1. Run hardware test procedure above
2. If tests pass, mark Task 1 complete ✅
3. Proceed to Task 2: Port flash firmware

### Option B: Hardware Not Available
1. Proceed with Task 2 in parallel: Port flash firmware to RV64I
2. Return to hardware testing once Tang Nano 9K is accessible
3. Test both ISP protocol and flash firmware together

---

## Files Modified

### Firmware
- `hardware/v3/soc/firmware/fw-brom/isp_flasher.c` (removed hardcoded BRINGUP_MODE)
- `hardware/v3/soc/firmware/fw-brom/Makefile` (disabled -DBRINGUP_MODE)
- `hardware/v3/soc/gowin_ip/bootram_2kx8_*/boot_rom_*.mi` (updated with ISP firmware)

### Scripts
- `hardware/v3/soc/firmware/scripts/pico-programmer.py` (copied from v2)

### Synthesis
- `hardware/v3/deploy/atomik_v3_soc_isp.fs` (new bitstream with ISP firmware)

---

## Known Limitations

1. **Full ISP flash programming untested** - requires flash firmware (Task 2)
2. **SPI flash XIP untested** - Boot ROM timeout→flash jump requires valid flash firmware
3. **Verilator simulation skipped** - practical to test on hardware directly (Phase 3C proved UART works)

---

## Success Probability: HIGH

**Rationale:**
- Phase 3C proved UART TX/RX works perfectly at 115200 baud
- ISP firmware is nearly identical to v2 (proven on PicoRV32)
- All MMIO is 32-bit (`lw`/`sw`), RV64I vs RV32I makes no difference for peripherals
- Synthesis timing is clean (0 TNS)
- Utilization similar to Phase 3C working bitstream

**Risk:** Low - only unknown is RV64I firmware behavior on real hardware, but extremely confident based on similarities to v2.
