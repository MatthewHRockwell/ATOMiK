"""
Metrics Collection API

Unified interface for collecting, normalizing, and validating
metrics from all pipeline stages and hardware benchmarks.
"""

from __future__ import annotations

import time
from dataclasses import dataclass
from typing import Any


@dataclass
class MetricEntry:
    """A single metric measurement."""
    name: str
    value: Any
    unit: str = ""
    source: str = ""
    timestamp: str = ""
    category: str = ""

    def to_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "value": self.value,
            "unit": self.unit,
            "source": self.source,
            "timestamp": self.timestamp,
            "category": self.category,
        }


class MetricsCollector:
    """
    Collects metrics from pipeline stages and hardware benchmarks.

    Provides a normalized API for recording metrics across all
    pipeline stages. Supports 4 metric categories:

    - pipeline: Token cost, generation time, differential savings
    - hardware: LUT/FF utilization, Fmax, timing margins
    - runtime: Operations/second, latency, throughput
    - quality: Test pass rate, lint error count
    """

    CATEGORIES = {"pipeline", "hardware", "runtime", "quality"}

    def __init__(self) -> None:
        self._entries: list[MetricEntry] = []

    def record(
        self,
        name: str,
        value: Any,
        unit: str = "",
        source: str = "",
        category: str = "pipeline",
    ) -> None:
        """Record a single metric."""
        self._entries.append(MetricEntry(
            name=name,
            value=value,
            unit=unit,
            source=source,
            timestamp=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            category=category,
        ))

    def record_pipeline(self, **kwargs: Any) -> None:
        """Record pipeline efficiency metrics."""
        for name, value in kwargs.items():
            self.record(name, value, category="pipeline", source="pipeline")

    def record_hardware(self, **kwargs: Any) -> None:
        """Record hardware synthesis metrics."""
        for name, value in kwargs.items():
            self.record(name, value, category="hardware", source="synthesis")

    def record_runtime(self, **kwargs: Any) -> None:
        """Record runtime performance metrics."""
        for name, value in kwargs.items():
            self.record(name, value, category="runtime", source="benchmark")

    def record_quality(self, **kwargs: Any) -> None:
        """Record quality metrics."""
        for name, value in kwargs.items():
            self.record(name, value, category="quality", source="verification")

    def get_by_category(self, category: str) -> list[MetricEntry]:
        """Get all metrics in a category."""
        return [e for e in self._entries if e.category == category]

    def get_summary(self) -> dict[str, dict[str, Any]]:
        """Get a categorized summary of all metrics."""
        summary: dict[str, dict[str, Any]] = {}
        for entry in self._entries:
            if entry.category not in summary:
                summary[entry.category] = {}
            summary[entry.category][entry.name] = entry.value
        return summary

    def to_flat_dict(self) -> dict[str, Any]:
        """Get all metrics as a flat dictionary."""
        return {e.name: e.value for e in self._entries}

    def get_all(self) -> list[dict[str, Any]]:
        """Get all metric entries as dicts."""
        return [e.to_dict() for e in self._entries]

    def merge(self, other: MetricsCollector) -> None:
        """Merge metrics from another collector."""
        self._entries.extend(other._entries)

    def clear(self) -> None:
        """Clear all collected metrics."""
        self._entries.clear()
