# Phase 3B: Flash XIP Validation

**Date:** February 23, 2026
**Status:** 🔄 **IN PROGRESS - Ready for Hardware Test**

## Objective

Validate that the ATOMiK v3 CPU can execute instructions directly from SPI flash (XIP - Execute-In-Place) at address 0x00000000.

## Changes Made

### 1. Boot ROM Firmware Update

**File:** `hardware/v3/soc/firmware/fw-brom/isp_flasher.c`

**Change:** Modified timeout behavior to jump to flash instead of test loop.

**Before:**
```c
if (waitcnt == FW_WAIT_MAXCNT) {
    // TEST: Don't jump - stay in BROM and print continuously
    while (1) {
        uart_putchar('T');
        uart_putchar('E');
        uart_putchar('S');
        uart_putchar('T');
        uart_putchar('\n');
        for (volatile int i = 0; i < 100000; i++);
    }
}
```

**After:**
```c
if (waitcnt == FW_WAIT_MAXCNT) {
    // Phase 3B: Jump to flash firmware at 0x00000000
    uart_putchar('J');  // 'J' = Jumping to flash
    volatile uint32_t *fp = (volatile uint32_t *)0x00000004;
    uint32_t w = *fp;
    for (int i = 7; i >= 0; i--) {
        uint8_t n = (w >> (4*i)) & 0xF;
        uart_putchar(n < 10 ? '0' + n : 'a' + n - 10);
    }
    uart_putchar('!');

    // Jump to flash entry point (0x00000000)
    void (*flash_entry)(void) = (void (*)(void))0x00000000;
    flash_entry();

    // Should never reach here
    while (1) {
        uart_putchar('E');  // 'E' = Error (jump failed)
        // ...
    }
}
```

**Diagnostic Output:**
- `J<hex>!` - Boot ROM jumping to flash (hex = first instruction word)
- `ERR\n` - Jump failed (should never happen)

### 2. Flash XIP Test Firmware

**File:** `hardware/v3/soc/firmware/fw-flash/test_flash_xip.c`

**Purpose:** Minimal firmware to prove XIP is working.

**Behavior:**
1. Initialize UART (115200 baud, 13.5 MHz clock)
2. Print banner identifying flash execution
3. Print current Program Counter (should be 0x000000xx)
4. Continuous heartbeat with incrementing counter

**Expected Output:**
```
=== ATOMiK v3 Flash XIP Test ===
Phase 3B: SPI Flash Execute-In-Place
Running from: 0x00000000 (SPI Flash)
Current PC: 0x000000xx

Flash XIP Working! (Heartbeat below)
FLASH-XIP-OK [00000000]
FLASH-XIP-OK [00000001]
FLASH-XIP-OK [00000002]
...
```

**Memory Usage:**
- Flash: 696 bytes (0.01% of 8 MB)
- RAM: 2 KB (25% of 8 KB)

### 3. Boot ROM BSRAM Update

**Files Updated:**
- `hardware/v3/soc/gowin_ip/bootram_2kx8_0/bootram_2kx8_0.v`
- `hardware/v3/soc/gowin_ip/bootram_2kx8_1/bootram_2kx8_1.v`
- `hardware/v3/soc/gowin_ip/bootram_2kx8_2/bootram_2kx8_2.v`
- `hardware/v3/soc/gowin_ip/bootram_2kx8_3/bootram_2kx8_3.v`

**Method:** Used `update_bootram.py` script to embed new Boot ROM firmware.

**Boot ROM Size:** 1,520 bytes (18.55% of 8 KB BROM)

## Deployment Procedure

### Prerequisites
- Tang Nano 9K connected via USB
- `/dev/ttyUSB1` accessible (UART console)
- Gowin EDA synthesis completed (bitstream ready)

### Step 1: Flash the Bitstream

```bash
cd /home/mattrock/Projects/ATOMiK/hardware/v3/synth
openFPGALoader -b tangnano9k -f impl/pnr/atomik_v3_soc.fs
```

**Expected:** Bitstream flashed to persistent flash, CRC verified.

### Step 2: Program Flash Firmware via ISP

```bash
cd /home/mattrock/Projects/ATOMiK/hardware/v3/soc/firmware/fw-flash
python3 /home/mattrock/Projects/TangNano-9K-example/picotiny/pico-programmer.py \
    build-xip/fw-flash-xip.v /dev/ttyUSB1
```

