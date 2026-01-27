# ATOMiK RTL Architecture Specification

**Version**: 2.0  
**Date**: January 25, 2026  
**Phase**: 3 - Hardware Synthesis  
**Status**: ✅ Implemented & Validated

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Module Definitions](#2-module-definitions)
3. [Interface Specifications](#3-interface-specifications)
4. [Microarchitecture Details](#4-microarchitecture-details)
5. [Hardware Optimization Strategy](#5-hardware-optimization-strategy)
6. [Target Constraints](#6-target-constraints)
7. [Verification Plan](#7-verification-plan)
8. [Integration with Existing System](#8-integration-with-existing-system)

---

## 1. Architecture Overview

### 1.1 Executive Summary

ATOMiK Core v2 implements delta-state compute in hardware, eliminating the **32% read penalty** observed in Phase 2
software benchmarks. The key innovation is **single-cycle state reconstruction** via parallel XOR, leveraging the
proven mathematical property:

```
current_state = initial_state ⊕ δ_accumulated
```

This is an **O(1) operation in hardware** (64-bit XOR), not O(N) reconstruction of the entire delta chain. The
hardware implementation transforms ATOMiK from a write-optimized architecture into a **universally fast**
architecture.

### 1.2 Block Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                         ATOMiK Core v2                              │
│                                                                     │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │                    Control Interface                        │    │
│  │  [clk, rst_n, operation, delta_in, current_state_out]      │    │
│  └─────────────┬──────────────────────────────┬────────────────┘    │
│                │                              │                     │
│                ▼                              ▼                     │
│  ┌─────────────────────────┐   ┌──────────────────────────────┐    │
│  │  Delta Accumulator      │   │  State Reconstructor         │    │
│  │  (atomik_delta_acc)     │   │  (atomik_state_rec)          │    │
│  │                         │   │                              │    │
│  │  ┌──────────────────┐   │   │  ┌────────────────────────┐  │    │
│  │  │ initial_state    │   │   │  │   XOR Tree (64-bit)    │  │    │
│  │  │     [63:0]       │───┼───┼─▶│                        │  │    │
│  │  └──────────────────┘   │   │  │  initial_state ⊕       │  │    │
│  │                         │   │  │  delta_accumulator     │  │    │
│  │  ┌──────────────────┐   │   │  │         ║              │  │    │
│  │  │ delta_accumulator│───┼───┼─▶│         ▼              │  │    │
│  │  │     [63:0]       │   │   │  │   current_state[63:0]  │  │    │
│  │  └────────▲─────────┘   │   │  └────────────────────────┘  │    │
│  │           │             │   │                              │    │
│  │        XOR with         │   └──────────────────────────────┘    │
│  │        delta_in         │                                       │
│  └────────────┼────────────┘                                       │
│               │                                                    │
│          [delta_in]                                                │
└───────────────┼────────────────────────────────────────────────────┘
                │
           [external delta source]
```

### 1.3 Data Flow

#### Accumulate (Write) Operation

```
Cycle N:   delta_valid=1, delta_in=0x1234567890ABCDEF
           ┌──────────────────────────────────────────────┐
           │  delta_accumulator ← delta_accumulator ⊕ δ   │
           └──────────────────────────────────────────────┘

Timing:    1 cycle (XOR has no carry propagation)
Latency:   Single-cycle accumulation
Throughput: 1 delta per cycle @ target frequency
```

#### Reconstruct (Read) Operation

```
Cycle N:   read_enable=1
           ┌──────────────────────────────────────────────┐
           │  current_state = initial_state ⊕ accumulator │
           └──────────────────────────────────────────────┘

Timing:    Combinational (0 cycles added latency)
Latency:   Same-cycle output
Throughput: Continuous read availability
```

**Critical Insight**: In software (Phase 2), state reconstruction required iterating through N deltas (O(N)
complexity, causing 32% penalty). In hardware, we maintain a running accumulator, so reconstruction is a single 
64-bit XOR operation (O(1) complexity).

### 1.4 Mathematical Foundation

From `math/proofs/ATOMiK/Properties.lean`, the following theorems guarantee correctness:

| Proven Theorem | Hardware Implication | Location |
|----------------|---------------------|----------|
| `delta_comm` | Delta order irrelevant → parallel composition possible | Properties.lean:50 |
| `delta_assoc` | Grouping irrelevant → tree reduction valid | Properties.lean:34 |
| `delta_self_inverse` | δ ⊕ δ = 0 → reversibility, debugging | Properties.lean:79 |
| `transition_compose` | Sequential = composed delta | Transition.lean |

These properties enable aggressive hardware optimizations that would be unsafe in traditional architectures.

---

## 2. Module Definitions

### 2.1 Module: `atomik_delta_acc`

**Purpose**: Maintain initial state and accumulate deltas via XOR composition.

**Key Features**:
- Dual-register architecture (initial_state, delta_accumulator)
- Single-cycle XOR accumulation
- Configurable delta width (64-bit default)
- Synchronous reset to known state

**Port Summary**:
```verilog
module atomik_delta_acc #(
    parameter DELTA_WIDTH = 64
)(
    input  wire                    clk,
    input  wire                    rst_n,
    input  wire [DELTA_WIDTH-1:0]  delta_in,
    input  wire                    delta_valid,
    input  wire [DELTA_WIDTH-1:0]  initial_state_in,
    input  wire                    load_initial,
    output wire [DELTA_WIDTH-1:0]  initial_state_out,
    output wire [DELTA_WIDTH-1:0]  delta_accumulator_out,
    output wire                    accumulator_zero
);
```

**Behavior**:
- **Reset**: `initial_state ← 0`, `delta_accumulator ← 0`
- **Load**: `initial_state ← initial_state_in` (when `load_initial=1`)
- **Accumulate**: `delta_accumulator ← delta_accumulator ⊕ delta_in` (when `delta_valid=1`)

### 2.2 Module: `atomik_state_rec`

**Purpose**: Combinational state reconstruction from initial state and accumulator.

**Key Features**:
- Pure combinational logic (0-cycle latency)
- 64-bit XOR tree (balanced for timing)
- Always-available output (no enable signal needed)

**Port Summary**:
```verilog
module atomik_state_rec #(
    parameter STATE_WIDTH = 64
)(
    input  wire [STATE_WIDTH-1:0]  initial_state,
    input  wire [STATE_WIDTH-1:0]  delta_accumulator,
    output wire [STATE_WIDTH-1:0]  current_state
);
```

**Behavior**:
- **Continuous**: `current_state = initial_state ⊕ delta_accumulator`

### 2.3 Module: `atomik_core_v2`

**Purpose**: Top-level integration of delta accumulator and state reconstructor.

**Key Features**:
- Unified control interface
- Operation multiplexing (load, accumulate, read)
- Status flags (ready, zero)
- Clock domain: single synchronous domain

**Port Summary**:
```verilog
module atomik_core_v2 #(
    parameter DATA_WIDTH = 64
)(
    // Clock and Reset
    input  wire                   clk,
    input  wire                   rst_n,

    // Control Interface
    input  wire [1:0]             operation,      // 00=NOP, 01=LOAD, 10=ACCUMULATE, 11=READ
    input  wire [DATA_WIDTH-1:0]  data_in,
    output reg  [DATA_WIDTH-1:0]  data_out,
    output reg                    data_ready,

    // Status Interface
    output wire                   accumulator_zero,
    output wire [DATA_WIDTH-1:0]  debug_initial_state,
    output wire [DATA_WIDTH-1:0]  debug_accumulator
);
```

**Operation Encoding**:
```
2'b00 (NOP):        No operation, hold state
2'b01 (LOAD):       Load initial_state ← data_in
2'b10 (ACCUMULATE): Accumulate delta ← data_in
2'b11 (READ):       Output current_state to data_out
```

---

## 3. Interface Specifications

### 3.1 Timing Diagrams

#### 3.1.1 LOAD Operation

```
Clock:    ┌───┐   ┌───┐   ┌───┐   ┌───┐
          │   │   │   │   │   │   │   │
        ──┘   └───┘   └───┘   └───┘   └──

operation:  ══╤═══════╤═══════╤═══════╤══
            00│  01   │  00   │  00   │00
              └───────┘       └───────┘

data_in:    ══╤═══════╤═══════╤═══════╤══
            XX│ S_init│  XX   │  XX   │XX
              └───────┘       └───────┘

data_ready:───┐       ┌───────────────────
              │       │
            ──┘       └───────────────────

internal:   ══╤═══════════════════════════
  initial_  00│     S_init
  state       └───────────────────────────
```

**Latency**: 1 cycle (registered load)

#### 3.1.2 ACCUMULATE Operation

```
Clock:    ┌───┐   ┌───┐   ┌───┐   ┌───┐
          │   │   │   │   │   │   │   │
        ──┘   └───┘   └───┘   └───┘   └──

operation:  ══╤═══════╤═══════╤═══════╤══
            00│  10   │  10   │  00   │00
              └───────┴───────┘

data_in:    ══╤═══════╤═══════╤═══════╤══
            XX│  δ1   │  δ2   │  XX   │XX
              └───────┴───────┘

data_ready:───┐       ┌───────────────────
              │       │
            ──┘       └───────────────────

internal:   ══╤═══╤═══════╤═══════════════
  delta_    00│ δ1│ δ1⊕δ2 │  δ1⊕δ2
  accum       └───┴───────┘
```

**Latency**: 1 cycle per delta
**Throughput**: 1 delta/cycle (back-to-back accumulation supported)

#### 3.1.3 READ Operation

```
Clock:    ┌───┐   ┌───┐   ┌───┐   ┌───┐
          │   │   │   │   │   │   │   │
        ──┘   └───┘   └───┘   └───┘   └──

operation:  ══╤═══════╤═══════╤═══════╤══
            00│  11   │  00   │  11   │00
              └───────┘       └───────┘

data_out:   ══╤═══════════════╤═══════════
            XX│      S_current │  S_current
              └───────────────┴───────────

data_ready:───┐               ┌───────────
              │               │
            ──┘               └───────────
```

**Latency**: 1 cycle (combinational XOR + output register)
**Note**: State reconstruction happens combinationally; output register adds 1 cycle for timing closure.

### 3.2 Reset Behavior

```verilog
On rst_n assertion (active-low):
    initial_state     <= 64'h0
    delta_accumulator <= 64'h0
    data_out          <= 64'h0
    data_ready        <= 1'b0
```

**Hold Time**: Minimum 2 clock cycles for clean reset propagation.

**Reset Synchronization**: Core reset is gated by PLL lock signal:
```verilog
wire core_rst_n = sys_rst_n & pll_lock;
```

### 3.3 Port Descriptions

#### `atomik_core_v2` Detailed Port Table

| Port Name | Direction | Width | Description |
|-----------|-----------|-------|-------------|
| `clk` | Input | 1 | System clock (50-94.5 MHz target) |
| `rst_n` | Input | 1 | Active-low synchronous reset |
| `operation[1:0]` | Input | 2 | Operation selector (see encoding) |
| `data_in[63:0]` | Input | 64 | Input data (initial_state or delta) |
| `data_out[63:0]` | Output | 64 | Current state (for READ operation) |
| `data_ready` | Output | 1 | Pulsed high when operation completes |
| `accumulator_zero` | Output | 1 | High when delta_accumulator = 0 |
| `debug_initial_state[63:0]` | Output | 64 | Debug: Current initial_state value |
| `debug_accumulator[63:0]` | Output | 64 | Debug: Current delta_accumulator value |

---

## 4. Microarchitecture Details

### 4.1 Register Specifications

#### Initial State Register

```verilog
reg [63:0] initial_state;

Purpose:  Store the base state (S₀) from which all deltas are applied
Width:    64 bits (configurable via parameter)
Reset:    64'h0000_0000_0000_0000
Update:   On LOAD operation only
Fanout:   1 (to XOR tree in state_rec module)
```

#### Delta Accumulator Register

```verilog
reg [63:0] delta_accumulator;

Purpose:  Maintain XOR composition of all accumulated deltas
Width:    64 bits (configurable via parameter)
Reset:    64'h0000_0000_0000_0000
Update:   On ACCUMULATE operation: delta_accumulator <= delta_accumulator ^ data_in
Fanout:   2 (to XOR tree, to accumulator_zero detector)
```

### 4.2 Combinational Logic Paths

#### State Reconstruction XOR Tree

```verilog
// Single-level 64-bit XOR (Gowin synthesis optimizes this)
assign current_state = initial_state ^ delta_accumulator;

Critical Path:
    Register Output (initial_state)
         ↓
    64-bit XOR gate (LUT-based, ~1.5ns on GW1NR-9 @ C6 speed grade)
         ↓
    Register Input (data_out)

Estimated Delay: ~1.8ns (includes routing)
Timing Margin:   At 50 MHz (20ns period): 18.2ns margin
                 At 94.5 MHz (10.58ns period): 8.78ns margin
```

**Optimization Note**: The 64-bit XOR is implemented as 64 independent LUT2 gates in parallel (no carry chain). 
This ensures minimal delay even without pipelining.

#### Accumulator Zero Detection

```verilog
// OR-reduction to check if accumulator is all zeros
assign accumulator_zero = ~(|delta_accumulator);

Purpose:  Fast zero detection for delta algebra properties
Use Case: Verify δ ⊕ δ = 0 (self-inverse property)
Delay:    ~0.8ns (tree of OR gates)
```

### 4.3 Control FSM

**Design Decision**: No FSM required.

**Rationale**:
- All operations are single-cycle (LOAD, ACCUMULATE) or combinational (READ)
- No multi-cycle sequencing needed
- Operation decoding is purely combinational
- Simpler than FSM: lower area, lower latency, easier verification

**Operation Decoder** (combinational):

```verilog
wire load_en       = (operation == 2'b01);
wire accumulate_en = (operation == 2'b10);
wire read_en       = (operation == 2'b11);

always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        initial_state     <= 64'd0;
        delta_accumulator <= 64'd0;
        data_out          <= 64'd0;
        data_ready        <= 1'b0;
    end else begin
        // LOAD operation
        if (load_en) begin
            initial_state <= data_in;
            data_ready    <= 1'b1;
        end

        // ACCUMULATE operation
        else if (accumulate_en) begin
            delta_accumulator <= delta_accumulator ^ data_in;
            data_ready        <= 1'b1;
        end

        // READ operation
        else if (read_en) begin
            data_out   <= current_state;  // from combinational XOR
            data_ready <= 1'b1;
        end

        // NOP operation
        else begin
            data_ready <= 1'b0;
        end
    end
end
```

---

## 5. Hardware Optimization Strategy

### 5.1 Leveraging Proven Mathematical Properties

#### Commutativity Enables Parallel Reduction

**Theorem** (from `Properties.lean:50`):
```lean
theorem delta_comm (a b : Delta) :
    Delta.compose a b = Delta.compose b a
```

**Hardware Implication**: Multiple deltas can be XOR'd in any order.

**Future Optimization** (Phase 3 extension):
```
Input:  δ₁, δ₂, δ₃, δ₄ (4 deltas arriving in parallel)

Sequential:  3 cycles
    Cycle 1: δ₁ ⊕ δ₂ = T₁
    Cycle 2: T₁ ⊕ δ₃ = T₂
    Cycle 3: T₂ ⊕ δ₄ = result

Parallel Tree:  2 cycles (log₂(4))
    Cycle 1: (δ₁ ⊕ δ₂) | (δ₃ ⊕ δ₄)
             T₁         | T₂
    Cycle 2: T₁ ⊕ T₂ = result

Speedup: 1.5x (scales with input count)
```

**Design Hook**: The `atomik_delta_acc` module can be extended with a `parallel_width` parameter to accept multiple
deltas per cycle.

#### Associativity Enables Tree Reduction Correctness

**Theorem** (from `Properties.lean:34`):
```lean
theorem delta_assoc (a b c : Delta) :
    Delta.compose (Delta.compose a b) c = Delta.compose a (Delta.compose b c)
```

**Hardware Implication**: Tree reduction produces identical results to sequential reduction.

```
Valid (associativity):
    ((δ₁ ⊕ δ₂) ⊕ δ₃) ⊕ δ₄  =  δ₁ ⊕ (δ₂ ⊕ (δ₃ ⊕ δ₄))

Tree structure doesn't affect correctness → use fastest topology
```

### 5.2 Why XOR is Single-Cycle

Traditional addition (`a + b`) requires carry propagation:
```
  1010 1100  (a)
+ 0011 0101  (b)
-----------
  1101 0001  (sum)
  ↑↑↑↑ ↑↑↑↑
  Carry chain (critical path ~64 gate delays)
```

XOR has no carry propagation:
```
  1010 1100  (a)
⊕ 0011 0101  (b)
-----------
  1001 1001  (result)

Each bit independent → 64 parallel LUT2 gates
Delay: 1 LUT (~1.5ns on GW1NR-9)
```

**Performance Impact**:
- Add: ~6ns for 64-bit ripple carry (limits Fmax to 166 MHz)
- XOR: ~1.5ns for 64-bit parallel (supports Fmax > 300 MHz)

This is why ATOMiK achieves single-cycle accumulation even at 94.5 MHz.

### 5.3 State Reconstruction: O(1) vs O(N)

#### Software (Phase 2 Penalty)

```python
def reconstruct(self) -> int:
    result = self.initial_state
    for delta in self.delta_history:  # O(N) loop
        result ^= delta
    return result
```

**Cost**:
- N XOR operations (sequential)
- N iterations
- Phase 2 measured: 32% slower on read-heavy workloads

#### Hardware (Phase 3 Optimization)

```verilog
assign current_state = initial_state ^ delta_accumulator;  // O(1) XOR
```

**Cost**:
- 1 XOR operation (parallel 64-bit)
- 0 iterations
- **Eliminates the 32% penalty completely**

**Why it works**: We maintain a *running accumulator* (δ_acc = δ₁ ⊕ δ₂ ⊕ ... ⊕ δₙ), so reconstruction is always a
single XOR, regardless of N.

### 5.4 Extension Points for Future Optimization

#### Parallel Delta Input (Multi-Port)

```verilog
module atomik_delta_acc #(
    parameter DELTA_WIDTH = 64,
    parameter PARALLEL_PORTS = 4  // NEW
)(
    input  wire [DELTA_WIDTH-1:0]  delta_in [0:PARALLEL_PORTS-1],
    input  wire [PARALLEL_PORTS-1:0] delta_valid,
    ...
);

// Tree reduction (log₂ depth)
wire [DELTA_WIDTH-1:0] level1 [0:PARALLEL_PORTS/2-1];
for (i = 0; i < PARALLEL_PORTS/2; i = i + 1) begin
    assign level1[i] = delta_in[2*i] ^ delta_in[2*i+1];
end
// ... continue tree
```

**Speedup**: log₂(N) cycles for N deltas (vs N cycles sequential)
**Phase 2 Target**: 85% parallel efficiency → ~3.4x speedup on 4 cores

#### Pipelined Accumulator (Higher Throughput)

```verilog
// Add pipeline stage to break critical path
reg [63:0] delta_pipeline;
always @(posedge clk) begin
    delta_pipeline    <= delta_in ^ delta_accumulator;
    delta_accumulator <= delta_pipeline;
end
```

**Trade-off**: 2-cycle latency, but supports Fmax > 150 MHz

---

## 6. Target Constraints

### 6.1 FPGA Target Device

| Parameter | Value |
|-----------|-------|
| **Device** | GW1NR-LV9QN88PC6/I5 (Gowin GW1NR-9) |
| **Package** | QN88P (88-pin QFN with embedded PSRAM) |
| **Speed Grade** | C6 (commercial) / I5 (industrial) |
| **LUTs** | 8,640 total (target: ≤6,912 = 80%) |
| **Flip-Flops** | 6,480 total |
| **BSRAM** | 26 blocks (18Kb each) |
| **PLL** | 2 available |
| **Board** | Sipeed Tang Nano 9K |
| **Tool** | Gowin EDA V1.9.11.03 Education |

### 6.2 Clock Configuration

#### Input Clock

```
Source:     On-board crystal oscillator (Tang Nano 9K)
Frequency:  27 MHz
Tolerance:  ±50 ppm (standard crystal spec)
Duty Cycle: 50% ±5%
```

#### PLL Configuration (Current Production)

```verilog
// From rtl/pll/atomik_pll_94p5m.v (ACTIVE)
FCLKIN    = 27 MHz
IDIV_SEL  = 1       // Divide input by 2
FBDIV_SEL = 6       // Multiply by 7
ODIV_SEL  = 8       // VCO divider

Calculation:
    fCLKOUT = 27 MHz × (6+1) / (1+1) = 27 × 7 / 2 = 94.5 MHz
    fVCO    = 94.5 MHz × 8 = 756 MHz (within 400-1200 MHz range)

Output:     94.5 MHz
```

#### Alternative PLL Configurations

| Module | Output | Use Case |
|--------|--------|----------|
| `atomik_pll_94p5m` | 94.5 MHz | **Current production** |
| `atomik_pll_81m` | 81.0 MHz | Conservative timing |
| `Gowin_rPLL` (dynamic) | Variable | Runtime frequency selection |

**Reference**: See `docs/reference/gowin/CLOCK_REFERENCE.md` for PLL formulas and configuration details.

### 6.3 Timing Constraints

#### Target Frequencies

```sdc
# Conservative target (guaranteed timing closure)
create_clock -period 20.000 -name clk_sys [get_ports sys_clk]  # 50 MHz

# Production target (current PLL configuration)  
create_clock -period 10.582 -name clk_sys [get_ports sys_clk]  # 94.5 MHz
```

#### Critical Paths

```sdc
# Path 1: Accumulator feedback (accumulate operation)
set_max_delay -from [get_pins delta_accumulator_reg*/C] \
              -to   [get_pins delta_accumulator_reg*/D] 10.582

# Path 2: State reconstruction (read operation)
set_max_delay -from [get_pins initial_state_reg*/Q] \
              -to   [get_pins data_out_reg*/D] 10.582
```

#### Input/Output Constraints

```sdc
# Input setup/hold relative to clk
set_input_delay -clock clk_sys -max 3.0 [get_ports {operation* data_in*}]
set_input_delay -clock clk_sys -min 0.5 [get_ports {operation* data_in*}]

# Output delay (data_out must be stable before next cycle)
set_output_delay -clock clk_sys -max 3.0 [get_ports {data_out* data_ready}]
```

### 6.4 Design Constraints

| Parameter | Value | Justification |
|-----------|-------|---------------|
| **Delta Width** | 64 bits | Matches Phase 1 proofs (`DELTA_WIDTH = 64` in Basic.lean) |
| **State Width** | 64 bits | Matches Phase 2 benchmarks (Python int = 64-bit) |
| **Reset Type** | Synchronous, active-low | Matches existing atomik_top.v interface |
| **Interface** | Internal 64-bit buses | Integrates with atomik_top.v (not direct IO) |
| **Operation Latency** | 1 cycle | Single-cycle accumulate/load, combinational read |

### 6.5 Resource Budget

| Resource | Estimated Usage | Total Available | Utilization |
|----------|----------------|----------------|-------------|
| **LUTs** | ~160 | 8,640 | 1.9% |
| **Flip-Flops** | ~195 | 6,480 | 3.0% |
| **BSRAM** | 0 | 26 | 0% |
| **IO** | N/A (internal module) | 88 | N/A |

**Note**: `atomik_core_v2` is an internal module that integrates with `atomik_top.v` via internal 64-bit buses.
External IO is handled by the existing top-level interface (UART, LEDs, etc.).

**Margin**: >95% resources available for future expansion (multi-port, pipelining, genome storage, etc.).

---

## 7. Verification Plan

### 7.1 Test Vector Strategy

#### TV1: Delta Algebra Properties

**Purpose**: Verify theorems from `Properties.lean` in hardware.

```
Test 1 - Identity (delta_zero_right):
    LOAD initial_state = 0x1234567890ABCDEF
    ACCUMULATE delta   = 0x0000000000000000
    READ current_state
    EXPECT: 0x1234567890ABCDEF (unchanged)

Test 2 - Self-Inverse (delta_self_inverse):
    LOAD initial_state = 0xAAAAAAAAAAAAAAAA
    ACCUMULATE delta   = 0x5555555555555555
    ACCUMULATE delta   = 0x5555555555555555  (same delta twice)
    READ current_state
    EXPECT: 0xAAAAAAAAAAAAAAAA (δ ⊕ δ = 0, so state unchanged)

Test 3 - Commutativity (delta_comm):
    Test 3a:
        LOAD initial_state = 0x0
        ACCUMULATE δ1 = 0x1111111111111111
        ACCUMULATE δ2 = 0x2222222222222222
        READ current_state → S_a

    Test 3b:
        LOAD initial_state = 0x0
        ACCUMULATE δ2 = 0x2222222222222222
        ACCUMULATE δ1 = 0x1111111111111111
        READ current_state → S_b

    EXPECT: S_a == S_b (order irrelevant)

Test 4 - Associativity (delta_assoc):
    Test 4a: ((δ1 ⊕ δ2) ⊕ δ3)
        LOAD 0x0
        ACCUMULATE 0xFFFF
        ACCUMULATE 0x00FF
        ACCUMULATE 0x0F0F
        READ → S_a

    Test 4b: (δ1 ⊕ (δ2 ⊕ δ3))
        Compute δ_composed = 0x00FF ⊕ 0x0F0F = 0x0FF0
        LOAD 0x0
        ACCUMULATE 0xFFFF
        ACCUMULATE 0x0FF0
        READ → S_b

    EXPECT: S_a == S_b
```

#### TV2: Equivalence with Python Reference

**Purpose**: Bit-exact match with `experiments/benchmarks/atomik/delta_engine.py`.

```python
# Generate test vectors
from delta_engine import DeltaEngine

engine = DeltaEngine(initial_state=0x123456789ABCDEF0)
test_vectors = []

# Sequence of operations
deltas = [0x1111111111111111, 0x2222222222222222, 0x3333333333333333]
for delta in deltas:
    engine.accumulate(delta)
    test_vectors.append({
        'operation': 'ACCUMULATE',
        'data_in': delta,
        'expected_accumulator': engine.delta_accumulator,
        'expected_state': engine.reconstruct()
    })

# Export to Verilog testbench
```

#### TV3: Phase 2 Workload Replay

**Purpose**: Run actual Phase 2 benchmark workloads through RTL.

```
Workload: Matrix Operations (W1.1)
    - 32x32 matrix (1024 elements × 64 bits = 8KB)
    - 5 matrix operations → 5 deltas

RTL Test:
    1. Load initial_state from matrix[0]
    2. Accumulate 5 deltas (one per operation)
    3. Read current_state
    4. Compare with Python delta_engine.reconstruct()
```

### 7.2 Test Vector File Format

Test vectors are stored in `sim/vectors/` with the following format:

```
# sim/vectors/tv_properties.txt
# Format: OP, DATA_IN, EXPECTED_STATE, EXPECTED_ACCUM, EXPECTED_ZERO
# OP: L=LOAD, A=ACCUMULATE, R=READ, N=NOP
# Use '-' for don't-care values

# Test 1: Identity property
L, 0x1234567890ABCDEF, -, 0x0000000000000000, 1
A, 0x0000000000000000, -, 0x0000000000000000, 1
R, -, 0x1234567890ABCDEF, -, -

# Test 2: Self-inverse property
L, 0xAAAAAAAAAAAAAAAA, -, 0x0000000000000000, 1
A, 0x5555555555555555, -, 0x5555555555555555, 0
A, 0x5555555555555555, -, 0x0000000000000000, 1
R, -, 0xAAAAAAAAAAAAAAAA, -, -

# Test 3: Commutativity
L, 0x0000000000000000, -, 0x0000000000000000, 1
A, 0x1111111111111111, -, 0x1111111111111111, 0
A, 0x2222222222222222, -, 0x3333333333333333, 0
R, -, 0x3333333333333333, -, -
```

### 7.3 Simulation Methodology

#### 7.3.1 Unit Tests (Per Module)

```bash
# Test atomik_delta_acc
iverilog -o sim/tb_delta_acc.vvp \
    sim/stubs/gowin_rpll_stub.v \
    rtl/atomik_delta_acc.v \
    sim/tb_delta_acc.v
vvp sim/tb_delta_acc.vvp
# EXPECT: All assertions pass, $finish with success

# Test atomik_state_rec
iverilog -o sim/tb_state_rec.vvp \
    rtl/atomik_state_rec.v \
    sim/tb_state_rec.v
vvp sim/tb_state_rec.vvp

# Test atomik_core_v2
iverilog -o sim/tb_core_v2.vvp \
    rtl/atomik_delta_acc.v \
    rtl/atomik_state_rec.v \
    rtl/atomik_core_v2.v \
    sim/tb_core_v2.v
vvp sim/tb_core_v2.vvp
```

#### 7.3.2 Waveform Analysis

```bash
# Generate VCD for waveform viewing
iverilog -o sim/tb_core_v2.vvp -DVCD_OUTPUT \
    rtl/atomik_delta_acc.v \
    rtl/atomik_state_rec.v \
    rtl/atomik_core_v2.v \
    sim/tb_core_v2.v
vvp sim/tb_core_v2.vvp
gtkwave sim/atomik_core_v2.vcd
```

**Key Signals to Monitor**:
- `clk`, `rst_n`
- `operation[1:0]`, `data_in[63:0]`, `data_out[63:0]`
- `initial_state[63:0]`, `delta_accumulator[63:0]`
- `current_state[63:0]` (internal, from state_rec)
- `data_ready`, `accumulator_zero`

### 7.4 Coverage Metrics

```
Functional Coverage:
    ✓ All operation codes (00, 01, 10, 11)
    ✓ All 64 bits toggled independently
    ✓ Accumulator overflow (wraps correctly via XOR)
    ✓ Back-to-back operations
    ✓ Reset during operation

Code Coverage (Verilog):
    ✓ 100% line coverage
    ✓ 100% branch coverage (if/else)
    ✓ 100% toggle coverage (all bits 0→1, 1→0)
```

### 7.5 Validation Gates

| Gate | Metric | Threshold | Tool |
|------|--------|-----------|------|
| **Functional Correctness** | Test vectors pass | 100% | iverilog + VVP |
| **Python Equivalence** | Bit-exact match | 100% | Cocotb or custom testbench |
| **Property Verification** | Proven theorems hold | All 4 properties | Directed tests |
| **Code Coverage** | Line/branch/toggle | ≥95% | Verilator --coverage |
| **Timing Closure** | Slack | ≥0 ns @ 94.5 MHz | Gowin EDA timing report |
| **Resource Utilization** | LUT usage | ≤80% (6,912 LUTs) | Gowin EDA utilization report |

### 7.6 Test Deliverables

```
sim/
├── stubs/
│   └── gowin_rpll_stub.v         # PLL simulation model
├── tb_delta_acc.v                # Unit test: Delta accumulator
├── tb_state_rec.v                # Unit test: State reconstructor
├── tb_core_v2.v                  # Integration test: Full core
├── vectors/
│   ├── tv_properties.txt         # Delta algebra property tests
│   ├── tv_equivalence.txt        # Python reference equivalence
│   └── tv_corners.txt            # Corner case tests
└── run_tests.sh                  # Automated test runner
```

---

## 8. Integration with Existing System

### 8.1 Interface with `atomik_top.v`

The existing `atomik_top.v` integrates `atomik_core` (v3.4), which implements polymorphic encryption. For ATOMiK 
Core v2, we add the delta architecture as a new capability while maintaining compatibility.

#### Current Integration (v3.4)

```verilog
// atomik_top.v lines 138-151
atomik_core core_inst (
    .clk               (clk_int),
    .rst_n             (core_rst_n),
    .scramble_threshold(w_poly_freq),
    .polymorph_seed    (w_poly_seed),
    .otp_en            (w_otp_en),
    .data_in           (core_data_in),
    .data_valid        (core_data_valid),
    .data_out          (w_data_out),
    .data_ready        (w_data_ready)
);
```

#### Proposed Integration (v2 - Delta Architecture)

```verilog
// atomik_top.v (modified for delta architecture)
atomik_core_v2 #(
    .DATA_WIDTH(64)
) core_v2_inst (
    .clk               (clk_int),
    .rst_n             (core_rst_n),

    // Map operation from genome/config
    .operation         (w_operation),          // NEW: from BIOS
    .data_in           (w_delta_in),           // NEW: delta input
    .data_out          (w_current_state_out),  // Current state
    .data_ready        (w_data_ready),

    // Status
    .accumulator_zero  (w_accumulator_zero),   // NEW: status flag
    .debug_initial_state(w_debug_init),        // NEW: debug
    .debug_accumulator (w_debug_accum)         // NEW: debug
);
```

### 8.2 Co-existence Strategy

**Recommended Approach**: Dual-core configuration (selectable via parameter or BIOS command)

```verilog
// Dual-core configuration
parameter ENABLE_DELTA_CORE = 1;

generate
    if (ENABLE_DELTA_CORE) begin : gen_core_v2
        atomik_core_v2 core_v2_inst (...);  // Delta architecture
    end else begin : gen_core_v3
        atomik_core core_inst (...);        // Polymorphic engine
    end
endgenerate
```

This maintains compatibility with existing FPGA builds while enabling delta architecture testing.

### 8.3 Pin Assignment (Tang Nano 9K)

Existing pin assignments from `constraints/atomik_constraints.cst`:

```
IO_LOC "sys_clk" 52;      // 27 MHz crystal input
IO_LOC "sys_rst_n" 4;     // Reset button (directly active-low)
IO_LOC "uart_rx" 18;      // UART receive (directly from BL702)
IO_LOC "uart_tx" 17;      // UART transmit
IO_LOC "led[0]" 10;       // LED outputs (active-low accent)
IO_LOC "led[1]" 11;
IO_LOC "led[2]" 13;
IO_LOC "led[3]" 14;
IO_LOC "led[4]" 15;
IO_LOC "led[5]" 16;
```

**No changes required**: Core v2 is internal module, uses same external IO as top-level.

**Reference**: See `docs/reference/gowin/TANG_NANO_9K_PINOUT.md` for complete pin mapping.

### 8.4 Build Script Integration

```tcl
# synth/gowin_synth.tcl (add core v2 to project)
add_file -type verilog "rtl/atomik_delta_acc.v"
add_file -type verilog "rtl/atomik_state_rec.v"
add_file -type verilog "rtl/atomik_core_v2.v"

# Synthesis options
set_option -use_dsp 0              # No DSP blocks needed (XOR is LUT-based)
set_option -use_bsram 0            # No block RAM needed
set_option -rw_check_on_ram 1      # Register write checks

# Place & Route options
set_option -frequency 94.5         # Target 94.5 MHz (current PLL)
set_option -fanout_guide 30        # Max fanout control
set_option -timing_driven 1        # Timing-driven P&R
```

---

## 9. Summary and Next Steps

### 9.1 Architecture Summary

ATOMiK Core v2 implements delta-state computation in hardware with:

- **Single-cycle accumulation**: XOR has no carry propagation
- **O(1) state reconstruction**: current_state = initial_state ⊕ accumulator
- **Eliminates 32% read penalty**: Observed in Phase 2 software benchmarks
- **Provably correct**: Backed by 92 theorems from Phase 1 Lean4 proofs
- **Resource efficient**: <2% LUT utilization on target FPGA

### 9.2 Performance Predictions

| Metric | Software (Phase 2) | Hardware (Phase 3 Target) | Improvement |
|--------|-------------------|--------------------------|-------------|
| **Accumulate Latency** | ~10 ns (Python overhead) | 10.6 ns @ 94.5 MHz (1 cycle) | Comparable |
| **Read Latency** | Variable (O(N) reconstruction) | 10.6 ns @ 94.5 MHz (1 cycle) | **Eliminates O(N) penalty** |
| **Throughput** | ~10 Mops/s (Python) | 94.5 Mops/s @ 94.5 MHz | **9.5x faster** |
| **Memory Traffic** | 95% reduction (Phase 2) | 99% reduction (delta only) | Maintains advantage |
| **Parallel Efficiency** | 0.85 (simulated) | 0.85 (tree reduction) | Hardware-ready |

### 9.3 Next Steps (Phase 3 Task Sequence)

| Task | Description | Depends On | Deliverable |
|------|-------------|------------|-------------|
| **T3.1** | RTL architecture spec | Phase 2 complete | `specs/rtl_architecture.md` ✓ |
| **T3.2** | Implement atomik_delta_acc.v | T3.1 | `rtl/atomik_delta_acc.v` |
| **T3.3** | Implement atomik_state_rec.v | T3.1 | `rtl/atomik_state_rec.v` |
| **T3.4** | Implement atomik_core_v2.v | T3.2, T3.3 | `rtl/atomik_core_v2.v` |
| **T3.5** | Write testbenches, simulate | T3.4 | `sim/tb_*.v`, passing tests |
| **T3.6** | Timing constraints | T3.5 | `constraints/atomik_timing.sdc` |
| **T3.7** | FPGA synthesis scripts | T3.6 | `synth/gowin_synth.tcl` |
| **T3.8** | Resource utilization analysis | T3.7 | `math/benchmarks/results/RESOURCE_UTILIZATION.md` |
| **T3.9** | Hardware validation report | T3.8 | `archive/PHASE_3_COMPLETION_REPORT.md` |

### 9.4 Design Decisions Summary

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Data Width** | 64 bits | Match Phase 1 proofs and Phase 2 benchmarks |
| **Control Interface** | 2-bit operation code | Simple, no FSM required |
| **Accumulator Style** | Running accumulator | Enables O(1) reconstruction |
| **State Reconstruction** | Combinational | Zero added latency |
| **Reset Type** | Synchronous, active-low | Match existing atomik_top.v |
| **Target Frequency** | 94.5 MHz (production) | Current PLL configuration |
| **Parallel Ports** | 1 (Phase 3 baseline) | Extension point for future optimization |

---

## Appendices

### Appendix A: Glossary

| Term | Definition |
|------|------------|
| **Delta** | XOR difference between two states (δ = S₁ ⊕ S₂) |
| **Delta Composition** | XOR combination of multiple deltas (δ₁ ⊕ δ₂ ⊕ ... ⊕ δₙ) |
| **Accumulator** | Register holding XOR composition of all deltas |
| **State Reconstruction** | Computing current state from initial state and accumulator |
| **SCORE** | State-Centric Operation with Register Execution (traditional model) |
| **Commutativity** | Property that order doesn't matter (δ₁ ⊕ δ₂ = δ₂ ⊕ δ₁) |
| **Associativity** | Property that grouping doesn't matter ((δ₁ ⊕ δ₂) ⊕ δ₃ = δ₁ ⊕ (δ₂ ⊕ δ₃)) |
| **Self-Inverse** | Property that δ ⊕ δ = 0 |

### Appendix B: References to Phase 1 Proofs

| Theorem | File:Line | Hardware Use |
|---------|-----------|--------------|
| `delta_comm` | Properties.lean:50 | Enables parallel composition |
| `delta_assoc` | Properties.lean:34 | Validates tree reduction |
| `delta_self_inverse` | Properties.lean:79 | Zero detection, debugging |
| `delta_zero_left` | Properties.lean:64 | Identity element verification |
| `transition_compose` | Transition.lean | State reconstruction correctness |

### Appendix C: Resource Estimation Breakdown

```
atomik_delta_acc:
    Registers: 64 (initial_state) + 64 (delta_accumulator) = 128 FFs
    LUTs:      64 (XOR) + 10 (control) + 8 (OR-tree zero detect) = 82 LUTs

atomik_state_rec:
    Registers: 0 (combinational only)
    LUTs:      64 (XOR tree) = 64 LUTs

atomik_core_v2:
    Registers: 128 (from delta_acc) + 64 (data_out) + 2 (control) = 194 FFs
    LUTs:      82 (delta_acc) + 64 (state_rec) + 15 (control decode) = 161 LUTs

TOTAL:
    Flip-Flops: 194 / 6,480 = 2.99%
    LUTs:       161 / 8,640 = 1.86%
```

**Margin**: >95% resources available for future expansion.

### Appendix D: Timing Estimate (94.5 MHz, 10.58ns period)

```
Critical Path 1: Accumulator Feedback Loop
    FF output (delta_accumulator) →  0.5 ns (Tco)
    Routing to XOR                →  0.8 ns
    64-bit XOR (LUT2 × 64)        →  1.5 ns
    Routing to FF input           →  0.8 ns
    FF setup time                 →  0.3 ns
    TOTAL:                           3.9 ns
    SLACK:                           6.68 ns @ 94.5 MHz ✓

Critical Path 2: State Reconstruction
    FF output (initial_state)     →  0.5 ns
    Routing to XOR                →  0.8 ns
    64-bit XOR                    →  1.5 ns
    Routing to data_out FF        →  0.8 ns
    FF setup time                 →  0.3 ns
    TOTAL:                           3.9 ns
    SLACK:                           6.68 ns @ 94.5 MHz ✓
```

**Conclusion**: Timing closure feasible at 94.5 MHz with >6ns margin.

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-25 | Claude (Sonnet) | Initial draft |
| 1.1 | 2026-01-25 | Claude (Opus) | Corrected PLL frequency (94.5 MHz), fixed UART pin assignments, added test vector format, clarified internal integration |
| 2.0 | 2026-01-25 | Claude (Opus) | Implementation complete - all hardware tests passing, timing closure achieved |

---

## Appendix E: Implementation Results

### Actual vs Predicted Performance

| Metric | Predicted | Actual | Status |
|--------|-----------|--------|--------|
| Frequency | 94.5 MHz | 94.9 MHz (Fmax) | ✅ +0.4% |
| LUT Usage | 161 (1.9%) | 579 (7%)* | ✅ Well under budget |
| FF Usage | 194 (3.0%) | 537 (9%)* | ✅ Well under budget |
| Timing Slack | >6 ns | 0.049 ns | ✅ Meeting constraints |
| Accumulate Latency | 1 cycle | 1 cycle | ✅ |
| Read Latency | 1 cycle | 1 cycle | ✅ |
| Hardware Tests | 100% pass | 10/10 pass | ✅ |

*Higher than estimated due to UART interface and command state machine (not included in core estimate)

### Hardware Validation Summary

| Test | Property | Result |
|------|----------|--------|
| Test 2 | Load/Read roundtrip | ✅ Pass |
| Test 3 | Accumulator zero detection | ✅ Pass |
| Test 4 | Single delta accumulation | ✅ Pass |
| Test 6 | Self-inverse (δ ⊕ δ = 0) | ✅ Pass |
| Test 7 | Identity (S ⊕ 0 = S) | ✅ Pass |
| Test 8 | Multiple deltas | ✅ Pass |
| Test 9 | State reconstruction | ✅ Pass |

### Deliverables Produced

| Deliverable | Location | Status |
|-------------|----------|--------|
| Delta accumulator | `rtl/atomik_delta_acc.v` | ✅ |
| State reconstructor | `rtl/atomik_state_rec.v` | ✅ |
| Core v2 integration | `rtl/atomik_core_v2.v` | ✅ |
| Top-level with UART | `rtl/atomik_top.v` | ✅ |
| Physical constraints | `constraints/atomik_constraints.cst` | ✅ |
| Timing constraints | `constraints/timing_constraints.sdc` | ✅ |
| Synthesis scripts | `synth/gowin_synth.tcl` | ✅ |
| Hardware test script | `scripts/test_hardware.py` | ✅ |
| FPGA bitstream | `impl/pnr/ATOMiK.fs` | ✅ |
| Resource report | `math/benchmarks/results/RESOURCE_UTILIZATION.md` | ✅ |
| Completion report | `archive/PHASE_3_COMPLETION_REPORT.md` | ✅ |
