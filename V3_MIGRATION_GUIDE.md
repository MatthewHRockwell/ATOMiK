# ATOMiK v3 Migration Guide

**Date:** March 6, 2026
**Audience:** Firmware developers porting v2 (PicoRV32 + MMIO) code to v3 (custom RV64I + custom instructions)

---

## Overview

ATOMiK v3 replaces the v2 MMIO-based interface with custom RISC-V instructions wired directly into the CPU's execute stage. This guide covers the API changes, firmware porting steps, and architectural differences.

**Key changes:**
- MMIO register writes/reads → Custom instruction inline assembly
- 32-bit data width → 64-bit data width
- Dual-clock (25.2 MHz CPU + 81 MHz ATOMiK) → Single-clock (21.6 MHz)
- PicoRV32 (RV32I, pipelined) → atomik_v3_cpu (RV64I, multi-cycle)
- New `SWAP` operation (no v2 equivalent)

---

## API Migration

### v2 API (MMIO)

```c
#include "atomik.h"

// All operations go through memory-mapped registers at 0xC0000000
atomik_init(bank);                    // Soft reset via CONFIG register
atomik_load(bank, value);             // Write to LOAD register
atomik_accumulate(bank, delta);       // Write to ACCUM register
uint32_t state = atomik_state(bank);  // Read from STATE register
int unchanged = atomik_unchanged(bank); // Read STATUS register bit 0
```

### v3 API (Custom Instructions)

```c
#include "atomik_v3.h"

// All operations are custom RISC-V instructions (opcode 0x0B)
uint64_t z;
z = atomik_load(addr, init_state);    // atomik.load: set initial state for slot
z = atomik_accum(delta);              // atomik.accum: XOR delta into accumulator
uint64_t state = atomik_read(addr);   // atomik.read: reconstruct state
uint64_t old = atomik_swap(addr);     // atomik.swap: swap reference (new in v3)
```

### Side-by-Side Comparison

| Operation | v2 (MMIO) | v3 (Custom Instruction) |
|-----------|-----------|------------------------|
| **Initialize** | `atomik_init(bank)` | No equivalent needed (accumulator starts at zero) |
| **Load initial state** | `atomik_load(bank, value)` | `atomik_load(addr, value)` |
| **Accumulate delta** | `atomik_accumulate(bank, delta)` | `atomik_accum(delta)` |
| **Read current state** | `atomik_state(bank)` | `atomik_read(addr)` |
| **Check unchanged** | `atomik_unchanged(bank)` | Check return value of `atomik_accum()` (0 = accumulator zero) |
| **Read accumulator** | `atomik_get_delta(bank)` | No direct equivalent (use `atomik_read(addr) ^ initial`) |
| **Undo** | `atomik_undo(bank, delta)` | `atomik_accum(delta)` (same as accumulate — XOR is self-inverse) |
| **Swap reference** | N/A | `atomik_swap(addr)` — atomic reference update |
| **Fingerprint** | `atomik_fingerprint(bank, buf, n)` | Manual loop: load(0,0) + accum each word + read(0) |

### Key Differences

1. **No `bank` parameter**: v3 uses `addr` (8-bit slot address into a 256-entry state table). Banks are for parallel hardware instances, not software addressing.

2. **Return values**: All v3 instructions return a value in `rd`:
   - `atomik_load` and `atomik_accum` return the `acc_zero` flag (1 = accumulator is zero)
   - `atomik_read` returns the reconstructed state (initial ⊕ accumulator)
   - `atomik_swap` returns the previous current state before the swap

3. **64-bit data**: All values are `uint64_t`. When porting 32-bit code, either zero-extend or use the full 64-bit width.

4. **No soft reset register**: The accumulator is cleared by `atomik_load` (which sets initial state and resets the accumulator for that address slot).

---

## Tracked Memory Operations

### v2 (atomik_mem.c)

```c
// 32-bit word operations via MMIO
void atomik_memcpy_tracked(void *dst, const void *src, uint32_t len) {
    atomik_load(0, 0);  // Reset fingerprint
    uint32_t *d = (uint32_t*)dst;
    const uint32_t *s = (const uint32_t*)src;
    for (uint32_t i = 0; i < len/4; i++) {
        d[i] = s[i];
        atomik_accumulate(0, s[i]);  // MMIO write per word
    }
}
```

