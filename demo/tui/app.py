"""Textual TUI application for the ATOMiK 3-Node Demo.

Keybindings:
    space / enter  — advance to next act
    r              — run all acts
    1-5            — jump to specific act
    q              — quit
"""

from __future__ import annotations

from typing import Any

from textual.app import App, ComposeResult
from textual.binding import Binding
from textual.containers import Horizontal
from textual.widgets import Footer, Static

from demo.config import PALETTE
from demo.orchestrator import DemoMode, DemoOrchestrator
from demo.tui.narration_bar import NarrationBar
from demo.tui.node_panel import NodePanel
from demo.tui.summary_panel import SummaryPanel
from demo.tui.throughput_chart import ThroughputChart


class DemoApp(App):
    """ATOMiK 3-Node VC Demo — Terminal Dashboard."""

    TITLE = "ATOMiK 3-Node Demo"
    CSS = f"""
    Screen {{
        background: {PALETTE['base']};
        color: {PALETTE['text']};
    }}
    #header-bar {{
        dock: top;
        height: 3;
        background: {PALETTE['surface0']};
        color: {PALETTE['text']};
        text-align: center;
        padding: 1;
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
    }}
    SummaryPanel {{
        width: 1fr;
        border: solid {PALETTE['surface1']};
        margin: 0 1;
    }}
    ThroughputChart {{
        width: 1fr;
        border: solid {PALETTE['surface1']};
        margin: 0 1;
    }}
    NarrationBar {{
        dock: bottom;
        height: 4;
        background: {PALETTE['mantle']};
        color: {PALETTE['subtext0']};
        padding: 1;
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
        yield Static(
            "ATOMiK  Delta-State Computing in Silicon  3-Node Demo",
            id="header-bar",
        )
        with Horizontal(id="node-row"):
            yield NodePanel(index=0, id="node-0")
            yield NodePanel(index=1, id="node-1")
            yield NodePanel(index=2, id="node-2")
        with Horizontal(id="bottom-row"):
            yield SummaryPanel(id="summary")
            yield ThroughputChart(id="chart")
        yield NarrationBar(id="narration")
        yield Footer()

    def on_mount(self) -> None:
        self.orchestrator.setup()
        self._refresh_panels()
        narration = self.query_one("#narration", NarrationBar)
        hw = self.orchestrator.state.hw_count
        sim = self.orchestrator.state.sim_count
        narration.set_text(
            f"Ready. {hw} hardware + {sim} simulated node(s). "
            f"Press SPACE to step through acts, or R to run all."
        )

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

    def _on_demo_event(self, event: str, data: dict[str, Any]) -> None:
        """Called by orchestrator on state changes."""
        if event == "act_start" and self._presentation:
            try:
                narration = self.query_one("#narration", NarrationBar)
                narration.set_text(data.get("narration", ""))
            except Exception:
                pass
