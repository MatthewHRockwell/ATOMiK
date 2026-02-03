"""Demo configuration: node profiles, UART settings, tool paths."""

from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
REPO_ROOT = Path(__file__).resolve().parent.parent
BITSTREAM_DIR = REPO_ROOT / "demos" / "bitstreams"
SWEEP_BITSTREAM_DIR = REPO_ROOT / "hardware" / "sweep" / "impl" / "pnr"
SWEEP_RESULTS_PATH = REPO_ROOT / "hardware" / "sweep" / "sweep_results.json"

# openFPGALoader – resolve from PATH or a known install location
_OPENFPGA_HINTS = [
    "openFPGALoader",
    r"C:\msys64\mingw64\bin\openFPGALoader.exe",
]
OPENFPGALOADER = os.environ.get("OPENFPGALOADER_PATH", "openFPGALoader")

# ---------------------------------------------------------------------------
# UART defaults
# ---------------------------------------------------------------------------
UART_BAUDRATE = 115200
UART_TIMEOUT = 2.0
FTDI_VID = 0x0403
FTDI_PID = 0x6010  # Tang Nano 9K dual-channel FTDI

# ---------------------------------------------------------------------------
# Node configuration
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class NodeConfig:
    """Static configuration for a single demo node."""
    name: str
    domain: str
    n_banks: int
    freq_mhz: float
    throughput_mops: float
    bitstream_filename: str
    description: str
    color: str  # Catppuccin Mocha accent colour


NODE_CONFIGS: list[NodeConfig] = [
    NodeConfig(
        name="Node 1",
        domain="Finance",
        n_banks=4,
        freq_mhz=81.0,
        throughput_mops=324.0,
        bitstream_filename="project_N4_F81p0.fs",
        description="Tick processing + instant undo",
        color="#f9e2af",  # yellow
    ),
    NodeConfig(
        name="Node 2",
        domain="Sensor",
        n_banks=8,
        freq_mhz=67.5,
        throughput_mops=540.0,
        bitstream_filename="project_N8_F67p5.fs",
        description="Multi-stream fusion + alerts",
        color="#89b4fa",  # blue
    ),
    NodeConfig(
        name="Node 3",
        domain="Peak",
        n_banks=16,
        freq_mhz=67.5,
        throughput_mops=1069.5,
        bitstream_filename="project_N16_F67p5.fs",
        description="1 Gops/s milestone",
        color="#a6e3a1",  # green
    ),
]

# ---------------------------------------------------------------------------
# Catppuccin Mocha palette (subset used by TUI + web)
# ---------------------------------------------------------------------------
PALETTE = {
    "base": "#1e1e2e",
    "mantle": "#181825",
    "crust": "#11111b",
    "surface0": "#313244",
    "surface1": "#45475a",
    "surface2": "#585b70",
    "overlay0": "#6c7086",
    "text": "#cdd6f4",
    "subtext0": "#a6adc8",
    "green": "#a6e3a1",
    "yellow": "#f9e2af",
    "blue": "#89b4fa",
    "red": "#f38ba8",
    "mauve": "#cba6f7",
    "peach": "#fab387",
    "teal": "#94e2d5",
}
