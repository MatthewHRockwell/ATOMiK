"""Benchmark API — trigger runs and return cached results."""

from __future__ import annotations

import json
from pathlib import Path

RESULTS_PATH = Path("software/demos/state_sync_benchmark/results/results.json")


def get_cached_results() -> dict | None:
    """Return cached benchmark results if available."""
    if RESULTS_PATH.exists():
        return json.loads(RESULTS_PATH.read_text(encoding="utf-8"))
    return None


def run_benchmarks() -> dict:
    """Run the benchmark suite and return results."""
    from software.demos.state_sync_benchmark.benchmarks import run_all
    from software.demos.state_sync_benchmark.report import save_results

    results = run_all()
    save_results(results, str(RESULTS_PATH.parent))
    return {"scenarios": results}
