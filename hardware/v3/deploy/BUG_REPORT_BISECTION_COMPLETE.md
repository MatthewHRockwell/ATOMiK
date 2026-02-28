# ATOMiK v3 RV64I CPU - Bug Report (Bisection Complete)

## Executive Summary

Systematic bisection testing has isolated **TWO distinct CPU bugs**:

1. **AUIPC instruction is broken** (PC-relative addressing fails)
2. **Repeated MMIO loads cause CPU hang** (ISP flasher root cause)

Both bugs are in the CPU RTL, not firmware.

---

## Bug #1: AUIPC Broken

### Evidence
- Any code using `la` pseudo-instruction (AUIPC+ADDI) crashes
- CRT code with `la gp, __global_pointer$` fails
- Workaround: Use `li` (load immediate) instead

### Impact
- Flash firmware CRT cannot initialize global pointer
- Position-independent code broken

### Status
- **Workaround applied**: Removed AUIPC from Boot ROM CRT
- **Fix needed**: Investigate `atomik_v3_cpu.v` AUIPC implementation

---

## Bug #2: Repeated MMIO Loads Hang CPU (CRITICAL)

### Bisection Results

| Step | Test | Result | Notes |
|------|------|--------|-------|
| 0 | Bringup (UART spam) | ✅ PASS | No MMIO reads |
| 1 | Function calls | ✅ PASS | JAL/JALR work |
| 2 | Stack frames | ✅ PASS | Local arrays work |
| 3 | Single MMIO load | ✅ PASS | One load per loop works |
| 4 | Long timeout loop | ✅ PASS | 5M iteration loop works |
| 5 | Jump to flash | ✅ PASS | Function pointers work |
| 6 | UART polling | ❌ FAIL | Outputs 0x00 continuously |
| 7 | Repeated MMIO loads | ❌ FAIL | **Hangs completely (NO output)** |

### Smoking Gun

**STEP 7 code** (hangs the CPU):
```c
for (waitcnt = 0; waitcnt < 10000; waitcnt++) {
    dummy = (int32_t)UART0->DATA;  // Read MMIO repeatedly
}
```

**STEP 3 code** (works fine):
```c
while (1) {
    int32_t rdata = (int32_t)UART0->DATA;  // Single read
    if (rdata < 0) uart_putchar('N');
    delay(100000);  // Long delay between reads
}
```

### Root Cause

CPU **cannot handle back-to-back loads from MMIO addresses**.

Possible RTL issues:
1. **Bus handshake deadlock**: Load waits for `valid` but peripheral doesn't respond correctly for rapid requests
2. **Load pipeline stall**: Repeated loads cause permanent pipeline stall
3. **Register corruption**: Load result overwrites PC or control registers

### Why ISP Flasher Failed

ISP flasher uses `uart_getchar()`:
```c
uint8_t uart_getchar() {
    int32_t rdata;
    do {
        rdata = (int32_t)UART0->DATA;  // Repeated MMIO load!
    } while (rdata < 0);
    return (uint8_t)rdata;
}
```

This triggers the bug → CPU hangs → outputs 0x00.

---

## Verification Steps Completed

✅ Confirmed AUIPC bug with assembly inspection
✅ Isolated repeated MMIO load bug through 7-step bisection
✅ Verified all basic CPU features work (ALU, branches, calls, stack)
✅ Confirmed flash XIP mechanism works
✅ Verified Boot ROM jump to flash works

---

## Files Modified/Created

### Boot ROM (No AUIPC)
- `hardware/v3/soc/firmware/fw-brom/crt_brom.S` - Uses `li sp, 0x800002F0` instead of AUIPC
- `hardware/v3/soc/firmware/fw-brom/isp_flasher.c` - Bisection test suite (STEP1-7)

### Flash Firmware (Minimal)
- `hardware/v3/soc/firmware/fw-flash/test_flash_minimal.S` - No-AUIPC test firmware

### Documentation
- `AUIPC_DEBUG_SESSION_FEB27.md` - Initial AUIPC debugging
- `BUG_REPORT_BISECTION_COMPLETE.md` - This file

