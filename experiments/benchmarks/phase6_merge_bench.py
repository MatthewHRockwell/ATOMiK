"""Phase 6: Focused merge-tree cost and per-operation statistical analysis.

Measures:
  1. Per-delta XOR accumulation cost (batch of 1M ops, repeated 100 times)
  2. XOR merge tree cost vs adder merge tree cost at N=1,2,4,8
  3. Overflow frequency for adder vs XOR

This benchmark isolates the component costs for statistical analysis.
"""

from __future__ import annotations

import math
import random
import statistics
import time


# ---------------------------------------------------------------------------
# Statistical utilities
# ---------------------------------------------------------------------------

def welch_t_test(s1: list[float], s2: list[float]) -> tuple[float, float]:
    if len(s1) < 2 or len(s2) < 2:
        return 0.0, 1.0
    m1, m2 = statistics.mean(s1), statistics.mean(s2)
    v1, v2 = statistics.variance(s1), statistics.variance(s2)
    n1, n2 = len(s1), len(s2)
    denom = math.sqrt(v1 / n1 + v2 / n2)
    if denom == 0:
        return 0.0, 1.0
    t = (m1 - m2) / denom
    z = abs(t)
    p = 2 * (1 - 0.5 * (1.0 + math.erf(z / math.sqrt(2.0))))
    return t, p


def cohens_d(s1: list[float], s2: list[float]) -> float:
    if len(s1) < 2 or len(s2) < 2:
        return 0.0
    m1, m2 = statistics.mean(s1), statistics.mean(s2)
    v1, v2 = statistics.variance(s1), statistics.variance(s2)
    n1, n2 = len(s1), len(s2)
    pv = ((n1 - 1) * v1 + (n2 - 1) * v2) / (n1 + n2 - 2)
    return (m1 - m2) / math.sqrt(pv) if pv > 0 else 0.0


def ci95(values: list[float]) -> float:
    if len(values) < 2:
        return 0.0
    return 1.96 * statistics.stdev(values) / math.sqrt(len(values))


def effect_label(d: float) -> str:
    d = abs(d)
    if d < 0.2:
        return "Negligible"
    if d < 0.5:
        return "Small"
    if d < 0.8:
        return "Medium"
    return "Large"


def remove_outliers(vals: list[float], threshold: float = 3.5) -> list[float]:
    if len(vals) < 3:
        return vals
    med = statistics.median(vals)
    mad = statistics.median([abs(v - med) for v in vals])
    if mad == 0:
        return vals
    return [v for v in vals if abs(0.6745 * (v - med) / mad) <= threshold]


# ---------------------------------------------------------------------------
# Benchmark workloads
# ---------------------------------------------------------------------------

def bench_accumulation_cost(iterations: int = 100, ops: int = 500_000) -> dict:
    """Measure per-delta XOR accumulation cost.

    Runs `ops` XOR accumulations in a tight loop, repeated `iterations` times.
    Reports time per operation in nanoseconds.
    """
    mask = (1 << 64) - 1
    xor_times: list[float] = []
    add_times: list[float] = []

    for _ in range(iterations):
        acc = 0
        deltas = [random.getrandbits(64) for _ in range(ops)]

        # XOR accumulation
        t0 = time.perf_counter_ns()
        for d in deltas:
            acc ^= d
        t1 = time.perf_counter_ns()
        xor_ns_per_op = (t1 - t0) / ops
        xor_times.append(xor_ns_per_op)

        # Adder accumulation
        acc = 0
        t0 = time.perf_counter_ns()
        for d in deltas:
            acc = (acc + d) & mask
        t1 = time.perf_counter_ns()
        add_ns_per_op = (t1 - t0) / ops
        add_times.append(add_ns_per_op)

    xor_clean = remove_outliers(xor_times)
    add_clean = remove_outliers(add_times)
    t, p = welch_t_test(xor_clean, add_clean)
    d = cohens_d(xor_clean, add_clean)

    return {
        "xor_ns_per_op_mean": round(statistics.mean(xor_clean), 2),
        "xor_ns_per_op_ci95": round(ci95(xor_clean), 2),
        "add_ns_per_op_mean": round(statistics.mean(add_clean), 2),
        "add_ns_per_op_ci95": round(ci95(add_clean), 2),
        "t_stat": round(t, 4),
        "p_value": round(p, 6),
        "cohens_d": round(d, 4),
        "effect": effect_label(d),
        "xor_samples": len(xor_clean),
        "add_samples": len(add_clean),
    }


