#!/usr/bin/env python3
"""
ATOMiK Performance Benchmark Runner

Orchestrates firmware benchmarks, computes statistics, stores results
in append-only JSONL pool, and detects regressions.

Usage:
    python perf_runner.py --port /dev/ttyUSB1
    python perf_runner.py --port /dev/ttyUSB1 --skip-existing
    python perf_runner.py --port /dev/ttyUSB1 --no-compare
"""

import argparse
import json
import os
import statistics
import subprocess
import sys
import time
from collections import defaultdict
from datetime import datetime
from pathlib import Path

# Add parent paths for imports
SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent
sys.path.insert(0, str(SCRIPT_DIR))

from perf_capture import PerfCapture

# Data directories
DATA_DIR = PROJECT_ROOT / "hardware" / "experiments" / "data" / "hardware_perf"
POOL_FILE = DATA_DIR / "perf_pool.jsonl"
RUNS_DIR = DATA_DIR / "runs"

# Regression threshold
REGRESSION_PCT = 5.0


def git_metadata():
    """Collect git metadata."""
    meta = {"git_hash": "unknown", "git_dirty": True}
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--short", "HEAD"],
            capture_output=True, text=True, cwd=str(PROJECT_ROOT), timeout=5
        )
        if result.returncode == 0:
            meta["git_hash"] = result.stdout.strip()

        result = subprocess.run(
            ["git", "diff", "--quiet"],
            capture_output=True, cwd=str(PROJECT_ROOT), timeout=5
        )
        meta["git_dirty"] = result.returncode != 0
    except (subprocess.TimeoutExpired, FileNotFoundError):
        pass
    return meta


def firmware_size():
    """Get firmware ELF size in bytes."""
    elf_path = PROJECT_ROOT / "hardware" / "picorv32" / "firmware" / "build" / "atomik-fw.elf"
    try:
        return elf_path.stat().st_size
    except FileNotFoundError:
        return 0


def compute_stats(values):
    """Compute statistical summary for a list of cycle counts.

    Returns dict with min, max, mean, median, stdev, p5, p95, count.
    """
    if not values:
        return {"min": 0, "max": 0, "mean": 0.0, "median": 0.0,
                "stdev": 0.0, "p5": 0, "p95": 0, "count": 0}

    sorted_v = sorted(values)
    n = len(sorted_v)

    # Percentile indices (nearest rank)
    p5_idx = max(0, int(n * 0.05))
    p95_idx = min(n - 1, int(n * 0.95))

    return {
        "min": min(values),
        "max": max(values),
        "mean": round(statistics.mean(values), 2),
        "median": round(statistics.median(values), 2),
        "stdev": round(statistics.stdev(values), 2) if n > 1 else 0.0,
        "p5": sorted_v[p5_idx],
        "p95": sorted_v[p95_idx],
        "count": n,
    }


def remove_outliers(values, threshold=3.5):
    """Remove outliers using modified Z-score (MAD-based).

    Returns filtered list.
    """
    if len(values) < 3:
        return list(values)

    med = statistics.median(values)
    mad = statistics.median([abs(v - med) for v in values])

    if mad == 0:
        return list(values)

    return [v for v in values
            if abs(0.6745 * (v - med) / mad) <= threshold]


def group_measurements(measurements):
    """Group ##PERF measurements by (test, size/count) key.

    Returns dict: key -> list of cycle counts.
    """
    groups = defaultdict(list)

    for m in measurements:
        test = m.get("test", "unknown")
        # Build key from test + optional size/count
        parts = [test]
        if "size" in m:
            parts.append(str(m["size"]))
        elif "count" in m:
            parts.append(str(m["count"]))
        key = "_".join(parts)

        if "cycles" in m:
            groups[key].append(m["cycles"])

    return dict(groups)


def build_benchmarks(groups):
    """Compute statistics for each measurement group.

    Returns dict: key -> stats dict.
    """
    benchmarks = {}
    for key, raw_values in groups.items():
        filtered = remove_outliers(raw_values)
        stats = compute_stats(filtered)

        # Preserve size/count metadata from the key suffix
        # Keys look like: "atomik_load", "memcpy_sw_32", "burst_accum_100"
        # Only the last segment after the final underscore is a number for
        # tests that have a size or count parameter.
        parts = key.rsplit("_", 1)
        if len(parts) == 2 and parts[1].isdigit():
            suffix_val = int(parts[1])
            base_name = parts[0]
            mem_tests = ("memcpy_sw", "memcpy_atomik", "memset_sw",
                         "memset_atomik", "fingerprint", "change_detect",
                         "memcmp_sw")
            if base_name in mem_tests:
                stats["size"] = suffix_val
            else:
                stats["count"] = suffix_val

        stats["unit"] = "cycles"
        stats["outliers_removed"] = len(raw_values) - len(filtered)
        benchmarks[key] = stats

    return benchmarks


