"""ASCII horizontal bar chart showing throughput per node."""

from __future__ import annotations

from typing import Any

from rich.text import Text
from textual.widgets import Static

_GOPS_TARGET = 1000.0  # 1 Gops/s reference line
_BAR_WIDTH = 30

# Node accent colors (Catppuccin Mocha)
NODE_COLORS = ["#f9e2af", "#89b4fa", "#a6e3a1"]  # yellow, blue, green


class ThroughputChart(Static):
    """Horizontal bar chart: N=4 / N=8 / N=16 vs 1 Gops/s line."""

    def __init__(self, **kwargs: Any) -> None:
        super().__init__(**kwargs)
        self._snapshots: list[dict[str, Any]] = []

    def on_mount(self) -> None:
        self.update(self._build_content())

    def update_data(self, snapshots: list[dict[str, Any]]) -> None:
        self._snapshots = list(snapshots)
        self.update(self._build_content())

    def _build_content(self) -> Text:
        text = Text()

        # Header
        text.append("  ▌ ", style="bold #89b4fa")
        text.append("Logical Node Throughput", style="bold #cdd6f4")
        text.append("\n")
        text.append("  ────────────────────────────────────────\n", style="#45475a")

        if not self._snapshots:
            text.append("  Awaiting data...", style="italic #6c7086")
            return text

        max_val = max(s.get("throughput_mops", 1) for s in self._snapshots)
        scale = max(max_val, _GOPS_TARGET) * 1.1  # 10% headroom

        for i, s in enumerate(self._snapshots):
            n = s.get("n_banks", 0)
            tp = s.get("throughput_mops", 0)
            bar_len = int(_BAR_WIDTH * tp / scale) if scale > 0 else 0
            bar = "█" * bar_len
            color = NODE_COLORS[i] if i < len(NODE_COLORS) else "#cdd6f4"

            # Node label
            text.append(f"  N={n:<2} ", style="#a6adc8")

            # Badge
            if s.get("is_hardware"):
                text.append("HW", style="bold #a6e3a1")
            else:
                text.append("SM", style="bold #89b4fa")

            text.append(" │", style="#45475a")

            # Bar
            text.append(bar, style=f"bold {color}")
            remaining = _BAR_WIDTH - bar_len
            text.append("░" * remaining, style="#313244")

            text.append("│ ", style="#45475a")

            # Value
            if tp >= 1000:
                text.append(f"{tp / 1000:.2f}", style=f"bold {color}")
                text.append(" G", style="#6c7086")
            else:
                text.append(f"{tp:>4.0f}", style=f"bold {color}")
                text.append(" M", style="#6c7086")
            text.append("\n")

        # 1 Gops/s reference line
        gops_pos = int(_BAR_WIDTH * _GOPS_TARGET / scale) if scale > 0 else _BAR_WIDTH
        text.append("       ", style="")
        text.append("  │", style="#45475a")
        text.append(" " * gops_pos, style="")
        text.append("▲", style="bold #f38ba8")
        text.append(" 1 Gops/s target\n", style="#f38ba8")

        # Aggregate
        text.append("  ────────────────────────────────────────\n", style="#45475a")
        total = sum(s.get("throughput_mops", 0) for s in self._snapshots)
        text.append("  Aggregate ", style="#6c7086")
        text.append(f"{total / 1000:.2f}", style="bold #a6e3a1")
        text.append(" Gops/s", style="#6c7086")

        return text
