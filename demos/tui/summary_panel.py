"""Summary panel: aggregate metrics and act results."""

from __future__ import annotations

from typing import Any

from rich.text import Text
from textual.widgets import Static

from demos.acts.base import ActResult


class SummaryPanel(Static):
    """Aggregate metrics, act result list, merge result."""

    def __init__(self, **kwargs: Any) -> None:
        super().__init__(**kwargs)
        self._results: list[ActResult] = []
        self._snapshots: list[dict[str, Any]] = []

    def on_mount(self) -> None:
        self.update(self._build_content())

    def update_results(
        self,
        results: list[ActResult],
        snapshots: list[dict[str, Any]],
    ) -> None:
        self._results = list(results)
        self._snapshots = list(snapshots)
        self.update(self._build_content())

    def _build_content(self) -> Text:
        text = Text()

        # Header
        text.append("  ▌ ", style="bold #cba6f7")
        text.append("Demo Summary", style="bold #cdd6f4")
        text.append("\n")
        text.append("  ────────────────────────────────\n", style="#45475a")

        # Node overview (logical partitions on single FPGA)
        hw = sum(1 for s in self._snapshots if s.get("is_hardware"))
        sim = len(self._snapshots) - hw
        total_throughput = sum(s.get("throughput_mops", 0) for s in self._snapshots)

        text.append("  Logical Nodes ", style="#6c7086")
        text.append(f"{len(self._snapshots)}", style="bold #cdd6f4")
        text.append("  (", style="#6c7086")
        text.append(f"{hw}", style="bold #a6e3a1")
        text.append(" HW / ", style="#6c7086")
        text.append(f"{sim}", style="bold #89b4fa")
        text.append(" SIM)\n", style="#6c7086")
        text.append("  ", style="")
        text.append("(partitions on single FPGA)\n", style="italic #585b70")

        text.append("  Aggregate ", style="#6c7086")
        text.append(f"{total_throughput / 1000:.2f}", style="bold #a6e3a1")
        text.append(" Gops/s\n", style="#6c7086")

        text.append("  ────────────────────────────────\n", style="#45475a")

        # Act results
        if not self._results:
            text.append("  ", style="")
            text.append("No acts run yet.", style="italic #6c7086")
        else:
            text.append("  Act Results:\n", style="#a6adc8")
            for r in self._results:
                if r.passed:
                    text.append("   ✓ ", style="bold #a6e3a1")
                else:
                    text.append("   ✗ ", style="bold #f38ba8")
                text.append(f"Act {r.act_number}", style="bold #cdd6f4")
                text.append(f" {r.title}\n", style="#a6adc8")

            passed = sum(1 for r in self._results if r.passed)
            total = len(self._results)
            text.append("\n")
            text.append("  Score ", style="#6c7086")
            score_color = "#a6e3a1" if passed == total else "#f9e2af"
            text.append(f"{passed}/{total}", style=f"bold {score_color}")
            text.append(" passed", style="#6c7086")

        return text
