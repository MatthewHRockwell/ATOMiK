# ATOMiK Architecture

## Delta-State Computation in Silicon

> **IP & PATENT NOTICE**
>
> This repository contains software benchmarks, hardware description language (HDL) implementations, formal mathematical proofs, and validation artifacts for the **ATOMiK Architecture**.
>
> The underlying architecture, execution model, and methods demonstrated here are **Patent Pending**.
>
> While the source code in this repository may be licensed under the **BSD 3‑Clause License** for evaluation, testing, and benchmarking purposes, **no rights—express or implied—are granted to the underlying ATOMiK hardware architecture, execution model, or associated patents**.
>
> Commercial use, hardware integration, or derivative architectural implementations require a separate license.

---

## Development Status

| Phase | Description | Status | Milestone |
|-------|-------------|--------|-----------|
| **Phase 1** | Mathematical Formalization | ✅ **Complete** | 92 theorems verified in Lean4 |
| **Phase 2** | SCORE Comparison | ✅ **Complete** | 95-100% memory reduction validated |
| **Phase 3** | Hardware Synthesis | ✅ **Complete** | 10/10 hardware tests, 7% LUT @ 94.5 MHz |
| **Phase 4** | SDK Development | 🔄 Ready | Python/Rust/JS SDKs |

**Latest**: Phase 3 complete (January 25, 2026). ATOMiK Core v2 validated on Tang Nano 9K FPGA with all delta algebra properties verified in silicon. Single-cycle operations for LOAD, ACCUMULATE, and READ—no performance trade-offs. See [`reports/PHASE_3_COMPLETION_REPORT.md`](reports/PHASE_3_COMPLETION_REPORT.md) for details.

---

## The Core Idea

Traditional architectures store and retrieve complete state vectors. ATOMiK stores only **what changed** (deltas) and reconstructs state on demand:

```
Traditional:  State₁ → Store 64 bits → Load 64 bits → State₁
ATOMiK:       State₀ ⊕ Δ₁ ⊕ Δ₂ ⊕ ... ⊕ Δₙ = State_current (single XOR)
```

**Why this matters**:

| Advantage | Mechanism |
|-----------|-----------|
| **95-100% memory reduction** | Stream sparse deltas instead of dense state vectors |
| **Single-cycle operations** | XOR has no carry propagation—64-bit ops complete in one cycle |
| **Natural parallelism** | Commutativity enables lock-free multi-accumulator designs |
| **Reversibility built-in** | Self-inverse property (δ ⊕ δ = 0) enables undo without checkpoints |

---

## Mathematical Foundation

ATOMiK's delta operations form an **Abelian group** (Δ, ⊕, 𝟎), formally verified in Lean4:

| Property | Formula | Hardware Implication |
|----------|---------|---------------------|
| **Closure** | δ₁ ⊕ δ₂ ∈ Δ | Any delta combination is valid |
| **Associativity** | (δ₁ ⊕ δ₂) ⊕ δ₃ = δ₁ ⊕ (δ₂ ⊕ δ₃) | Tree reduction is mathematically sound |
| **Commutativity** | δ₁ ⊕ δ₂ = δ₂ ⊕ δ₁ | Order-independent parallel accumulation |
| **Identity** | δ ⊕ 𝟎 = δ | Zero-delta is no-op (filtering optimization) |
| **Self-Inverse** | δ ⊕ δ = 𝟎 | Instant undo—apply same delta to revert |

These properties enable **hardware optimizations impossible with traditional arithmetic**:
- **No carry propagation**: Unlike addition, XOR computes all 64 bits in parallel
- **Order independence**: Multiple hardware units can accumulate deltas without synchronization
- **Guaranteed reversibility**: No need to store checkpoints for undo operations

**Verification**: 92 theorems proven in Lean4, including Turing completeness via counter machine simulation. See [`math/proofs/`](math/proofs/).

---

## Performance

### Hardware-Validated Results

