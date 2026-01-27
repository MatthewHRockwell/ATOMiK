"""
Baseline Snapshot Management

Creates, compares, and updates baseline snapshots for regression
detection. Baselines are automatically created after the first
successful run per schema and updated on explicit request.
"""

from __future__ import annotations

import json
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass
class BaselineSnapshot:
    """A baseline snapshot of pipeline metrics for a schema."""
    schema_name: str
    created_at: str = ""
    run_count: int = 0
    metrics: dict[str, float] = field(default_factory=dict)
    test_counts: dict[str, int] = field(default_factory=dict)
    hardware_metrics: dict[str, float] = field(default_factory=dict)

    def __post_init__(self) -> None:
        if not self.created_at:
            self.created_at = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())

    def to_dict(self) -> dict[str, Any]:
        return {
            "schema_name": self.schema_name,
            "created_at": self.created_at,
            "run_count": self.run_count,
            "metrics": self.metrics,
            "test_counts": self.test_counts,
            "hardware_metrics": self.hardware_metrics,
        }

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> BaselineSnapshot:
        return cls(
            schema_name=data.get("schema_name", ""),
            created_at=data.get("created_at", ""),
            run_count=data.get("run_count", 0),
            metrics=data.get("metrics", {}),
            test_counts=data.get("test_counts", {}),
            hardware_metrics=data.get("hardware_metrics", {}),
        )


class BaselineManager:
    """
    Manages baseline snapshots for regression detection.

    Baselines are stored as JSON files in a configurable directory.
    Each schema has its own baseline file. Baselines are created
    automatically after the first successful run and can be updated
    explicitly.

    Example:
        >>> manager = BaselineManager(".atomik/baselines")
        >>> manager.create_baseline("trade_packet", current_metrics)
        >>> baseline = manager.get_baseline("trade_packet")
        >>> if baseline:
        ...     print(f"Baseline from {baseline.created_at}")
    """

    def __init__(self, baseline_dir: str | Path = ".atomik/baselines") -> None:
        self.baseline_dir = Path(baseline_dir)
        self.baseline_dir.mkdir(parents=True, exist_ok=True)

    def _baseline_path(self, schema_name: str) -> Path:
        return self.baseline_dir / f"{schema_name}_baseline.json"

    def get_baseline(self, schema_name: str) -> BaselineSnapshot | None:
        """Load a baseline snapshot for a schema."""
        path = self._baseline_path(schema_name)
        if not path.exists():
            return None
        with open(path, encoding="utf-8") as f:
            data = json.load(f)
        return BaselineSnapshot.from_dict(data)

    def create_baseline(
        self,
        schema_name: str,
        metrics: dict[str, Any],
        test_counts: dict[str, int] | None = None,
        hardware_metrics: dict[str, float] | None = None,
    ) -> BaselineSnapshot:
        """
        Create a new baseline snapshot.

        Args:
            schema_name: Schema identifier.
            metrics: Current run metrics.
            test_counts: Test pass/fail counts per language.
            hardware_metrics: Hardware synthesis metrics.

        Returns:
            The created BaselineSnapshot.
        """
        # Convert all metric values to float
        float_metrics: dict[str, float] = {}
        for k, v in metrics.items():
            try:
                float_metrics[k] = float(v)
            except (ValueError, TypeError):
                pass

        snapshot = BaselineSnapshot(
            schema_name=schema_name,
            run_count=1,
            metrics=float_metrics,
            test_counts=test_counts or {},
            hardware_metrics=hardware_metrics or {},
        )

        self._save(snapshot)
        return snapshot

    def update_baseline(
        self,
        schema_name: str,
        metrics: dict[str, Any],
        test_counts: dict[str, int] | None = None,
        hardware_metrics: dict[str, float] | None = None,
    ) -> BaselineSnapshot:
        """
        Update an existing baseline with new metrics.

        If no baseline exists, creates one. Otherwise updates
        the baseline using exponential moving average to smooth
        over run-to-run variance.

        Args:
            schema_name: Schema identifier.
            metrics: New run metrics.
            test_counts: Updated test counts.
            hardware_metrics: Updated hardware metrics.

        Returns:
            Updated BaselineSnapshot.
        """
        existing = self.get_baseline(schema_name)
        if existing is None:
            return self.create_baseline(
                schema_name, metrics, test_counts, hardware_metrics
            )

        # EMA with alpha=0.3 (70% old, 30% new)
        alpha = 0.3
        for k, v in metrics.items():
            try:
                new_val = float(v)
            except (ValueError, TypeError):
                continue
            old_val = existing.metrics.get(k, new_val)
            existing.metrics[k] = old_val * (1 - alpha) + new_val * alpha

        if test_counts:
            existing.test_counts.update(test_counts)
        if hardware_metrics:
            for k, v in hardware_metrics.items():
                old_val = existing.hardware_metrics.get(k, v)
                existing.hardware_metrics[k] = old_val * (1 - alpha) + v * alpha

        existing.run_count += 1
        self._save(existing)
        return existing

    def create_if_missing(
        self,
        schema_name: str,
        metrics: dict[str, Any],
        test_counts: dict[str, int] | None = None,
        hardware_metrics: dict[str, float] | None = None,
    ) -> tuple[BaselineSnapshot, bool]:
        """
        Create a baseline only if none exists.

        Returns:
            Tuple of (snapshot, created) where created is True if new.
        """
        existing = self.get_baseline(schema_name)
        if existing is not None:
            return existing, False
        snapshot = self.create_baseline(
            schema_name, metrics, test_counts, hardware_metrics
        )
        return snapshot, True

    def delete_baseline(self, schema_name: str) -> bool:
        """Delete a baseline snapshot. Returns True if found."""
        path = self._baseline_path(schema_name)
        if path.exists():
            path.unlink()
            return True
        return False

    def list_baselines(self) -> list[str]:
        """List all schema names that have baselines."""
        return [
            p.stem.replace("_baseline", "")
            for p in self.baseline_dir.glob("*_baseline.json")
        ]

    def _save(self, snapshot: BaselineSnapshot) -> None:
        """Persist a baseline snapshot to disk."""
        path = self._baseline_path(snapshot.schema_name)
        with open(path, "w", encoding="utf-8") as f:
            json.dump(snapshot.to_dict(), f, indent=2)
