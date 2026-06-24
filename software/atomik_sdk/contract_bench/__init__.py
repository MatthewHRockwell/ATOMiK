"""
atomik_sdk.contract_bench — prove ATOMiK hits a customer's aggregation benchmark.

Take a customer aggregation/reduction workload + the throughput-at-correctness
bar they gate commitment on, evaluate it against the SILICON-MEASURED parallel-
bank engine, and emit a PASS/SURPASS report (Markdown to hand over, JSON for
atomik.tech).

    >>> from atomik_sdk.contract_bench import WorkloadSpec, evaluate
    >>> spec = WorkloadSpec("Counter rollup", aggregation="sum",
    ...                     required_throughput=50e6, customer="Acme")
    >>> print(evaluate(spec).verdict)
    SURPASS
"""

from __future__ import annotations

from .harness import evaluate
from .report import BenchmarkReport, Gate
from .silicon import ABELIAN_AGGREGATIONS, ENGINE, SiliconMeasurement, is_abelian
from .spec import WorkloadSpec
from .sync import REFERENCE_SYNC, SyncReport, SyncSpec, evaluate_sync

__all__ = [
    "WorkloadSpec",
    "evaluate",
    "BenchmarkReport",
    "Gate",
    "ENGINE",
    "SiliconMeasurement",
    "is_abelian",
    "ABELIAN_AGGREGATIONS",
    # sync / replica-convergence archetype class
    "SyncSpec",
    "SyncReport",
    "evaluate_sync",
    "REFERENCE_SYNC",
]
