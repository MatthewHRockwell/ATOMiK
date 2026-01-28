"""COM port discovery for Tang Nano 9K boards.

Scans serial ports filtering by FTDI VID/PID (0x0403:0x6010).
"""

from __future__ import annotations

import logging
from dataclasses import dataclass

from serial.tools import list_ports

from demo.config import FTDI_VID, FTDI_PID

log = logging.getLogger(__name__)


@dataclass
class DiscoveredBoard:
    """A detected Tang Nano 9K board."""
    port: str
    serial_number: str | None
    description: str
    cable_index: int  # 0-based for openFPGALoader --cable-index


def discover_boards() -> list[DiscoveredBoard]:
    """Scan for all connected Tang Nano 9K FTDI interfaces.

    Each physical board exposes two FTDI channels; we want the one used for
    UART (typically the second interface on Windows, the first /dev/ttyUSBx
    on Linux).  We de-duplicate by serial number so each physical board
    appears once.
    """
    boards: list[DiscoveredBoard] = []
    seen_serials: set[str] = set()

    for info in sorted(list_ports.comports(), key=lambda p: p.device):
        if info.vid == FTDI_VID and info.pid == FTDI_PID:
            serial_no = info.serial_number or info.device
            # FTDI dual-channel: skip duplicate serial (keep first match)
            if serial_no in seen_serials:
                continue
            seen_serials.add(serial_no)

            board = DiscoveredBoard(
                port=info.device,
                serial_number=info.serial_number,
                description=info.description or "Tang Nano 9K",
                cable_index=len(boards),
            )
            boards.append(board)
            log.info("Found board: %s on %s (S/N: %s)", board.description, board.port, serial_no)

    if not boards:
        log.warning("No Tang Nano 9K boards detected (FTDI %04X:%04X)", FTDI_VID, FTDI_PID)

    return boards
