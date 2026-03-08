# ATOMiK Zynq-7020 Port Architecture Specification

**Version**: 1.0.0
**Status**: PROPOSED
**Date**: March 7, 2026
**Target Board**: ALINX AX7020 (XC7Z020-2CLG400I)

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Platform Comparison](#2-platform-comparison)
3. [Architecture Overview](#3-architecture-overview)
4. [ATOMiK PL Architecture](#4-atomik-pl-architecture)
5. [Memory Map](#5-memory-map)
6. [Software Architecture](#6-software-architecture)
7. [Multi-Bank Scaling Plan](#7-multi-bank-scaling-plan)
8. [HDMI Strategy](#8-hdmi-strategy)
9. [External Validation](#9-external-validation)
10. [Validation Plan](#10-validation-plan)
11. [Risk Register](#11-risk-register)
12. [Non-Goals](#12-non-goals)

---

## 1. Executive Summary

ATOMiK is being ported from the Gowin GW1NR-9 (Tang Nano 9K) to the Xilinx Zynq-7020 (ALINX AX7020) as an AXI4-Lite peripheral in the Programmable Logic (PL), accessed from Linux userspace on the ARM Cortex-A9 Processing System (PS) via the UIO framework. No custom RISC-V CPU is ported -- the dual-core A9 at 667 MHz provides orders of magnitude more host compute than the current RV64I at 21.6 MHz. The 6x larger FPGA fabric (53,200 LUT6 vs. 8,640 LUT4) enables scaling from 1 bank to 256 parallel accumulator banks, a configuration impossible on the current platform, while Linux userspace access replaces bare-metal firmware with standard C libraries and Python bindings.

---

## 2. Platform Comparison

| Resource | Gowin GW1NR-9 (Tang Nano 9K) | Xilinx XC7Z020 (ALINX AX7020) | Ratio |
|----------|:---:|:---:|:---:|
| **LUT** | 8,640 LUT4 | 53,200 LUT6 | ~12x effective |
| **Flip-Flops** | 6,480 FF | 106,400 FF | 16x |
| **Block RAM** | 26 BSRAM (18Kb each) | 140 BRAM36 (36Kb each) | 10x capacity |
| **DSP** | 20 DSP | 220 DSP48E1 | 11x |
| **PLL/MMCM** | 2 PLL | 4 MMCM + 2 PLL | 3x |
| **Host CPU** | Custom RV64I @ 21.6 MHz | Dual Cortex-A9 @ 667 MHz | ~60x single-core |
| **Host OS** | Bare-metal (no OS) | Linux (PetaLinux / Ubuntu) | -- |
| **DRAM** | None (PSRAM only) | 1 GB DDR3 (32-bit) | -- |
| **ATOMiK Interface** | Custom instruction (1 cycle) | AXI4-Lite MMIO (~4-8 cycles) | -- |
| **Board Price** | $13.50 | ~$150 | 11x |
| **v3 SoC Utilization** | 6,287 LUT (73%) | -- (target: <10%) | -- |
| **ATOMiK N=1 Cost** | 477 LUT4 | ~120 LUT6 (estimate) | -- |
| **ATOMiK N=16 Cost** | 1,779 LUT4 | ~500 LUT6 (estimate) | -- |
| **Max ATOMiK Banks** | 1 (in SoC) | ~256 (estimate) | 256x |

**LUT equivalence note**: Xilinx LUT6 implements any 6-input function; Gowin LUT4 implements any 4-input function. A single LUT6 can replace 1-2 LUT4 depending on logic depth. The 64-bit XOR accumulator maps efficiently to LUT6 (one XOR per LUT6 vs. two levels in LUT4). Resource estimates for Zynq use LUT6 counts.

---

## 3. Architecture Overview

```
 ALINX AX7020 — ATOMiK SoC
 ================================================================

 ┌─── Processing System (PS) ────────────────────────────────────┐
 │                                                                │
 │  ┌──────────────┐  ┌──────────────┐                           │
 │  │ Cortex-A9 #0 │  │ Cortex-A9 #1 │  667 MHz, ARMv7-A        │
 │  └──────┬───────┘  └──────┬───────┘                           │
 │         │                 │                                    │
 │         └────────┬────────┘                                    │
 │                  │                                             │
 │         ┌────────▼────────┐  ┌────────────────┐               │
 │         │  L1/L2 Cache    │  │  DDR3 Controller│               │
 │         │  (32KB/512KB)   │  │  1 GB @ 533 MHz │               │
 │         └────────┬────────┘  └────────┬───────┘               │
 │                  │                    │                        │
 │         ┌────────▼────────────────────▼───────┐               │
 │         │         AXI Interconnect             │               │
 │         │  GP0 Master ──────────────────────┐  │               │
 │         └───────────────────────────────────┼──┘               │
 │                                             │                  │
 └─────────────────────────────────────────────┼──────────────────┘
                                               │
                       ┌───── AXI4-Lite (32-bit, GP0) ─────┐
                       │                                    │
 ┌─── Programmable Logic (PL) ─────────────────────────────────────┐
 │                     │                                            │
 │          ┌──────────▼──────────┐                                │
 │          │  AXI4-Lite Wrapper  │  FCLK0 domain (100 MHz)       │
 │          │  (atomik_axi_wrap)  │                                │
 │          └──────────┬──────────┘                                │
 │                     │                                           │
 │          ┌──────────▼──────────┐                                │
 │          │    CDC Bridge       │  Toggle-handshake              │
 │          │  (AXI → ATOMiK)    │  FCLK0 ↔ MMCM                 │
 │          └──────────┬──────────┘                                │
 │                     │                                           │
 │          ┌──────────▼──────────┐                                │
 │          │  atomik_v3_parallel │  ATOMiK clock domain           │
 │          │  N_BANKS = 1..256   │  (MMCM-generated, ~200 MHz)   │
 │          │                     │                                │
 │          │  ┌───────────────┐  │                                │
 │          │  │ Bank 0        │  │                                │
 │          │  │ Bank 1        │  │  256x64-bit state table       │
 │          │  │  ...          │  │  (BRAM36)                     │
 │          │  │ Bank N-1      │  │                                │
 │          │  └───────┬───────┘  │                                │
 │          │          │          │                                │
 │          │  ┌───────▼───────┐  │                                │
 │          │  │ XOR Merge Tree│  │  log2(N) depth                │
 │          │  └───────┬───────┘  │                                │
 │          │          │          │                                │
 │          │  ┌───────▼───────┐  │                                │
 │          │  │ State Recon.  │  │  current = init ^ merged_acc  │
 │          │  └───────────────┘  │                                │
 │          └─────────────────────┘                                │
 │                                                                 │
 └─────────────────────────────────────────────────────────────────┘
```

**Key architectural difference from v3 SoC**: On the Gowin platform, ATOMiK is wired directly into the CPU execute stage as a custom instruction (1-cycle latency). On Zynq, ATOMiK is a memory-mapped AXI4-Lite peripheral accessed over the GP0 port from Linux userspace. This adds latency (~4-8 AXI cycles + UIO overhead) but eliminates the need to port the custom RV64I CPU, provides Linux-grade tooling, and unlocks massive scaling headroom.

---

## 4. ATOMiK PL Architecture

### 4.1 AXI4-Lite Wrapper Design

The wrapper (`atomik_axi_wrap.v`) bridges the 32-bit AXI4-Lite GP0 bus to the ATOMiK core's 64-bit interface. The 32-bit bus requires a LO/HI register pair for 64-bit operations, with the HI write triggering the actual operation. This is the same pattern used in the v3 LSU 64-to-32 adapter.

**Operation sequencing:**

```
LOAD:   write LOAD_ADDR → write LOAD_DATA_LO → write LOAD_DATA_HI (triggers LOAD)
ACCUM:  write ACCUM_LO → write ACCUM_HI (triggers ACCUM, uses current active addr)
READ:   read STATE_LO → read STATE_HI (combinational from current_state output)
SWAP:   write SWAP_ADDR (triggers SWAP on active address)
```

**Write timing**: The HI write triggers the operation on the next clock edge. The wrapper holds `load_en` / `accum_en` / `swap_en` high for exactly one ATOMiK clock cycle via the CDC bridge.

**Read timing**: `STATE_LO` and `STATE_HI` are registered snapshots of `current_state`. A read of `STATE_LO` captures and latches the full 64-bit value, ensuring `STATE_HI` returns the upper half of the same sample. This prevents tearing between two AXI reads.

### 4.2 Multi-Bank Instantiation

The Zynq port instantiates `atomik_v3_parallel` with a parameterized `N_BANKS`. The module is unchanged from v3 -- only the synthesis attributes differ (section 4.4). The wrapper exposes `CONFIG.bank_select` for software-controlled bank targeting, and `STATUS.n_banks` for runtime discovery.

For N > 1, the parallel input mode (`delta_parallel_in`, `delta_parallel_valid`) is exposed via additional AXI registers in an extended register map (base + 0x100, one 64-bit register pair per bank). Single-bank sequential mode remains available via the standard ACCUM registers regardless of N.

### 4.3 BSRAM State Table Mapping to Xilinx BRAM36

The ATOMiK state table is 256x64-bit (16 Kbit). On Gowin, this maps to 2 SDPB blocks (256x32 low + 256x32 high). On Xilinx, it maps to a single BRAM36 configured as 512x36 or directly as 256x72 (using the extra parity bits for width). Vivado will infer this from the behavioral RTL when guided by `(* ram_style = "block" *)`.

| Platform | State Table Config | Blocks Used | Block Type |
|----------|-------------------|:-----------:|------------|
| Gowin GW1NR-9 | 2x SDPB (256x32) | 2 of 26 | BSRAM 18Kb |
| Xilinx XC7Z020 | 1x BRAM36 (256x72) | 1 of 140 | BRAM36 36Kb |

The Xilinx BRAM36 is true dual-port, enabling simultaneous read and write in a single cycle without the write-first / read-first conflict resolution needed on Gowin SDPB. This simplifies the SWAP operation timing.

### 4.4 Synthesis Attributes

Gowin and Xilinx use different synthesis pragmas. The RTL source will use `ifdef` guards or a separate Xilinx-specific wrapper.

| Gowin Attribute | Xilinx Equivalent | Purpose |
|----------------|-------------------|---------|
| `(* syn_keep = 1 *)` | `(* DONT_TOUCH = "yes" *)` | Prevent optimization of XOR merge tree |
| `(* syn_preserve = 1 *)` | `(* DONT_TOUCH = "yes" *)` | Prevent accumulator register merging |
| `(* syn_ramstyle = "block_ram" *)` | `(* ram_style = "block" *)` | Force state table into BRAM |

**Implementation approach**: A thin Xilinx wrapper file (`atomik_zynq_wrap.v`) will instantiate the core modules with Xilinx-specific attributes applied at the port level. The core ATOMiK RTL (`atomik_v3_atomik.v`, `atomik_v3_parallel.v`) remains unchanged, preserving single-source for both platforms.

### 4.5 Clock Strategy

```
 PS FCLK0 (100 MHz)                    MMCM (ATOMiK domain)
 ┌──────────────────┐                  ┌──────────────────────┐
 │ AXI4-Lite bus    │                  │ CLKOUT0: ~200 MHz    │
 │ AXI wrapper      │◄── CDC ────────►│ ATOMiK core          │
 │ UIO registers    │   bridge         │ (target Fmax)        │
 └──────────────────┘                  └──────────────────────┘
```

- **FCLK0** (PS-generated): 100 MHz default, drives the AXI interconnect and wrapper logic. Configurable via PS configuration wizard (50-250 MHz range).
- **MMCM** (PL-generated): Dedicated MMCM generates the ATOMiK core clock. Target frequency is ~200 MHz for N=1 (XC7Z020 -2 speed grade can sustain this for XOR-only logic). For large N, the merge tree depth increases and the target frequency will drop.
- **CDC bridge**: Toggle-handshake between FCLK0 and MMCM domains, same architecture as the v2 CDC bridge. The AXI wrapper completes the AXI transaction only after the CDC handshake confirms the ATOMiK operation has been committed.

**Frequency targets by bank count** (all ~estimates, pending synthesis):

| N_BANKS | Merge Tree Depth | Target ATOMiK Clock | Estimated Throughput |
|:-------:|:-----------------:|:-------------------:|:--------------------:|
| 1 | 0 | ~250 MHz | ~250 Mops/s |
| 16 | 4 | ~200 MHz | ~3,200 Mops/s |
| 64 | 6 | ~180 MHz | ~11,520 Mops/s |
| 256 | 8 | ~150 MHz | ~38,400 Mops/s |

### 4.6 CDC Between AXI and ATOMiK Domains

The CDC bridge uses a toggle-handshake protocol (proven in v2 production):

1. AXI write arrives in FCLK0 domain; wrapper latches data into holding registers.
2. Wrapper toggles a request signal.
3. CDC synchronizer (2-FF) detects the toggle in ATOMiK domain.
4. ATOMiK domain asserts `load_en` / `accum_en` / `swap_en` for one cycle.
5. ATOMiK domain toggles an acknowledge signal.
6. CDC synchronizer (2-FF) detects the ack in FCLK0 domain.
7. Wrapper completes the AXI BVALID/BREADY handshake.

**Latency**: 4 FCLK0 cycles + 4 ATOMiK cycles (worst case, two 2-FF synchronizers in each direction). For FCLK0=100 MHz and ATOMiK=200 MHz, this is ~60 ns per operation.

For reads, the `current_state` output is continuously valid (combinational from state table and accumulator). The CDC path captures it into FCLK0 domain registers on each read-side handshake.

---

## 5. Memory Map

### 5.1 Zynq Global Address Map

| Region | Address Range | Description |
|--------|--------------|-------------|
| PS DDR3 | `0x0000_0000` - `0x3FFF_FFFF` | 1 GB main memory (Linux) |
| PL GP0 | `0x4000_0000` - `0x7FFF_FFFF` | General-purpose AXI slaves |
| PL GP1 | `0x8000_0000` - `0xBFFF_FFFF` | Second GP port (unused) |
| PS peripherals | `0xE000_0000` - `0xFFFF_FFFF` | UART, SPI, GIC, etc. |

### 5.2 ATOMiK Register Map

**Base address**: `0x43C0_0000` (typical Vivado auto-assignment in GP0 range)

| Offset | Name | R/W | Width | Description |
|:------:|------|:---:|:-----:|-------------|
| `0x00` | `LOAD_ADDR` | W | 8 | Set active state table address [7:0] |
| `0x04` | `LOAD_DATA_LO` | W | 32 | Initial state bits [31:0], latched |
| `0x08` | `LOAD_DATA_HI` | W | 32 | Initial state bits [63:32], **triggers LOAD** |
| `0x0C` | `ACCUM_LO` | W | 32 | Delta bits [31:0], latched |
| `0x10` | `ACCUM_HI` | W | 32 | Delta bits [63:32], **triggers ACCUM** |
| `0x14` | `STATE_LO` | R | 32 | Current state bits [31:0] (snapshot on read) |
| `0x18` | `STATE_HI` | R | 32 | Current state bits [63:32] (same snapshot) |
| `0x1C` | `STATUS` | R | 32 | `{15'b0, acc_zero, n_banks[7:0], version[7:0]}` |
| `0x20` | `SWAP_ADDR` | W | 8 | Set swap address [7:0], **triggers SWAP** |
| `0x24` | `CONFIG` | R/W | 32 | `{22'b0, enable, bank_select[7:0]}` |

**STATUS register fields:**

| Bits | Field | Description |
|:----:|-------|-------------|
| [7:0] | `version` | Hardware version (0x10 = Zynq port v1.0) |
| [15:8] | `n_banks` | Number of instantiated banks (runtime discovery) |
| [16] | `acc_zero` | 1 when merged accumulator is all zeros |
| [31:17] | Reserved | Read as zero |

**CONFIG register fields:**

| Bits | Field | Description |
|:----:|-------|-------------|
| [7:0] | `bank_select` | Target bank for sequential ACCUM (round-robin if 0xFF) |
| [8] | `enable` | ATOMiK core enable (1 = active, 0 = clock-gated) |
| [31:9] | Reserved | Write as zero |

### 5.3 UIO Address Translation

From Linux userspace, the physical base address is accessed via UIO `mmap`:

```c
int fd = open("/dev/uio0", O_RDWR);
void *base = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
// base + 0x00 → LOAD_ADDR
// base + 0x10 → ACCUM_HI
// etc.
```

The 4 KB page mapping covers the entire register space with room for future expansion.

---

## 6. Software Architecture

### 6.1 Linux on PS

**Primary**: PetaLinux 2023.2+ (Xilinx-supported, BSP available for AX7020). Provides kernel, device tree, root filesystem, and cross-compilation toolchain.

**Secondary**: Ubuntu 22.04 daily image for Zynq (community-supported). Provides apt-get package management and standard userspace, useful for Python benchmarking.

**Kernel requirements**: UIO support (`CONFIG_UIO=y`, `CONFIG_UIO_PDRV_GENIRQ=y`). Both PetaLinux and Ubuntu kernels include this by default.

### 6.2 UIO Driver

UIO (Userspace I/O) maps PL peripheral registers directly into userspace virtual memory, bypassing the kernel for register access. No custom kernel module is needed.

**Device tree node:**

```dts
atomik@43c00000 {
    compatible = "generic-uio";
    reg = <0x43c00000 0x1000>;
    interrupt-parent = <&intc>;
    interrupts = <0 29 4>;  /* Optional: SPI #29, active-high level */
};
```

**Access pattern**: `open("/dev/uio0")` then `mmap` the register page. Reads and writes go directly to PL hardware with no system call overhead after the initial mmap.

**Interrupt support** (optional): The ATOMiK wrapper can assert an interrupt on `acc_zero` transitions or SWAP completion. UIO delivers this as a file descriptor event (`read()` on `/dev/uio0` blocks until interrupt, `write()` re-enables). This is a stretch goal -- polling is sufficient for bringup.

### 6.3 libatomik (C Library)

A minimal C library wrapping UIO access. Header-only candidate given the simplicity.

```c
/* libatomik.h — ATOMiK UIO userspace library */

#include <stdint.h>

typedef struct {
    volatile uint32_t *regs;   /* mmap'd register base */
    int fd;                     /* /dev/uioN file descriptor */
    uint8_t n_banks;            /* from STATUS register */
    uint8_t version;            /* from STATUS register */
} atomik_t;

/* Lifecycle */
atomik_t *atomik_open(const char *uio_dev);   /* "/dev/uio0" */
void      atomik_close(atomik_t *a);

/* Operations (match ATOMiK algebra) */
void     atomik_load(atomik_t *a, uint8_t addr, uint64_t initial_state);
void     atomik_accum(atomik_t *a, uint64_t delta);
uint64_t atomik_read(atomik_t *a);
void     atomik_swap(atomik_t *a, uint8_t addr);

/* Status */
int      atomik_acc_zero(atomik_t *a);
uint8_t  atomik_bank_count(atomik_t *a);
```

**Implementation sketch** for `atomik_accum`:

```c
void atomik_accum(atomik_t *a, uint64_t delta) {
    a->regs[ACCUM_LO / 4] = (uint32_t)(delta & 0xFFFFFFFF);
    a->regs[ACCUM_HI / 4] = (uint32_t)(delta >> 32);   /* triggers ACCUM */
}
```

The LO write is latched; the HI write triggers the operation. Two 32-bit MMIO writes per 64-bit accumulate.

### 6.4 Python Bindings

```python
# atomik_zynq.py — ctypes wrapper around libatomik

import ctypes, mmap, os

class ATOMiK:
    def __init__(self, uio_dev="/dev/uio0"):
        self.fd = os.open(uio_dev, os.O_RDWR)
        self.mm = mmap.mmap(self.fd, 0x1000, mmap.MAP_SHARED,
                            mmap.PROT_READ | mmap.PROT_WRITE)
        # Read STATUS for bank count / version
        status = self._read32(0x1C)
        self.version = status & 0xFF
        self.n_banks = (status >> 8) & 0xFF

    def load(self, addr: int, initial_state: int):
        self._write32(0x00, addr & 0xFF)
        self._write32(0x04, initial_state & 0xFFFFFFFF)
        self._write32(0x08, (initial_state >> 32) & 0xFFFFFFFF)

    def accum(self, delta: int):
        self._write32(0x0C, delta & 0xFFFFFFFF)
        self._write32(0x10, (delta >> 32) & 0xFFFFFFFF)

    def read(self) -> int:
        lo = self._read32(0x14)
        hi = self._read32(0x18)
        return (hi << 32) | lo

    def swap(self, addr: int):
        self._write32(0x20, addr & 0xFF)

    def _write32(self, offset, value):
        self.mm[offset:offset+4] = value.to_bytes(4, 'little')

    def _read32(self, offset) -> int:
        return int.from_bytes(self.mm[offset:offset+4], 'little')
```

### 6.5 Benchmark Harness

The Zynq benchmark harness reuses the same test vectors and validation criteria as the v3 Gowin platform, enabling direct cross-platform comparison.

**Test categories:**

| Test | Method | Gowin Equivalent |
|------|--------|------------------|
| LOAD/READ roundtrip | `load(0, X)` then `read()`, assert `== X` | ATOMIK.LOAD + ATOMIK.READ |
| ACCUM correctness | `load(0, A)`, `accum(D)`, assert `read() == A^D` | ATOMIK.ACCUM |
| SWAP correctness | Full SWAP sequence, verify ref update | ATOMIK.SWAP |
| Multi-address | Iterate 256 addresses, verify isolation | State table coverage |
| acc_zero detection | Load, verify `acc_zero==1`, accum, verify `acc_zero==0` | STATUS bit check |
| Burst throughput | N accumulates, measure wall-clock time | Cycle-count benchmark |
| Cross-platform parity | Run identical vector set on Gowin and Zynq | Bit-exact comparison |

**Timing methodology**: Linux `clock_gettime(CLOCK_MONOTONIC)` for wall-clock; ATOMiK-internal cycle counter (optional, via STATUS extension) for hardware cycles. Report both.

---

## 7. Multi-Bank Scaling Plan

### 7.1 Phased Rollout

| Phase | N_BANKS | Goal | Success Criteria |
|:-----:|:-------:|------|-----------------|
| 1 | 1 | Bringup | AXI read/write works from Linux, all functional tests pass |
| 2 | 16 | Match v2/v3 peak | ~3,200 Mops/s throughput, cross-platform parity with Gowin N=16 |
| 3 | 64 | Zynq-exclusive | ~11,520 Mops/s, XOR merge tree timing closure at 6 levels |
| 4 | 256 | Stress test | ~38,400 Mops/s, explore resource ceiling and thermal limits |

### 7.2 Resource Projections

All estimates are marked ~estimate. Per-bank cost on Xilinx is derived from Gowin measurements (65 LUT4 + 64 FF per bank), adjusted for LUT6 efficiency (~0.5x LUT count, 1x FF count). The AXI wrapper and CDC bridge are fixed overhead independent of N.

| N_BANKS | ATOMiK LUT6 | ATOMiK FF | BRAM36 | Total LUT6 (w/ wrapper) | XC7Z020 LUT6 % |
|:-------:|:-----------:|:---------:|:------:|:-----------------------:|:---------------:|
| 1 | ~35 | ~70 | 1 | ~250 | ~0.5% |
| 16 | ~560 | ~1,100 | 1 | ~775 | ~1.5% |
| 64 | ~2,250 | ~4,200 | 1 | ~2,465 | ~4.6% |
| 256 | ~9,000 | ~16,700 | 1 | ~9,215 | ~17.3% |

**Notes:**
- LUT6 per bank: ~35 (accumulator XOR + control) -- ~estimate
- FF per bank: ~66 (64-bit accumulator + 2 control) -- ~estimate
- XOR merge tree: adds ~32 LUT6 per doubling of N -- ~estimate
- AXI wrapper + CDC: ~215 LUT6 fixed overhead -- ~estimate
- State table: 1 BRAM36 regardless of N (256x64-bit shared)
- N=256 uses 17.3% of LUT6, leaving 83% for HDMI, debug, or future expansion

### 7.3 Throughput Scaling

```
Throughput (Mops/s, ~estimate)

  40,000 ┤                                                    ■ N=256
         │                                                   /
  30,000 ┤                                                  /
         │                                                 /
  20,000 ┤                                                /
         │                                              /
  10,000 ┤                                   ■ N=64   /
         │                                  /        /
   3,200 ┤                      ■ N=16    /        /
         │                     /        /        /
     864 ┤  ■ Gowin N=16     /        /        /      (Gowin peak for reference)
     250 ┤  ■ N=1          /        /        /
       0 ┤─────────────────────────────────────────
              1      16      64     256     Banks
```

Throughput scales linearly with N (each bank processes one delta per cycle). The bottleneck shifts from ATOMiK to AXI bus bandwidth at high N when using sequential mode; parallel input mode (direct PL-to-PL wiring) bypasses this.

---

## 8. HDMI Strategy

### 8.1 Strategy A: Linux Framebuffer via DRM/KMS (Primary)

The ALINX AX7020 has HDMI output. The simplest path is to use the Linux DRM/KMS subsystem with an HDMI transmitter IP in PL (Xilinx AXI VDMA + HDMI TX, or the Digilent open-source HDMI IP).

- **Pros**: Standard Linux display stack, works with Qt/GTK, trivial to display ATOMiK results
- **Cons**: No delta-driven display (traditional framebuffer), significant PL resource usage for video pipeline
- **Use case**: Development UI, benchmark dashboards, demonstration

### 8.2 Strategy B: PL-Driven HDMI with Delta Display (Stretch)

A delta-driven display pipeline in PL, similar to the v3 Gowin architecture. The ATOMiK accumulator output drives display updates -- only changed pixels are rewritten.

- **Pros**: Demonstrates delta-state display on Zynq, architectural showcase
- **Cons**: Significant RTL effort, duplicates v3 Gowin work, not required for benchmarking
- **Status**: Stretch goal, not on critical path

### 8.3 Recommendation

Start with Strategy A (Linux framebuffer). Use it for development and demonstration. Strategy B is only pursued if Strategy A is production-stable and there is specific need for a PL-driven delta display on this platform.

---

## 9. External Validation

### 9.1 USB-C Inline Power Sensor

A USB-C inline power meter (on order) measures board-level power consumption at the supply input. This provides total system power (PS + PL + DDR3 + peripherals).

**Measurement points:**
- Idle (Linux booted, ATOMiK clock-gated via CONFIG.enable=0)
- ATOMiK active, N=1, continuous ACCUM at max rate
- ATOMiK active, N=16, continuous ACCUM at max rate
- ATOMiK active, N=256, continuous ACCUM at max rate
- Delta: (active - idle) isolates ATOMiK switching power

**Comparison to Gowin**: The v3 Gowin platform measures 1.849 mW for ATOMiK at 21.6 MHz (0.085 nJ/cycle). The Zynq platform will have higher absolute power (larger die, DDR3, PS always-on) but the delta measurement isolates ATOMiK PL power for comparison.

### 9.2 Thermocouple

A thermocouple (on order) will be affixed to the Zynq die or heatsink to measure thermal response under sustained ATOMiK workloads.

**Measurements:**
- Ambient temperature
- Idle die temperature (Linux booted, no ATOMiK activity)
- Sustained load die temperature (N=256, continuous ACCUM, 10 minutes)
- Thermal time constant (time from idle to steady-state under load)

### 9.3 Measurement Methodology

All power and thermal measurements will follow this protocol:

1. Record ambient temperature at start
2. Boot Linux, wait 60 seconds for thermal stabilization
3. Record idle baseline (power and temperature)
4. Start ATOMiK workload, record at 1-second intervals for 600 seconds
5. Stop workload, record cooldown at 1-second intervals for 300 seconds
6. All data logged to JSON with timestamps, matching the v3 `perf_pool.jsonl` format

---

## 10. Validation Plan

### 10.1 Unit: AXI4-Lite Protocol Simulation

- Vivado IP Integrator block design with AXI VIP (Verification IP) as master
- Stimulus: write all registers, read back, verify timing and protocol compliance
- Tool: Vivado Simulator or Verilator (AXI VIP requires Vivado Simulator)
- Pass criteria: zero protocol violations, all BRESP/RRESP = OKAY

### 10.2 Integration: Linux UIO Read/Write

- Boot PetaLinux on hardware
- Run `libatomik` test suite from userspace
- Verify all ATOMiK operations (LOAD, ACCUM, READ, SWAP) through the UIO path
- Pass criteria: all functional tests pass, bit-exact match with expected values

### 10.3 Performance: Cycle-Accurate Benchmarks

- Measure per-operation latency (LOAD, ACCUM, READ, SWAP) via `clock_gettime`
- Measure burst throughput (1K, 10K, 100K accumulates)
- Compare to Gowin v3 measurements (accounting for clock frequency differences)
- Report in same format as v3 `perf_pool.jsonl`

### 10.4 Cross-Platform: Same Tests on Gowin and Zynq

| Validation Vector | Gowin (v3) | Zynq | Comparison |
|-------------------|:----------:|:----:|:----------:|
| LOAD/READ 256 addresses | PASS/FAIL | PASS/FAIL | Bit-exact |
| ACCUM 1000 random deltas | PASS/FAIL | PASS/FAIL | Bit-exact |
| SWAP full cycle | PASS/FAIL | PASS/FAIL | Bit-exact |
| acc_zero transitions | PASS/FAIL | PASS/FAIL | Bit-exact |
| Burst 100K ACCUM throughput | X Mops/s | Y Mops/s | Report both |
| Power per operation | X nJ/op | Y nJ/op | Report both |

The ATOMiK algebra guarantees bit-exact results regardless of platform. Any divergence between Gowin and Zynq indicates a hardware bug, not an algorithmic difference.

---

## 11. Risk Register

| # | Risk | Impact | Likelihood | Mitigation |
|:-:|------|:------:|:----------:|------------|
| 1 | AXI4-Lite timing violation at high FCLK0 | ATOMiK operations corrupt data | Low | Start at 100 MHz FCLK0 (conservative); increase only after timing closure verified |
| 2 | CDC metastability between FCLK0 and MMCM domains | Spurious operations or missed data | Low | Proven 2-FF synchronizer design from v2 production; add `ASYNC_REG` constraints |
| 3 | BRAM36 inference fails (Vivado distributes to LUTs) | State table consumes LUT instead of BRAM | Medium | Explicit `(* ram_style = "block" *)` attribute; verify in post-synthesis utilization report |
| 4 | XOR merge tree timing at N=256 (8 levels) | ATOMiK Fmax drops below useful threshold | Medium | Pipelined merge tree (1 register stage per 4 XOR levels); accept reduced Fmax for stress test |
| 5 | UIO mmap latency higher than expected | Benchmark throughput disappointing | Medium | This is expected -- UIO adds ~100-200 ns per access vs. custom instruction. Focus on throughput via burst mode and parallel banks |
| 6 | PetaLinux BSP not available for ALINX AX7020 | Extended bringup time for device tree and boot | Medium | ALINX provides Vivado projects and Linux images; fallback to manual device tree based on XC7Z020 reference |
| 7 | USB-C power sensor insufficient resolution | Cannot isolate ATOMiK power delta | Low | Most inline sensors resolve to 1 mW; ATOMiK delta at N=256 should be 50-500 mW, well above noise floor |
| 8 | Zynq die thermal limits at sustained N=256 | Thermal throttling or shutdown | Low | XC7Z020 Tj max = 100C (industrial grade); heatsink on AX7020 is adequate for ~2-3W PL power |

---

## 12. Non-Goals

The following are explicitly out of scope for this port:

- **No RV64I CPU port**: The custom RV64I CPU (`atomik_v3_cpu`) is not ported to Zynq. The dual Cortex-A9 at 667 MHz running Linux provides far more host compute capability than needed. ATOMiK is accessed as a peripheral, not as a custom instruction.

- **No Tang Nano 9K replacement**: The Gowin platform continues in parallel. The Zynq port is an additional platform for scaling experiments and Linux integration, not a replacement. Both platforms share the same ATOMiK RTL core.

- **No ATOMiK algebra changes**: The delta-state algebra (XOR accumulator, state reconstruction, SWAP) is unchanged. All 92 Lean4 theorems apply to both platforms. The reconstruction equation remains `current_state = initial_state XOR accumulator`.

- **No custom kernel module**: UIO provides sufficient access for all ATOMiK operations. A custom kernel driver would add maintenance burden without measurable benefit for the current use case. If interrupt-driven operation becomes necessary, UIO's built-in interrupt support is used first.

- **No high-performance AXI**: The GP0 port (32-bit, ~100 MHz) is used rather than the HP ports (64-bit, higher bandwidth). ATOMiK operations are register-level MMIO, not bulk DMA transfers. The GP0 bandwidth (~400 MB/s) far exceeds the register access rate.

- **No multi-die or multi-board**: This spec covers a single XC7Z020 on a single AX7020 board. Multi-FPGA scaling is a future consideration.