### v3 (atomik_mem.c)

```c
// 64-bit word operations via custom instructions
void atomik_memcpy_tracked(void *dst, const void *src, uint64_t len) {
    atomik_load(0, 0);  // Reset fingerprint
    uint64_t *d = (uint64_t*)dst;
    const uint64_t *s = (const uint64_t*)src;
    for (uint64_t i = 0; i < len/8; i++) {
        d[i] = s[i];
        atomik_accum(s[i]);  // Custom instruction per word
    }
}
```

**Performance impact:** The custom instruction eliminates the MMIO write cycle (bus arbitration + CDC handshake), making ATOMiK-tracked memcpy **84.5% faster** than software memcpy on v3 (vs 12% slower on v2).

---

## Clock Architecture

### v2 (Dual-Clock)

```
Crystal (27 MHz) → HDMI PLL → 126 MHz → CLKDIV ÷5 → 25.2 MHz (CPU)
                                 └─────────────────→ 126 MHz (HDMI serial)
Crystal (27 MHz) → ATOMiK PLL → 81 MHz (ATOMiK core)

CDC bridge between 25.2 MHz bus and 81 MHz ATOMiK domain
```

### v3 (Dual-PLL, Single CPU+ATOMiK Domain)

```
Crystal (27 MHz) → PLL1 → 108 MHz → CLKDIV ÷5 → 21.6 MHz (CPU + ATOMiK)
Crystal (27 MHz) → PLL2 → 126 MHz → CLKDIV ÷5 → 25.2 MHz (HDMI pixel)
                                 └──────────────→ 126 MHz (HDMI serial)

CDC bridge only for display MMIO (CPU → pixel domain)
```

**Key difference:** v3's CPU and ATOMiK share the same clock domain. No CDC needed for ATOMiK operations. The only CDC bridge is for the display pipeline MMIO (0xC0000000).

---

## Firmware Build Changes

### Toolchain

| Parameter | v2 | v3 |
|-----------|-----|-----|
| **Compiler** | `riscv64-unknown-elf-gcc` | `riscv64-unknown-elf-gcc` |
| **Architecture** | `-march=rv32i -mabi=ilp32` | `-march=rv64i -mabi=lp64` |
| **Optimization** | `-O3` | `-Os` |
| **Critical flag** | `-fno-builtin` | `-fno-builtin` |
| **Word size** | 32-bit (`uint32_t`) | 64-bit (`uint64_t`) |
| **Pointer size** | 4 bytes | 8 bytes |

### Linker Script Changes

v3 uses 64-bit addresses but the Tang Nano 9K memory map is unchanged:

```ld
/* v3 linker_flash.ld */
MEMORY {
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 8M
    SRAM  (rwx) : ORIGIN = 0x40000000, LENGTH = 8K
}
```

### Startup Code (crt_flash.S)

v3's `crt_flash.S` uses 64-bit registers and instructions:

```asm
# v2 (RV32I)
la gp, __global_pointer$
la sp, _stack_start
lw a0, 0(a1)      # 32-bit load

# v3 (RV64I)
la gp, __global_pointer$
la sp, _stack_start
ld a0, 0(a1)      # 64-bit load
```

---

## Common Porting Patterns

### Pattern 1: MMIO Register Access → Custom Instruction

```c
// v2: Memory-mapped volatile write
*(volatile uint32_t*)(0xC0000004) = delta;  // ACCUM register

// v3: Inline assembly custom instruction
uint64_t result;
__asm__ volatile (".insn r 0x0B, 1, 0, %0, %1, x0"
    : "=r"(result) : "r"(delta));
```

Use the `atomik_v3.h` wrappers instead of raw assembly.

### Pattern 2: Bank Selection → Address Slot

```c
// v2: Bank 0, bank 1, bank 2...
atomik_load(0, value);    // Bank 0
atomik_load(1, value);    // Bank 1

// v3: Address slots 0-255
atomik_load(0, value);    // Slot 0
atomik_load(1, value);    // Slot 1 (same state table, different address)
```

v3 uses a 256-entry BSRAM state table addressed by an 8-bit slot address. This is conceptually different from v2's bank parameter (which selected parallel hardware instances).