**ISP Protocol:**
1. Press Reset button (S1) on Tang Nano 9K
2. Immediately send `0x55` byte (ISP sync)
3. Boot ROM responds `0x56` (ACK)
4. Erase sectors, program pages
5. Reset to boot from flash

### Step 3: Monitor Serial Output

```bash
picocom -b 115200 /dev/ttyUSB1
# or
hexdump -C /dev/ttyUSB1
```

**Expected Boot Sequence:**

1. **Boot ROM Diagnostic:**
   ```
   J<8 hex digits>!
   ```
   Example: `J6f000001!` (first instruction = `j 10` from crtStart)

2. **Flash Firmware Banner:**
   ```
   === ATOMiK v3 Flash XIP Test ===
   Phase 3B: SPI Flash Execute-In-Place
   Running from: 0x00000000 (SPI Flash)
   Current PC: 0x000000xx

   Flash XIP Working! (Heartbeat below)
   FLASH-XIP-OK [00000000]
   FLASH-XIP-OK [00000001]
   ...
   ```

## Validation Criteria

| Criterion | Method | Status |
|-----------|--------|--------|
| SPI flash readable | Boot ROM reads 0x00000004, prints hex | ⏳ Pending |
| Jump to flash works | `jalr` executes, PC = 0x00000000 | ⏳ Pending |
| Instruction fetch from flash | crtStart executes, jumps to crtInit | ⏳ Pending |
| Flash startup code | Data copy, BSS clear, ctors run | ⏳ Pending |
| main() execution | Banner prints via UART | ⏳ Pending |
| Continuous operation | Heartbeat increments every ~1 second | ⏳ Pending |

## Known Issues & Workarounds

### Issue 1: Corrupted Flash Content
**Symptom:** Boot ROM jumps but flash firmware doesn't run (no banner).

**Diagnosis:**
- Check Boot ROM diagnostic hex output (should match `crtStart` = `6f000001`)
- If hex shows `ffffffff`, flash is erased/corrupted

**Fix:** Re-run ISP programmer to reflash firmware.

### Issue 2: ISP Mode Trigger Too Early
**Symptom:** Boot ROM enters ISP mode instead of jumping to flash.

**Diagnosis:** UART RX noise during power-on can trigger 0x55 detection.

**Workaround:** Boot ROM flushes 200,000 UART reads before timeout check.

### Issue 3: SPI Flash Timing Violation
**Symptom:** PC jumps to flash but execution fails (ERR output).

**Diagnosis:** `spimemio_puya.v` clock timing issue at 13.5 MHz.

**Fix:** Review SPI flash timing constraints, add wait states if needed.

## Success Criteria

**Phase 3B Complete When:**
- ✅ Boot ROM diagnostic prints `J<hex>!`
- ✅ Flash firmware banner appears
- ✅ Program Counter shows 0x000000xx address range
- ✅ Heartbeat increments continuously

**Phase 3 Next Steps:**
- **Phase 3C:** Port full v2 firmware to RV64I (UART menu, ATOMiK tests)
- **Phase 3D:** Validate ATOMiK custom instructions
- **Phase 3E:** Performance benchmarking vs v2

## Files Modified

| File | Status | Purpose |
|------|--------|---------|
| `fw-brom/isp_flasher.c` | Modified | Jump to flash after timeout |
| `fw-flash/test_flash_xip.c` | New | Minimal XIP test firmware |
| `fw-flash/Makefile.xip` | New | Build script for XIP test |
| `gowin_ip/bootram_2kx8_*/bootram_2kx8_*.v` | Modified | Updated BROM init data |
| `docs/PHASE3B_FLASH_XIP.md` | New | This document |

## Build Artifacts

```
fw-brom/build/fw-brom.elf          # Boot ROM ELF (1,520 bytes)
fw-brom/build/fw-brom_*.mi         # Boot ROM BSRAM init files
fw-flash/build-xip/fw-flash-xip.elf # Flash firmware ELF (696 bytes)
fw-flash/build-xip/fw-flash-xip.v   # Flash firmware Verilog hex
```

## Timeline

- **Boot ROM Update:** Feb 23, 2026 - Complete ✅
- **Flash Test Firmware:** Feb 23, 2026 - Complete ✅
- **BSRAM Update:** Feb 23, 2026 - Complete ✅
- **Hardware Test:** Feb 23, 2026 - **PENDING FPGA** ⏳

---

**Status:** Ready for hardware validation. All firmware built, Boot ROM updated, awaiting synthesis and FPGA deployment.
