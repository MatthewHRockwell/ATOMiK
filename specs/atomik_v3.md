# ATOMiK v3 Architecture Specification

**Version**: 3.0.1
**Status**: PROPOSED
**Date**: February 16, 2026

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Design Philosophy](#2-design-philosophy)
3. [v2 → v3 Delta](#3-v2--v3-delta)
4. [ATOMiK-Aligned RV64I Core](#4-atomik-aligned-rv64i-core)
5. [Core Datapath Architecture](#5-core-datapath-architecture)
6. [Display-Driven Architecture](#6-display-driven-architecture)
7. [I/O Architecture](#7-io-architecture)
8. [Program Model](#8-program-model)
9. [AI-Aligned Architecture](#9-ai-aligned-architecture)
10. [Resource Projections](#10-resource-projections)
11. [Relationship to v2](#11-relationship-to-v2)
12. [Design Decisions](#12-design-decisions)
13. [Clock and PLL Strategy](#13-clock-and-pll-strategy)

---

## 1. Executive Summary

**Hardware shaped by what changes, not by what exists.**

ATOMiK v3 is a complete SoC architecture built around the delta-state algebra: a custom RV64I control plane, a BSRAM-backed delta-state accelerator, and a display pipeline driven entirely by change. The target is the same GW1NR-9 FPGA (Tang Nano 9K, $13.50), but v3 achieves a smaller LUT footprint than v2, a wider datapath (64-bit native), and fundamentally more capability.

Where v2 proved that delta-state acceleration works on an FPGA, v3 asks: what does a computer look like when change — not state — is the primitive?

**Key numbers:**

| Metric | v2 (Deployed) | v3 (Proposed) |
|--------|:---:|:---:|
| CPU | PicoRV32 (32-bit) | Custom RV64I (64-bit) |
| ATOMiK ↔ CPU latency | 3 cycles (CDC) | 1 cycle (direct wire) |
| Reference states | 1 (register) | 576 (BSRAM table) |
| CLS per bit | ~1.7 | 1.0 |
| SoC LUT4 | 3,838 (44%) | ~3,100 (36%) |
| Datapath width | 32-bit | 64-bit (parameterizable) |

The algebra is unchanged. All 92 Lean4 theorems apply without modification. The reconstruction equation remains:

```
current_state = initial_state ⊕ accumulator
```

---

## 2. Design Philosophy

### 2.1 Change-Driven vs. State-Driven

Traditional architectures store state and compute transitions. ATOMiK stores transitions and reconstructs state.

The identity property of XOR (`δ = 0 → state unchanged`) means **static = zero cost**. When nothing changes, the hardware does nothing: no switching activity, no bus traffic, no power consumed. Work scales with change, not existence.

### 2.2 Division of Labor

The CPU handles the **static**: program counter, branching, I/O polling, address arithmetic — operations that rarely change the delta-state.

ATOMiK handles the **dynamic**: state accumulation, display updates, change detection, context switching — operations that are inherently about what changed.

### 2.3 No Translation Layer

In v2, the CPU communicates with ATOMiK through a memory-mapped I/O bridge with clock domain crossing — 3 cycles of latency and a protocol boundary between action and state change.

In v3, the CPU has native ATOMiK instructions. A delta injection is a single instruction, a single cycle, a single wire. The action *is* the state change.

---

## 3. v2 → v3 Delta

### 3.1 What v2 Proved

- Delta-state acceleration on FPGA is feasible and efficient
- Single-cycle XOR accumulation at 81 MHz on GW1NR-9
- 76–80% faster change detection vs. software memcmp
- Deterministic latency (stdev ≤ 0.5 cycles across all operations)
- Sub-linear LUT scaling for parallel banks (3.7x growth for 16x throughput)
- Production-grade timing closure (zero TNS, +23% ATOMiK margin)

### 3.2 What v3 Changes

| Component | v2 | v3 | Why |
|-----------|----|----|-----|
| CPU | PicoRV32 (RV32I, borrowed) | Custom RV64I (ATOMiK-aligned) | Native 64-bit, custom delta instructions, eliminates CDC latency |
| Initial state storage | Registers (32 FF per bank) | BSRAM (576 entries, shared) | 50% register savings, hundreds of reference states, instant context switching |
| CLS mapping | ~1.7 CLS/bit, 50% LUT waste | 1.0 CLS/bit, 100% utilization | 41% CLS reduction, free mask/detect inputs |
| CLS3 | Unused | Scanline delta mask + color index shift registers | Display integration at zero additional LUT cost |
| Display pipeline | Separate from ATOMiK | Change-driven (pixel velocity model) | Static pixels = zero cost, temporal delta compression |
| Program model | External stimulus via MMIO | Delta injection streams, hardware-native recording | Programs as accumulator sequences |
| I/O (inter-board) | USB serial (~12 Mbit/s) | IDES16/OSER16 (162 Mbit/s per LVDS pair) | 13.5x bandwidth for multi-node delta sync |

### 3.3 What Stays

- The algebra: Abelian group under XOR composition (commutative, associative, self-inverse, identity)
- The formal model: 92 Lean4 theorems in `math/proofs/ATOMiK/`
- The reconstruction equation: `current_state = initial_state ⊕ accumulator`
- The target device: GW1NR-LV9QN88PC6/I5 (Tang Nano 9K)

---

## 4. ATOMiK-Aligned RV64I Core

### 4.1 Why Custom

PicoRV32 is a 32-bit-only core designed for general-purpose soft CPU applications. It cannot be extended to 64-bit without a complete rewrite, and it has no mechanism for custom instructions that bypass the bus.

Existing open-source RV64I cores (e.g., VexiiRiscv at ~3,400 LUT) are designed for performance — pipelines, caches, branch prediction — features that increase area without benefiting ATOMiK's use case.

ATOMiK needs a CPU that:
- Provides 64-bit registers for native-width delta operations
- Executes custom ATOMiK instructions in 1 cycle with direct wiring (no bus, no CDC)
- Fits in ~2,000 LUT4 on GW1NR-9
- Runs standard `riscv64-unknown-elf-gcc` compiled code

### 4.2 Base ISA

RV64I: the 64-bit integer base instruction set (47 instructions). No M (multiply/divide), C (compressed), or F (floating-point) extensions initially.

This is the minimum viable RISC-V 64-bit core. Standard GCC cross-compilation with `-march=rv64i -mabi=lp64` produces compatible binaries. Multiply and divide are handled in software (as with the current RV32I PicoRV32 configuration).

### 4.3 Custom ATOMiK Instructions

RISC-V reserves four custom opcode ranges (`custom-0` through `custom-3`) for ISA extensions. ATOMiK v3.0 uses `custom-0` (opcode `0001011`) for four core delta-state operations:

#### v3.0 Core Instructions

| Instruction | funct3 | funct7 | Semantics | Latency |
|-------------|:------:|:------:|-----------|:-------:|
| `ATOMIK.LOAD rs1` | `0x0` | `0x00` | `initial_state ← BSRAM[rs1[8:0]]` | 1 cycle |
| `ATOMIK.ACCUM rs1` | `0x1` | `0x00` | `accumulator ← accumulator ⊕ rs1` | 1 cycle |
| `ATOMIK.READ rd` | `0x2` | `0x00` | `rd ← initial_state ⊕ accumulator` | 1 cycle |
| `ATOMIK.SWAP rs1` | `0x3` | `0x00` | `bsram_addr ← rs1[8:0]` (context switch) | 1 cycle |

**Encoding format** (R-type, 32 bits):

```
[31:25]  [24:20]  [19:15]  [14:12]  [11:7]   [6:0]
funct7   rs2      rs1      funct3   rd       opcode
0000000  00000    src      000-011  dest     0001011 (CUSTOM_0)
```

- `ATOMIK.LOAD`: funct3=000, rs1=BSRAM address register, rd=x0, rs2=x0
- `ATOMIK.ACCUM`: funct3=001, rs1=delta value register, rd=x0, rs2=x0
- `ATOMIK.READ`: funct3=010, rs1=x0, rd=destination register, rs2=x0
- `ATOMIK.SWAP`: funct3=011, rs1=BSRAM address register, rd=x0, rs2=x0

#### v3.1 Planned Extensions

| Instruction | funct3 | Semantics | Status |
|-------------|:------:|-----------|--------|
| `ATOMIK.MASK rs1` | `0x4` | `mask_register ← rs1` | Deferred — requires CLS LUT4[1] I2/I3 wiring validated on silicon |
| `ATOMIK.DETECT rd` | `0x5` | `rd ← change_detection_flags` | Deferred — depends on MASK infrastructure |

MASK and DETECT exploit the two free LUT4 inputs on the reconstruction XOR (Section 5.2). They are architecturally sound but add decode complexity without immediate v3.0 use cases. Deferring to v3.1 keeps the initial custom decode minimal and testable.

**Key difference from v2**: These instructions bypass the bus entirely. The ATOMiK accumulator, BSRAM state table, and reconstruction logic are wired directly into the CPU's register file read/write paths. No bus arbitration, no CDC bridge, no protocol overhead.

**GCC integration**: Custom instructions are invoked via inline assembly using the `.insn` directive:

```c
// Inline assembly for ATOMIK.ACCUM (funct3=0x1, funct7=0x00)
static inline void atomik_accum(uint64_t delta) {
    asm volatile (".insn r 0x0B, 0x1, 0x00, x0, %0, x0" : : "r"(delta));
}

// Inline assembly for ATOMIK.READ (funct3=0x2, funct7=0x00)
static inline uint64_t atomik_read(void) {
    uint64_t result;
    asm volatile (".insn r 0x0B, 0x2, 0x00, %0, x0, x0" : "=r"(result));
    return result;
}

// Inline assembly for ATOMIK.LOAD (funct3=0x0, funct7=0x00)
static inline void atomik_load(uint64_t bsram_addr) {
    asm volatile (".insn r 0x0B, 0x0, 0x00, x0, %0, x0" : : "r"(bsram_addr));
}

// Inline assembly for ATOMIK.SWAP (funct3=0x3, funct7=0x00)
static inline void atomik_swap(uint64_t bsram_addr) {
    asm volatile (".insn r 0x0B, 0x3, 0x00, x0, %0, x0" : : "r"(bsram_addr));
}
```

### 4.4 Microarchitecture

Multi-cycle (no pipeline). Each instruction takes 3–5 cycles depending on type:

```
Fetch:     1 cycle (SPI flash read via XIP adapter)
Decode:    1 cycle (instruction decode + register read from BSRAM)
Execute:   1 cycle (ALU/branch/ATOMiK operation)
Memory:    1 cycle (load/store only)
Writeback: 1 cycle (register write to BSRAM)
```

ATOMiK custom instructions skip the Memory stage — the Execute stage directly accesses the accumulator/BSRAM state table through internal wiring.

**Register file**: 32 × 64-bit registers stored in **1 BSRAM block** (true dual-port, 288×64-bit configuration). BSRAM provides simultaneous read of RS1 and RS2 via its two independent read ports, avoiding the need to split register reads across multiple cycles. The 1-cycle BSRAM read latency is hidden within the Decode stage — register values are available at the start of Execute with zero pipeline bubble.

**Why BSRAM over distributed registers**: A 32×64-bit register file in distributed logic would consume ~200 LUT4 (32 entries × 64 bits × dual read mux). One BSRAM block (of 26 available) provides the same functionality at zero LUT cost. The 1-cycle read latency is a non-issue because the multi-cycle microarchitecture already dedicates an entire cycle to Decode.

**Estimated footprint**: 1,500–2,500 LUT4, depending on decode complexity. Register file is in BSRAM (0 LUT).

### 4.5 Resource Comparison

| Core | ISA | LUT4 | Features | ATOMiK Integration |
|------|-----|-----:|----------|-------------------|
| PicoRV32 (current) | RV32I | ~1,800 | Proven, production | MMIO via CDC bridge (3 cycles) |
| VexiiRiscv | RV64IMC | ~3,400 | Pipeline, caches | Would require bus wrapper |
| Custom RV64I (proposed) | RV64I + custom | ~2,000 | Multi-cycle, minimal | Direct wire (1 cycle) |

### 4.6 Bus Architecture

Internal: 64-bit data bus between CPU registers and ATOMiK.

External: 32-bit SPI flash adapter for instruction fetch. The CPU executes from external SPI flash via XIP (execute-in-place), same as the current PicoRV32 SoC. The 64→32 width adapter lives **inside the CPU's load/store unit**, keeping the peripheral bus simple and unchanged:

```
CPU (64-bit internal) ──► 64→32 adapter ──► SPI Flash (32-bit, XIP)
      (load/store        (inside CPU)
       unit)         ──► 64-bit direct ──► ATOMiK (BSRAM + accumulator)
                     ──► 32-bit bus    ──► UART, GPIO, HDMI
```

The adapter splits 64-bit loads/stores into two 32-bit bus transactions (lower word first, then upper word). This adds 1 cycle of latency for 64-bit peripheral access but keeps the external bus at 32 bits — matching v2's proven peripheral infrastructure and avoiding re-validation of UART, GPIO, and HDMI interfaces.

---

## 5. Core Datapath Architecture

### 5.1 BSRAM State Tables

In v2, `initial_state` is stored in flip-flops — 32 or 64 bits per bank, one reference state per bank, updated via MMIO write. This is simple but wasteful: the initial state is read-only during normal operation, and flip-flops are a premium resource on GW1NR-9.

In v3, `initial_state` moves to BSRAM:

```
┌───────────────────────────────────────────────────┐
│  BSRAM State Table (1 block = 18 Kbit)            │
│                                                   │
│  Configuration: 576 × 32-bit or 288 × 64-bit     │
│                                                   │
│  ┌──────────────────┐                             │
│  │ Addr  Content    │                             │
│  │ ┌──────────────┐ │                             │
│  │ │[0]: OS idle  │─┼──► state_read[DW-1:0]      │
│  │ │[1]: Editor   │ │     (sync read, 1 cycle)    │
│  │ │[2]: Shell    │ │                             │
│  │ │[3]: Game     │ │    Can hold:                │
│  │ │[4]: Media    │ │    - Program contexts       │
│  │ │...           │ │    - Display references     │
│  │ │[N]: Custom   │ │    - Checkpoint states      │
│  │ └──────────────┘ │    - Machine states         │
│  └──────────────────┘                             │
│                                                   │
│  Access Pattern:                                  │
│  - WRITTEN: only on context init (rare)           │
│  - READ: synchronous (1-cycle latency for BSRAM)  │
│  - NEVER XOR'd: it's the reference, not a delta  │
│                                                   │
│  Result:                                          │
│  → Saves 32/64 flip-flops per bank               │
│  → Enables hundreds of reference states           │
│  → Atomic context switch (change address pointer) │
│  → 1 BSRAM block (of 26 available) = 14–15/26    │
└───────────────────────────────────────────────────┘
```

**Impact on bank scaling**: In v2, each additional bank costs ~65 LUT + 64 FF. In v3, each additional bank costs ~45 LUT + 32 FF (accumulator only — initial state is shared via BSRAM). This is a 42% CLS reduction per bank.

### 5.2 Optimized CLS Mapping

Gowin's CLS (Configurable Logic Slice) contains 2 LUT4 + 2 REG. In v2, the accumulator and initial state registers map to CLS at ~1.7 CLS per datapath bit, with 50% of LUT4 resources unused and CLS3 entirely idle.

v3 achieves 1.0 CLS per datapath bit with 100% utilization:

```
              ATOMiK v3 — Gowin GW1NR-9 CLS-Level Mapping
              Per-Bit Datapath (repeat DW times)

    ┌──── CLS (1 per datapath bit) ──────────────────────────┐
    │                                                        │
    │  ┌────────────────────┐  ┌───────────┐                 │
    │  │ LUT4[0]            │  │  REG[0]   │                 │
    │  │                    │  │           │                 │
    │  │ I0: acc[i]     ◄─┐├─►│ D     Q  ├──► acc[i]       │
    │  │ I1: delta_in[i]  │ │  │ CE: 1    │      │          │
    │  │ I2: acc_en       │ │  │ CLK,RST  │      │          │
    │  │ I3: load_clr     │ │  └───────────┘      │          │
    │  │                  │ │                      │          │
    │  │ f = ld ? 0       │ │    feedback ─────────┘          │
    │  │   : en ? acc⊕δ   │ │                                 │
    │  │   : acc  (hold)  │ │                                 │
    │  └────────────────────┘                                 │
    │                                                        │
    │  ┌────────────────────┐  ┌───────────┐                 │
    │  │ LUT4[1]            │  │  REG[1]   │                 │
    │  │                    │  │           │                 │
    │  │ I0: init[i] (S₀)  │  │ D     Q  ├──► dout[i]      │
    │  │ I1: acc[i]  (Σδ)  │  │ CE: rd_en │                 │
    │  │ I2: mask_bit[i]   │  │ CLK,RST   │                 │
    │  │ I3: (detect ctrl) │  └───────────┘                 │
    │  │                    │                                 │
    │  │ f = (S₀ ⊕ Σδ)     │  REG[1] captures reconstructed │
    │  │   & ~mask_bit      │  state only when read_en fires │
    │  │                    │                                 │
    │  └────────────────────┘                                 │
    │                                                        │
    └────────────────────────────────────────────────────────┘

    Per-Bit Resource Summary:
    ├─ LUT4[0]: accumulator XOR + conditional clear
    │           (4 inputs: acc feedback, delta_in, acc_en, load_clr)
    ├─ LUT4[1]: reconstruction XOR + 2 free inputs for mask/detect
    │           (4 inputs: initial_state, accumulator, mask, detect_ctrl)
    ├─ REG[0]:  accumulator flip-flop (feedback loop)
    └─ REG[1]:  output capture register (CE = read_en)

    Efficiency:
    - v2: ~1.7 CLS/bit → v3: 1.0 CLS/bit (41% reduction)
    - v2: 50% LUT waste → v3: 0% LUT waste
    - v2: no mask/detect → v3: hardware mask + change detect (free)
```

**LUT4[1] free inputs**: The reconstruction XOR (`initial_state ⊕ accumulator`) uses only 2 of LUT4's 4 inputs. The remaining 2 inputs absorb bit-level masking and change detection at zero additional LUT cost. This is a feature that would require separate logic in v2.

### 5.3 CLS3 SREG Utilization

Each CFU (Configurable Function Unit) on GW1NR-9 contains 4 CLS blocks. CLS3 has a special capability: its REGs can be configured as shift registers (SREG) with serial input/output.

In v2, CLS3 is entirely unused — it exists physically but contributes nothing.

v3 uses CLS3 SREG for display integration:

| SREG | Function | Description |
|------|----------|-------------|
| REG[0] | Scanline delta mask | 1-bit-per-pixel shift register: "did this pixel change?" Gates per-pixel update during HDMI scanout. |
| REG[1] | Color delta index | 8-bit index stream into delta color LUT. Compact encoding of pixel color transitions. |

Additionally, the SREG chain provides a debug scan capability: the entire ATOMiK state can be shifted out serially for verification without adding dedicated debug ports.

**Clock domain crossing**: CLS3 SREG input is clocked in the ATOMiK domain (81 MHz). SREG output feeds the HDMI pixel pipeline (25.2 MHz). Since this is a faster→slower crossing (3.2:1 ratio), a **registered handshake** is sufficient — the ATOMiK domain registers output data, asserts a valid flag, and the pixel clock domain captures it on the next rising edge. No dual-clock FIFO is needed because the consumer (pixel clock) is always slower than the producer (ATOMiK clock), so data is guaranteed stable before capture. This is the simplest viable CDC for this path.

### 5.4 Parameterized Width

The `DW` parameter controls the datapath width:

| Configuration | DW | CLS per bank | Use Case |
|:---:|:---:|:---:|---|
| Production (current) | 32 | 32 | Tang Nano 9K, area-constrained |
| Standalone | 64 | 64 | Full-width delta operations, native RV64I match |

The custom RV64I core enables native 64-bit delta operations. When `DW=64`, a single `ATOMIK.ACCUM` instruction injects a full 64-bit delta in one cycle — no split operations, no multi-cycle MMIO sequences.

---

## 6. Display-Driven Architecture

### 6.1 Pixel Velocity Model

Traditional display architectures answer: "What color is pixel[x,y]?" This requires a full framebuffer refresh every frame, even when 95% of pixels are static.

ATOMiK asks: "How is pixel[x,y] *changing*?" The per-pixel first derivative — pixel velocity — determines the display pipeline's workload:

```
pixel[x,y](t) = pixel_ref[x,y] ⊕ Δ_velocity[x,y](t)
```

For static regions, `Δ_velocity = 0` (the identity element). The hardware does literally nothing: no switching activity, no bus traffic, no power consumed. Work scales with change, not existence.

### 6.2 Temporal Delta Frames

Instead of full framebuffer refreshes, v3 represents display updates as delta streams applied to a reference frame:

```
frame[n] = frame[0] ⊕ Δ₁ ⊕ Δ₂ ⊕ ... ⊕ Δₙ
```

**Reference frame storage**: A full 640×480×24-bit reference frame requires 7.37 Mbit — far exceeding the GW1NR-9's total BSRAM capacity of 468 Kbit (26 × 18 Kbit). The reference frame therefore **cannot** be stored in on-chip BSRAM. Two viable strategies:

1. **Scanline-based reconstruction** (v3.0 target): Maintain per-scanline reference data in a single BSRAM block (640 × 24-bit = 15.4 Kbit per scanline, fits in one 18 Kbit block). The CPU or DMA pre-loads the next scanline's reference while the current scanline streams out. This is natural for HDMI's line-by-line scan pattern.

2. **External PSRAM** (future): The Tang Nano 9K has a 64 Mbit PSRAM (8 MB) on-board. A full reference frame (7.37 Mbit) fits easily, with bandwidth to spare at QSPI rates. This path requires a PSRAM controller but enables full-frame random access.

**No drift**: Unlike H.264/H.265, which accumulate prediction errors and require periodic keyframe refreshes, ATOMiK delta frames are mathematically exact. The self-inverse property (`Δ ⊕ Δ = 0`) guarantees that `frame[0]` is always reachable by re-applying the accumulated deltas. This is proven by the `delta_self_inverse` theorem in `Properties.lean:79`.

**Compression characteristics**:

| Scenario | Pixels Changed | Delta Size | vs. Full Frame |
|----------|:-:|:-:|:-:|
| Static display | 0% | 0 bits | ∞x reduction |
| Typical GUI (editor, terminal) | ~5% | 368 Kbit | 20x |
| GUI + delta color LUT | ~5% (8-bit index) | 123 Kbit | 60x |
| Video (1% change) | ~1% | 73.7 Kbit | 100x |
| Video + delta color LUT | ~1% (8-bit index) | 24.6 Kbit | 300x |

Full frame reference: 640 × 480 × 24-bit = 7.37 Mbit per frame.

### 6.3 Delta Color LUT

An 8-bit index into a 256-entry **transition delta table** provides 3x additional compression on top of spatial compression. This is the same ATOMiK algebra applied at pixel scale: each LUT entry is a full 64-bit transition delta, not a color.

```
pixel_current = pixel_reference ⊕ LUT[index]
```

When `index = 0`, `LUT[0] = 0` (identity element) — the pixel is unchanged (zero cost). When `index > 0`, the LUT entry encodes the XOR delta between the reference pixel and the target pixel. This is exactly the reconstruction equation applied per-pixel.

**BSRAM allocation**: 256 × 64-bit = 16,384 bits, stored in a single BSRAM block (well within 18 Kbit capacity). The full 64-bit width means each entry can encode transitions for multiple pixel components or packed pixel formats — not limited to 24-bit RGB.

**CLS3 SREG integration**: The call-and-response pattern works as follows:
- **REG[0] (the call)**: "Did this pixel change?" — 1-bit delta mask, shifted out per pixel clock
- **REG[1] (the response)**: "Which transition?" — 8-bit LUT index for changed pixels

Changed pixels look up their transition delta in the LUT; unchanged pixels skip the lookup entirely. The LUT is reprogrammable for different application contexts (text rendering, UI themes, video palettes).

**Why 64-bit entries matter**: Traditional color palettes store absolute colors (256 × 24-bit = RGB values). ATOMiK's LUT stores transition *deltas* at full datapath width. A single entry can encode a color space transformation, a multi-channel state transition, or a packed update to multiple pixel attributes — all composed via the same XOR algebra.

### 6.4 Scanline Integration

The CLS3 SREG (Section 5.3) ties the display pipeline directly to the ATOMiK delta state:

```
┌──────────────────────────────────────────────────┐
│  HDMI Pixel Clock (25.2 MHz)                     │
│           ↓                                      │
│  Scanline position (row, column)                 │
│           ↓                                      │
│  ┌────────────────────────────────────────────┐  │
│  │ ATOMiK Reconstruction                      │  │
│  │ current_state = initial_state ⊕ accumulator│  │
│  │ Extract pixel[row,col] from current_state  │  │
│  └────────────────────────────────────────────┘  │
│           ↓                                      │
│  ┌────────────────────────────────────────────┐  │
│  │ CLS3 SREG (scanline delta mask)            │  │
│  │ Shift out: "did this pixel change?"        │  │
│  └────────────────────────────────────────────┘  │
│           ↓                                      │
│  Changed:   send new pixel color (from LUT)      │
│  Unchanged: hold previous color                  │
│           ↓                                      │
│  TMDS Encoder → HDMI output                      │
└──────────────────────────────────────────────────┘
```

During each scanline, the SREG shifts out one bit per pixel clock, indicating whether that pixel has changed since the last frame. Only changed pixels require new color data from the delta color LUT. Static pixels are free.

---

## 7. I/O Architecture

### 7.1 IDES16/OSER16 for Inter-Board Delta Streaming

Gowin GW1NR-9 provides IDES16 (input deserializer) and OSER16 (output serializer) primitives for high-speed pin-level I/O. These are not used for XOR computation — they expand pin bandwidth for delta stream transmission between boards.

**Bandwidth calculation** (at 81 MHz fabric clock, DDR):

```
1 LVDS pair × 2 (DDR) × 81 MHz = 162 Mbit/s per pin pair
16-bit parallel output per fabric clock (IDES16 deserializes)
```

| Delta Width | Throughput | Fabric Cycles per Delta |
|:-:|:-:|:-:|
| 8-bit | ~253M deltas/sec | 0.5 |
| 16-bit | ~127M deltas/sec | 1 |
| 32-bit | ~81M deltas/sec | 2 |
| 64-bit | ~40M deltas/sec | 4 |

### 7.2 Multi-Node Topology

```
┌────────────────────────────────────────────────────┐
│  Node A (Master)                                   │
│  acc = [initial ⊕ Δ₁ ⊕ Δ₂ ⊕ ... ⊕ Δₙ]            │
│         ↓                                          │
│  OSER16 → LVDS pins (162 Mbit/s)                  │
│         ↓                                          │
│  ═══════════ interconnect ═════════════════════    │
│         ↓                        ↓                 │
│  Node B                    Node C                  │
│  IDES16 ← LVDS             IDES16 ← LVDS          │
│  acc_b ⊕= stream           acc_c ⊕= stream        │
│         ↓                        ↓                 │
│  Both nodes converge to same state                 │
│  (XOR commutativity: order doesn't matter)         │
└────────────────────────────────────────────────────┘
```

Each node maintains its own accumulator and independently reconstructs state. Delta streams flow between boards at wire speed. Commutativity (`delta_comm`, `Properties.lean:50`) guarantees that the order of delta arrival does not affect the final state.

### 7.3 Integration with CPU External Bus

The IDES16/OSER16 interface connects to the CPU via a standard peripheral register interface on the 25.2 MHz bus domain. The CPU configures stream parameters (destination address, delta width) and the I/O primitive operates autonomously, injecting received deltas directly into the ATOMiK accumulator without CPU intervention.

---

## 8. Program Model

### 8.1 Programs as Delta Streams

In v3, launching a program is:
1. Select a BSRAM reference state (`ATOMIK.LOAD` or `ATOMIK.SWAP`)
2. Inject a sequence of deltas (`ATOMIK.ACCUM`)

The delta sequence *is* the program. There is no separate "program" representation — the accumulator holds the program's effect on state.

### 8.2 Context Switching

`ATOMIK.SWAP rs1` changes the BSRAM address pointer in 1 cycle. The accumulator **persists** across SWAP — deltas accumulated against the previous reference state remain in the accumulator register. This is the correct default: the accumulator represents "total change applied," and that change history is independent of which reference state is selected.

**Context switch patterns** (all use the 4 v3.0 instructions):

| Pattern | Sequence | Effect |
|---------|----------|--------|
| **Instant context switch** | `ATOMIK.SWAP addr` | Change reference, accumulator continues. `current_state` now reflects new reference ⊕ existing deltas. |
| **Full context switch** | `ATOMIK.SWAP addr` then `ATOMIK.LOAD addr` | Change reference + clear accumulator. Clean slate against new reference. |
| **Fork** | `ATOMIK.READ rd` then store `rd` to new BSRAM entry, then `ATOMIK.SWAP new_addr` | Snapshot current state as new reference, switch to it. |

`ATOMIK.LOAD` always clears the accumulator (same as v2). `ATOMIK.SWAP` never clears the accumulator. This gives software full control: a "clean switch" is SWAP + LOAD (2 instructions, 2 cycles), while a "hot switch" (retaining delta history) is a single SWAP.

Compared to v2's 3-cycle MMIO save/restore sequence with manual state management, this is 1–2 instructions with no bus overhead.

### 8.3 Hardware-Native Session Recording

Capturing the delta stream *is* capturing the program. Every `ATOMIK.ACCUM` that fires can be logged to a circular buffer (in BSRAM or external flash). Replay is exact: re-applying the same delta sequence from the same initial state produces the identical final state.

This is not an approximation or a heuristic — it is a mathematical guarantee from the delta-state algebra. The composition property (`transition_compose`, `Transition.lean`) proves that `transition(transition(s, δ₁), δ₂) = transition(s, δ₁ ⊕ δ₂)`.

### 8.4 Reverse Engineering the Accumulator

Given the current accumulator value, one can decompose it into constituent deltas to understand what sequence of changes produced the current state. Since `δ ⊕ δ = 0` (self-inverse), applying a known delta to the accumulator "removes" that delta's effect, revealing the remaining changes.

This enables debugging, state inspection, and program analysis at the hardware level.

### 8.5 Undo/Branching

The self-inverse property enables instant revert: applying the same delta a second time undoes its effect. State forking is equally natural — read the current state, store it as a new BSRAM reference, and explore a different delta sequence. The original state is preserved in BSRAM and can be returned to at any time.

---

## 9. AI-Aligned Architecture

### 9.1 Actions as Deltas

An AI model's output — move cursor, highlight icon, open window — maps directly to an `ATOMIK.ACCUM` operation. There is no translation layer between the model's action representation and the hardware state change.

```
AI decision: "move cursor right 5 pixels"
  → Δ_cursor = encode_position_delta(+5, 0)
  → ATOMIK.ACCUM Δ_cursor  (1 cycle, 1 instruction)
  → Display updates automatically (pixel velocity propagates)
```

### 9.2 Spatial Reasoning in Hardware

Cursor-icon overlap, hit detection, and spatial relationships can be evaluated using the free LUT4 inputs on the reconstruction XOR (Section 5.2). A boolean function of `initial_state[i] ⊕ accumulator[i]` combined with mask and detect bits produces spatial predicates in combinational logic — no CPU cycles consumed.

### 9.3 Zero Translation Layer

The path from AI action to hardware state change to display feedback is:

```
AI action → ATOMIK.ACCUM → accumulator update → reconstruction XOR → pixel output
```

Every step is a single clock cycle or combinational. There is no software stack, no driver, no framebuffer copy, no display list traversal between the action and its visible result.

### 9.4 Natural RL Alignment

The ATOMiK architecture maps directly to reinforcement learning primitives:

| RL Concept | ATOMiK Mapping |
|------------|---------------|
| State | Accumulator bits (or reconstructed state) |
| Action | Delta injection (`ATOMIK.ACCUM`) |
| Transition | XOR composition (deterministic, 1 cycle) |
| Observation | Display output (pixel velocity) |
| Reward | Observable change (accumulator zero flag, or `ATOMIK.DETECT` in v3.1) |

---

## 10. Resource Projections

### 10.1 v3 SoC Estimated Budget

| Subsystem | Est. LUT4 | Notes |
|-----------|----------:|-------|
| Custom RV64I core | ~1,800 | Multi-cycle, RV64I base + 4 custom instructions. Register file in BSRAM (0 LUT). |
| ATOMiK v3 (1 bank, BSRAM state) | ~200 | Accumulator only, initial state in BSRAM |
| 64→32 bus adapter | ~100 | Inside load/store unit, splits 64-bit ops to 32-bit peripheral bus |
| CLS3 SREG CDC | ~50 | Registered handshake (faster→slower), minimal logic |
| UART + GPIO | ~250 | Same as v2 |
| HDMI output | ~700 | Same as v2 (126 MHz serializer) |
| **Total** | **~3,100** | **36% of GW1NR-9** |

For comparison, the current v2 SoC uses 3,838 LUT4 (44%). **v3 is estimated to be smaller than v2 despite having a wider datapath and more features.** The primary savings come from the BSRAM register file (~200 LUT freed) and elimination of the CDC bridge (~200 LUT freed from v2's toggle-handshake bridge).

### 10.2 Per-Bit CLS Comparison

| Architecture | CLS per Bit | LUT4 per Bit | REG per Bit | Source |
|:---:|:---:|:---:|:---:|---|
| v2 (measured) | ~1.7 | ~2.4 (50% waste) | ~2.0 | Synthesis reports |
| v3 (projected) | 1.0 | 2.0 (0% waste) | 2.0 | CLS mapping analysis |
| Improvement | **41% reduction** | **17% reduction** | — | |

### 10.3 BSRAM Impact

| BSRAM Block | v2 | v3 | Notes |
|-------------|:--:|:--:|-------|
| Boot ROM | 2 | 2 | Unchanged |
| SRAM (data/stack) | 10 | 10 | Unchanged |
| CPU register file | 0 | 1 | 32×64-bit, true dual-port (RS1/RS2 simultaneous read) |
| ATOMiK state table | 0 | 1 | 288×64-bit reference states, instant context switch |
| Delta color LUT (optional) | 0 | 0–1 | 256×64-bit transition deltas |
| **Total** | **12/26 (47%)** | **14–15/26 (54–58%)** |

The two v3-specific BSRAM blocks (register file + state table) trade 2 blocks of BSRAM (a resource with 54% headroom in v2) for ~200 LUT savings (register file) and 576 reference state entries (state table). The delta color LUT adds a third block when display integration is enabled.

### 10.4 Scaling

| Configuration | Est. LUT4 | LUT % | Feasibility |
|---------------|----------:|------:|-------------|
| v3 SoC (1 bank, 32-bit) | ~3,100 | 36% | Comfortable |
| v3 SoC (1 bank, 64-bit) | ~3,200 | 37% | Comfortable |
| v3 SoC + N=4 banks | ~3,350 | 39% | Comfortable |
| v3 SoC + N=8 banks | ~3,650 | 42% | Comfortable |
| v3 SoC + N=16 banks | ~4,350 | 50% | Feasible (BSRAM-backed scaling) |

BSRAM-backed initial state changes the scaling curve. In v2, N=16 required 1,779 LUT standalone; in v3, the per-bank marginal cost drops from ~65 LUT to ~45 LUT because the initial state register is eliminated.

---

## 11. Relationship to v2

v2 remains the deployed, validated, production proof-of-concept. It runs on real hardware, passes all tests, and demonstrates the core thesis: delta-state acceleration works on commodity FPGA.

v3 is the architectural optimization target. It builds on v2's validated algebra, formal proofs, and benchmark methodology. Specifically:

- **v2 hardware tests and benchmarks remain valid** as baseline comparisons for v3
- **v2's 92 Lean4 theorems apply without modification** to v3 (same algebra, same reconstruction equation)
- **v2's production SoC can continue operating** while v3 is developed — they target the same FPGA and can share the toolchain
- **v2's firmware methodology** (bare-metal C, SPI XIP, UART test harness) transfers directly to v3 with the new instruction set

The transition path is: validate custom RV64I core independently → integrate ATOMiK v3 datapath → port firmware → benchmark against v2 baseline → deploy.

---

## 12. Design Decisions

The following questions were raised during spec review and resolved prior to implementation.

### 12.1 Verification Strategy — RESOLVED

**Decision**: Verilator as primary simulation engine, iverilog for module-level tests.

Verilator provides 10–100x faster simulation than iverilog for the full SoC, critical for running the RV64I compliance suite. iverilog remains useful for quick single-module iteration (as in v2 development). The compliance target is the **rv64ui-p-\*** subset (RV64I unprivileged integer tests, physical addressing mode) — this covers the 47 base integer instructions without requiring virtual memory or supervisor mode.

### 12.2 BSRAM Read Latency — RESOLVED

**Decision**: BSRAM 1-cycle read latency is hidden in the Decode stage. Zero pipeline bubble.

The multi-cycle microarchitecture already dedicates a full cycle to Decode (instruction decode + register read). BSRAM synchronous read fits naturally within this cycle: address is presented at the start of Decode, data is available at the start of Execute. This applies to both the register file (Section 4.4) and the ATOMiK state table (Section 5.1). No additional pipeline stages or stall logic required.

### 12.3 Custom Instruction Count — RESOLVED

**Decision**: 4 instructions for v3.0 (LOAD, ACCUM, READ, SWAP). MASK and DETECT deferred to v3.1.

See Section 4.3 for the complete encoding table and rationale. The 4 core instructions cover all fundamental delta-state operations: reference selection, accumulation, reconstruction, and context switching. MASK/DETECT require validating the CLS LUT4[1] free-input wiring on silicon — a risk that should not gate v3.0 bringup.

### 12.4 CLS3 SREG Clock Domain — RESOLVED

**Decision**: Registered handshake (faster→slower direction). No FIFO needed.

See Section 5.3. The ATOMiK domain (81 MHz) is always faster than the HDMI pixel clock (25.2 MHz). A registered handshake with a valid flag provides correct CDC at minimal cost (~10 LUT). The 3.2:1 frequency ratio guarantees data stability.

### 12.5 64→32 Bus Adapter — RESOLVED

**Decision**: Adapter lives inside the CPU's load/store unit.

See Section 4.6. This keeps the peripheral bus at 32 bits (matching v2's proven infrastructure) while giving the CPU internal 64-bit width for ATOMiK operations. The adapter splits 64-bit peripheral accesses into two 32-bit bus transactions.

### 12.6 Patent Implications — DEFERRED

The question of whether custom ISA extensions require separate patent claims beyond the existing provisional patent is deferred to legal review. Not a blocking concern for architecture or implementation.

### 12.7 Register File Implementation — RESOLVED

**Decision**: BSRAM register file (1 block, true dual-port for simultaneous RS1/RS2 read).

See Section 4.4. Gowin BSRAM in 288×64-bit configuration provides native dual-port access, reading both source operands in a single Decode cycle. This saves ~200 LUT compared to distributed register implementation. The 1-cycle read latency is absorbed by the Decode stage.

---

## 13. Clock and PLL Strategy

### 13.1 v2 Baseline

The v2 SoC uses both available PLLs:
- **PLL1 (HDMI)**: 126 MHz serializer, 25.2 MHz pixel clock (CLKDIV), 25.2 MHz CPU clock (shared)
- **PLL2 (ATOMiK)**: 81 MHz dedicated ATOMiK core clock

### 13.2 v3 Strategy: Parallel Banks + Display PLL

Multi-bank parallel configurations naturally reduce achievable Fmax (v2 data: N=16 drops from 96 MHz to 66 MHz). Rather than fighting this with aggressive timing closure, v3 embraces the operating point:

- **Width over speed**: N=16 at 66 MHz delivers 1,056 Mops/s. The Fmax drop is a known, validated characteristic of the XOR merge tree — not a timing failure.
- **Freed PLL opportunity**: In configurations where the CPU and ATOMiK share a clock domain (both running at the parallel Fmax), the dedicated ATOMiK PLL is freed. This PLL can be repurposed for **HD display streaming throughput** — driving a higher pixel clock for 720p/1080p output or a faster serializer for IDES16/OSER16 inter-board streaming.
- **Clock strategy evolves with configuration**: Single-bank v3.0 bringup uses the v2 clock topology (2 PLLs: HDMI + ATOMiK). Multi-bank configurations can consolidate CPU + ATOMiK onto one PLL and use the second for display/streaming bandwidth.

This aligns with the architecture: as parallelism increases, the clock budget shifts from raw speed to streaming throughput — exactly where the display pipeline needs it.

---

## Appendix A: v2 → v3 Comparison Table

```
┌──────────────────┬──────────────────┬──────────────────┐
│ Metric           │ v2 (Current)     │ v3 (Proposed)    │
├──────────────────┼──────────────────┼──────────────────┤
│ CPU              │ PicoRV32 (32-bit)│ Custom RV64I     │
│                  │ 3 cycles via CDC │ 1 cycle direct   │
├──────────────────┼──────────────────┼──────────────────┤
│ Initial State    │ Registers (32 FF)│ BSRAM (shared)   │
│                  │ 1 reference      │ 576 references   │
├──────────────────┼──────────────────┼──────────────────┤
│ CLS per Bit      │ ~1.7 CLS/bit    │ 1.0 CLS/bit     │
│                  │ 50% LUT waste   │ 100% utilization │
├──────────────────┼──────────────────┼──────────────────┤
│ CLS3 SREG        │ Unused          │ Scanline delta   │
│                  │                 │ + color index    │
├──────────────────┼──────────────────┼──────────────────┤
│ Per-Bank Regs    │ 64 FF           │ 32 FF            │
│ (1 bank @ 32-bit)│ (init + acc)    │ (acc only)       │
├──────────────────┼──────────────────┼──────────────────┤
│ Per-Bank LUTs    │ ~77 LUT4        │ ~45 LUT4         │
│ (1 bank @ 32-bit)│                 │ (42% reduction)  │
├──────────────────┼──────────────────┼──────────────────┤
│ Context Switch   │ 3-cycle MMIO    │ 1-cycle SWAP     │
│                  │ manual save     │ atomic (addr)    │
├──────────────────┼──────────────────┼──────────────────┤
│ Change Detection │ Software compare│ HW + free LUT    │
│                  │ O(n) CPU cycles │ O(1) combinational│
├──────────────────┼──────────────────┼──────────────────┤
│ Bit Masking      │ Manual in SW    │ LUT4 I2/I3       │
│                  │ per operation   │ (2 free inputs)  │
├──────────────────┼──────────────────┼──────────────────┤
│ I/O Bandwidth    │ Serial USB      │ IDES16/OSER16    │
│ (inter-board)    │ ~12 Mbit/s      │ 162 Mbit/s       │
├──────────────────┼──────────────────┼──────────────────┤
│ Display Pipeline │ Separate HDMI   │ SREG delta mask  │
│                  │ no integration  │ integrated       │
├──────────────────┼──────────────────┼──────────────────┤
│ Session Record   │ SW-only (slow)  │ Capture delta    │
│                  │                 │ stream (native)  │
├──────────────────┼──────────────────┼──────────────────┤
│ SoC LUT Usage    │ 3,838 (44%)     │ ~3,100 (36%)     │
│ (Tang Nano 9K)   │                 │ (less than v2)   │
├──────────────────┼──────────────────┼──────────────────┤
│ SoC Datapath     │ 32-bit          │ 64-bit           │
│ Width            │                 │ (parameterizable)│
├──────────────────┼──────────────────┼──────────────────┤
│ Custom ISA       │ None            │ custom-0..3      │
│                  │ generic MMIO    │ ATOMIK.* ops     │
├──────────────────┼──────────────────┼──────────────────┤
│ Delta-State      │ 92 Lean4 thms   │ Same algebra     │
│ Algebra          │ Verified        │ (unchanged)      │
├──────────────────┼──────────────────┼──────────────────┤
│ Production       │ Deployed        │ Proposed         │
│ Status           │ on Tang 9K      │                  │
└──────────────────┴──────────────────┴──────────────────┘
```

## Appendix B: Single-Bank Datapath Diagram

```
┌──────────────────────────────────────────────────────────────┐
│                  ATOMiK v3 Single-Bank Datapath              │
│                                                              │
│         BSRAM                    Registers                   │
│    (state table)                                             │
│    ┌─────────────┐                                           │
│    │state[0]:OS  │                   ┌──────┐    ┌────────┐ │
│    │state[1]:app │──────addr────────►│ S₀   ├───►│        │ │
│    │state[2]:game│    select (read)   │(read)│    │┌────┐ │ │
│    │...          │                   └──────┘    ││ ⊕  ├──┼─► current_state
│    │state[N]     │                               │└────┘ │ │
│    └─────────────┘              ┌────────┐       │  ▲    │ │
│                           ┌────►│  acc   ├───┬──┼──┘     │ │
│                           │     │[DW-1:0]│   │  │        │ │
│                  delta ───┤     │  REG   │◄──┘  │        │ │
│                  inject    │     └────────┘      │        │ │
│                           │     (feedback)      │        │ │
│                           │                     └────────┘ │
│                                                              │
│  v2 → v3 Savings (per bank, 32-bit):                        │
│    REG:  64 → 32 (50% reduction)                            │
│    LUT4: ~77 → ~45 (42% reduction)                          │
│    CLS:  ~55 → ~32 (42% reduction)                          │
│    BSRAM: 0 → shared (1 block for ALL banks/contexts)       │
└──────────────────────────────────────────────────────────────┘
```

## Appendix C: Full CLS-Level CFU Mapping

```
              ATOMiK v3 — CFU-Level Mapping (handles 2 bits)
              Repeat DW/2 times (16 CFUs for 32-bit, 32 for 64-bit)

    ┌─────────────────────────────────────────────────────────────┐
    │  CFU                                                        │
    │  ┌─────────┐                                                │
    │  │   CRU   │  clk ──► all REGs       (configured by P&R)   │
    │  │         │  rst_n ► async clear                           │
    │  └─────────┘                                                │
    │                                                             │
    │  ┌──── CLS0 (bit [i]) ──────────────────────────────────┐   │
    │  │  LUT4[0]: acc XOR + clear    REG[0]: accumulator[i]  │   │
    │  │  LUT4[1]: recon XOR + mask   REG[1]: data_out[i]     │   │
    │  └──────────────────────────────────────────────────────┘   │
    │                                                             │
    │  ┌──── CLS1 (bit [i+1]) ────────────────────────────────┐   │
    │  │  (identical to CLS0 for bit i+1)                      │   │
    │  └──────────────────────────────────────────────────────┘   │
    │                                                             │
    │  ┌──── CLS2 (optional: overflow/control) ───────────────┐   │
    │  │  Available for control logic, bank select decode,     │   │
    │  │  or additional datapath bits if needed                │   │
    │  └──────────────────────────────────────────────────────┘   │
    │                                                             │
    │  ┌──── CLS3 (SREG capable) ─────────────────────────────┐   │
    │  │  REG[0] as SREG: scanline delta mask shift register  │   │
    │  │  REG[1] as SREG: color delta index shift register    │   │
    │  │  Optional: debug scan chain                          │   │
    │  └──────────────────────────────────────────────────────┘   │
    └─────────────────────────────────────────────────────────────┘
```

## Appendix D: Referenced Files

| File | Description | Relevance to v3 |
|------|-------------|-----------------|
| `specs/rtl_architecture.md` | v2 RTL spec | Format reference, v2 baseline |
| `specs/formal_model.md` | Formal algebra (92 theorems) | Unchanged, v3 builds on same math |
| `hardware/rtl/atomik_core_v2.v` | Current core implementation | v3 replaces control interface |
| `hardware/rtl/atomik_delta_acc.v` | Current accumulator | v3 removes register-based initial_state |
| `hardware/rtl/atomik_state_rec.v` | Current reconstructor | v3 feeds from BSRAM instead of registers |
| `hardware/rtl/atomik_parallel_acc.v` | Parallel banks architecture | v3 reduces per-bank cost via BSRAM |
| `hardware/picorv32/atomik_bus_wrapper.v` | Current SoC wrapper | v3 replaces PicoRV32 with custom RV64I |
| `hardware/picorv32/atomik_cdc_bridge.v` | CDC bridge | v3 eliminates for ATOMiK path (direct wire) |
| `docs/reference/gowin/RESOURCE_BUDGET_GUIDE.md` | GW1NR-9 resource data | Resource estimates validated against |
| `ROADMAP.md` | Master roadmap | Will reference v3 spec after creation |

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 3.0 | 2026-02-16 | Claude (Opus) | Initial v3 architecture specification |
| 3.0.1 | 2026-02-16 | Claude (Opus 4.6) | Comprehensive update: resolved all 7 open questions (Q6 deferred); corrected delta color LUT to 256×64-bit transition deltas; corrected reference frame storage (BSRAM impossibility → scanline/PSRAM strategy); reduced v3.0 to 4 custom instructions (MASK/DETECT deferred to v3.1); added exact instruction encoding table; added BSRAM register file (1 block, true dual-port); defined SWAP accumulator persist/clear semantics; added clock/PLL strategy (parallel + display PLL); updated resource projections; added 64→32 adapter and CLS3 CDC decisions |