def build_comparisons(benchmarks):
    """Compare sw vs atomik variants at each size.

    Returns dict of comparison results.
    """
    comparisons = {}

    # Pairs: (sw_test, atomik_test, comparison_name)
    pairs = [
        ("memcpy_sw", "memcpy_atomik", "memcpy"),
        ("memset_sw", "memset_atomik", "memset"),
        ("memcmp_sw", "change_detect", "change_detect"),
    ]

    for sw_prefix, atomik_prefix, comp_name in pairs:
        # Find all sizes for this pair
        sw_keys = {k: v for k, v in benchmarks.items() if k.startswith(sw_prefix + "_")}
        atomik_keys = {k: v for k, v in benchmarks.items() if k.startswith(atomik_prefix + "_")}

        for sw_key, sw_stats in sw_keys.items():
            size = sw_stats.get("size")
            if size is None:
                continue

            atomik_key = f"{atomik_prefix}_{size}"
            if atomik_key not in atomik_keys:
                continue

            atomik_stats = atomik_keys[atomik_key]
            sw_mean = sw_stats["mean"]
            atomik_mean = atomik_stats["mean"]

            overhead_pct = 0.0
            if sw_mean > 0:
                overhead_pct = round((atomik_mean - sw_mean) / sw_mean * 100, 2)

            comparisons[f"{comp_name}_{size}"] = {
                "sw_mean": sw_mean,
                "atomik_mean": atomik_mean,
                "overhead_pct": overhead_pct,
                "size": size,
            }

    return comparisons


def load_previous_run():
    """Load the last entry from the JSONL pool.

    Returns dict or None.
    """
    if not POOL_FILE.exists():
        return None

    last_line = None
    with open(POOL_FILE, "r") as f:
        for line in f:
            line = line.strip()
            if line:
                last_line = line

    if last_line:
        try:
            return json.loads(last_line)
        except json.JSONDecodeError:
            return None
    return None


def detect_regressions(current, previous):
    """Compare current benchmarks to previous run.

    Returns list of (key, change_pct, direction) tuples.
    """
    if not previous or "benchmarks" not in previous:
        return []

    changes = []
    prev_bench = previous["benchmarks"]

    for key, stats in current.items():
        if key not in prev_bench:
            continue

        cur_mean = stats["mean"]
        prev_mean = prev_bench[key].get("mean", 0)

        if prev_mean == 0:
            continue

        change_pct = (cur_mean - prev_mean) / prev_mean * 100

        if abs(change_pct) > REGRESSION_PCT:
            direction = "REGRESSION" if change_pct > 0 else "IMPROVEMENT"
            changes.append((key, round(change_pct, 1), direction))

    return sorted(changes, key=lambda x: abs(x[1]), reverse=True)


