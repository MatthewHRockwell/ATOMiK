"""Per-node panel widget for the TUI dashboard.

Displays: name, domain, N_BANKS, [HW]/[SIM] badge, hex state,
delta count, throughput, color-coded status.
"""

from __future__ import annotations

from typing import Any

from rich.text import Text
from textual.widgets import Static

from demo.config import NODE_CONFIGS

# Node accent colors (Catppuccin Mocha)
NODE_COLORS = ["#f9e2af", "#89b4fa", "#a6e3a1"]  # yellow, blue, green

# Domain taglines for VC context
DOMAIN_TAGLINES = {
    "Finance": "Instant P&L • Undo = compliance",
    "Sensor": "Fuse streams • Detect in ns",
    "Peak": "1 Gops/s on $10 hardware",
}

# Node semantics note
NODE_SEMANTICS_NOTE = "Logical partition on single FPGA"


class NodePanel(Static):
    """Widget showing the state of a single demo node."""

    def __init__(self, index: int = 0, **kwargs: Any) -> None:
        super().__init__(**kwargs)
        self._index = index
        self._snap: dict[str, Any] | None = None
        self._color = NODE_COLORS[index] if index < len(NODE_COLORS) else "#cdd6f4"
        if index < len(NODE_CONFIGS):
            cfg = NODE_CONFIGS[index]
            self._default_name = cfg.name
            self._default_domain = cfg.domain
        else:
            self._default_name = f"Node {index + 1}"
            self._default_domain = "—"

    def on_mount(self) -> None:
        self.update(self._build_content())

    def update_from_snapshot(self, snap: dict[str, Any]) -> None:
        self._snap = snap
        self.update(self._build_content())

    def _build_content(self) -> Text:
        text = Text()

        if not self._snap:
            text.append("  ▌ ", style=f"bold {self._color}")
            text.append(f"Logical {self._default_name}\n", style="bold #cdd6f4")
            text.append(f"  Domain: {self._default_domain}\n", style="#6c7086")
            text.append("  Status: Awaiting setup...", style="#6c7086")
            return text

        s = self._snap
        is_hw = s["is_hardware"]
        badge_style = "bold #a6e3a1" if is_hw else "bold #89b4fa"
        badge_text = " HW " if is_hw else " SIM "
        acc_status = "ZERO" if s["accumulator_zero"] else "NON-ZERO"
        domain = s["domain"]
        tagline = DOMAIN_TAGLINES.get(domain, "")

        # Header with colored accent - show "Logical Node N"
        text.append("  ▌ ", style=f"bold {self._color}")
        text.append(f"Logical {s['name']}", style="bold #cdd6f4")
        text.append("  ")
        text.append(badge_text, style=f"{badge_style} reverse")
        text.append("\n")

        # Domain and tagline
        text.append(f"  {domain}", style=f"bold {self._color}")
        if tagline:
            text.append(f"  {tagline}", style="italic #6c7086")
        text.append("\n")

        # Metrics section
        text.append("  ────────────────────────────\n", style="#45475a")

        # Banks and Frequency
        text.append("  Banks ", style="#6c7086")
        text.append(f"{s['n_banks']}", style="bold #cdd6f4")
        text.append("  │  ", style="#45475a")
        text.append("Freq ", style="#6c7086")
        text.append(f"{s['freq_mhz']:.1f}", style="bold #cdd6f4")
        text.append(" MHz\n", style="#6c7086")

        # Throughput (highlighted)
        text.append("  Throughput ", style="#6c7086")
        tp = s["throughput_mops"]
        if tp >= 1000:
            text.append(f"{tp / 1000:.2f}", style=f"bold {self._color}")
            text.append(" Gops/s", style="#6c7086")
        else:
            text.append(f"{tp:,.0f}", style=f"bold {self._color}")
            text.append(" Mops/s", style="#6c7086")
        text.append("\n")

        # State section
        text.append("  ────────────────────────────\n", style="#45475a")
        text.append("  State ", style="#6c7086")
        text.append(f"{s['state_hex']}", style="bold #cba6f7")
        text.append("\n")

        # Accumulator and Deltas
        text.append("  Acc ", style="#6c7086")
        acc_style = "#a6e3a1" if s["accumulator_zero"] else "#f9e2af"
        text.append(acc_status, style=f"bold {acc_style}")
        text.append("  │  ", style="#45475a")
        text.append("Deltas ", style="#6c7086")
        text.append(f"{s['delta_count']}", style="bold #cdd6f4")
        text.append("\n")

        # Last delta
        text.append("  Last Δ ", style="#6c7086")
        text.append(f"{s['last_delta_hex']}", style="#a6adc8")

        return text
