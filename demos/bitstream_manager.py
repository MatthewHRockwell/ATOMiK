"""Bitstream management and openFPGALoader wrapper.

Handles locating bitstream files and programming Tang Nano 9K boards.
"""

from __future__ import annotations

import logging
import shutil
import subprocess
from pathlib import Path

from demos.config import BITSTREAM_DIR, SWEEP_BITSTREAM_DIR, OPENFPGALOADER

log = logging.getLogger(__name__)


def locate_bitstream(filename: str) -> Path | None:
    """Find a bitstream file, checking demo/bitstreams/ then sweep/impl/pnr/."""
    for directory in (BITSTREAM_DIR, SWEEP_BITSTREAM_DIR):
        path = directory / filename
        if path.exists():
            return path
    log.warning("Bitstream not found: %s", filename)
    return None


def openfpgaloader_available() -> bool:
    """Check whether openFPGALoader is installed and on PATH."""
    return shutil.which(OPENFPGALOADER) is not None


def program_board(
    bitstream_path: Path,
    cable_index: int = 0,
    *,
    board: str = "tangnano9k",
    timeout: float = 30.0,
) -> bool:
    """Program a Tang Nano 9K using openFPGALoader.

    Args:
        bitstream_path: Path to the .fs bitstream file.
        cable_index: 0-based cable index for multi-board setups.
        board: Board identifier for openFPGALoader.
        timeout: Maximum seconds to wait for programming.

    Returns:
        True if programming succeeded.
    """
    if not bitstream_path.exists():
        log.error("Bitstream file does not exist: %s", bitstream_path)
        return False

    cmd = [
        OPENFPGALOADER,
        "-b", board,
        "--cable-index", str(cable_index),
        str(bitstream_path),
    ]
    log.info("Programming: %s", " ".join(cmd))

    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        if result.returncode == 0:
            log.info("Programming succeeded for cable_index=%d", cable_index)
            return True
        else:
            log.error("Programming failed (rc=%d): %s", result.returncode, result.stderr)
            return False
    except FileNotFoundError:
        log.error("openFPGALoader not found. Install it or set OPENFPGALOADER_PATH.")
        return False
    except subprocess.TimeoutExpired:
        log.error("Programming timed out after %.0fs", timeout)
        return False
