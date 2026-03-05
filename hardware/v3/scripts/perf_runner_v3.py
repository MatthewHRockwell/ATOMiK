#!/usr/bin/env python3
"""
ATOMiK v3 Performance Benchmark Runner

Captures benchmarks from v3 SoC (RV64I + custom instructions),
computes statistics, stores results, and compares against v2 baseline.

Usage:
    python perf_runner_v3.py --port /dev/ttyUSB1
    python perf_runner_v3.py --port /dev/ttyUSB1 --compare-v2
"""

import argparse
import json
import os
import sys
import time
from datetime import datetime
from pathlib import Path

# Add parent paths for imports (reuse v2 infrastructure)
SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent.parent  # hardware/v3/scripts -> ATOMiK/
V2_SCRIPTS = PROJECT_ROOT / "hardware" / "scripts"
sys.path.insert(0, str(V2_SCRIPTS))

from perf_capture import PerfCapture

# Reuse analysis functions from v2 runner
sys.path.insert(0, str(V2_SCRIPTS))
import importlib.util
spec = importlib.util.spec_from_file_location("perf_runner", V2_SCRIPTS / "perf_runner.py")
perf_runner_v2 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(perf_runner_v2)

# Data directories
V3_DATA_DIR = PROJECT_ROOT / "hardware" / "v3" / "experiments" / "data" / "hardware_perf"
V3_POOL_FILE = V3_DATA_DIR / "perf_pool.jsonl"
V3_RUNS_DIR = V3_DATA_DIR / "runs"

V2_POOL_FILE = PROJECT_ROOT / "hardware" / "experiments" / "data" / "hardware_perf" / "perf_pool.jsonl"

# v3 SoC config
V3_SOC_CONFIG = {
    "cpu_khz": 21600,
    "atomik_khz": 21600,   # Same clock domain (no CDC in v3)
    "n_banks": 1,
    "arch": "RV64I",
    "cpu": "atomik_v3_cpu",
    "atomik_interface": "custom_instructions",
}


def git_metadata():
    """Collect git metadata."""
    import subprocess
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
    except Exception:
        pass
    return meta


def firmware_size():
    """Get v3 firmware ELF size."""
    elf = PROJECT_ROOT / "hardware" / "v3" / "soc" / "firmware" / "fw-flash" / "build" / "fw-flash.elf"
    try:
        return elf.stat().st_size
    except FileNotFoundError:
        return 0


def load_v2_baseline():
    """Load the latest v2 baseline from the v2 pool."""
    if not V2_POOL_FILE.exists():
        return None
    last_line = None
    with open(V2_POOL_FILE, "r") as f:
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


def compare_v2_v3(v2_data, v3_benchmarks):
    """Compare v2 MMIO-based vs v3 custom instruction performance.

    Returns list of comparison dicts.
    """
    if not v2_data or "benchmarks" not in v2_data:
        return []

    v2_bench = v2_data["benchmarks"]
    comparisons = []

    for key, v3_stats in v3_benchmarks.items():
        if key not in v2_bench:
            continue
        v2_mean = v2_bench[key].get("mean", 0)
        v3_mean = v3_stats["mean"]
        if v2_mean == 0:
            continue

        change_pct = round((v3_mean - v2_mean) / v2_mean * 100, 1)
        direction = "faster" if change_pct < 0 else "slower"

        comparisons.append({
            "test": key,
            "v2_mean": v2_mean,
            "v3_mean": v3_mean,
            "change_pct": change_pct,
            "direction": direction,
        })

    return sorted(comparisons, key=lambda x: x["change_pct"])


