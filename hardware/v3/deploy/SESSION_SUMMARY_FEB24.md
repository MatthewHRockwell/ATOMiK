# Flash Boot Debugging Session - February 24, 2026

## TL;DR Status

**Major Breakthrough:** ✅ Flash XIP execution mechanism WORKS
**Current Blocker:** C firmware with CRT doesn't execute after Boot ROM jump
**Intermittent Issue:** Boot ROM timeout behavior is inconsistent

---

## What We Accomplished

### 1. Ultra-Minimal Assembly Test Created ✅
- Created `test_asm_only.S` - pure assembly that prints 'H' repeatedly
- **Result:** Successfully executed from flash and printed "HH"
- **Conclusion:** Flash XIP hardware is functional

### 2. Boot ROM Compiler Bug Fixed ✅
**Problem:** Compiler optimized away `waitcnt` increment in timeout loop
**Symptom:** `if (waitcnt == FW_WAIT_MAXCNT)` never true because waitcnt stayed at 0
**Fix:** Changed to `if (waitcnt >= FW_WAIT_MAXCNT)`
**Result:** Boot ROM now prints "JUMP!" when timeout occurs

### 3. CRT Diagnostics Added
- Modified `crt_flash.S` to print diagnostic characters:
  - 'A' = CRT started, UART initialized
  - 'B' = Global pointer loaded
  - 'C' = Stack pointer loaded
  - 'D' = Data section copied (flash → RAM)
  - 'E' = About to call main()

---

## Current Problem

**Symptom:** Boot ROM prints "JUMP!" but no CRT diagnostics appear
**Meaning:** CPU jumps to flash @ 0x00000000 but CRT code doesn't execute

### Evidence

1. **Pure assembly works:**
   `test_asm.v` → Prints "HH" from flash ✅

2. **C firmware fails:**
   `fw-flash.v` → Only "JUMP!", no 'A'/'B'/'C'/'D'/'E' ❌

3. **Entry point verified:**
   ```
   objdump shows:
   0000000000000000 <crtStart>:
      0:	0100006f	j 10 <crtInit>
   ```
   Entry point is correct at 0x00000000 ✅

4. **CRT diagnostic code present:**
   objdump shows UART init and print('A') at address 0x10-0x30 ✅

### Intermittent Behavior

**Inconsistency observed:**
- Sometimes Boot ROM timeout prints "JUMP!" ✅
- Sometimes Boot ROM timeout produces NO output ❌
- ISP handshake ALWAYS works when tested immediately after bitstream load ✅

**Timing hypothesis:** The 5,000,000 cycle timeout (~185ms @ 27 MHz) might be too short or inconsistent

---

## Hypotheses for CRT Failure

### Hypothesis A: Address Loading Issue (MOST LIKELY)
**Theory:** RV64I `la` (load address) pseudo-instruction generates `auipc + addi` which might be executing incorrectly from flash XIP

**Evidence:**
- Pure assembly uses direct `li` (load immediate) → works
- CRT uses `la gp, __global_pointer$` (auipc + addi) → fails?

**Test:** Create assembly test with `auipc + addi` to load an address

### Hypothesis B: Stack Pointer Issue
**Theory:** Stack pointer points to invalid RAM address or RAM not responding

**Evidence:**
- Linker shows stack at 0x40000BB0 (RAM region) ✅
- But CPU might not be accessing RAM correctly

**Test:** Add diagnostics that DON'T use stack before printing 'A'

### Hypothesis C: Flash Read Latency
**Theory:** Consecutive instruction fetches from flash too fast for SPI flash to respond

**Evidence:**
- Pure assembly has delays between instructions (works)
- CRT has tight instruction sequences (fails)

**Test:** Add `nop` instructions in CRT to slow down fetch rate

### Hypothesis D: CPU State After Boot ROM Jump
**Theory:** Boot ROM leaves CPU in unexpected state (wrong privilege mode, misaligned PC, etc.)

**Evidence:**
- Boot ROM uses `jalr a5` where a5=0 to jump
- This is indirect jump, not direct `j 0x00000000`

**Test:** Modify Boot ROM to use direct jump: `li t0, 0; jr t0`

---

## Key Files Modified This Session