| Operation | Cycles | Latency @ 94.5 MHz |
|-----------|--------|-------------------|
| **LOAD** | 1 | 10.6 ns |
| **ACCUMULATE** | 1 | 10.6 ns |
| **READ** | 1 | 10.6 ns |

All operations are **single-cycle with identical cost**. There are no trade-offs between read and write performance.

### Memory Traffic Comparison

| Scenario | Traditional | ATOMiK | Reduction |
|----------|-------------|--------|-----------|
| 1000 state updates | 128 KB transferred | 0 KB (register-local) | **100%** |
| Streaming pipeline | Full state per stage | Delta per stage | **95-99%** |
| Parallel aggregation | Lock + full state sync | Lock-free delta merge | **Eliminates contention** |

### Parallelization Advantage

Because XOR is commutative and associative, multiple processing units can accumulate deltas **independently** and merge results **without locks**:

```
Unit A: acc_A = δ₁ ⊕ δ₃ ⊕ δ₅
Unit B: acc_B = δ₂ ⊕ δ₄ ⊕ δ₆
Final:  acc   = acc_A ⊕ acc_B  (same result regardless of distribution)
```

Phase 2 measured **85% parallel efficiency** in software. Hardware implementations can achieve near-linear scaling.

---

## Hardware Implementation

### Phase 3 Results

| Metric | Result |
|--------|--------|
| **Target Device** | Gowin GW1NR-9 (Tang Nano 9K) |
| **Clock Frequency** | 94.5 MHz (Fmax: 94.9 MHz) |
| **Logic Utilization** | 7% (579/8640 LUTs) |
| **Register Utilization** | 9% (537/6693 FFs) |
| **Hardware Tests** | 10/10 passing |
| **Throughput** | 94.5 million operations/second |

### Architecture

<p align="center">
  <img src="docs/diagrams/atomik_core_v2_logic.svg" alt="ATOMiK Core v2 Logic Gate Diagram" width="800"/>
</p>

<details>
<summary>ASCII Version (click to expand)</summary>

```
┌─────────────────────────────────────────────────────────────┐
│                     ATOMiK Core v2                          │
│                                                             │
│  ┌─────────────────────┐    ┌─────────────────────────┐    │
│  │  Delta Accumulator  │    │  State Reconstructor    │    │
│  │                     │    │                         │    │
│  │  initial_state[63:0]├────►  XOR (combinational)    │    │
│  │         +           │    │         │               │    │
│  │  accumulator[63:0]  ├────►         ▼               │    │
│  │         ▲           │    │  current_state[63:0]    │    │
│  │         │           │    │                         │    │
│  │     XOR(delta_in)   │    └─────────────────────────┘    │
│  └─────────────────────┘                                   │
│                                                             │
│  All operations: 1 cycle                                   │
│    LOAD:       initial_state ← data_in                     │
│    ACCUMULATE: accumulator ← accumulator ⊕ data_in         │
│    READ:       data_out ← initial_state ⊕ accumulator      │
└─────────────────────────────────────────────────────────────┘
```
</details>

**Key insight**: The entire datapath uses only XOR gates and registers. No carry chains, no multipliers, no complex control logic. This is why single-cycle operation is achievable at high clock frequencies.

### Delta Algebra Verified in Silicon

| Property | Hardware Test | Result |
|----------|---------------|--------|
| Self-Inverse (δ ⊕ δ = 0) | Accumulate same delta twice | ✅ Returns to original state |
| Identity (S ⊕ 0 = S) | Accumulate zero | ✅ State unchanged |
| Closure | Accumulate multiple deltas | ✅ Correct composition |
| Load/Read roundtrip | Load → Read | ✅ Bit-exact match |

---

## What ATOMiK Is

- **A delta-state accelerator**: Single-cycle accumulation with O(1) state reconstruction
- **A formally verified architecture**: 92 theorems in Lean4, validated in silicon
- **A hardware-first design**: Verilog is the reference implementation
- **Inherently parallel**: Commutativity enables lock-free multi-unit designs
- **Naturally reversible**: Self-inverse property provides undo without checkpoints

