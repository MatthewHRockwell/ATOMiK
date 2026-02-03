"""CLI entry: python -m software.demos.state_sync_benchmark"""

from __future__ import annotations

from .benchmarks import run_all
from .report import print_results, save_results

OUTPUT_DIR = "software/demos/state_sync_benchmark/results"


def main() -> None:
    results = run_all()
    print_results(results)
    save_results(results, OUTPUT_DIR)
    print(f"Results saved to {OUTPUT_DIR}/")


if __name__ == "__main__":
    main()
