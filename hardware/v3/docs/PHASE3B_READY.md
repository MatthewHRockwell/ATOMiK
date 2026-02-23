# Phase 3B: Flash XIP - Ready for Hardware Validation

**Date:** February 23, 2026
**Status:** ✅ **READY FOR HARDWARE TEST**

## Summary

All software preparation for Phase 3B Flash XIP validation is complete:

1. ✅ Boot ROM updated to jump to flash (instead of test loop)
2. ✅ Flash XIP test firmware built (696 bytes)
3. ✅ Boot ROM BSRAM IP updated with new firmware
4. ✅ SoC synthesized successfully
5. ✅ Deployment script created

**Awaiting:** Physical access to Tang Nano 9K FPGA board.

## Build Artifacts

### Boot ROM (BROM)
- **Location:** `hardware/v3/soc/firmware/fw-brom/build/`
- **Size:** 1,520 bytes (18.55% of 8 KB)
- **Behavior:** Wait for ISP sync (0x55), if timeout → jump to flash at 0x00000000
- **Diagnostic:** Prints `J<hex>!` before jump (hex = first flash instruction)

### Flash Firmware (XIP Test)
- **Location:** `hardware/v3/soc/firmware/fw-flash/build-xip/`
- **Size:** 696 bytes (0.01% of 8 MB flash)
- **RAM:** 2 KB (25% of 8 KB)
- **Behavior:** Print banner + heartbeat with counter

### SoC Bitstream
- **Location:** `hardware/v3/synth/impl/pnr/atomik_v3_soc.fs`
- **Resources:**
  - Logic: 4,172 / 8,640 (49%)
  - LUT: 3,846
  - ALU: 326
  - FF: 943
  - BSRAM: 14 / 26 (54%)
- **Clock:** 13.5 MHz (CLKDIV from 27 MHz crystal)
- **Synthesis:** Clean (warnings are benign, HDMI modules optimized away)

## Deployment Procedure

### Automated (Recommended)

```bash
cd /home/mattrock/Projects/ATOMiK/hardware/v3
./scripts/deploy_phase3b.sh
```

**Script Actions:**
1. Verify prerequisites (bitstream, firmware, UART port)
2. Flash bitstream to persistent flash (`openFPGALoader -f`)
3. Prompt for RESET button press
4. Program flash firmware via ISP (`pico-programmer.py`)

### Manual (Step-by-Step)

#### Step 1: Flash Bitstream
```bash
cd /home/mattrock/Projects/ATOMiK/hardware/v3/synth
openFPGALoader -b tangnano9k -f impl/pnr/atomik_v3_soc.fs
```

**Expected Output:**
```
Jedec ID          : 85 60 18
...
Erase SRAM        Done
SRAM Flash        [====================] 100.00%
Done
Erase Flash       [====================] 100.00%
Program Flash     [====================] 100.00%
verify            [====================] 100.00%
CRC check         [pass]
```

#### Step 2: Program Flash Firmware
```bash
cd /home/mattrock/Projects/ATOMiK/hardware/v3/soc/firmware/fw-flash
python3 /home/mattrock/Projects/TangNano-9K-example/picotiny/pico-programmer.py \
    build-xip/fw-flash-xip.v /dev/ttyUSB1
```

**During Programming:**
1. Press RESET button (S1) when prompted
2. ISP sync: Send 0x55 → receive 0x56
3. Erase sectors
4. Program pages
5. Reset to boot from flash

#### Step 3: Monitor Serial Output
```bash
picocom -b 115200 /dev/ttyUSB1
```

**or**

```bash
hexdump -C /dev/ttyUSB1
```

## Expected Output

### Success Sequence

```
J6f000001!

=== ATOMiK v3 Flash XIP Test ===
Phase 3B: SPI Flash Execute-In-Place
Running from: 0x00000000 (SPI Flash)
Current PC: 0x000000a4

Flash XIP Working! (Heartbeat below)
FLASH-XIP-OK [00000000]
FLASH-XIP-OK [00000001]
FLASH-XIP-OK [00000002]
...
```

### Diagnostic Breakdown

1. **`J6f000001!`** - Boot ROM diagnostic
   - `J` = Jumping to flash
   - `6f000001` = First instruction at 0x00000004 (should be `j 10` from crtStart)
   - `!` = Jump executed

2. **Banner** - Flash firmware executing from 0x00000000

3. **`Current PC: 0x000000xx`** - Proves PC is in flash address space (not BROM at 0x8000xxxx)

4. **Heartbeat** - Continuous output proves stable execution

### Failure Modes

| Symptom | Diagnosis | Fix |
|---------|-----------|-----|
| No output | UART not working | Check Phase 3A (UART fix should work) |
| `TEST\n` repeating | Old BROM (not updated) | Re-run update_bootram.py and resynthesize |
| `Jffffffff!` | Flash empty/erased | Re-run ISP programmer |
| `J<hex>!` then nothing | Flash XIP not working | Check spimemio timing, review flash init |
| `ERR\n` repeating | Jump failed (should never happen) | Check CPU jump instruction encoding |

## Validation Checklist

- [ ] Boot ROM diagnostic appears (`J<hex>!`)
- [ ] First instruction hex matches expected (0x6f = `jal` or `j`)
- [ ] Flash firmware banner appears
- [ ] Program Counter shows 0x000000xx range
- [ ] Heartbeat increments continuously
- [ ] Counter increments approximately every 1 second

**If all items pass:** Phase 3B complete ✅

## Next Steps (After Phase 3B)

### Phase 3C: Main Firmware Development
- Port v2 firmware to RV64I
- UART menu with all v2 features
- ATOMiK test wrappers (custom instructions)

### Phase 3D: ATOMiK Custom Instructions
- Test ATOMIK.LOAD / ATOMIK.ACCUM / ATOMIK.READ
- Verify zero bus overhead vs v2 MMIO
- Performance comparison

### Phase 3E: Full Validation
- RISC-V compliance tests on full SoC
- ATOMiK functionality tests
- Performance benchmarks vs v2

## Files Ready for Commit

**New Files:**
- `soc/firmware/fw-flash/test_flash_xip.c`
- `soc/firmware/fw-flash/Makefile.xip`
- `scripts/deploy_phase3b.sh`
- `docs/PHASE3B_FLASH_XIP.md`
- `docs/PHASE3B_READY.md`

**Modified Files:**
- `soc/firmware/fw-brom/isp_flasher.c` (jump to flash)
- `soc/gowin_ip/bootram_2kx8_*/bootram_2kx8_*.v` (updated BROM init data)

**Build Artifacts (not committed):**
- `synth/impl/pnr/atomik_v3_soc.fs` (bitstream)
- `soc/firmware/fw-brom/build/*` (BROM binaries)
- `soc/firmware/fw-flash/build-xip/*` (flash firmware binaries)

---

**Bottom Line:** Phase 3B software preparation complete. Ready for hardware deployment when FPGA is available.
