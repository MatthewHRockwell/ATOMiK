# ATOMiK Architecture

## Hardware‑Native Transient State Computation

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
| **Phase 2** | SCORE Comparison | ✅ **Complete** | 95-100% memory reduction, 22-55% speedup validated |
| **Phase 3** | Hardware Synthesis | 🔄 Ready | Verified RTL from proven model |
| **Phase 4** | SDK Development | ⏳ Pending | Python/Rust/JS SDKs |

**Latest**: Phase 2 complete (January 24, 2026). Benchmark comparison validates ATOMiK's 95-100% memory traffic reduction and 22-55% speed improvement on write-heavy workloads vs traditional SCORE. Statistical significance achieved (p < 0.05) on 75% of comparisons. See [`reports/comparison.md`](reports/comparison.md) for details.

---

## Architectural Abstract

**ATOMiK** is a hardware-native compute architecture that replaces persistent architectural state with **transient state evolution**. Computation is expressed as bounded, deterministic delta propagation across register-local state, eliminating bulk memory traffic, cache coherency overhead, and speculative execution. The result is a cycle-bounded execution model capable of nanosecond-scale decision latency, well-suited for FPGA and ASIC implementation where determinism, security, and energy efficiency are first-order constraints.

---

## Overview

**ATOMiK** is a stateless, hardware‑native compute architecture that reframes computation as **transient state evolution** rather than persistent state storage.

Instead of repeatedly loading, storing, and reconciling full system state, ATOMiK operates exclusively on **register‑local deltas**—capturing only what has changed, when it changed, and how it evolved. Computation is performed as a bounded sequence of deterministic state transitions that exist only long enough to produce a result.

This execution model:

* Breaks the classical memory wall by eliminating bulk memory traffic
* Minimizes data movement and external memory dependencies
* Enables deterministic, nanosecond‑scale decision latency
* Eliminates entire classes of state‑based security vulnerabilities
* Maps naturally to FPGA fabric without cache hierarchies or speculation

---

## Formal Verification

The mathematical foundations of ATOMiK have been **formally verified** in Lean4, establishing rigorous proofs for the delta-state algebra that underlies the architecture.

### Proven Properties

| Property | Description | Theorem |
|----------|-------------|---------|
| **Closure** | Delta composition produces valid deltas | `delta_closure` |
| **Associativity** | Grouping doesn't affect composition | `delta_assoc` |
| **Commutativity** | Order doesn't affect composition | `delta_comm` |
| **Identity** | Zero delta is no-op | `delta_identity` |
| **Self-Inverse** | Any delta XOR itself yields zero | `delta_inverse` |
| **Determinism** | Same inputs always produce same output | `determinism_guarantees` |
| **Turing Completeness** | ATOMiK can compute any computable function | `turing_completeness_summary` |

### Verification Artifacts

```
math/proofs/
├── ATOMiK/
│   ├── Basic.lean          # Core type definitions
│   ├── Delta.lean          # Delta operations
│   ├── Closure.lean        # Closure proofs
│   ├── Properties.lean     # Algebraic properties
│   ├── Composition.lean    # Sequential/parallel operators
│   ├── Transition.lean     # State transitions, determinism
│   ├── Equivalence.lean    # Computational equivalence
│   └── TuringComplete.lean # Turing completeness via CM simulation
├── ATOMiK.lean             # Root module
└── lakefile.lean           # Build configuration
```

**Build & Verify**:
```bash
cd math/proofs
lake build  # All proofs verified, 0 sorry statements
```

---

## Performance Validation

The theoretical advantages proven in Phase 1 have been **empirically validated** through comprehensive benchmarking against traditional SCORE (State-Centric Operation with Register Execution) architecture.

### Benchmark Results

| Metric | ATOMiK vs Baseline | Significance |
|--------|-------------------|--------------|
| **Memory Traffic** | 95-100% reduction | ✅ Verified (MB → KB) |
| **Execution Speed (write-heavy)** | +22% to +55% faster | ✅ p < 0.001 |
| **Execution Speed (read-heavy)** | -32% slower | Trade-off for reconstruction cost |
| **Parallel Efficiency** | 0.85 vs 0.0 | ✅ Commutative composition |
| **Cache Performance** | +16% to +23% | ✅ Smaller delta footprint |

### Workload Analysis

**ATOMiK excels at**:
- Write-heavy workloads (< 30% reads): Matrix operations, streaming pipelines
- Long operation chains: Orders of magnitude memory reduction
- Parallel composition: 85% efficiency (vs impossible for baseline)

**Trade-offs**:
- Read-heavy workloads (> 70% reads): Reconstruction overhead
- Crossover point: ~50% read ratio

### Statistical Rigor

