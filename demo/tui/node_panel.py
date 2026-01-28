"""Per-node panel widget for the TUI dashboard.

Displays: name, domain, N_BANKS, [HW]/[SIM] badge, hex state,
delta count, throughput, color-coded status.
"""

from __future__ import annotations

from typing import Any

from rich.text import Text
from textual.widgets import Static

from demo.config import NODE_CONFIGS


class NodePanel(Static):
    """Widget showing the state of a single demo node."""

    def __init__(self, index: int = 0, **kwargs: Any) -> None:
        super().__init__(**kwargs)
        self._index = index
        self._snap: dict[str, Any] | None = None
        if index < len(NODE_CONFIGS):
            cfg = NODE_CONFIGS[index]
            self._default_name = cfg.name
            self._default_domain = cfg.domain
        else:
            self._default_name = f"Node {index + 1}"
            self._default_domain = "—"

    def on_mount(self) -> None:
        self.update(Text(self._render_content()))

    def update_from_snapshot(self, snap: dict[str, Any]) -> None:
        self._snap = snap
        self.update(Text(self._render_content()))

    def _render_content(self) -> str:
        if not self._snap:
            return (
                f"  {self._default_name}\n"
                f"  Domain: {self._default_domain}\n"
                f"  Status: Awaiting setup..."
            )

        s = self._snap
        badge = "[HW]" if s["is_hardware"] else "[SIM]"
        acc_status = "ZERO" if s["accumulator_zero"] else "NON-ZERO"

        lines = [
            f"  {s['name']} {badge}",
            f"  Domain: {s['domain']}",
            f"  N_BANKS: {s['n_banks']}",
            f"  Freq: {s['freq_mhz']:.1f} MHz",
            f"  Throughput: {s['throughput_mops']:,.0f} Mops/s",
            "  ──────────────────────",
            f"  State: {s['state_hex']}",
            f"  Acc: {acc_status}",
            f"  Deltas: {s['delta_count']}",
            f"  Last Δ: {s['last_delta_hex']}",
            f"  Elapsed: {s['elapsed_ms']:.1f} ms",
        ]
        return "\n".join(lines)
