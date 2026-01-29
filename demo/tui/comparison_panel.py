"""Comparison panel: ATOMiK vs Traditional (SCORE) metrics."""

from __future__ import annotations

from typing import Any

from rich.text import Text
from textual.widgets import Static


# Comparison data from Phase 2 and Phase 6 benchmarks
COMPARISONS = [
    {
        "label": "Memory Traffic",
        "atomik": "32 KB",
        "traditional": "251 MB",
        "improvement": "99.99%",
        "note": "Matrix ops workload",
    },
    {
        "label": "Op Latency",
        "atomik": "10.6 ns",
        "traditional": "71 ns",
        "improvement": "35.7%",
        "note": "XOR vs adder chain",
    },
    {
        "label": "Parallel Efficiency",
        "atomik": "85%",
        "traditional": "0%",
        "improvement": "16x",
        "note": "Commutativity enables",
    },
    {
        "label": "Undo Overhead",
        "atomik": "0 bytes",
        "traditional": "Full copy",
        "improvement": "100%",
        "note": "Self-inverse: δ⊕δ=0",
    },
]


class ComparisonPanel(Static):
    """ATOMiK vs Traditional (SCORE) comparison metrics."""

    def __init__(self, **kwargs: Any) -> None:
        super().__init__(**kwargs)

    def on_mount(self) -> None:
        self.update(self._build_content())

    def _build_content(self) -> Text:
        text = Text()

        # Header
        text.append("  ▌ ", style="bold #f38ba8")
        text.append("ATOMiK vs Traditional", style="bold #cdd6f4")
        text.append("\n")
        text.append("  ──────────────────────────────────────────\n", style="#45475a")

        for comp in COMPARISONS:
            # Label and improvement badge
            text.append(f"  {comp['label']:<18}", style="#a6adc8")
            text.append(f" {comp['improvement']:>7}", style="bold #a6e3a1")
            text.append(" better\n", style="#6c7086")

            # ATOMiK value
            text.append("    ", style="")
            text.append("ATOMiK ", style="#6c7086")
            text.append(f"{comp['atomik']:<12}", style="bold #a6e3a1")

            # vs
            text.append(" vs ", style="#45475a")

            # Traditional value (struck through effect via dimming)
            text.append(f"{comp['traditional']}", style="#585b70")
            text.append("\n", style="")

        # Footer note
        text.append("  ──────────────────────────────────────────\n", style="#45475a")
        text.append("  ", style="")
        text.append("Single FPGA • Logical partitions\n", style="italic #6c7086")
        text.append("  ", style="")
        text.append("Source: Phase 2 & 6 (p<0.0001)", style="italic #585b70")

        return text