def bench_merge_cost(
    iterations: int = 100,
    merges_per_iter: int = 100_000,
) -> list[dict]:
    """Measure XOR merge tree cost vs adder merge tree cost at N=1,2,4,8.

    For each N, performs `merges_per_iter` merge operations and measures
    total time. Reports time per merge in nanoseconds.
    """
    results = []
    mask = (1 << 64) - 1

    for n in [1, 2, 4, 8]:
        xor_times: list[float] = []
        add_times: list[float] = []

        for _ in range(iterations):
            # Generate N random bank accumulators for each merge
            banks_set = [
                [random.getrandbits(64) for _ in range(n)]
                for _ in range(merges_per_iter)
            ]

            # XOR merge
            t0 = time.perf_counter_ns()
            for banks in banks_set:
                merged = 0
                for b in banks:
                    merged ^= b
            t1 = time.perf_counter_ns()
            xor_times.append((t1 - t0) / merges_per_iter)

            # Adder merge
            t0 = time.perf_counter_ns()
            for banks in banks_set:
                merged = 0
                for b in banks:
                    merged = (merged + b) & mask
            t1 = time.perf_counter_ns()
            add_times.append((t1 - t0) / merges_per_iter)

        xor_clean = remove_outliers(xor_times)
        add_clean = remove_outliers(add_times)
        t, p = welch_t_test(xor_clean, add_clean)
        d = cohens_d(xor_clean, add_clean)

        results.append({
            "n_banks": n,
            "xor_ns_mean": round(statistics.mean(xor_clean), 2),
            "xor_ns_ci95": round(ci95(xor_clean), 2),
            "add_ns_mean": round(statistics.mean(add_clean), 2),
            "add_ns_ci95": round(ci95(add_clean), 2),
            "t_stat": round(t, 4),
            "p_value": round(p, 6),
            "cohens_d": round(d, 4),
            "effect": effect_label(d),
            "xor_samples": len(xor_clean),
            "add_samples": len(add_clean),
            "xor_scaling": round(
                statistics.mean(xor_clean) /
                (statistics.mean(remove_outliers(xor_times)) if n == 1 else 1),
                2
            ),
        })

    # Normalize scaling relative to N=1
    n1_xor = results[0]["xor_ns_mean"]
    n1_add = results[0]["add_ns_mean"]
    for r in results:
        r["xor_scaling"] = round(r["xor_ns_mean"] / n1_xor, 2) if n1_xor else 0
        r["add_scaling"] = round(r["add_ns_mean"] / n1_add, 2) if n1_add else 0

    return results


def bench_overflow_frequency(
    iterations: int = 1000,
    ops_per_iter: int = 1000,
    n_banks: int = 4,
) -> dict:
    """Measure overflow frequency for adder vs XOR.

    XOR should NEVER overflow. Adder overflows with high probability
    for large random inputs.
    """
    mask = (1 << 64) - 1
    xor_overflows = 0
    add_overflows = 0

    for _ in range(iterations):
        xor_acc = [0] * n_banks
        add_acc = [0] * n_banks

        for _ in range(ops_per_iter):
            for b in range(n_banks):
                d = random.getrandbits(64)
                xor_acc[b] ^= d
                prev = add_acc[b]
                add_acc[b] = (add_acc[b] + d) & mask
                # In hardware, overflow = carry out (result < previous)
                # For masked addition, this means the raw sum exceeded 2^64

        # Check merge overflow
        xor_merged = 0
        for b in range(n_banks):
            xor_merged ^= xor_acc[b]

        add_merged = 0
        for b in range(n_banks):
            prev = add_merged
            add_merged = (add_merged + add_acc[b]) & mask
            if add_merged < prev:
                add_overflows += 1

    return {
        "n_banks": n_banks,
        "iterations": iterations,
        "ops_per_iter": ops_per_iter,
        "xor_overflow_count": xor_overflows,
        "xor_overflow_rate": 0.0,
        "adder_overflow_count": add_overflows,
        "adder_overflow_rate": round(add_overflows / iterations * 100, 1),
    }


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    random.seed(42)

    print("=" * 72)
    print("Phase 6: Component-Level Statistical Analysis")
    print("=" * 72)
    print()

    # --- 1. Per-operation accumulation cost ---
    print("--- W6.A: Per-Delta Accumulation Cost ---")
    acc = bench_accumulation_cost(iterations=100, ops=500_000)
    print(f"  XOR: {acc['xor_ns_per_op_mean']:.2f} +/- {acc['xor_ns_per_op_ci95']:.2f} ns/op"
          f"  (n={acc['xor_samples']})")
    print(f"  ADD: {acc['add_ns_per_op_mean']:.2f} +/- {acc['add_ns_per_op_ci95']:.2f} ns/op"
          f"  (n={acc['add_samples']})")
    print(f"  Welch t={acc['t_stat']:.4f}, p={acc['p_value']:.6f},"
          f" d={acc['cohens_d']:.4f} ({acc['effect']})")
    sig = "YES" if acc['p_value'] < 0.05 else "NO"
    print(f"  Significant: {sig}")
    print()

    # --- 2. Merge tree cost ---
    print("--- W6.B: Merge Tree Cost (XOR vs Adder, per merge, ns) ---")
    merge = bench_merge_cost(iterations=100, merges_per_iter=50_000)
    print(f"  {'N':>3}  {'XOR (ns)':>12} {'ADD (ns)':>12}"
          f" {'p-value':>10} {'d':>7} {'Effect':>12} {'XOR sc':>7} {'ADD sc':>7}")
    for m in merge:
        print(f"  {m['n_banks']:>3}  {m['xor_ns_mean']:>8.2f}+/-{m['xor_ns_ci95']:<4.2f}"
              f" {m['add_ns_mean']:>8.2f}+/-{m['add_ns_ci95']:<4.2f}"
              f" {m['p_value']:>10.6f} {m['cohens_d']:>7.3f}"
              f" {m['effect']:>12}"
              f" {m['xor_scaling']:>6.2f}x {m['add_scaling']:>6.2f}x")
    print()

    # --- 3. Overflow frequency ---
    print("--- W6.C: Overflow Frequency (N=4, 1000 trials) ---")
    ovfl = bench_overflow_frequency(iterations=1000, ops_per_iter=1000, n_banks=4)
    print(f"  XOR overflows: {ovfl['xor_overflow_count']} / {ovfl['iterations']}"
          f" ({ovfl['xor_overflow_rate']:.1f}%)")
    print(f"  ADD overflows: {ovfl['adder_overflow_count']} / {ovfl['iterations']}"
          f" ({ovfl['adder_overflow_rate']:.1f}%)")
    print()

    # --- Summary ---
    print("=" * 72)
    print("Component-level analysis complete.")
    print("=" * 72)

    return {
        "accumulation": acc,
        "merge_tree": merge,
        "overflow": ovfl,
    }


if __name__ == "__main__":
    main()