### Pattern 3: Soft Reset → Load with Zero

```c
// v2: CONFIG register bit 0
atomik_init(bank);  // Writes to CONFIG register

// v3: Load zero initial state (clears accumulator for that slot)
atomik_load(addr, 0);
```

### Pattern 4: Fingerprint Computation

```c
// v2
uint32_t fp = atomik_fingerprint(0, (uint32_t*)buf, len/4);

// v3 (manual — no convenience wrapper)
atomik_load(0, 0);
uint64_t *p = (uint64_t*)buf;
for (uint64_t i = 0; i < len/8; i++)
    atomik_accum(p[i]);
uint64_t fp = atomik_read(0);
```

### Pattern 5: Change Detection

```c
// v2
int changed = !atomik_unchanged(0);

// v3
uint64_t acc_zero = atomik_accum(0);  // Accumulate zero delta (no-op)
int changed = !acc_zero;               // Return value is acc_zero flag
```

---

## Performance Comparison

| Metric | v2 (PicoRV32 + MMIO) | v3 (Custom CPU + Instructions) |
|--------|---------------------|-------------------------------|
| **ATOMiK roundtrip** | 285 cycles | 160 cycles (**-44%**) |
| **memcpy overhead** | +12.4% slower | **-84.5% faster** (6.4x speedup) |
| **Change detection** | 5.1x faster than sw | **9.4x faster** than sw |
| **Determinism** | stdev ≤ 0.5 cycles | stdev = **0.0 cycles** |
| **Software memcpy** | 15,439 cycles | 87,003 cycles (+464%) |
| **Burst accum/op** | 165 cycles | 243 cycles (+47%) |

**Key insight:** v3's multi-cycle CPU makes software memory operations expensive (5 cycles per instruction vs ~1 CPI on PicoRV32). ATOMiK custom instructions offset this by executing in a single instruction cycle, inverting the cost equation: ATOMiK tracking goes from overhead to speedup.

---

## Checklist for Porting v2 Firmware to v3

- [ ] Replace `#include "atomik.h"` with `#include "atomik_v3.h"`
- [ ] Change all `uint32_t` ATOMiK values to `uint64_t`
- [ ] Replace `atomik_init(bank)` with `atomik_load(addr, 0)`
- [ ] Replace `atomik_accumulate(bank, delta)` with `atomik_accum(delta)`
- [ ] Replace `atomik_state(bank)` with `atomik_read(addr)`
- [ ] Replace `atomik_unchanged(bank)` with checking `atomik_accum(0)` return value
- [ ] Update Makefile: `-march=rv64i -mabi=lp64`
- [ ] Update `CLK_FREQ` to `21600000` (was `25200000`)
- [ ] Update tracked memory operations to use 64-bit word loops
- [ ] Consider using `atomik_swap()` for reference state updates (new capability)
- [ ] Test all firmware functions on hardware via UART

---

## Files Reference

| v2 File | v3 Equivalent | Notes |
|---------|---------------|-------|
| `hardware/picorv32/firmware/atomik.h` | `hardware/v3/soc/firmware/fw-flash/atomik_v3.h` | MMIO → custom instructions |
| `hardware/picorv32/firmware/atomik_mem.c` | `hardware/v3/soc/firmware/fw-flash/atomik_mem.c` | 32-bit → 64-bit loops |
| `hardware/picorv32/firmware/firmware.c` | `hardware/v3/soc/firmware/fw-flash/firmware.c` | Full test suite ported |
| `hardware/rtl/atomik_bus_wrapper.v` | N/A (not needed) | No bus wrapper in v3 |
| `hardware/rtl/atomik_core_v2.v` | `hardware/v3/rtl/atomik_v3_atomik.v` | 64-bit, BSRAM state table |
| `hardware/rtl/atomik_parallel_acc.v` | `hardware/v3/rtl/atomik_v3_parallel.v` | Shared BSRAM, 64-bit |

---

**For full deployment instructions, see [`docs/PRODUCTION_DEPLOYMENT.md`](PRODUCTION_DEPLOYMENT.md).**
**For known issues and troubleshooting, see [`docs/KNOWN_ISSUES.md`](KNOWN_ISSUES.md).**
