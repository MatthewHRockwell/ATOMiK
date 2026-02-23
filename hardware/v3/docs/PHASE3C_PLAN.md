# Phase 3C: Main Firmware Development - Implementation Plan

**Date:** February 23, 2026
**Status:** 🔄 **IN PROGRESS**

## Objective

Port the full v2 firmware to RV64I, providing complete feature parity with v2 PicoRV32 SoC:
- UART interactive menu
- All ATOMiK test suites (X, P, K, M, H)
- Boot ROM ISP flasher
- Performance benchmarking capability

## Prerequisites

✅ **Phase 3A Complete:** UART working (manual_uart_tx @ 115200 baud)
✅ **Phase 3B Complete:** Flash XIP validated (CPU executes from flash)

## Architecture

### Boot Flow

```
Power On
   ↓
CPU Reset → Boot ROM @ 0x80000000
   ↓
Wait for ISP sync (0x55) or timeout
   ↓
If timeout → Jump to Flash @ 0x00000000
   ↓
Flash Firmware:
   - crtStart → crtInit (startup code)
   - Data copy (flash → RAM)
   - BSS clear
   - Constructors
   - main() → UART menu
```

### Memory Map

| Address | Component | Size | Purpose |
|---------|-----------|------|---------|
| 0x00000000 | Flash (XIP) | 8 MB | Main firmware code |
| 0x40000000 | SRAM | 8 KB | Stack, heap, data |
| 0x80000000 | Boot ROM | 8 KB | ISP flasher |
| 0x81000000 | SPI Flash Config | - | Flash control registers |
| 0x82000000 | GPIO | - | LED control |
| 0x83000000 | UART | - | Console I/O |

## Implementation Tasks

### Task 1: Restore Boot ROM Boot Flow

**Current State:** CPU reset PC = 0x00000000 (direct flash boot for Phase 3B)
**Target State:** CPU reset PC = 0x80000000 (Boot ROM → Flash)

**Actions:**
1. Modify `soc/atomik_v3_soc.v`: Change reset PC back to 0x80000000
2. Rebuild Boot ROM with production timeout (370ms validated)
3. Update BSRAM IP with Boot ROM firmware
4. Resynthesize SoC

**Validation:**
- Boot sequence: `J<hex>!` diagnostic → Flash firmware banner
- ISP mode accessible via 0x55 sync

### Task 2: Port v2 Main Firmware Structure

**Source:** `hardware/picorv32/sw/firmware.c` (v2 reference)
**Target:** `hardware/v3/soc/firmware/fw-flash/firmware.c`

**Key Differences:**
- 64-bit architecture (RV64I vs RV32I)
- 64-bit print functions (16-digit hex, not 8-digit)
- ATOMiK access via custom instructions (not MMIO)
- Same UART/GPIO/Flash MMIO interface (32-bit registers)

**Components to Port:**

#### 2.1 UART Menu System
```c
void print_menu() {
    print("\n=== ATOMiK v3 SoC ===\n");
    print("X - ATOMiK XOR tests\n");
    print("P - Performance benchmark\n");
    print("K - Stack operations\n");
    print("M - Memory tests\n");
    print("H - Help\n");
}
```

#### 2.2 ATOMiK Custom Instruction Wrappers

**File:** `soc/firmware/fw-flash/atomik_v3.h`

Already exists with placeholders. Implement inline assembly:

```c
// ATOMIK.LOAD rd, rs1, rs2 (opcode 0x0B, funct3=0x0, funct7=0x00)
static inline void atomik_load(uint64_t addr, uint64_t ref_state) {
    __asm__ volatile (
        ".insn r 0x0B, 0x0, 0x00, x0, %0, %1"
        : /* no outputs */
        : "r"(addr), "r"(ref_state)
        : "memory"
    );
}

// ATOMIK.ACCUM rd, rs1, rs2 (funct3=0x1, funct7=0x00)
static inline void atomik_accum(uint64_t addr, uint64_t length) {
    __asm__ volatile (
        ".insn r 0x0B, 0x1, 0x00, x0, %0, %1"
        : /* no outputs */
        : "r"(addr), "r"(length)
        : "memory"
    );
}

// ATOMIK.READ rd, rs1, rs2 (funct3=0x2, funct7=0x00)
static inline uint64_t atomik_read(void) {
    uint64_t result;
    __asm__ volatile (
        ".insn r 0x0B, 0x2, 0x00, %0, x0, x0"
        : "=r"(result)
        : /* no inputs */
        : "memory"
    );
    return result;
}
```

#### 2.3 ATOMiK Test Suites

Port from v2:
- **X Tests:** XOR delta accumulation (single block, multiple blocks)
- **P Tests:** Performance benchmarking (ops/sec, memory bandwidth)
- **K Tests:** Stack delta tracking
- **M Tests:** Memory change detection
- **H Tests:** Hardware verification

