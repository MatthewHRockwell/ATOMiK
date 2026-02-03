"""DemoOrchestrator — manages all 3 nodes and runs the 5-act sequence.

Supports three modes:
    auto     — discover hardware; fall back to simulation for missing boards
    simulate — all nodes use the software simulator
    hardware — require all 3 physical boards (fail fast otherwise)
"""

from __future__ import annotations

import logging
import time
from dataclasses import dataclass, field
from enum import Enum
from typing import Any, Callable

from demos.acts import ALL_ACTS
from demos.acts.base import ActBase, ActResult
from demos.bitstream_manager import locate_bitstream, openfpgaloader_available, program_board
from demos.config import NODE_CONFIGS, NodeConfig, UART_BAUDRATE, UART_TIMEOUT
from demos.discovery import discover_boards, DiscoveredBoard
from demos.hardware_interface import ATOMiKHardware
from demos.node import NodeController
from demos.simulator import ATOMiKSimulator

log = logging.getLogger(__name__)

EventCallback = Callable[[str, dict[str, Any]], None]


class DemoMode(Enum):
    AUTO = "auto"
    SIMULATE = "simulate"
    HARDWARE = "hardware"


@dataclass
class DemoState:
    """Observable state for dashboards."""
    mode: DemoMode = DemoMode.SIMULATE
    nodes_ready: int = 0
    hw_count: int = 0
    sim_count: int = 0
    current_act: int = 0
    act_results: list[ActResult] = field(default_factory=list)
    running: bool = False
    error: str | None = None