- **360 measurements** across 9 workloads
- **100 outliers** detected and removed (modified Z-score)
- **75% of comparisons** statistically significant (p < 0.05, Welch's t-test)
- **45 unit tests** passing (baseline + ATOMiK implementations)

See [`reports/comparison.md`](reports/comparison.md) for complete analysis.

---

## Execution Model Summary

At a high level, ATOMiK operates under the following principles:

1. **No Persistent Architectural State**
   The core does not maintain long‑lived global state. All computation occurs within tightly scoped register windows.

2. **Delta‑Only Propagation**
   Inputs are treated as deltas rather than full state vectors. Only the minimal information required to advance computation is propagated.

3. **Cycle‑Bounded Evaluation**
   Each computation completes in a known, bounded number of clock cycles, independent of historical system state.

4. **Hardware‑First Semantics**
   The Verilog implementation is the reference execution path. Software models exist only to validate correctness and measurement.

### Mathematical Foundation

The delta-state algebra (Δ, ⊕, 𝟎) forms an **Abelian group**:

```
δ₁ ⊕ δ₂ ∈ Δ                    -- Closure
(δ₁ ⊕ δ₂) ⊕ δ₃ = δ₁ ⊕ (δ₂ ⊕ δ₃)  -- Associativity
δ ⊕ 𝟎 = δ                       -- Identity
δ ⊕ δ = 𝟎                       -- Self-inverse
δ₁ ⊕ δ₂ = δ₂ ⊕ δ₁              -- Commutativity
```

These properties enable **hardware optimization**: deltas can be accumulated in any order, composed before application, and reduced via parallel XOR trees.

---

## Repository Structure

```text
ATOMiK/
├── math/
│   └── proofs/             # ✅ Lean4 formal proofs (92 theorems)
├── rtl/                    # Verilog source (ATOMiK core, UART, glue logic)
├── software/
│   └── atomik_sdk/         # Python SDK (7 modules)
├── constraints/            # FPGA constraint files
├── docs/
│   ├── theory.md           # ✅ Theoretical foundations
│   └── ATOMiK_Development_Roadmap.md  # Master development plan
├── specs/
│   ├── formal_model.md     # ✅ Mathematical specification
│   └── equivalence_claims.md  # ✅ Computational equivalence proofs
├── reports/
│   ├── PROOF_VERIFICATION_REPORT.md     # ✅ Phase 1 verification
│   ├── comparison.md                    # ✅ Phase 2 SCORE comparison
│   └── PHASE_2_COMPLETION_REPORT.md     # ✅ Phase 2 completion summary
├── experiments/            # ✅ Phase 2 benchmarks (360 measurements)
│   ├── benchmarks/         # Baseline & ATOMiK implementations (2,100 LOC)
│   ├── data/               # Memory, overhead, scalability results
│   └── analysis/           # Statistical analysis and reports
├── hardware/               # Phase 3 synthesis (pending)
└── impl/                   # Gowin synthesis outputs
```

---

## Hardware Implementation

### Core Logic (`rtl/`)

The hardware directory contains a minimal but representative ATOMiK datapath:

* **`atomik_core.v`**
  Implements the transient state execution engine. This module is responsible for accepting delta inputs, performing bounded combinational/sequential evaluation, and emitting result deltas without retaining historical context.

* **`atomik_top.v`**
  Top‑level integration wrapper that binds the ATOMiK core to external interfaces and simulation infrastructure.

* **`uart_genome_loader.v`**
  UART interface for loading configuration and observing outputs.

The design intentionally avoids caches, DMA engines, or external memory controllers to ensure measured latency reflects **pure compute behavior**, not I/O artifacts.

---

## Simulation & Demo

The included demo video shows a complete simulation loop:

<div align="center">
<video src="https://github.com/user-attachments/assets/06de6427-d917-4722-9129-266b6e87520f" width="600" controls></video>
</div>

* Deterministic stimulus injection
* Transient state evaluation inside the ATOMiK core
* UART-serialized output
* Waveform inspection in GTKWave

---

## Latency Envelope (Representative)

The following table summarizes **cycle-level latency behavior** observed under simulation:

| Stage | Description | Cycles |
|------:|-------------|:------:|
| Input Latch | Delta capture | 1 |
| Core Evaluation | Transient state propagation | O(1–N) bounded |
| Result Commit | Output stabilization | 1 |
| Serialization (Optional) | UART visibility only | Variable |

Key properties:

* Latency is **bounded and deterministic**
* No dependency on prior execution history
* No cache warm-up, page faults, or speculative rollback

---

## What ATOMiK Is — and Is Not

**ATOMiK is:**

* A hardware-native transient state compute architecture
* A deterministic, cycle-bounded execution model
* A **formally verified** computational foundation
* A platform for ultra-low-latency decision logic

**ATOMiK is not:**

* A general-purpose CPU or soft-core processor
* A GPU, NPU, or data-parallel accelerator
* A firmware-driven state machine
* A software-emulated architecture

ATOMiK occupies a distinct point in the compute design space: **where computation is expressed as ephemeral state evolution rather than stored program execution**.

---

## Roadmap

### ✅ Phase 1: Mathematical Formalization (Complete)
- Delta-state algebra formally verified in Lean4
- 92 theorems proven, 0 sorry statements
- Turing completeness established
- [View Report](reports/PROOF_VERIFICATION_REPORT.md)

### ✅ Phase 2: SCORE Comparison (Complete)
- Benchmarked ATOMiK vs traditional state-centric architectures
- **Results**: 95-100% memory traffic reduction, 22-55% speed improvement on write-heavy workloads
- **Measurements**: 360 data points across 9 workloads (memory, overhead, scalability)
- **Statistical Validation**: 75% of comparisons statistically significant (p < 0.05)
- **Tests**: 45/45 unit tests passing (baseline + ATOMiK implementations)
- **Key Finding**: Parallel efficiency 0.85 vs 0.0 (ATOMiK vs baseline) due to commutative composition
- [View Comparison Report](reports/comparison.md) | [View Completion Report](reports/PHASE_2_COMPLETION_REPORT.md)

### 🔄 Phase 3: Hardware Synthesis (Ready)
- Synthesize verified RTL from proven mathematical model
- Delta accumulator and state reconstructor modules
- FPGA deployment on Gowin platform
- Informed by Phase 2: optimize for write-heavy workloads, implement parallel XOR tree

### ⏳ Phase 4: SDK Development (Pending)
- Multi-language SDKs (Python, Rust, JavaScript)
- Comprehensive API documentation
- Example applications and integration guides

**Full roadmap**: [`docs/ATOMiK_Development_Roadmap.md`](docs/ATOMiK_Development_Roadmap.md)

---

## Documentation

| Document | Description |
|----------|-------------|
| [Development Roadmap](docs/ATOMiK_Development_Roadmap.md) | Master development plan with agentic deployment instructions |
| [Theoretical Foundations](docs/theory.md) | Mathematical background and proof summaries |
| [Formal Model Specification](specs/formal_model.md) | Delta-state algebra definitions |
| [Equivalence Claims](specs/equivalence_claims.md) | Computational equivalence proofs |
| [Proof Verification Report](reports/PROOF_VERIFICATION_REPORT.md) | Phase 1 completion report (92 theorems verified) |
| [SCORE Comparison Report](reports/comparison.md) | Phase 2 benchmark results and analysis |
| [Phase 2 Completion Report](reports/PHASE_2_COMPLETION_REPORT.md) | Phase 2 task completion summary |

---

## Building & Verification

### Lean4 Proofs
```bash
cd math/proofs
lake build
# Expected: Build completed successfully, 0 errors
```

### Benchmarks
```bash
cd experiments/benchmarks

# Run unit tests
python baseline/test_baseline.py    # 13 tests
python atomik/test_atomik.py        # 19 tests
python test_metrics.py              # 13 tests

# Execute full benchmark suite
python runner.py
# Generates data in experiments/data/{memory,overhead,scalability}/

# Run statistical analysis
cd ../analysis
python analyze.py
# Generates statistics.md with significance tests
```

### Python SDK
```bash
cd software/atomik_sdk
pip install -e .
pytest tests/
```

### Verilog Simulation
```bash
cd rtl
iverilog -o sim atomik_core.v atomik_top.v tb/*.v
vvp sim
gtkwave dump.vcd
```

---

## Audience Alignment

### FPGA & Hardware Engineers
Focus on `rtl/`, `constraints/`, and the hardware implementation sections. The design emphasizes cycle-bounded execution with no hidden software abstraction.

### Researchers & Technical Reviewers
Focus on `math/proofs/`, `docs/theory.md`, and the formal verification sections. All mathematical claims are machine-verified.

### Investors & Strategic Partners
Focus on the Overview, Roadmap, and Development Status sections. Phase 1 completion demonstrates technical feasibility and rigorous methodology.

---

## CI/CD Status

[![ATOMiK CI](https://github.com/[owner]/ATOMiK/actions/workflows/atomik-ci.yml/badge.svg)](https://github.com/[owner]/ATOMiK/actions)

Automated verification on every push:
- Lean4 proof checking (`[proof]` commits)
- Python linting and tests
- Verilog simulation (when enabled)

---

## Licensing & Contact

Source files are provided under the **BSD 3-Clause License** for evaluation only, subject to the patent notice above.

For licensing inquiries, commercial integration, or architectural collaboration, please contact the repository owner.

---

*Last updated: January 24, 2026 (Phase 2 Complete)*