**Adaptation:**
- Replace MMIO register writes with custom instruction calls
- Update print formatting for 64-bit values
- Keep test logic identical to v2

#### 2.4 Utility Functions

```c
// 64-bit hex print (16 digits)
void print_hex64(uint64_t v, int digits);

// 64-bit decimal print (powers-of-10 table, no multiply)
void print_dec64(uint64_t v);

// Cycle counter access
uint64_t get_cycles(void) {
    uint64_t cycles;
    __asm__ volatile ("rdcycle %0" : "=r"(cycles));
    return cycles;
}
```

### Task 3: Build System Integration

**Makefile updates:**
- Use existing `fw-flash/Makefile` (already configured for RV64I)
- Verify `-march=rv64i -mabi=lp64 -Os -fno-builtin`
- Ensure linker script uses `elf64-littleriscv`

**Build artifacts:**
```bash
cd hardware/v3/soc/firmware/fw-flash
make clean && make

# Expected outputs:
# - build/fw-flash.elf  (main firmware ELF)
# - build/fw-flash.hex  (Intel hex format)
# - build/fw-flash.v    (Verilog hex for ISP)
# - build/fw-flash.asm  (disassembly)
```

### Task 4: Hardware Deployment & Validation

**Deployment Steps:**
1. Synthesize SoC with Boot ROM boot flow
2. Flash bitstream to Tang Nano 9K
3. Program flash firmware via ISP (pico-programmer.py)
4. Verify UART menu appears
5. Test all menu options

**Validation Checklist:**
- [ ] Boot sequence: Boot ROM → Flash firmware
- [ ] UART menu displays correctly
- [ ] 'H' command shows help
- [ ] 'X' tests run and report results
- [ ] 'P' benchmarks execute
- [ ] All tests pass (same as v2 behavior)

### Task 5: ATOMiK Custom Instruction Validation

**Test Plan:**
1. Load reference state via ATOMIK.LOAD
2. Accumulate delta via ATOMIK.ACCUM
3. Read fingerprint via ATOMIK.READ
4. Compare result with expected value

**Expected Behavior:**
- Zero bus overhead (no CDC, no MMIO wait states)
- Same functional correctness as v2
- Performance improvement due to direct-wire integration

## Timeline Estimate

| Task | Estimated Time | Notes |
|------|----------------|-------|
| 1. Restore Boot ROM | 1 hour | Modify reset PC, rebuild, resynthesize |
| 2. Port firmware | 3-4 hours | UART menu, ATOMiK tests, utilities |
| 3. Build system | 30 min | Verify Makefile, build artifacts |
| 4. Hardware deploy | 1-2 hours | Synthesis, ISP, UART validation |
| 5. ATOMiK validation | 1-2 hours | Custom instruction tests |
| **Total** | **7-10 hours** | Single development session |

## Success Criteria

**Phase 3C Complete When:**
- ✅ Boot ROM → Flash boot flow working
- ✅ UART menu interactive and responsive
- ✅ All v2 ATOMiK tests ported and passing
- ✅ ATOMiK custom instructions functional
- ✅ Performance matches or exceeds v2

## Known Challenges

### Challenge 1: ATOMiK Custom Instruction Encoding
**Issue:** `.insn r` format must match CPU decode logic exactly.
**Mitigation:** Verify encoding against `atomik_v3_decode.v` before testing.

### Challenge 2: 64-bit Print Functions
**Issue:** No printf library support, manual decimal conversion needed.
**Mitigation:** Port v2's powers-of-10 table approach, extend to 64-bit.

### Challenge 3: ISP Timing
**Issue:** 370ms timeout window can be tight for manual ISP.
**Mitigation:** Use automated ISP script timing trick from Phase 3B troubleshooting.

## Reference Files

### v2 Reference (for porting)
- `hardware/picorv32/sw/firmware.c` - Main firmware
- `hardware/picorv32/sw/printf.c` - Print utilities
- `hardware/picorv32/sw/atomik_tests.c` - ATOMiK test suites

### v3 Target Files
- `hardware/v3/soc/firmware/fw-flash/firmware.c` - Main firmware
- `hardware/v3/soc/firmware/fw-flash/atomik_v3.h` - Custom instruction wrappers
- `hardware/v3/soc/firmware/fw-flash/crt_flash.S` - Startup code (already RV64I)
- `hardware/v3/soc/firmware/fw-flash/linker_flash.ld` - Linker script (already 64-bit)

### CPU Integration
- `hardware/v3/rtl/atomik_v3_decode.v` - Custom instruction decode logic
- `hardware/v3/rtl/atomik_v3_atomik.v` - ATOMiK datapath integration
- `hardware/v3/soc/atomik_v3_soc.v` - SoC top-level

---

**Phase 3C Status:** Ready to begin. All prerequisites met, clear path forward.

**Next Action:** Start with Task 1 (Restore Boot ROM boot flow), then proceed sequentially through firmware porting.