def print_summary(run_data, changes):
    """Print formatted summary table."""
    benchmarks = run_data.get("benchmarks", {})
    existing = run_data.get("existing_tests", {})
    comparisons = run_data.get("comparisons", {})

    print("\n" + "=" * 70)
    print("  ATOMiK Performance Benchmark Results")
    print("=" * 70)
    print(f"  Timestamp:  {run_data['timestamp']}")
    print(f"  Git:        {run_data['git_hash']}" +
          (" (dirty)" if run_data["git_dirty"] else ""))
    print(f"  FW size:    {run_data['firmware_size_bytes']} bytes")

    soc = run_data.get("soc_config", {})
    if soc:
        print(f"  CPU:        {soc.get('cpu_khz', '?')} kHz")
        print(f"  ATOMiK:     {soc.get('atomik_khz', '?')} kHz")

    # Existing tests
    if existing:
        print("\n--- Existing Tests ---")
        for name, info in existing.items():
            status = "PASS" if info["passed"] == info["total"] else "FAIL"
            extra = ""
            if "t11_cycles" in info:
                extra = f"  (T11: {info['t11_cycles']} cycles)"
            print(f"  {name}: {info['passed']}/{info['total']} {status}{extra}")

    # ATOMiK core ops
    print("\n--- ATOMiK Core Ops (cycles) ---")
    print(f"  {'Test':<22} {'Mean':>8} {'Med':>8} {'Min':>6} {'Max':>6} {'StdDev':>8}")
    print(f"  {'-'*22} {'-'*8} {'-'*8} {'-'*6} {'-'*6} {'-'*8}")

    core_tests = ["atomik_load", "atomik_accum", "atomik_read",
                  "atomik_roundtrip", "mmio_write", "mmio_read"]
    for t in core_tests:
        if t in benchmarks:
            s = benchmarks[t]
            print(f"  {t:<22} {s['mean']:>8.1f} {s['median']:>8.1f} "
                  f"{s['min']:>6} {s['max']:>6} {s['stdev']:>8.1f}")

    # Memory ops by size
    print("\n--- Memory Operations (cycles) ---")
    mem_tests = ["memcpy_sw", "memcpy_atomik", "memset_sw", "memset_atomik",
                 "fingerprint", "change_detect", "memcmp_sw"]
    sizes = [32, 64, 128, 256]

    for sz in sizes:
        print(f"\n  Size = {sz} bytes:")
        print(f"  {'Test':<22} {'Mean':>8} {'Med':>8} {'Min':>6} {'Max':>6}")
        print(f"  {'-'*22} {'-'*8} {'-'*8} {'-'*6} {'-'*6}")
        for t in mem_tests:
            key = f"{t}_{sz}"
            if key in benchmarks:
                s = benchmarks[key]
                print(f"  {t:<22} {s['mean']:>8.1f} {s['median']:>8.1f} "
                      f"{s['min']:>6} {s['max']:>6}")

    # Comparisons
    if comparisons:
        print("\n--- SW vs ATOMiK Overhead ---")
        print(f"  {'Comparison':<22} {'SW':>8} {'ATOMiK':>8} {'Overhead':>10}")
        print(f"  {'-'*22} {'-'*8} {'-'*8} {'-'*10}")
        for key in sorted(comparisons.keys()):
            c = comparisons[key]
            print(f"  {key:<22} {c['sw_mean']:>8.1f} {c['atomik_mean']:>8.1f} "
                  f"{c['overhead_pct']:>+9.1f}%")

    # Burst & scaling
    print("\n--- Burst & Scaling (cycles) ---")
    burst_tests = sorted([k for k in benchmarks if k.startswith("burst_accum")])
    ckpt_tests = sorted([k for k in benchmarks
                         if k.startswith("checkpoint_create") or
                         k.startswith("checkpoint_verify")])

    if burst_tests or ckpt_tests:
        print(f"  {'Test':<28} {'Mean':>8} {'Med':>8} {'Min':>6} {'Max':>6}")
        print(f"  {'-'*28} {'-'*8} {'-'*8} {'-'*6} {'-'*6}")
        for t in burst_tests + ckpt_tests:
            s = benchmarks[t]
            print(f"  {t:<28} {s['mean']:>8.1f} {s['median']:>8.1f} "
                  f"{s['min']:>6} {s['max']:>6}")

    # CPU baselines
    print("\n--- CPU Baselines (cycles, count=1000) ---")
    cpu_tests = ["loop_overhead_1000", "func_call_1000", "xor_bench_1000"]
    print(f"  {'Test':<22} {'Mean':>8} {'Med':>8} {'Min':>6} {'Max':>6}")
    print(f"  {'-'*22} {'-'*8} {'-'*8} {'-'*6} {'-'*6}")
    for t in cpu_tests:
        if t in benchmarks:
            s = benchmarks[t]
            print(f"  {t:<22} {s['mean']:>8.1f} {s['median']:>8.1f} "
                  f"{s['min']:>6} {s['max']:>6}")

    # Regressions / improvements
    if changes:
        print("\n--- Changes vs Previous Run ---")
        for key, pct, direction in changes:
            marker = "!!!" if direction == "REGRESSION" else "+++"
            print(f"  {marker} {key}: {pct:+.1f}% ({direction})")
    elif POOL_FILE.exists():
        print("\n  No significant changes vs previous run.")

    print("\n" + "=" * 70)


