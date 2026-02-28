# ATOMiK v3 Phase 3D: Flash Boot Debugging - Session End Summary

## Current Status: BLOCKED - Need CPU RTL Investigation

### What We Know (100% Confirmed)

1. **AUIPC instruction is broken** ✅
   - Any code using `la` pseudo-instruction (AUIPC+ADDI) crashes
   - Affects CRT startup code (global pointer, stack pointer initialization)
   - Workaround: Use `li` (load immediate) instead of `la`

2. **CPU executes from Boot ROM** ✅
   - Bringup mode (simple UART spam loop) works perfectly
   - Proves: CPU fetch, UART TX, register writes all functional

3. **Flash XIP controller works** ✅ (from previous session)
   - test_asm_only.S printed "HH" from flash in earlier test
   - Problem is Boot ROM crashes before jumping to flash

### Mystery: ISP Mode Fails Even Without AUIPC

**Symptoms:**
- Bringup mode: Prints 'T' continuously ✅
- ISP mode: Outputs 0x00 (null bytes) continuously ❌

**Same configuration:**
- Same CRT (no AUIPC)
- Same compiler flags
- Same stack pointer (0x800002F0)
- Same UART initialization

**Only difference:**
- Bringup mode: Simple while loop printing 'T'
- ISP mode: Complex ISP flasher logic (UART getchar, SPI flashio, etc.)

**ISP main() should print:**
- Nothing initially (waiting for 0x55 handshake or timeout)
- After ~185ms timeout: "JUMP!" then jump to flash

**But we get:**
- Continuous stream of 0x00 bytes
- Suggests code IS running but outputting zeros

### Investigated & Ruled Out

- ❌ Flash XIP controller issue → Flash works
- ❌ RAM access issue → Bringup uses RAM (stack), works fine
- ❌ Stack pointer overflow → Tested multiple sp values
- ❌ Compiler optimization bugs → Compared assembly, no hidden AUIPC
- ❌ UART initialization → Both modes init UART identically
- ❌ Firmware size → ISP mode only 1600 bytes (20% of 8KB BROM)

### Next Steps (Recommended)

#### Option 1: CPU RTL Investigation (Most Likely to Succeed)
1. **Check AUIPC implementation in atomik_v3_cpu.v**
   - Verify PC is correctly read and added to immediate
   - Check if PC value is valid during AUIPC execution
   - Test in Verilator simulation with single AUIPC instruction

2. **Investigate why ISP mode outputs 0x00**
   - Add Verilator testbench loading ISP firmware
   - Single-step through main() execution
   - Check register values, PC, instruction fetch

3. **Consider CPU bugs beyond AUIPC**
   - Function calls/returns (JAL/JALR)?
   - Register file corruption?
   - Memory-mapped I/O timing?

#### Option 2: Work Around AUIPC Bug (Faster, Less Robust)
1. **Rewrite ALL firmware to avoid PC-relative addressing**
   - No `la` pseudo-instructions
   - No function pointers stored in data section
   - All addresses as absolute `li` loads

2. **Create custom linker script**
   - Place all code/data at known fixed addresses
   - Avoid need for dynamic relocation

3. **Test with ultra-minimal ISP firmware**
   - Strip ISP flasher down to bare minimum
   - Just print "JUMP!" after delay, then jump to 0x00000000
   - See if simpler code works

#### Option 3: Alternative Approach
1. **Use different CPU core**
   - Consider PicoRV32 (v2 proven working)
   - Or SERV (tiny, well-tested)
   - ATOMiK can still be integrated as coprocessor

2. **Fix RV64I CPU from scratch**
   - Start with minimal RV64I subset
   - Add instructions one by one with testbenches
   - Build up to full implementation

### Key Files Modified This Session

```
hardware/v3/soc/firmware/fw-brom/crt_brom.S
  - Removed AUIPC (no `la` instructions)
  - Set sp = 0x800002F0 using `li` (direct load)

hardware/v3/soc/firmware/fw-brom/isp_flasher.c
  - Added BRINGUP_MODE #ifdef for minimal test
  - Bringup works, ISP mode fails (mystery)

hardware/v3/deploy/AUIPC_DEBUG_SESSION_FEB27.md
  - Comprehensive debugging log
```

### Build Commands (Known Working)

```bash
# Bringup mode (WORKS)
cd /home/mattrock/Projects/ATOMiK/hardware/v3/soc/firmware/fw-brom
make clean
make CFLAGS="-march=rv64i -mabi=lp64 -Os -fno-builtin -ffunction-sections -fdata-sections -MD -fstrict-volatile-bitfields -DBRINGUP_MODE"

# Update BSRAM and synthesize
cd /home/mattrock/Projects/ATOMiK/hardware/v3/soc
python3 firmware/scripts/update_bootram.py firmware/fw-brom/build gowin_ip
cd ../synth
LD_PRELOAD=/lib/x86_64-linux-gnu/libstdc++.so.6 LD_LIBRARY_PATH=/opt/gowin/IDE/lib:/lib/x86_64-linux-gnu QT_PLUGIN_PATH=/opt/gowin/IDE/plugins/qt /opt/gowin/IDE/bin/gw_sh synth_v3_soc.tcl

# Test on hardware
openFPGALoader -b tangnano9k impl/pnr/atomik_v3_soc.fs
# Wait 3-5 seconds for ISP bootloader timeout
python3 -c "
import serial, time
ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=2)
time.sleep(1)
data = ser.read(500)
ser.close()
print(repr(data.decode('utf-8', errors='replace')))
"
```

### Expected vs Actual Output

**Bringup mode:**
- Expected: `TTTTTTTTTTTTTT...`
- Actual: `TTTTTTTTTTTTTT...` ✅

**ISP mode:**
- Expected: (nothing for 185ms) then `JUMP!` then jump to flash
- Actual: `\x00\x00\x00\x00\x00\x00\x00...` ❌

### Time Spent This Session
~4 hours of systematic debugging. Major progress on identifying AUIPC bug, but hit wall on ISP mode mystery.

### Recommended Action
Take a break. When resuming:
1. Start with CPU RTL investigation (most promising)
2. Create minimal Verilator testbench for AUIPC
3. If AUIPC works in simulation but not hardware → synthesis/timing issue
4. If AUIPC fails in simulation → CPU implementation bug

---

**Bottom line:** The ATOMiK v3 RV64I CPU has a confirmed bug with the AUIPC instruction, and possibly other issues affecting complex firmware. This needs CPU-level debugging, not firmware debugging.
