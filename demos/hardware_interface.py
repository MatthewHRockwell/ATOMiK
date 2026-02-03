"""NodeInterface protocol and ATOMiKHardware implementation.

Refactored from scripts/test_hardware.py to provide a clean Protocol that
both real hardware and the simulator can satisfy.
"""

from __future__ import annotations

import struct
import time
from typing import Protocol, runtime_checkable

import serial


# ---------------------------------------------------------------------------
# Protocol — shared contract for hardware + simulator
# ---------------------------------------------------------------------------

@runtime_checkable
class NodeInterface(Protocol):
    """Minimal interface to an ATOMiK accumulator (hardware or simulated)."""

    def load(self, initial_state: int) -> None:
        """Load a 64-bit initial state."""
        ...

    def accumulate(self, delta: int) -> None:
        """XOR-accumulate a 64-bit delta."""
        ...

    def read(self) -> int:
        """Read the current reconstructed state (initial XOR acc)."""
        ...

    def status(self) -> bool:
        """Return True when the accumulator is zero."""
        ...

    @property
    def is_hardware(self) -> bool:
        """True if backed by real FPGA hardware."""
        ...


# ---------------------------------------------------------------------------
# Hardware implementation (UART to Tang Nano 9K)
# ---------------------------------------------------------------------------

class ATOMiKHardware:
    """UART interface to ATOMiK Core v2 on the Tang Nano 9K FPGA.

    Protocol: every command is a 1-byte opcode optionally followed by an
    8-byte big-endian payload.

    Opcodes:
        L + 8B  -> Load initial state
        A + 8B  -> Accumulate delta
        R       -> Read state (returns 8B)
        S       -> Status (returns 1B, bit 7 = accumulator_zero)
        D       -> Debug: read initial_state (returns 8B)
    """

    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 2.0) -> None:
        self.port = port
        self.ser = serial.Serial(port, baudrate, timeout=timeout)
        time.sleep(0.2)  # connection stabilisation
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()

    # -- Protocol methods ---------------------------------------------------

    def load(self, initial_state: int) -> None:
        cmd = b"L" + struct.pack(">Q", initial_state & 0xFFFF_FFFF_FFFF_FFFF)
        self.ser.write(cmd)
        time.sleep(0.05)

    def accumulate(self, delta: int) -> None:
        cmd = b"A" + struct.pack(">Q", delta & 0xFFFF_FFFF_FFFF_FFFF)
        self.ser.write(cmd)
        time.sleep(0.05)

    def read(self) -> int:
        self.ser.reset_input_buffer()
        self.ser.write(b"R")
        time.sleep(0.1)
        data = self.ser.read(8)
        if len(data) < 8:
            time.sleep(0.1)
            data += self.ser.read(8 - len(data))
        if len(data) != 8:
            raise TimeoutError(
                f"Read timeout on {self.port}: got {len(data)} bytes, expected 8"
            )
        return struct.unpack(">Q", data)[0]

    def status(self) -> bool:
        self.ser.reset_input_buffer()
        self.ser.write(b"S")
        time.sleep(0.05)
        data = self.ser.read(1)
        if len(data) != 1:
            raise TimeoutError(f"Status timeout on {self.port}")
        return (data[0] & 0x80) != 0

    @property
    def is_hardware(self) -> bool:
        return True

    # -- Lifecycle ----------------------------------------------------------

    def close(self) -> None:
        self.ser.close()

    def __enter__(self) -> "ATOMiKHardware":
        return self

    def __exit__(self, *exc: object) -> None:
        self.close()
