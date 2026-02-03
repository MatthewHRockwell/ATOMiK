"""Benchmark runner — executes all scenarios and collects results."""

from __future__ import annotations

from dataclasses import asdict
from typing import Any

from .scenarios import ALL_SCENARIOS


def run_all() -> list[dict[str, Any]]:
    """Run every scenario and return results as dicts."""
    results: list[dict[str, Any]] = []
    for scenario_cls in ALL_SCENARIOS:
        scenario = scenario_cls()
        result = scenario.run()
        results.append(asdict(result))
    return results
