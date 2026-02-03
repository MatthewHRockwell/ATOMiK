"""Textual TUI application for the ATOMiK 3-Node Demo.

Keybindings:
    space / enter  — advance to next act
    r              — run all acts
    1-5            — jump to specific act
    q              — quit
"""

from __future__ import annotations

from typing import Any

from rich.text import Text
from textual.app import App, ComposeResult
from textual.binding import Binding
from textual.containers import Horizontal
from textual.widgets import Footer, Static

from demos.config import PALETTE
from demos.orchestrator import DemoMode, DemoOrchestrator
from demos.tui.comparison_panel import ComparisonPanel
from demos.tui.narration_bar import NarrationBar
from demos.tui.node_panel import NodePanel
from demos.tui.summary_panel import SummaryPanel
from demos.tui.throughput_chart import ThroughputChart


class TitleBar(Static):
    """macOS-style title bar with traffic lights."""

    def compose(self) -> ComposeResult:
        return []

    def on_mount(self) -> None:
        self._render_title_bar()

    def _render_title_bar(self) -> None:
        """Render the title bar with traffic lights and centered title."""
        text = Text()
        # Traffic lights (macOS style)
        text.append("  ")
        text.append("●", style="bold #ff5f57")  # Red
        text.append(" ")
        text.append("●", style="bold #febc2e")  # Yellow
        text.append(" ")
        text.append("●", style="bold #28c840")  # Green
        text.append("          ")
        # Centered title
        text.append("ATOMiK", style="bold #89b4fa")
        text.append("  ", style="#6c7086")
        text.append("Delta-State Computing in Silicon", style="#6c7086")
        text.append("  ", style="#6c7086")
        text.append("3-Node Demo", style="#a6adc8")
        self.update(text)