## What ATOMiK Is Not

- **Not a general-purpose CPU**: No instruction fetch, branching, or general ALU
- **Not a cache replacement**: It's orthogonal—reduces the data that needs caching
- **Not limited to specific data types**: Any data representable as bit vectors works

## Ideal Use Cases

| Application | Why ATOMiK Fits |
|-------------|-----------------|
| **Event sourcing** | Deltas are events; reconstruct state on demand |
| **Streaming analytics** | Continuous delta accumulation, periodic state output |
| **Financial tick processing** | High-frequency updates with sparse state queries |
| **Sensor fusion** | Multiple delta streams merged via commutative XOR |
| **Undo/redo systems** | Self-inverse property = instant reversion |
| **Distributed aggregation** | Lock-free delta merge across nodes |
| **Video/image processing** | Frame deltas instead of full frames |

---

## Repository Structure

```text
ATOMiK/
├── math/proofs/            # ✅ Lean4 formal proofs (92 theorems)
├── rtl/                    # ✅ Verilog source (Phase 3 complete)
│   ├── atomik_delta_acc.v  # Delta accumulator module
│   ├── atomik_state_rec.v  # State reconstructor module  
│   ├── atomik_core_v2.v    # Core v2 integration
│   └── atomik_top.v        # Top-level with UART interface
├── experiments/            # ✅ Phase 2 benchmarks (360 measurements)
├── constraints/            # ✅ FPGA timing and physical constraints
├── synth/                  # ✅ Synthesis scripts (Gowin EDA)
├── scripts/                # ✅ Hardware validation tests
├── docs/                   # Theory and development roadmap
├── specs/                  # Formal model and RTL architecture
├── reports/                # Phase completion reports
└── impl/pnr/ATOMiK.fs      # ✅ FPGA bitstream (Tang Nano 9K)
```

---

## Quick Start

### Verify Mathematical Proofs
```bash
cd math/proofs && lake build
# All 92 theorems verified, 0 sorry statements
```

### Run Performance Benchmarks
```bash
cd experiments/benchmarks && python runner.py
# 360 measurements with statistical analysis
```

### Synthesize & Program FPGA
```powershell
cd synth && .\run_synthesis.ps1
openFPGALoader -b tangnano9k ..\impl\pnr\ATOMiK.fs
```

### Validate Hardware
```bash
python scripts/test_hardware.py COM6
# 10/10 tests passing
```

---

## Roadmap

| Phase | Status | Key Achievement |
|-------|--------|-----------------|
| **Phase 1**: Mathematical Formalization | ✅ Complete | 92 theorems, Turing completeness proven |
| **Phase 2**: Performance Benchmarking | ✅ Complete | 95-100% memory reduction, parallelization validated |
| **Phase 3**: Hardware Synthesis | ✅ Complete | Silicon validation, single-cycle operations confirmed |
| **Phase 4**: SDK Development | 🔄 Ready | Python/Rust/JS SDKs |

**Full roadmap**: [`docs/ATOMiK_Development_Roadmap.md`](docs/ATOMiK_Development_Roadmap.md)

---

## Documentation

| Document | Description |
|----------|-------------|
| [Theoretical Foundations](docs/theory.md) | Mathematical background and proof summaries |
| [Formal Model](specs/formal_model.md) | Delta-state algebra definitions |
| [RTL Architecture](specs/rtl_architecture.md) | Hardware design specification |
| [Benchmark Analysis](reports/comparison.md) | Phase 2 performance results |
| [Hardware Validation](reports/PHASE_3_COMPLETION_REPORT.md) | Phase 3 silicon verification |

---

## Licensing & Contact

Source files are provided under the **BSD 3-Clause License** for evaluation only, subject to the patent notice above.

For licensing inquiries, commercial integration, or architectural collaboration, please contact the repository owner.

---

*Last updated: January 25, 2026 (Phase 3 Complete)*
