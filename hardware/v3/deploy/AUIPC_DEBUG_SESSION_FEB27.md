# ATOMiK v3 Phase 3D: Flash Boot Failure - Debug Session Feb 27

## Critical Discovery: AUIPC Instruction is Broken

### Evidence
1. **Bringup mode WITHOUT AUIPC works**:
   - CRT uses `li sp, 0x800002F0` (immediate load, no AUIPC)
   - main() prints 'T' successfully ✅
   - UART configured, CPU alive, instruction execution confirmed

2. **Any code path using AUIPC fails**:
   - Original CRT with `la gp, __global_pointer$` → expands to AUIPC+ADDI
   - Result: No output, null bytes on UART ❌
   - Flash firmware also uses AUIPC in CRT → fails ❌

3. **Assembly verification**:
   - Working bringup: `grep auipc build/fw-brom.asm` → no results
   - Broken CRT: Contains `auipc gp, 0x1` at crtInit

## Reproduction

### Working Configuration (Bringup Mode, No AUIPC)
```assembly
# crt_brom.S
crtInit:
  li sp, 0x800002F0    # Direct load - NO AUIPC
  call main

# isp_flasher.c
#ifdef BRINGUP_MODE
int main() {
    UART0->CLKDIV = 232;
    for (volatile int i = 0; i < 2000; i++);
    while (1) {
        UART0->DATA = 'T';
        for (volatile int i = 0; i < 100000; i++);
    }
}
#endif
```

**Result**: Continuous 'T' output on UART ✅

### Broken Configuration (ISP Mode)
Same CRT, same compiler flags, only difference:
```c
#else  // ISP mode (not BRINGUP_MODE)
int main() {
    UART0->DATA = 'M';  // Ultra-minimal test
    UART0->DATA = 'M';
    UART0->DATA = 'M';
    // ... ISP code ...
}
#endif
```

**Result**: Continuous null bytes (0x00) on UART ❌

## Investigated Hypotheses

### ❌ Hypothesis 1: Flash XIP Controller Issue
- **Test**: Ultra-minimal assembly test (test_asm_only.S) executing from flash
- **Result**: When Boot ROM uses AUIPC, flash never executes
- **Conclusion**: Flash XIP works, but Boot ROM crashes before jumping

### ❌ Hypothesis 2: RAM Access Issue
- **Evidence**: Bringup mode writes to stack (RAM), works fine
- **Conclusion**: RAM is functional

### ❌ Hypothesis 3: Compiler Optimization Bug
- **Initial find**: `waitcnt` loop counter wasn't incrementing (compiler optimized away)
- **Fix**: Made `waitcnt` volatile, changed `==` to `>=`
- **Result**: Didn't solve AUIPC issue

### ❌ Hypothesis 4: Stack Pointer Overflow
- **Analysis**: ISP main() allocates 384 bytes stack frame
- **Tested**: sp=0x800002F0, sp=0x800002D0, sp=0x80002000
- **Result**: Only sp=0x800002F0 works for bringup, but ISP mode fails regardless

### ⚠️ Hypothesis 5: Code/Data Corruption
- **Observation**: ISP mode consistently outputs 0x00 bytes
- **Analysis**: If UART->DATA writes are happening but value is 0, suggests:
  - Register a4 (holding 'M') is being cleared
  - OR instruction fetch is corrupted
  - OR CPU is stuck in unexpected loop writing zeros

## Current Status

**Working**:
- ✅ CPU executes instructions from Boot ROM
- ✅ UART transmits data
- ✅ Simple bringup code (no AUIPC, no complex C) runs
- ✅ Flash XIP controller functional (proven by earlier session)

**Broken**:
- ❌ Any CRT code using AUIPC
- ❌ ISP flasher mode (even with no AUIPC in CRT)
- ❌ Flash firmware execution (uses AUIPC in its CRT)

**Mystery**:
- Why does ISP mode fail even after removing ALL AUIPC from CRT?
- Why continuous 0x00 output instead of no output?
- Bringup mode and ISP mode compile to same binary except for main() body

## Key Questions

1. **Is AUIPC completely broken, or only in specific contexts?**
   - From Boot ROM (0x80000000)?
   - From Flash (0x00000000)?
   - Both?

2. **Why does ISP mode output 0x00 instead of crashing silently?**
   - Suggests code IS running
   - But register values are wrong
   - Or wrong code path is executing

3. **What's different between bringup and ISP mode besides #ifdef?**
   - Compiler might generate different code
   - Different optimization paths
   - Different register usage

## Next Steps (Recommended)

1. **Compare bringup vs ISP assembly side-by-side**
   - Check if compiler uses AUIPC anywhere in ISP mode
   - Look for hidden AUIPC in function calls, GOT access, etc.

2. **Test AUIPC in isolation**
   - Create pure assembly test with single AUIPC instruction
   - Execute from Boot ROM address space
   - Check if PC is correctly read

3. **Investigate CPU RTL**
   - Review atomik_v3_cpu.v PC handling
   - Check if AUIPC implementation is correct
   - Simulate AUIPC instruction in Verilator

4. **Work around AUIPC bug**
   - If AUIPC is fundamentally broken, avoid it entirely
   - Use -fno-pic, -mcmodel=medany, explicit li instructions
   - May need custom linker script to avoid PC-relative addressing

## Build Commands

```bash
# Bringup mode (works)
make clean && make CFLAGS="...  -DBRINGUP_MODE"

# ISP mode (broken)
make clean && make  # (no DBRINGUP_MODE)

# Update BSRAM and synthesize
cd /home/mattrock/Projects/ATOMiK/hardware/v3/soc
python3 firmware/scripts/update_bootram.py firmware/fw-brom/build gowin_ip
cd ../synth
LD_PRELOAD=... gw_sh synth_v3_soc.tcl

# Test
openFPGALoader -b tangnano9k impl/pnr/atomik_v3_soc.fs
python3 -c "import serial; ..."
```

## Timeline

- **Phase 3C**: CPU + ATOMiK verified in simulation (53/54 compliance)
- **Phase 3D start**: Hardware bringup, ISP flasher, flash boot
- **Feb 24**: Discovered flash firmware doesn't execute after Boot ROM jump
- **Feb 27 morning**: Created systematic microtest suite (AUIPC, RAM, NOP-padding)
- **Feb 27 afternoon**: BREAKTHROUGH - Bringup without AUIPC works!
- **Feb 27 evening**: MYSTERY - ISP mode fails even without AUIPC in CRT

## Critical Files

- `hardware/v3/soc/firmware/fw-brom/crt_brom.S` - Boot ROM startup (no AUIPC version)
- `hardware/v3/soc/firmware/fw-brom/isp_flasher.c` - Boot ROM main (BRINGUP_MODE vs ISP)
- `hardware/v3/rtl/atomik_v3_cpu.v` - CPU implementation (AUIPC handling?)
- `hardware/v3/rtl/atomik_v3_decode.v` - Instruction decoder
- `hardware/v3/rtl/atomik_v3_control.v` - Control logic

## References

- RV64I AUIPC: `rd = PC + (imm << 12)`
- Used by: la pseudo-instruction, PIC code, GOT access
- Alternative: li with absolute address (requires 2-3 instructions for 64-bit)
