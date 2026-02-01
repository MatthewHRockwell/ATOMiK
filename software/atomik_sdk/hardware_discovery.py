"""
Centralised hardware and tool discovery for ATOMiK.

All FPGA-related tool lookups, board detection, and serial-port
discovery live here so that ``cli.py``, ``hardware.py``,
``fpga_pipeline.py``, and ``phase6_hw_sweep.py`` share one
implementation.

Every function is a pure helper (no class state).
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Gowin EDA root
# ---------------------------------------------------------------------------

def find_gowin_root() -> Path | None:
    """Locate the Gowin EDA installation directory.

    Search order:
      1. ``GOWIN_HOME`` environment variable
      2. ``C:\\Gowin\\*`` and ``C:\\Program Files\\Gowin\\*`` (Windows)
      3. ``/opt/Gowin/*`` (Linux)

    Returns the versioned directory (e.g.
    ``Gowin_V1.9.11.03_Education_x64``) or ``None``.
    """
    env = os.environ.get("GOWIN_HOME")
    if env:
        p = Path(env)
        if p.is_dir():
            return p

    if sys.platform == "win32":
        probe_bases = [Path("C:/Gowin"), Path("C:/Program Files/Gowin")]
    else:
        probe_bases = [Path("/opt/Gowin")]

    for base in probe_bases:
        if base.is_dir():
            candidates = sorted(base.iterdir(), reverse=True)
            for c in candidates:
                if c.is_dir() and (c / "IDE" / "bin").is_dir():
                    return c
    return None


# ---------------------------------------------------------------------------
# Generic tool finder
# ---------------------------------------------------------------------------

_GOWIN_TOOL_SUBPATHS: dict[str, str] = {
    "gw_sh": "IDE/bin/gw_sh",
    "programmer_cli": "Programmer/bin/programmer_cli",
}


def find_tool(name: str, config_path: str | None = None) -> str | None:
    """Return an absolute path (or bare command name) for *name*.

    Priority:
      1. Explicit *config_path* (expanded, verified)
      2. ``shutil.which(name)``
      3. Gowin EDA subpath (with ``.exe`` suffix on Windows)
      4. ``OPENFPGALOADER_PATH`` env var (for openFPGALoader)
    """
    if config_path:
        expanded = os.path.expandvars(config_path)
        if os.path.isfile(expanded):
            return expanded
        if shutil.which(expanded):
            return expanded

    found = shutil.which(name)
    if found:
        return found

    # Gowin subpath fallback
    subpath = _GOWIN_TOOL_SUBPATHS.get(name)
    if subpath:
        root = find_gowin_root()
        if root:
            suffix = ".exe" if sys.platform == "win32" else ""
            candidate = root / (subpath + suffix)
            if candidate.exists():
                return str(candidate)

    # openFPGALoader env-var / MSYS2 fallback
    if name == "openFPGALoader":
        env_path = os.environ.get("OPENFPGALOADER_PATH")
        if env_path and os.path.isfile(env_path):
            return env_path
        if sys.platform == "win32":
            msys2 = Path(r"C:\msys64\ucrt64\bin\openFPGALoader.exe")
            if msys2.is_file():
                return str(msys2)

    return None


# ---------------------------------------------------------------------------
# Board detection
# ---------------------------------------------------------------------------

def detect_board() -> str | None:
    """Detect a connected Tang Nano 9K (or similar Gowin) FPGA board.

    Tries ``programmer_cli --scan-cables`` first, then
    ``openFPGALoader --detect`` as fallback.

    Returns a board identifier string (e.g. ``"tangnano9k"``) or
    ``None`` if no board is found.
    """
    prog = find_tool("programmer_cli")
    if prog:
        try:
            result = subprocess.run(
                [prog, "--scan-cables"],
                capture_output=True, text=True, timeout=10,
            )
            if "cable found" in result.stdout.lower():
                return "tangnano9k"
        except (subprocess.TimeoutExpired, OSError):
            pass

    openfpga = find_tool("openFPGALoader")
    if openfpga:
        try:
            result = subprocess.run(
                [openfpga, "--detect"],
                capture_output=True, text=True, timeout=10,
            )
            stdout_lower = result.stdout.lower()
            if "gw1nr" in stdout_lower or "tangnano9k" in stdout_lower:
                return "tangnano9k"
        except (subprocess.TimeoutExpired, OSError):
            pass

    return None


# ---------------------------------------------------------------------------
# Serial-port detection
# ---------------------------------------------------------------------------

_FTDI_VID = 0x0403
_FTDI_PID = 0x6010

_KEYWORD_HINTS = ("tang", "gowin", "ft2232")


def detect_com_port() -> str | None:
    """Auto-detect the serial (COM / ttyUSB) port for the FPGA board.

    Matches FTDI VID:PID ``0403:6010`` first, then falls back to
    description-keyword matching.
    """
    try:
        import serial.tools.list_ports
    except ImportError:
        return None

    for port in serial.tools.list_ports.comports():
        if port.vid == _FTDI_VID and port.pid == _FTDI_PID:
            return port.device

    # Keyword fallback
    for port in serial.tools.list_ports.comports():
        desc = (port.description or "").lower()
        if any(kw in desc for kw in _KEYWORD_HINTS):
            return port.device

    return None
