"""
atomik_zynq.py - ATOMiK Python Bindings for Zynq UIO

Provides direct memory-mapped access to ATOMiK hardware on Zynq via the
Linux UIO framework. No ctypes dependency on libatomik.so — uses Python's
built-in mmap and struct modules for register I/O.

The 32-bit AXI to 64-bit datapath splitting is handled internally:
software writes the low 32 bits first, then the high 32 bits (which
triggers the hardware operation).

Usage:
    from atomik_zynq import ATOMiK

    with ATOMiK() as a:
        a.load(0, 0xDEADBEEFCAFEBABE)
        a.accum(0x00000000000000FF)
        state = a.read()
        print(f"State: 0x{state:016x}")

Mock mode (for testing without hardware):
    a = ATOMiK.mock()
    a.load(0, 0x1234)
    assert a.read() == 0x1234

ATOMiK Project — March 2026
SPDX-License-Identifier: MIT
"""

import mmap
import os
import struct

# Register offsets (match atomik_axi4lite_wrapper.v)
_REG_LOAD_ADDR    = 0x00
_REG_LOAD_DATA_LO = 0x04
_REG_LOAD_DATA_HI = 0x08
_REG_ACCUM_LO     = 0x0C
_REG_ACCUM_HI     = 0x10
_REG_STATE_LO     = 0x14
_REG_STATE_HI     = 0x18
_REG_STATUS       = 0x1C
_REG_SWAP_ADDR    = 0x20
_REG_CONFIG       = 0x24

_MMAP_SIZE = 0x1000  # UIO maps at page granularity


class ATOMiK:
    """ATOMiK hardware access via UIO memory-mapped registers."""

    def __init__(self, uio_dev="/dev/uio0"):
        """Open an ATOMiK device.

        Args:
            uio_dev: UIO device path (e.g. "/dev/uio0").
                     Pass None for mock mode (internal use).
        """
        if uio_dev is None:
            # Mock mode — initialized by ATOMiK.mock()
            return

        self._fd = os.open(uio_dev, os.O_RDWR | os.O_SYNC)
        self._mm = mmap.mmap(self._fd, _MMAP_SIZE, mmap.MAP_SHARED,
                             mmap.PROT_READ | mmap.PROT_WRITE)

        # Read STATUS register for device info
        status = self._read32(_REG_STATUS)
        self.version = (status >> 16) & 0xFF
        self.n_banks = (status >> 8) & 0xFF

    def close(self):
        """Release resources."""
        if hasattr(self, '_mm') and self._mm:
            self._mm.close()
            self._mm = None
        if hasattr(self, '_fd') and self._fd >= 0:
            os.close(self._fd)
            self._fd = -1

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    # =========================================================================
    # ATOMiK Operations (match the delta-state algebra)
    # =========================================================================

    def load(self, addr: int, initial_state: int):
        """LOAD: Initialize a context with an initial state.

        Sets the active address, writes initial_state to the BSRAM state
        table, and clears the accumulator.

        Args:
            addr: Context address (0-255).
            initial_state: 64-bit initial state value.
        """
        self._write32(_REG_LOAD_ADDR, addr & 0xFF)
        self._write32(_REG_LOAD_DATA_LO, initial_state & 0xFFFFFFFF)
        self._write32(_REG_LOAD_DATA_HI, (initial_state >> 32) & 0xFFFFFFFF)

    def accum(self, delta: int):
        """ACCUM: Accumulate a delta (acc = acc XOR delta).

        Args:
            delta: 64-bit delta value.
        """
        self._write32(_REG_ACCUM_LO, delta & 0xFFFFFFFF)
        self._write32(_REG_ACCUM_HI, (delta >> 32) & 0xFFFFFFFF)

    def read(self) -> int:
        """READ: Get the current reconstructed state.

        Returns initial_state XOR accumulator. The LO read latches a 64-bit
        atomic snapshot; HI returns the upper half from that snapshot.

        Returns:
            64-bit current state.
        """
        lo = self._read32(_REG_STATE_LO)
        hi = self._read32(_REG_STATE_HI)
        return (hi << 32) | lo

    def swap(self, addr: int):
        """SWAP: Promote current state to new reference, switch address, clear accumulator.

        state_table[active_addr] = current_state; active_addr = addr; acc = 0.

        Args:
            addr: Context address to switch to (0-255).
        """
        self._write32(_REG_SWAP_ADDR, addr & 0xFF)

    # =========================================================================
    # Status
    # =========================================================================

    @property
    def acc_zero(self) -> bool:
        """True if the accumulator is zero."""
        return bool(self._read32(_REG_STATUS) & 1)

    @property
    def enabled(self) -> bool:
        """True if the ATOMiK core is enabled."""
        return bool(self._read32(_REG_CONFIG) & 1)

    @enabled.setter
    def enabled(self, value: bool):
        """Enable or disable the ATOMiK core.

        Disabling asserts reset (accumulator cleared on re-enable).
        """
        self._write32(_REG_CONFIG, 1 if value else 0)

    # =========================================================================
    # Register I/O
    # =========================================================================

    def _write32(self, offset: int, value: int):
        """Write a 32-bit value to a register offset."""
        self._mm[offset:offset + 4] = struct.pack('<I', value & 0xFFFFFFFF)

    def _read32(self, offset: int) -> int:
        """Read a 32-bit value from a register offset."""
        return struct.unpack('<I', self._mm[offset:offset + 4])[0]

    # =========================================================================
    # Mock backend (for testing without hardware)
    # =========================================================================

    @classmethod
    def mock(cls):
        """Create a mock ATOMiK instance backed by software simulation.

        The mock simulates the hardware register behavior identically to
        atomik_axi4lite_wrapper.v. No /dev/uio0 needed.

        Returns:
            ATOMiK instance with mock backend.
        """
        return _MockATOMiK()


class _MockATOMiK(ATOMiK):
    """Software simulation of ATOMiK hardware for testing."""

    def __init__(self):
        self._state_table = [0] * 256
        self._accumulator = 0
        self._active_addr = 0
        self._snapshot = 0
        self._load_addr = 0
        self._load_data_lo = 0
        self._accum_lo = 0
        self._enable = True
        self.version = 1
        self.n_banks = 1
        self._fd = -1
        self._mm = None

    def close(self):
        pass  # Nothing to release

    def _current_state(self) -> int:
        return self._state_table[self._active_addr] ^ self._accumulator

    def load(self, addr: int, initial_state: int):
        self._active_addr = addr & 0xFF
        self._state_table[self._active_addr] = initial_state & 0xFFFFFFFFFFFFFFFF
        self._accumulator = 0

    def accum(self, delta: int):
        self._accumulator ^= (delta & 0xFFFFFFFFFFFFFFFF)

    def read(self) -> int:
        self._snapshot = self._current_state()
        return self._snapshot

    def swap(self, addr: int):
        # SWAP: save current state, switch address, clear accumulator
        self._state_table[self._active_addr] = self._current_state()
        self._active_addr = addr & 0xFF
        self._accumulator = 0

    @property
    def acc_zero(self) -> bool:
        return self._accumulator == 0

    @property
    def enabled(self) -> bool:
        return self._enable

    @enabled.setter
    def enabled(self, value: bool):
        self._enable = bool(value)
        if not self._enable:
            self._accumulator = 0

    def _write32(self, offset, value):
        raise RuntimeError("Mock uses overridden methods, not register I/O")

    def _read32(self, offset):
        raise RuntimeError("Mock uses overridden methods, not register I/O")
