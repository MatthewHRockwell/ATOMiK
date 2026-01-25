# ATOMiK Benchmark Suite Design

**Version**: 1.0
**Date**: January 24, 2026
**Phase**: 2 - SCORE Comparison
**Task**: T2.1

---

## Executive Summary

This document specifies the benchmark suite for comparing ATOMiK's delta-state architecture against traditional SCORE (State-Centric Operation with Register Execution). The benchmarks measure three critical performance dimensions:

1. **Memory Efficiency**: Storage and traffic reduction from delta encoding
2. **Computational Overhead**: Cycle count and operation complexity
3. **Scalability**: Performance across varying problem sizes and parallelism

---

## 1. Benchmark Categories

### 1.1 Memory Efficiency Benchmarks

**Objective**: Quantify memory traffic reduction and cache utilization improvements

**Workloads**:
- **W1.1 Matrix Operations**: Dense matrix multiplication (64x64, 128x128, 256x256)
  - Traditional: Store full matrices, read-modify-write
  - ATOMiK: Store delta transformations, compose via XOR
  - Metrics: Bytes read/written, cache misses, allocation count

- **W1.2 State Machines**: Finite state automaton with 256-10,000 states
  - Traditional: Store current state + history buffer
  - ATOMiK: Store delta chain
  - Metrics: Total memory allocated, peak memory usage

- **W1.3 Streaming Data**: Process 1M-10M data points with 10-100 transformations
  - Traditional: Store intermediate results at each stage
  - ATOMiK: Accumulate deltas, reconstruct final state only
  - Metrics: Memory footprint, allocation frequency

### 1.2 Computational Overhead Benchmarks

**Objective**: Compare operation costs (XOR composition vs traditional operations)

**Workloads**:
- **W2.1 Delta Composition**: Chain 10-10,000 delta operations
  - Traditional: Sequential state updates (read → modify → write)
  - ATOMiK: XOR accumulation (read → XOR → write to accumulator)
  - Metrics: CPU cycles, instruction count, branch mispredictions

- **W2.2 State Reconstruction**: Reconstruct state from delta history
  - Traditional: Already has state (baseline = 1 operation)
  - ATOMiK: XOR all deltas (N operations)
  - Metrics: Latency, throughput (ops/sec)

- **W2.3 Mixed Read/Write**: Interleaved state queries and modifications
  - Traditional: Direct state access (cheap reads, expensive writes)
  - ATOMiK: Reconstruct on read (expensive), compose on write (cheap)
  - Metrics: Average latency per operation type

### 1.3 Scalability Benchmarks

**Objective**: Evaluate performance across problem sizes and parallel workloads

**Workloads**:
- **W3.1 Problem Size Scaling**: Run W1.1 and W2.1 with exponentially increasing sizes
  - Sizes: 2^4, 2^8, 2^12, 2^16, 2^20 elements
  - Metrics: Time complexity (linear, log, quadratic), memory scaling factor

- **W3.2 Parallel Composition**: Leverage commutativity for parallel delta accumulation
  - Traditional: Serial state updates (data dependencies)
  - ATOMiK: Parallel XOR reduction (order-independent)
  - Metrics: Speedup factor, parallel efficiency

- **W3.3 Cache Locality**: Measure cache hit rates with varying working set sizes
  - Sizes: 1KB, 64KB, 1MB, 10MB (L1, L2, L3, RAM)
  - Metrics: Cache miss rate, memory bandwidth utilization

---

## 2. Implementation Strategy

### 2.1 Baseline Implementation (Traditional SCORE)

**File**: `experiments/benchmarks/baseline/state_manager.py`

```python
class StateManager:
    """Traditional stateful architecture - read/modify/write pattern"""

    def __init__(self, initial_state: int):
        self.state = initial_state  # Full state storage
        self.history = []           # Optional history buffer

    def read(self) -> int:
        return self.state

    def write(self, new_state: int):
        self.state = new_state

    def modify(self, operation):
        self.state = operation(self.state)
```

**Characteristics**:
- O(1) state access
- O(N) memory for N-depth history
- Serial dependency chain (write depends on read)

### 2.2 ATOMiK Implementation (Delta-State)

**File**: `experiments/benchmarks/atomik/delta_engine.py`

```python
class DeltaEngine:
    """Delta-state architecture - compose/accumulate pattern"""

    def __init__(self, initial_state: int):
        self.initial_state = initial_state
        self.delta_accumulator = 0  # XOR accumulator
        self.delta_history = []     # Delta chain

    def accumulate(self, delta: int):
        self.delta_accumulator ^= delta
        self.delta_history.append(delta)

    def reconstruct(self) -> int:
        return self.initial_state ^ self.delta_accumulator

    def compose_deltas(self, deltas: list) -> int:
        result = 0
        for d in deltas:
            result ^= d
        return result
```

**Characteristics**:
- O(N) reconstruction cost (N = delta count)
- O(1) delta storage per operation
- Parallel composition possible (XOR commutative)

### 2.3 Workload Implementations

**File**: `experiments/benchmarks/baseline/workloads.py` and `experiments/benchmarks/atomik/workloads.py`

Each workload (W1.1-W3.3) implemented in both variants with:
- Identical input generation
- Equivalent computational semantics
- Instrumented metrics collection

---

## 3. Metrics Framework

### 3.1 Collected Metrics

**File**: `experiments/benchmarks/metrics.py`

| Metric | Unit | Collection Method |
|--------|------|-------------------|
| Execution Time | seconds | `time.perf_counter()` |
| Peak Memory | bytes | `tracemalloc.get_traced_memory()` |
| Memory Traffic | bytes | Custom allocator tracking |
| Cache Misses | count | `perf` counters (Linux) / estimates |
| Instruction Count | count | CPU cycle counters |
| Allocation Count | count | `gc.get_stats()` |

