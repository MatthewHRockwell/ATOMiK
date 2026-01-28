"""Summary panel: aggregate metrics and act results."""

from __future__ import annotations

from typing import Any

from rich.text import Text
from textual.widgets import Static

from demo.acts.base import ActResult


class SummaryPanel(Static):
    """Aggregate metrics, act result list, merge result."""

    def __init__(self, **kwargs: Any) -> None:
        super().__init__(**kwargs)
        self._results: list[ActResult] = []
        self._snapshots: list[dict[str, Any]] = []

    def on_mount(self) -> None:
        self.update(Text(self._build_content()))

    def update_results(
        self,
        results: list[ActResult],
        snapshots: list[dict[str, Any]],
    ) -> None:
        self._results = list(results)
        self._snapshots = list(snapshots)
        self.update(Text(self._build_content()))

    def _build_content(self) -> str:
        lines = ["  Demo Summary", "  ════════════════════════"]

        # Node overview
        hw = sum(1 for s in self._snapshots if s.get("is_hardware"))
        sim = len(self._snapshots) - hw
        total_throughput = sum(s.get("throughput_mops", 0) for s in self._snapshots)
        lines.append(f"  Nodes: {len(self._snapshots)} ({hw} HW / {sim} SIM)")
        lines.append(f"  Total throughput: {total_throughput:,.0f} Mops/s")
        lines.append(f"  ({total_throughput / 1000:.2f} Gops/s aggregate)")
        lines.append("")

        # Act results
        if not self._results:
            lines.append("  No acts run yet.")
        else:
            lines.append("  Act Results:")
            for r in self._results:
                icon = "+" if r.passed else "X"
                lines.append(f"    [{icon}] Act {r.act_number}: {r.title}")
                lines.append(f"        {r.summary}")

            passed = sum(1 for r in self._results if r.passed)
            total = len(self._results)
            lines.append("")
            lines.append(f"  Score: {passed}/{total} passed")

        return "\n".join(lines)
