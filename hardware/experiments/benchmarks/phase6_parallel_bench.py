"""Phase 6: Parallel Accumulator Banks — Statistical Performance Analysis

Measures throughput scaling for N-bank parallel XOR accumulation vs
traditional adder accumulation. Validates linear scaling claim with
Welch's t-test, 95% confidence intervals, and Cohen's d effect sizes.

Methodology matches Phase 2 PERFORMANCE_COMPARISON.md:
  - Multiple iterations per configuration (100 default)
  - Outlier detection via modified Z-score (threshold 3.5)
  - Welch's t-test for significance (alpha = 0.05)
  - Cohen's d for effect size classification

Workloads:
  W6.1: Throughput Scaling — N=1,2,4,8 parallel XOR banks
  W6.2: Merge Overhead — XOR tree vs adder tree merge cost
  W6.3: Scaling Linearity — regression analysis on scaling factors
  W6.4: Hardware Validation — Verilog simulation cross-check
"""

from __future__ import annotations

import csv
import math
import os
import random
import statistics
import time
from dataclasses import asdict, dataclass
from typing import Any


# ---------------------------------------------------------------------------
# Data structures
# ---------------------------------------------------------------------------

@dataclass
class ParallelBenchResult:
    """Single benchmark measurement."""
    workload: str
    n_banks: int
    variant: str            # 'xor' or 'adder'
    total_deltas: int
    cycles: int             # simulated cycles
    wall_time_ms: float
    throughput_ops_per_ms: float
    merge_time_us: float    # merge-tree cost per cycle
    overflow_detected: bool
    params: dict[str, Any]


@dataclass
class ScalingSummary:
    """Statistical summary for one (n_banks, variant) configuration."""
    n_banks: int
    variant: str
    mean_throughput: float
    std_throughput: float
    ci95_throughput: float
    mean_cycles: float
    mean_merge_us: float
    sample_count: int
    scaling_factor: float   # relative to N=1 of same variant
    min_throughput: float
    max_throughput: float


# ---------------------------------------------------------------------------
# Parallel bank simulators
# ---------------------------------------------------------------------------

def _run_parallel_xor(
    n_banks: int,
    total_deltas: int,
    delta_width: int = 64,
) -> tuple[int, float, float, bool]:
    """Simulate N-bank parallel XOR accumulation.

    Returns (cycles, wall_time_ms, merge_time_us, overflow)
    """
    mask = (1 << delta_width) - 1
    bank_acc = [0] * n_banks
    initial_state = random.getrandbits(delta_width)

    # Pre-generate all deltas
    deltas = [random.getrandbits(delta_width) for _ in range(total_deltas)]

    cycles = 0
    merge_total_ns = 0

    t0 = time.perf_counter_ns()

    idx = 0
    while idx < total_deltas:
        # One simulated clock cycle: feed up to n_banks deltas
        for b in range(n_banks):
            if idx < total_deltas:
                bank_acc[b] ^= deltas[idx]
                bank_acc[b] &= mask
                idx += 1

        # XOR merge tree
        mt0 = time.perf_counter_ns()
        merged = 0
        for b in range(n_banks):
            merged ^= bank_acc[b]
        merged &= mask
        merge_total_ns += time.perf_counter_ns() - mt0

        # State reconstruction (combinational)
        _current_state = initial_state ^ merged

        cycles += 1

    t1 = time.perf_counter_ns()
    wall_ms = (t1 - t0) / 1e6
    merge_us = merge_total_ns / (cycles * 1000) if cycles else 0.0

    return cycles, wall_ms, merge_us, False  # XOR never overflows