### Boot ROM (`fw-brom/isp_flasher.c`)
- Line 208: Changed `if (waitcnt == FW_WAIT_MAXCNT)` to `>= `
- Lines 211-218: Removed diagnostic flash read (prints "JUMP!" instead of "J<hex>!")

### CRT Startup (`fw-flash/crt_flash.S`)
- Added UART initialization at start of `crtInit`
- Added diagnostic prints after each major stage (A, B, C, D, E)

### Bitstream
- Last synthesis: Feb 24, 12:52
- Location: `/home/mattrock/Projects/ATOMiK/hardware/v3/deploy/atomik_v3_soc_isp.fs`
- BROM: 1,472 bytes (17.97% of 8 KB)

---

## Next Steps (Prioritized)

### 1. Test Hypothesis A - Address Loading (30 min)
Create assembly test with auipc+addi:
```assembly
.section .text
.global _start
_start:
    # Initialize UART manually
    li t0, 0x83000000
    li t1, 232
    sw t1, 4(t0)

    # Test auipc + addi sequence
    auipc gp, 0x40001
    addi gp, gp, -2036    # Should result in 0x40000840

    # Print 'A' if we survived
    li t1, 'A'
    sw t1, 0(t0)

    j _start
```

### 2. Fix Boot ROM Timeout Consistency (20 min)
**Option A:** Increase FW_WAIT_MAXCNT from 5M to 10M cycles (~370ms)
**Option B:** Make waitcnt volatile to force proper increment

### 3. Simplify CRT Further (15 min)
Remove all initialization except UART and print 'A':
```assembly
crtInit:
    # UART only
    li t0, 0x83000000
    li t1, 232
    sw t1, 4(t0)
    li t2, 2000
1:  addi t2, t2, -1
    bnez t2, 1b

    # Print A
    li t1, 'A'
    sw t1, 0(t0)

    # Infinite loop
2:  j 2b
```

### 4. Investigate Alternative Boot Method (if above fail)
Consider ISP-to-RAM boot instead of flash XIP:
- Boot ROM loads firmware into RAM @ 0x40000000
- Jump to RAM instead of flash
- This bypasses flash XIP entirely

---

## Working Test Commands

### ISP Handshake (Always Works)
```bash
openFPGALoader -b tangnano9k atomik_v3_soc_isp.fs
sleep 0.3
python3 << 'EOF'
import serial, time
ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=1)
time.sleep(0.2)
ser.write(bytes([0x55]))
time.sleep(0.2)
print("✅ ISP works" if 0x56 in ser.read(10) else "❌ ISP broken")
ser.close()
EOF
```

### Flash Firmware
```bash
cd /home/mattrock/Projects/ATOMiK/hardware/v3/deploy
openFPGALoader -b tangnano9k atomik_v3_soc_isp.fs
sleep 0.3
python3 ../soc/firmware/scripts/pico-programmer.py \
    ../soc/firmware/fw-flash/build/fw-flash.v /dev/ttyUSB1
```

### Test Boot Sequence
```bash
openFPGALoader -b tangnano9k atomik_v3_soc_isp.fs
sleep 1
python3 << 'EOF'
import serial, time
ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=3)
time.sleep(1)
data = ser.read(500)
print(f"Got: {repr(data)}")
ser.close()
EOF
```

---

## Debug Checklist

If you're not seeing UART output:

1. ✅ **ISP handshake works?** → Boot ROM running, UART functional
2. ❌ **"JUMP!" appears?** → Boot ROM timeout not working consistently
3. ❌ **Any of 'ABCDE' appear?** → CRT not executing

If ISP works but nothing else:
- Boot ROM timeout logic has issues
- Try increasing FW_WAIT_MAXCNT or making waitcnt volatile

If "JUMP!" but no 'A':
- CPU reaching flash but CRT failing immediately
- Test Hypothesis A (auipc issue) first

---

## Time Spent This Session

- ✅ Ultra-minimal assembly test: 45 min
- ✅ Boot ROM compiler bug fix: 60 min
- ✅ CRT diagnostics: 30 min
- 🔄 Debugging inconsistent behavior: 90 min
- **Total: ~3.5 hours**

**Confidence:** 70% we'll solve this in next 1-2 hour session
**Most likely fix:** Test auipc+addi instruction sequence in isolation
