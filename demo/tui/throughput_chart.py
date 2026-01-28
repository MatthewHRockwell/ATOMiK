"""ASCII horizontal bar chart showing throughput per node."""

from __future__ import annotations

from typing import Any

from rich.text import Text
from textual.widgets import Static

_GOPS_TARGET = 1000.0  # 1 Gops/s reference line
_BAR_WIDTH = 35


class ThroughputChart(Static):
    """Horizontal bar chart: N=4 / N=8 / N=16 vs 1 Gops/s line."""

    def __init__(self, **kwargs: Any) -> None:
        super().__init__(**kwargs)
        self._snapshots: list[dict[str, Any]] = []

    def on_mount(self) -> None:
        self.update(Text(self._build_content()))

    def update_data(self, snapshots: list[dict[str, Any]]) -> None:
        self._snapshots = list(snapshots)
        self.update(Text(self._build_content()))

    def _build_content(self) -> str:
        lines = [
            "  Throughput Scaling",
            "  ══════════════════════════════════════════",
        ]

        if not self._snapshots:
            lines.append("  Awaiting data...")
            return "\n".join(lines)

        max_val = max(s.get("throughput_mops", 1) for s in self._snapshots)
        scale = max(max_val, _GOPS_TARGET)

        for s in self._snapshots:
            n = s.get("n_banks", 0)
            tp = s.get("throughput_mops", 0)
            bar_len = int(_BAR_WIDTH * tp / scale) if scale > 0 else 0
            bar = "#" * bar_len
            badge = "HW" if s.get("is_hardware") else "SM"
            lines.append(
                f"  N={n:<2} [{badge}] |{bar:<{_BAR_WIDTH}}| {tp:>7,.0f} Mops/s"
            )

        # 1 Gops/s reference line
        gops_pos = int(_BAR_WIDTH * _GOPS_TARGET / scale) if scale > 0 else _BAR_WIDTH
        ruler = " " * gops_pos + "|"
        lines.append(f"          {' ' * 1}{ruler} 1 Gops/s")

        # Aggregate
        total = sum(s.get("throughput_mops", 0) for s in self._snapshots)
        lines.append("")
        lines.append(f"  Aggregate: {total:,.0f} Mops/s ({total / 1000:.2f} Gops/s)")

        return "\n".join(lines)