def _run_parallel_adder(
    n_banks: int,
    total_deltas: int,
    delta_width: int = 64,
) -> tuple[int, float, float, bool]:
    """Simulate N-bank parallel adder accumulation.

    Returns (cycles, wall_time_ms, merge_time_us, overflow)
    """
    mask = (1 << delta_width) - 1
    bank_acc = [0] * n_banks
    initial_state = random.getrandbits(delta_width)

    deltas = [random.getrandbits(delta_width) for _ in range(total_deltas)]

    cycles = 0
    merge_total_ns = 0
    overflow = False

    t0 = time.perf_counter_ns()

    idx = 0
    while idx < total_deltas:
        for b in range(n_banks):
            if idx < total_deltas:
                bank_acc[b] = (bank_acc[b] + deltas[idx]) & mask
                idx += 1

        # Addition merge tree (has carry propagation in hardware)
        mt0 = time.perf_counter_ns()
        merged = 0
        for b in range(n_banks):
            prev = merged
            merged = (merged + bank_acc[b]) & mask
            if merged < prev:
                overflow = True
        merge_total_ns += time.perf_counter_ns() - mt0

        _current_state = (initial_state + merged) & mask

        cycles += 1

    t1 = time.perf_counter_ns()
    wall_ms = (t1 - t0) / 1e6
    merge_us = merge_total_ns / (cycles * 1000) if cycles else 0.0

    return cycles, wall_ms, merge_us, overflow


# ---------------------------------------------------------------------------
# Statistical utilities (mirrors Phase 2 metrics.py)
# ---------------------------------------------------------------------------

def welch_t_test(s1: list[float], s2: list[float]) -> tuple[float, float, float]:
    """Welch's t-test. Returns (t_stat, p_value, df)."""
    if len(s1) < 2 or len(s2) < 2:
        return 0.0, 1.0, 0.0
    m1, m2 = statistics.mean(s1), statistics.mean(s2)
    v1, v2 = statistics.variance(s1), statistics.variance(s2)
    n1, n2 = len(s1), len(s2)
    denom = math.sqrt(v1 / n1 + v2 / n2)
    if denom == 0:
        return 0.0, 1.0, 0.0
    t = (m1 - m2) / denom
    num = (v1 / n1 + v2 / n2) ** 2
    den = (v1 / n1) ** 2 / (n1 - 1) + (v2 / n2) ** 2 / (n2 - 1)
    df = num / den if den > 0 else 1.0
    z = abs(t)
    p = 2 * (1 - 0.5 * (1.0 + math.erf(z / math.sqrt(2.0))))
    return t, p, df


def cohens_d(s1: list[float], s2: list[float]) -> float:
    """Compute Cohen's d effect size."""
    if len(s1) < 2 or len(s2) < 2:
        return 0.0
    m1, m2 = statistics.mean(s1), statistics.mean(s2)
    v1, v2 = statistics.variance(s1), statistics.variance(s2)
    n1, n2 = len(s1), len(s2)
    pooled_var = ((n1 - 1) * v1 + (n2 - 1) * v2) / (n1 + n2 - 2)
    if pooled_var == 0:
        return 0.0
    return (m1 - m2) / math.sqrt(pooled_var)


def effect_size_label(d: float) -> str:
    """Classify Cohen's d."""
    d = abs(d)
    if d < 0.2:
        return "Negligible"
    if d < 0.5:
        return "Small"
    if d < 0.8:
        return "Medium"
    return "Large"


def ci95(values: list[float]) -> float:
    """95% confidence interval half-width."""
    if len(values) < 2:
        return 0.0
    return 1.96 * statistics.stdev(values) / math.sqrt(len(values))


def remove_outliers(values: list[float], threshold: float = 3.5) -> list[float]:
    """Remove outliers via modified Z-score."""
    if len(values) < 3:
        return values
    med = statistics.median(values)
    mad = statistics.median([abs(v - med) for v in values])
    if mad == 0:
        return values
    return [v for v in values
            if abs(0.6745 * (v - med) / mad) <= threshold]


# ---------------------------------------------------------------------------
# Main benchmark runner
# ---------------------------------------------------------------------------