def print_v3_summary(run_data, v2_comparisons=None):
    """Print formatted v3 benchmark summary."""
    benchmarks = run_data.get("benchmarks", {})
    comparisons = run_data.get("comparisons", {})
    soc = run_data.get("soc_config", {})

    print("\n" + "=" * 72)
    print("  ATOMiK v3 Performance Benchmark Results")
    print("=" * 72)
    print(f"  Timestamp:  {run_data['timestamp']}")
    print(f"  Git:        {run_data['git_hash']}" +
          (" (dirty)" if run_data["git_dirty"] else ""))
    print(f"  FW size:    {run_data['firmware_size_bytes']} bytes")
    print(f"  CPU:        {soc.get('cpu', 'atomik_v3_cpu')} @ {soc.get('cpu_khz', '?')} kHz")
    print(f"  ATOMiK:     {soc.get('atomik_interface', 'custom_instructions')}")
    print(f"  Arch:       {soc.get('arch', 'RV64I')}")

    # ATOMiK core ops
    print("\n--- ATOMiK Core Ops (cycles) ---")
    print(f"  {'Test':<22} {'Mean':>8} {'Med':>8} {'Min':>6} {'Max':>6} {'StdDev':>8}")
    print(f"  {'-'*22} {'-'*8} {'-'*8} {'-'*6} {'-'*6} {'-'*8}")

    core_tests = ["atomik_load", "atomik_accum", "atomik_read",
                  "atomik_roundtrip", "atomik_swap"]
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

    # SW vs ATOMiK overhead
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

    # v2 vs v3 comparison
    if v2_comparisons:
        print("\n--- v2 (MMIO) vs v3 (Custom Instructions) ---")
        print(f"  {'Test':<28} {'v2':>8} {'v3':>8} {'Change':>10}")
        print(f"  {'-'*28} {'-'*8} {'-'*8} {'-'*10}")
        for c in v2_comparisons:
            print(f"  {c['test']:<28} {c['v2_mean']:>8.1f} {c['v3_mean']:>8.1f} "
                  f"{c['change_pct']:>+9.1f}%")

    print("\n" + "=" * 72)


def main():
    parser = argparse.ArgumentParser(description="ATOMiK v3 Performance Benchmark Runner")
    parser.add_argument("--port", required=True, help="Serial port (e.g., /dev/ttyUSB1)")
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument("--compare-v2", action="store_true",
                        help="Compare against v2 baseline")
    parser.add_argument("--skip-existing", action="store_true",
                        help="Skip X/P existing test commands")
    args = parser.parse_args()

    V3_DATA_DIR.mkdir(parents=True, exist_ok=True)
    V3_RUNS_DIR.mkdir(parents=True, exist_ok=True)

    git_meta = git_metadata()
    fw_size = firmware_size()
    timestamp = datetime.now().strftime("%Y-%m-%dT%H:%M:%S")

    print(f"Connecting to {args.port} at {args.baudrate} baud...")
    cap = PerfCapture(args.port, args.baudrate)

    try:
        print("Running v3 performance benchmark suite (press 'R')...")
        perf = cap.run_perf_suite()

        meta = perf["meta"]
        measurements = perf["measurements"]
        print(f"  Captured {len(measurements)} measurements")

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

    # Process measurements using v2's analysis functions
    groups = perf_runner_v2.group_measurements(measurements)
    benchmarks = perf_runner_v2.build_benchmarks(groups)
    comparisons = perf_runner_v2.build_comparisons(benchmarks)

    # Override SOC config with v3 values (firmware ##META may have some)
    soc_config = dict(V3_SOC_CONFIG)
    if "cpu_khz" in meta:
        soc_config["cpu_khz"] = meta["cpu_khz"]

    run_data = {
        "timestamp": timestamp,
        "git_hash": git_meta["git_hash"],
        "git_dirty": git_meta["git_dirty"],
        "firmware_size_bytes": fw_size,
        "soc_version": "v3",
        "soc_config": soc_config,
        "existing_tests": existing,
        "benchmarks": benchmarks,
        "comparisons": comparisons,
    }

    # v2 comparison
    v2_comparisons = []
    if args.compare_v2:
        v2_data = load_v2_baseline()
        if v2_data:
            v2_comparisons = compare_v2_v3(v2_data, benchmarks)
            run_data["v2_comparison"] = v2_comparisons

    # Save results
    with open(V3_POOL_FILE, "a") as f:
        f.write(json.dumps(run_data) + "\n")

    run_filename = datetime.now().strftime("%Y-%m-%d_%H%M%S") + ".json"
    run_path = V3_RUNS_DIR / run_filename
    with open(run_path, "w") as f:
        json.dump(run_data, f, indent=2)

    print(f"\nResults saved:")
    print(f"  Pool: {V3_POOL_FILE}")
    print(f"  Snapshot: {run_path}")

    print_v3_summary(run_data, v2_comparisons)

    return 0


if __name__ == "__main__":
    sys.exit(main())