def main():
    parser = argparse.ArgumentParser(description="ATOMiK Performance Benchmark Runner")
    parser.add_argument("--port", required=True, help="Serial port (e.g., /dev/ttyUSB1)")
    parser.add_argument("--baudrate", type=int, default=115200, help="UART baud rate")
    parser.add_argument("--skip-existing", action="store_true",
                        help="Skip running X/P existing tests")
    parser.add_argument("--no-compare", action="store_true",
                        help="Skip regression comparison")
    parser.add_argument("--reset", action="store_true",
                        help="Reload bitstream via openFPGALoader before connecting")
    parser.add_argument("--bitstream", type=str, default=None,
                        help="Path to .fs bitstream file (for --reset)")
    parser.add_argument("--init-delay", type=float, default=0,
                        help="Seconds to wait before connecting (e.g., 5 after bitstream load)")
    args = parser.parse_args()

    # Ensure data directories exist
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    RUNS_DIR.mkdir(parents=True, exist_ok=True)

    # Collect metadata
    git_meta = git_metadata()
    fw_size = firmware_size()
    timestamp = datetime.now().strftime("%Y-%m-%dT%H:%M:%S")

    # Reset board if requested
    if args.reset:
        bitstream = args.bitstream
        if not bitstream:
            # Default to known-good picotiny.fs location
            default_bs = Path.home() / "Projects" / "TangNano-9K-example" / "picotiny" / "picotiny.fs"
            if default_bs.exists():
                bitstream = str(default_bs)
            else:
                print("ERROR: No bitstream specified and default not found.")
                print("  Use --bitstream /path/to/picotiny.fs")
                return 1

        print(f"Resetting board: loading {bitstream}...")
        result = subprocess.run(
            ["openFPGALoader", "-b", "tangnano9k", bitstream],
            capture_output=True, text=True, timeout=15
        )
        if result.returncode != 0:
            print(f"ERROR: openFPGALoader failed: {result.stderr}")
            return 1
        print("  Bitstream loaded. Waiting for ISP bootloader timeout...")
        time.sleep(5)

    if args.init_delay > 0:
        print(f"Waiting {args.init_delay}s before connecting...")
        time.sleep(args.init_delay)

    print(f"Connecting to {args.port} at {args.baudrate} baud...")
    cap = PerfCapture(args.port, args.baudrate)

    try:
        # Run performance suite
        print("Running performance benchmark suite (press 'R')...")
        perf = cap.run_perf_suite()

        meta = perf["meta"]
        measurements = perf["measurements"]
        print(f"  Captured {len(measurements)} measurements")

        # Run existing tests
        existing = {}
        if not args.skip_existing:
            print("Running existing tests (X, P)...")
            existing = cap.run_existing_tests()
            hw = existing.get("atomik_hw", {})
            integ = existing.get("integration", {})
            print(f"  ATOMiK HW: {hw.get('passed', '?')}/{hw.get('total', '?')}")
            print(f"  Integration: {integ.get('passed', '?')}/{integ.get('total', '?')}")

    finally:
        cap.close()

    # Process measurements
    groups = group_measurements(measurements)
    benchmarks = build_benchmarks(groups)
    comparisons = build_comparisons(benchmarks)

    # Build run record
    run_data = {
        "timestamp": timestamp,
        "git_hash": git_meta["git_hash"],
        "git_dirty": git_meta["git_dirty"],
        "firmware_size_bytes": fw_size,
        "soc_config": {
            "cpu_khz": meta.get("cpu_khz", 25175),
            "atomik_khz": meta.get("atomik_khz", 81000),
            "n_banks": meta.get("banks", 1),
        },
        "existing_tests": existing,
        "benchmarks": benchmarks,
        "comparisons": comparisons,
    }

    # Compare to previous run
    changes = []
    if not args.no_compare:
        previous = load_previous_run()
        changes = detect_regressions(benchmarks, previous)

    # Store results
    # 1. Append to JSONL pool
    with open(POOL_FILE, "a") as f:
        f.write(json.dumps(run_data) + "\n")

    # 2. Save per-run snapshot
    run_filename = datetime.now().strftime("%Y-%m-%d_%H%M%S") + ".json"
    run_path = RUNS_DIR / run_filename
    with open(run_path, "w") as f:
        json.dump(run_data, f, indent=2)

    print(f"\nResults saved:")
    print(f"  Pool: {POOL_FILE}")
    print(f"  Snapshot: {run_path}")

    # Print summary
    print_summary(run_data, changes)

    # Exit code: 1 if any regressions
    has_regressions = any(d == "REGRESSION" for _, _, d in changes)
    return 1 if has_regressions else 0


if __name__ == "__main__":
    sys.exit(main())