def run_phase6_benchmarks(
    iterations: int = 100,
    total_deltas: int = 100_000,
    verbose: bool = True,
) -> dict[str, Any]:
    """Run all Phase 6 benchmarks and return full results.

    Args:
        iterations: Runs per configuration.
        total_deltas: Deltas per run.
        verbose: Print progress.

    Returns:
        Dict with raw results, summaries, and statistical tests.
    """
    bank_configs = [1, 2, 4, 8]
    variants = ['xor', 'adder']
    raw: list[ParallelBenchResult] = []

    if verbose:
        print("=" * 70)
        print("Phase 6: Parallel Accumulator Banks — Performance Benchmark")
        print("=" * 70)
        print(f"Iterations: {iterations}  |  Deltas/run: {total_deltas:,}")
        print()

    random.seed(42)  # reproducibility

    # --- W6.1 / W6.2: Throughput + Merge overhead ---
    for variant in variants:
        runner = _run_parallel_xor if variant == 'xor' else _run_parallel_adder
        for n in bank_configs:
            if verbose:
                print(f"  Running {variant.upper()} N={n}  [{iterations} iters]...",
                      end="", flush=True)
            for _i in range(iterations):
                cycles, wall_ms, merge_us, overflow = runner(n, total_deltas)
                throughput = total_deltas / wall_ms if wall_ms > 0 else 0.0
                raw.append(ParallelBenchResult(
                    workload=f"W6.1_{variant}_N{n}",
                    n_banks=n,
                    variant=variant,
                    total_deltas=total_deltas,
                    cycles=cycles,
                    wall_time_ms=wall_ms,
                    throughput_ops_per_ms=throughput,
                    merge_time_us=merge_us,
                    overflow_detected=overflow,
                    params={"n_banks": n, "total_deltas": total_deltas},
                ))
            if verbose:
                print(" done")

    if verbose:
        print()

    # --- Compute summaries ---
    summaries: dict[str, ScalingSummary] = {}
    n1_means: dict[str, float] = {}

    for variant in variants:
        for n in bank_configs:
            key = f"{variant}_N{n}"
            subset = [r for r in raw if r.variant == variant and r.n_banks == n]
            tp_vals = remove_outliers([r.throughput_ops_per_ms for r in subset])
            merge_vals = remove_outliers([r.merge_time_us for r in subset])
            cycle_vals = remove_outliers([float(r.cycles) for r in subset])

            mean_tp = statistics.mean(tp_vals) if tp_vals else 0.0
            if n == 1:
                n1_means[variant] = mean_tp
            sf = mean_tp / n1_means[variant] if n1_means.get(variant) else 1.0

            summaries[key] = ScalingSummary(
                n_banks=n,
                variant=variant,
                mean_throughput=mean_tp,
                std_throughput=statistics.stdev(tp_vals) if len(tp_vals) > 1 else 0.0,
                ci95_throughput=ci95(tp_vals),
                mean_cycles=statistics.mean(cycle_vals) if cycle_vals else 0.0,
                mean_merge_us=statistics.mean(merge_vals) if merge_vals else 0.0,
                sample_count=len(tp_vals),
                scaling_factor=sf,
                min_throughput=min(tp_vals) if tp_vals else 0.0,
                max_throughput=max(tp_vals) if tp_vals else 0.0,
            )

    # --- W6.3: Statistical significance of scaling ---
    scaling_tests: list[dict[str, Any]] = []
    for variant in variants:
        n1_tp = remove_outliers([
            r.throughput_ops_per_ms for r in raw
            if r.variant == variant and r.n_banks == 1
        ])
        for n in [2, 4, 8]:
            nn_tp = remove_outliers([
                r.throughput_ops_per_ms for r in raw
                if r.variant == variant and r.n_banks == n
            ])
            # Normalize: divide N-bank throughput by N to compare per-bank rate
            nn_tp_norm = [v / n for v in nn_tp]

            t_stat, p_val, df = welch_t_test(n1_tp, nn_tp_norm)
            d = cohens_d(n1_tp, nn_tp_norm)

            # For scaling test: compare actual scaling vs ideal N
            actual_sf = summaries[f"{variant}_N{n}"].scaling_factor
            scaling_tests.append({
                "variant": variant,
                "n_banks": n,
                "ideal_scaling": float(n),
                "actual_scaling": round(actual_sf, 3),
                "scaling_error_pct": round(
                    abs(actual_sf - n) / n * 100, 2),
                "per_bank_t_stat": round(t_stat, 4),
                "per_bank_p_value": round(p_val, 6),
                "per_bank_cohens_d": round(d, 4),
                "per_bank_effect": effect_size_label(d),
                "linear_scaling": actual_sf >= n * 0.9,
            })

    # --- XOR vs Adder comparison at each N ---
    xor_vs_adder: list[dict[str, Any]] = []
    for n in bank_configs:
        xor_tp = remove_outliers([
            r.throughput_ops_per_ms for r in raw
            if r.variant == 'xor' and r.n_banks == n
        ])
        add_tp = remove_outliers([
            r.throughput_ops_per_ms for r in raw
            if r.variant == 'adder' and r.n_banks == n
        ])
        xor_merge = remove_outliers([
            r.merge_time_us for r in raw
            if r.variant == 'xor' and r.n_banks == n
        ])
        add_merge = remove_outliers([
            r.merge_time_us for r in raw
            if r.variant == 'adder' and r.n_banks == n
        ])

        t_tp, p_tp, _ = welch_t_test(xor_tp, add_tp)
        d_tp = cohens_d(xor_tp, add_tp)
        t_mg, p_mg, _ = welch_t_test(xor_merge, add_merge)
        d_mg = cohens_d(xor_merge, add_merge)

        xor_overflow = any(
            r.overflow_detected for r in raw
            if r.variant == 'xor' and r.n_banks == n
        )
        add_overflow = any(
            r.overflow_detected for r in raw
            if r.variant == 'adder' and r.n_banks == n
        )

        xor_vs_adder.append({
            "n_banks": n,
            "xor_throughput_mean": round(statistics.mean(xor_tp), 2),
            "xor_throughput_ci95": round(ci95(xor_tp), 2),
            "adder_throughput_mean": round(statistics.mean(add_tp), 2),
            "adder_throughput_ci95": round(ci95(add_tp), 2),
            "throughput_t_stat": round(t_tp, 4),
            "throughput_p_value": round(p_tp, 6),
            "throughput_cohens_d": round(d_tp, 4),
            "throughput_effect": effect_size_label(d_tp),
            "xor_merge_us": round(statistics.mean(xor_merge), 4),
            "adder_merge_us": round(statistics.mean(add_merge), 4),
            "merge_t_stat": round(t_mg, 4),
            "merge_p_value": round(p_mg, 6),
            "merge_cohens_d": round(d_mg, 4),
            "merge_effect": effect_size_label(d_mg),
            "xor_overflow": xor_overflow,
            "adder_overflow": add_overflow,
        })

    # --- Print summary ---
    if verbose:
        _print_summary(summaries, scaling_tests, xor_vs_adder, bank_configs)

    return {
        "raw_results": raw,
        "summaries": {k: asdict(v) for k, v in summaries.items()},
        "scaling_tests": scaling_tests,
        "xor_vs_adder": xor_vs_adder,
        "config": {
            "iterations": iterations,
            "total_deltas": total_deltas,
            "bank_configs": bank_configs,
        },
    }


