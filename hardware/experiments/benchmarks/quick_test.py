"""Quick test to verify benchmark execution works."""

import sys
import os
sys.path.insert(0, os.path.dirname(__file__))

from metrics import BenchmarkRunner
import baseline.workloads as baseline_wl
import atomik.workloads as atomik_wl

print("Testing baseline matrix workload...")
runner = BenchmarkRunner(output_dir="../data/test")

results = runner.run_workload(
    baseline_wl.MatrixWorkload,
    {'size': 8},
    'baseline',
    iterations=5,
    workload_run_params={'iterations': 5}
)
print(f"  Baseline: {len(results)} results collected")

results = runner.run_workload(
    atomik_wl.MatrixWorkload,
    {'size': 8},
    'atomik',
    iterations=5,
    workload_run_params={'iterations': 5}
)
print(f"  ATOMiK: {len(results)} results collected")

print("Test successful!")