### 3.2 Statistical Validation

**Requirements**:
- **Sample Size**: Minimum 100 iterations per workload
- **Confidence Interval**: 95% (z = 1.96)
- **Significance Test**: Welch's t-test, p < 0.05 threshold
- **Outlier Removal**: Modified Z-score > 3.5

**Analysis**:
```python
# Compute mean, std dev, 95% CI
mean = np.mean(samples)
std = np.std(samples, ddof=1)
ci = 1.96 * std / np.sqrt(len(samples))

# Compare baseline vs ATOMiK
t_stat, p_value = scipy.stats.ttest_ind(baseline_samples, atomik_samples, equal_var=False)
statistically_significant = p_value < 0.05
```

---

## 4. Expected Results

### 4.1 Hypothesis

Based on the proven mathematical properties (from Phase 1):

| Property | Baseline (SCORE) | ATOMiK | Expected Winner |
|----------|------------------|---------|-----------------|
| Memory per operation | O(S) full state | O(1) delta | ATOMiK |
| Composition cost | O(1) read/write | O(1) XOR | Similar |
| Reconstruction cost | O(1) access | O(N) XOR chain | Baseline |
| Parallel scaling | Serial (dependencies) | Parallel (commutative) | ATOMiK |
| Cache efficiency | Large state footprint | Small delta footprint | ATOMiK |

### 4.2 Projected Performance

**Memory Efficiency** (W1.1-W1.3):
- **Reduction**: 40-60% fewer bytes transferred for workloads with long operation chains
- **Peak memory**: 30-50% reduction when history/checkpointing is needed

**Computational Overhead** (W2.1-W2.3):
- **Delta composition**: 1.0-1.2x overhead (XOR is cheap, but N of them adds up)
- **State reconstruction**: 1.5-3.0x overhead when frequent reads required
- **Sweet spot**: Write-heavy workloads with infrequent reads

**Scalability** (W3.1-W3.3):
- **Problem size**: ATOMiK advantage grows with operation count (more deltas to compose in parallel)
- **Parallel efficiency**: 0.8-0.95 efficiency with 4-8 threads (vs 0.1-0.3 for baseline)
- **Cache performance**: 20-40% fewer cache misses for delta-based approach

---

## 5. Execution Plan

### 5.1 Implementation Tasks

| Task | Deliverable | Dependencies |
|------|-------------|--------------|
| T2.2 | `baseline/` implementation | None |
| T2.3 | `atomik/` implementation | None |
| T2.4 | `metrics.py` framework | None |

### 5.2 Benchmark Execution Tasks

| Task | Workloads | Output Directory | Dependencies |
|------|-----------|------------------|--------------|
| T2.5 | W1.1-W1.3 | `data/memory/` | T2.2-T2.4 |
| T2.6 | W2.1-W2.3 | `data/overhead/` | T2.2-T2.4 |
| T2.7 | W3.1-W3.3 | `data/scalability/` | T2.2-T2.4 |

### 5.3 Analysis Tasks

| Task | Description | Output |
|------|-------------|--------|
| T2.8 | Statistical analysis, visualization | `analysis/plots/`, `analysis/statistics.md` |
| T2.9 | Final comparison report | `reports/comparison.md` |

---

## 6. Validation Criteria

### 6.1 Correctness

- [ ] Both implementations produce identical outputs for same inputs
- [ ] Delta composition matches proven algebraic properties (associative, commutative)
- [ ] State reconstruction equals final state from traditional approach

### 6.2 Statistical Rigor

- [ ] All metrics have >100 samples per workload
- [ ] Confidence intervals computed for all measurements
- [ ] Significance tests applied to all comparisons
- [ ] Outliers identified and documented

### 6.3 Coverage

- [ ] All 9 workloads implemented and executed
- [ ] All 3 metric categories measured (memory, overhead, scalability)
- [ ] Multiple problem sizes tested for scaling analysis

---

## 7. Risk Mitigation

### 7.1 Implementation Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| Different algorithms | Invalid comparison | Use same logic, only change state representation |
| Python GC interference | Noisy measurements | Force GC before measurements, use `gc.disable()` |
| OS scheduling variance | High std deviation | Increase sample size, run on isolated cores |

### 7.2 Statistical Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| Insufficient samples | Low confidence | Require N≥100, increase if CI too wide |
| Non-normal distributions | Invalid t-test | Use Mann-Whitney U test if normality fails |
| Multiple comparisons | False positives | Apply Bonferroni correction |

---

## 8. Deliverables Summary

**Code**:
- `experiments/benchmarks/baseline/` - Traditional SCORE implementation
- `experiments/benchmarks/atomik/` - ATOMiK delta-state implementation
- `experiments/benchmarks/metrics.py` - Metrics collection framework
- `experiments/benchmarks/runner.py` - Benchmark orchestration

**Data**:
- `experiments/data/memory/` - Memory efficiency raw data (CSV)
- `experiments/data/overhead/` - Computational overhead raw data (CSV)
- `experiments/data/scalability/` - Scalability raw data (CSV)

**Analysis**:
- `experiments/analysis/plots/` - Visualizations (box plots, violin plots, scaling curves)
- `experiments/analysis/statistics.md` - Statistical summary
- `reports/comparison.md` - Final SCORE comparison report

---

## References

1. Phase 1 Proof Verification Report (`reports/PROOF_VERIFICATION_REPORT.md`)
2. Theoretical Foundations (`docs/theory.md`)
3. Lean4 Proofs (`math/proofs/ATOMiK/*.lean`)

---

*Design complete: January 24, 2026*
*Ready for implementation (T2.2-T2.4)*