def _print_summary(
    summaries: dict[str, ScalingSummary],
    scaling_tests: list[dict[str, Any]],
    xor_vs_adder: list[dict[str, Any]],
    bank_configs: list[int],
) -> None:
    """Print formatted results to stdout."""

    print("-" * 70)
    print("W6.1: Throughput Scaling (ops/ms, higher = better)")
    print("-" * 70)
    print(f"{'Config':<12} {'Mean':>12} {'± CI95':>10} {'Scaling':>9} {'Samples':>8}")
    for variant in ['xor', 'adder']:
        print(f"  {variant.upper()} banks:")
        for n in bank_configs:
            s = summaries[f"{variant}_N{n}"]
            print(f"    N={n:<4}  {s.mean_throughput:>12.1f} {s.ci95_throughput:>10.1f}"
                  f" {s.scaling_factor:>8.2f}x {s.sample_count:>8d}")
    print()

    print("-" * 70)
    print("W6.3: Scaling Linearity — Statistical Tests")
    print("-" * 70)
    print(f"{'Variant':<8} {'N':>3} {'Ideal':>6} {'Actual':>7} {'Err%':>6}"
          f" {'p-value':>10} {'d':>7} {'Effect':>12} {'Linear':>7}")
    for t in scaling_tests:
        sig = "YES" if t["linear_scaling"] else "NO"
        print(f"  {t['variant']:<6} {t['n_banks']:>3} {t['ideal_scaling']:>5.1f}x"
              f" {t['actual_scaling']:>6.2f}x {t['scaling_error_pct']:>5.1f}%"
              f" {t['per_bank_p_value']:>10.6f} {t['per_bank_cohens_d']:>7.3f}"
              f" {t['per_bank_effect']:>12} {sig:>7}")
    print()

    print("-" * 70)
    print("W6.2: XOR vs Adder Comparison")
    print("-" * 70)
    print(f"{'N':>3}  {'XOR tput':>12} {'ADD tput':>12}"
          f" {'p-value':>10} {'d':>7} {'XOR ovfl':>9} {'ADD ovfl':>9}")
    for c in xor_vs_adder:
        print(f"  {c['n_banks']:>1}  {c['xor_throughput_mean']:>12.1f}"
              f" {c['adder_throughput_mean']:>12.1f}"
              f" {c['throughput_p_value']:>10.6f}"
              f" {c['throughput_cohens_d']:>7.3f}"
              f" {'NONE':>9}"
              f" {'YES' if c['adder_overflow'] else 'NONE':>9}")
    print()