class DemoOrchestrator:
    """Top-level controller for the 3-node VC demo."""

    def __init__(
        self,
        mode: DemoMode = DemoMode.AUTO,
        on_event: EventCallback | None = None,
    ) -> None:
        self.mode = mode
        self._on_event = on_event
        self.state = DemoState(mode=mode)
        self.nodes: list[NodeController] = []
        self._hw_handles: list[ATOMiKHardware] = []

    # -- Lifecycle -----------------------------------------------------------

    def setup(self) -> None:
        """Discover/create nodes. Call before run()."""
        self._emit("setup_start", {"mode": self.mode.value})

        if self.mode == DemoMode.SIMULATE:
            self._setup_all_simulated()
        elif self.mode == DemoMode.HARDWARE:
            self._setup_all_hardware()
        else:
            self._setup_auto()

        self.state.nodes_ready = len(self.nodes)
        self.state.hw_count = sum(1 for n in self.nodes if n.is_hardware)
        self.state.sim_count = sum(1 for n in self.nodes if not n.is_hardware)

        self._emit("setup_complete", {
            "nodes": self.state.nodes_ready,
            "hw": self.state.hw_count,
            "sim": self.state.sim_count,
        })

    def teardown(self) -> None:
        """Release hardware resources."""
        for hw in self._hw_handles:
            try:
                hw.close()
            except Exception:
                pass
        self._hw_handles.clear()
        self.nodes.clear()
        self._emit("teardown", {})

    # -- Running acts --------------------------------------------------------

    def run_all(self) -> list[ActResult]:
        """Execute the full 5-act sequence."""
        self.state.running = True
        self.state.act_results.clear()
        self._emit("demo_start", {"acts": len(ALL_ACTS)})

        results: list[ActResult] = []
        for act_cls in ALL_ACTS:
            result = self.run_act(act_cls)
            results.append(result)

        self.state.running = False
        passed = sum(1 for r in results if r.passed)
        self._emit("demo_complete", {"passed": passed, "total": len(results)})
        return results

    def run_act(self, act_cls: type[ActBase]) -> ActResult:
        """Run a single act."""
        act = act_cls()
        self.state.current_act = act.number
        self._emit("act_start", {
            "number": act.number,
            "title": act.title,
            "narration": act.narration,
        })

        try:
            result = act.run(self.nodes)
        except Exception as e:
            log.exception("Act %d failed", act.number)
            result = ActResult(
                act_number=act.number,
                title=act.title,
                passed=False,
                summary=f"Error: {e}",
                details=[str(e)],
            )

        self.state.act_results.append(result)
        self._emit("act_complete", {
            "number": result.act_number,
            "title": result.title,
            "passed": result.passed,
            "summary": result.summary,
        })
        return result

    def run_single_act(self, act_number: int) -> ActResult | None:
        """Run a specific act by number (1-5)."""
        for act_cls in ALL_ACTS:
            if act_cls.number == act_number:
                return self.run_act(act_cls)
        log.error("No act with number %d", act_number)
        return None

    # -- Node snapshots for dashboards ---------------------------------------

    def snapshots(self) -> list[dict[str, Any]]:
        """Return snapshot dicts for all nodes (for JSON serialisation)."""
        results = []
        for node in self.nodes:
            snap = node.snapshot()
            results.append({
                "name": snap.node_name,
                "domain": snap.domain,
                "n_banks": snap.n_banks,
                "freq_mhz": snap.freq_mhz,
                "throughput_mops": snap.throughput_mops,
                "is_hardware": snap.is_hardware,
                "state_hex": snap.state_hex,
                "accumulator_zero": snap.accumulator_zero,
                "delta_count": snap.delta_count,
                "last_delta_hex": snap.last_delta_hex,
                "elapsed_ms": snap.elapsed_ms,
            })
        return results

    # -- Setup strategies ----------------------------------------------------

    def _setup_all_simulated(self) -> None:
        """Create 3 simulated nodes."""
        log.info("Setting up 3 simulated nodes")
        for cfg in NODE_CONFIGS:
            # Small latency makes real-time updates visible in dashboard
            backend = ATOMiKSimulator(latency=0.005)
            self.nodes.append(NodeController(cfg, backend, on_event=self._on_event))

    def _setup_all_hardware(self) -> None:
        """Require all 3 boards to be present."""
        boards = discover_boards()
        if len(boards) < 3:
            raise RuntimeError(
                f"Hardware mode requires 3 boards, found {len(boards)}. "
                "Use --mode auto for graceful degradation."
            )
        self._provision_hardware(boards[:3])

    def _setup_auto(self) -> None:
        """Discover boards; simulate missing ones."""
        boards = discover_boards()
        log.info("Auto mode: found %d board(s)", len(boards))

        for i, cfg in enumerate(NODE_CONFIGS):
            if i < len(boards):
                try:
                    self._provision_single_hw(cfg, boards[i])
                    continue
                except Exception as e:
                    log.warning("Board %d setup failed (%s), falling back to sim", i, e)
            # Fall back to simulator
            backend = ATOMiKSimulator(latency=0.005)
            self.nodes.append(NodeController(cfg, backend, on_event=self._on_event))
            log.info("%s -> simulated", cfg.name)

    def _provision_hardware(self, boards: list[DiscoveredBoard]) -> None:
        """Program and connect to multiple boards."""
        for i, (cfg, board) in enumerate(zip(NODE_CONFIGS, boards)):
            self._provision_single_hw(cfg, board)

    def _provision_single_hw(self, cfg: NodeConfig, board: DiscoveredBoard) -> None:
        """Program one board and create a hardware-backed node."""
        bs_path = locate_bitstream(cfg.bitstream_filename)
        if bs_path and openfpgaloader_available():
            log.info("Programming %s with %s", cfg.name, cfg.bitstream_filename)
            if not program_board(bs_path, cable_index=board.cable_index):
                raise RuntimeError(f"Failed to program {cfg.name}")
            time.sleep(1.0)  # post-programming delay

        hw = ATOMiKHardware(
            board.port,
            baudrate=UART_BAUDRATE,
            timeout=UART_TIMEOUT,
        )
        self._hw_handles.append(hw)
        self.nodes.append(NodeController(cfg, hw, on_event=self._on_event))
        log.info("%s -> hardware on %s", cfg.name, board.port)

    # -- Events --------------------------------------------------------------

    def _emit(self, event: str, data: dict[str, Any]) -> None:
        if self._on_event:
            self._on_event(event, data)