class DemoApp(App):
    """ATOMiK 3-Node VC Demo — Terminal Dashboard."""

    TITLE = "ATOMiK 3-Node Demo"
    CSS = f"""
    Screen {{
        background: {PALETTE['base']};
        color: {PALETTE['text']};
    }}
    TitleBar {{
        dock: top;
        height: 1;
        background: {PALETTE['surface0']};
        color: {PALETTE['text']};
        padding: 0 1;
    }}
    #hero-bar {{
        dock: top;
        height: 3;
        background: {PALETTE['mantle']};
        color: {PALETTE['text']};
        padding: 0 1;
    }}
    #node-row {{
        height: 1fr;
        min-height: 14;
    }}
    #bottom-row {{
        height: auto;
        max-height: 20;
    }}
    NodePanel {{
        width: 1fr;
        border: solid {PALETTE['surface1']};
        margin: 0 1;
        background: {PALETTE['surface0']};
    }}
    SummaryPanel {{
        width: 1fr;
        border: solid {PALETTE['surface1']};
        margin: 0 1;
        background: {PALETTE['surface0']};
    }}
    ThroughputChart {{
        width: 1fr;
        border: solid {PALETTE['surface1']};
        margin: 0 1;
        background: {PALETTE['surface0']};
    }}
    ComparisonPanel {{
        width: 1fr;
        border: solid {PALETTE['surface1']};
        margin: 0 1;
        background: {PALETTE['surface0']};
    }}
    NarrationBar {{
        dock: bottom;
        height: 3;
        background: {PALETTE['mantle']};
        color: {PALETTE['subtext0']};
        padding: 0 1;
        border-top: solid {PALETTE['surface1']};
    }}
    Footer {{
        background: {PALETTE['surface0']};
    }}
    """

    BINDINGS = [
        Binding("space", "next_act", "Next Act"),
        Binding("enter", "next_act", "Next Act", show=False),
        Binding("r", "run_all", "Run All"),
        Binding("1", "jump(1)", "Act 1", show=False),
        Binding("2", "jump(2)", "Act 2", show=False),
        Binding("3", "jump(3)", "Act 3", show=False),
        Binding("4", "jump(4)", "Act 4", show=False),
        Binding("5", "jump(5)", "Act 5", show=False),
        Binding("q", "quit", "Quit"),
    ]

    def __init__(
        self,
        mode: DemoMode = DemoMode.SIMULATE,
        presentation: bool = False,
    ) -> None:
        super().__init__()
        self._presentation = presentation
        self._next_act_index = 0
        self.orchestrator = DemoOrchestrator(mode=mode, on_event=self._on_demo_event)

    def compose(self) -> ComposeResult:
        yield TitleBar()
        yield Static(id="hero-bar")
        with Horizontal(id="node-row"):
            yield NodePanel(index=0, id="node-0")
            yield NodePanel(index=1, id="node-1")
            yield NodePanel(index=2, id="node-2")
        with Horizontal(id="bottom-row"):
            yield SummaryPanel(id="summary")
            yield ThroughputChart(id="chart")
            yield ComparisonPanel(id="comparison")
        yield NarrationBar(id="narration")
        yield Footer()

    def on_mount(self) -> None:
        self.orchestrator.setup()
        self._refresh_panels()
        self._update_hero_bar()
        narration = self.query_one("#narration", NarrationBar)
        hw = self.orchestrator.state.hw_count
        sim = self.orchestrator.state.sim_count
        narration.set_text(
            f"Ready. {hw} hardware + {sim} simulated node(s). "
            f"Press SPACE to step through acts, or R to run all."
        )

    def _update_hero_bar(self) -> None:
        """Update the hero metrics bar."""
        hero = self.query_one("#hero-bar", Static)
        text = Text()
        text.append("\n  ")
        # Device info
        text.append("Tang Nano 9K", style="bold #89b4fa")
        text.append("  │  ", style="#45475a")
        # Peak Throughput
        text.append("1.07", style="bold #a6e3a1")
        text.append(" Gops/s", style="#6c7086")
        text.append("  │  ", style="#45475a")
        # Hardware Cost
        text.append("$10", style="bold #a6e3a1")
        text.append(" FPGA", style="#6c7086")
        text.append("  │  ", style="#45475a")
        # Latency
        text.append("10.6", style="bold #a6e3a1")
        text.append(" ns", style="#6c7086")
        text.append("  │  ", style="#45475a")
        # Formal Proofs
        text.append("92", style="bold #cba6f7")
        text.append(" Proofs", style="#6c7086")
        text.append("  │  ", style="#45475a")
        # Total Operations (will be updated)
        total_deltas = sum(
            s.get("delta_count", 0) for s in self.orchestrator.snapshots()
        )
        text.append(f"{total_deltas:,}", style="bold #f9e2af")
        text.append(" ops", style="#6c7086")
        hero.update(text)

    def on_unmount(self) -> None:
        self.orchestrator.teardown()

    # -- Actions -------------------------------------------------------------

    def action_next_act(self) -> None:
        if self._next_act_index >= 5:
            narration = self.query_one("#narration", NarrationBar)
            narration.set_text("Demo complete. Press Q to quit.")
            return
        act_num = self._next_act_index + 1
        self._next_act_index += 1
        self._run_act(act_num)

    def action_run_all(self) -> None:
        results = self.orchestrator.run_all()
        self._next_act_index = 5
        self._refresh_panels()
        passed = sum(1 for r in results if r.passed)
        narration = self.query_one("#narration", NarrationBar)
        narration.set_text(f"All 5 acts complete: {passed}/5 passed.")

    def action_jump(self, act_number: int) -> None:
        self._run_act(act_number)

    # -- Internals -----------------------------------------------------------

    def _run_act(self, act_number: int) -> None:
        result = self.orchestrator.run_single_act(act_number)
        if result:
            self._refresh_panels()
            narration = self.query_one("#narration", NarrationBar)
            status = "PASS" if result.passed else "FAIL"
            narration.set_text(
                f"Act {result.act_number}: {result.title} [{status}] — {result.summary}"
            )

    def _refresh_panels(self) -> None:
        """Update all dashboard panels from current orchestrator state."""
        snapshots = self.orchestrator.snapshots()
        for i, snap in enumerate(snapshots):
            try:
                panel = self.query_one(f"#node-{i}", NodePanel)
                panel.update_from_snapshot(snap)
            except Exception:
                pass

        try:
            summary = self.query_one("#summary", SummaryPanel)
            summary.update_results(self.orchestrator.state.act_results, snapshots)
        except Exception:
            pass

        try:
            chart = self.query_one("#chart", ThroughputChart)
            chart.update_data(snapshots)
        except Exception:
            pass

        # Update hero bar with current ops count
        try:
            self._update_hero_bar()
        except Exception:
            pass

    def _on_demo_event(self, event: str, data: dict[str, Any]) -> None:
        """Called by orchestrator on state changes."""
        if event == "act_start" and self._presentation:
            try:
                narration = self.query_one("#narration", NarrationBar)
                narration.set_text(data.get("narration", ""))
            except Exception:
                pass