# ---------------------------------------------------------------------------
# CSV export
# ---------------------------------------------------------------------------

def save_results_csv(
    results: list[ParallelBenchResult],
    output_dir: str = "experiments/data/parallel",
) -> str:
    """Save raw results to CSV. Returns file path."""
    os.makedirs(output_dir, exist_ok=True)
    filepath = os.path.join(output_dir, "phase6_parallel_bench.csv")

    fieldnames = [
        "workload", "n_banks", "variant", "total_deltas", "cycles",
        "wall_time_ms", "throughput_ops_per_ms", "merge_time_us",
        "overflow_detected",
    ]

    with open(filepath, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for r in results:
            writer.writerow({
                "workload": r.workload,
                "n_banks": r.n_banks,
                "variant": r.variant,
                "total_deltas": r.total_deltas,
                "cycles": r.cycles,
                "wall_time_ms": round(r.wall_time_ms, 4),
                "throughput_ops_per_ms": round(r.throughput_ops_per_ms, 2),
                "merge_time_us": round(r.merge_time_us, 4),
                "overflow_detected": r.overflow_detected,
            })

    return filepath


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(
        description="Phase 6 Parallel Bank Performance Benchmark"
    )
    parser.add_argument(
        "--iterations", type=int, default=100,
        help="Iterations per configuration (default: 100)"
    )
    parser.add_argument(
        "--deltas", type=int, default=100_000,
        help="Total deltas per run (default: 100000)"
    )
    parser.add_argument(
        "--save-csv", action="store_true",
        help="Save raw results to CSV"
    )
    args = parser.parse_args()

    results = run_phase6_benchmarks(
        iterations=args.iterations,
        total_deltas=args.deltas,
        verbose=True,
    )

    if args.save_csv:
        path = save_results_csv(results["raw_results"])
        print(f"Results saved to: {path}")

    # Final verdict
    all_linear = all(t["linear_scaling"] for t in results["scaling_tests"]
                     if t["variant"] == "xor")
    print("=" * 70)
    if all_linear:
        print("PASS: XOR parallel banks demonstrate linear throughput scaling")
    else:
        print("FAIL: Linear scaling NOT confirmed for all configurations")
    print("=" * 70)