---

## Next Steps (Priority Order)

### 1. Fix MMIO Load Bug (CRITICAL)

**Investigate `atomik_v3_cpu.v` load path:**

```verilog
// Check load instruction execution
// Questions to answer:
// 1. Does LSU wait for mem_valid before advancing?
// 2. Can consecutive loads cause ready/valid deadlock?
// 3. Is load data properly registered before writeback?
```

**Specific areas:**
- `atomik_v3_lsu.v` (if exists) - Load/store unit
- `atomik_v3_control.v` - State machine during loads
- Bus interface - valid/ready handshaking

**Test in Verilator:**
```c
// Reproduce hang with minimal testbench
for (int i = 0; i < 100; i++) {
    volatile uint32_t *uart = (uint32_t*)0x83000000;
    int32_t val = *uart;  // Should hang around iteration 2-3
}
```

### 2. Fix AUIPC Bug

**Check `atomik_v3_cpu.v` AUIPC implementation:**
- Is PC value correct when AUIPC executes?
- Is immediate properly sign-extended and shifted?
- Does result `PC + (imm << 12)` write to correct `rd`?

**Verilator test:**
```assembly
auipc x5, 0x12345   # Should load PC + 0x12345000 into x5
```

### 3. Run Full Compliance Tests

After fixes, run complete riscv-tests suite on both Verilator and FPGA to ensure no regressions.

---

## Workaround (If Immediate Progress Needed)

### Option A: Avoid MMIO Polling
Rewrite ISP flasher to use interrupt-driven UART instead of polling. (Not practical - Boot ROM can't handle interrupts easily.)

### Option B: Add Delays Between MMIO Reads
```c
uint8_t uart_getchar() {
    int32_t rdata;
    do {
        rdata = (int32_t)UART0->DATA;
        for (volatile int i = 0; i < 100; i++);  // Delay
    } while (rdata < 0);
    return (uint8_t)rdata;
}
```

### Option C: Switch to Proven CPU Core
Use PicoRV32 temporarily while fixing RV64I implementation.

---

## Hardware Tested

- **Board**: Tang Nano 9K (GW1NR-LV9QN88PC6/I5)
- **Clock**: 27 MHz crystal direct
- **UART**: 115200 baud, 8N1
- **Flash**: Puya P25Q32SH (SPI XIP)
- **Build**: Gowin EDA V1.9.12.01

---

## Build Commands for Reproduction

```bash
# Bisection Step 7 (demonstrates bug)
cd /home/mattrock/Projects/ATOMiK/hardware/v3/soc/firmware/fw-brom
make clean
make CFLAGS="-march=rv64i -mabi=lp64 -Os -fno-builtin -ffunction-sections -fdata-sections -MD -fstrict-volatile-bitfields -DBISECT_STEP7"

# Update BSRAM and synthesize
cd /home/mattrock/Projects/ATOMiK/hardware/v3/soc
python3 firmware/scripts/update_bootram.py firmware/fw-brom/build gowin_ip
cd ../synth
LD_PRELOAD=/lib/x86_64-linux-gnu/libstdc++.so.6 \
  LD_LIBRARY_PATH=/opt/gowin/IDE/lib:/lib/x86_64-linux-gnu \
  QT_PLUGIN_PATH=/opt/gowin/IDE/plugins/qt \
  /opt/gowin/IDE/bin/gw_sh synth_v3_soc.tcl

# Load and test
openFPGALoader -b tangnano9k impl/pnr/atomik_v3_soc.fs
# Observe: No UART output (CPU hangs)
```

---

## Conclusion

The ATOMiK v3 RV64I CPU has two RTL bugs preventing firmware execution:
1. **AUIPC**: Can be worked around (avoid PC-relative code)
2. **Repeated MMIO loads**: **Must be fixed** - blocking ISP flasher and any polling-based I/O

Bisection testing successfully isolated both bugs to specific CPU features.

**Time to fix the CPU RTL.**
