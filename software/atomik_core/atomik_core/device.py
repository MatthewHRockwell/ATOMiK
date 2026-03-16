"""ATOMiK device bindings — /dev/atomik ioctl interface.

Drop-in replacements for AtomikContext and AtomikTable that route
operations through the Linux kernel module instead of pure-Python XOR.

The kernel module may be software-only or hardware-accelerated (when
an ATOMiK FPGA/ASIC core is present via UIO).  The API is identical
either way — the kernel handles dispatch.

Requires:
  - Linux with the atomik kernel module loaded
  - /dev/atomik character device accessible (chmod or udev rule)

SPDX-License-Identifier: Apache-2.0
"""

from __future__ import annotations

import os
import sys
import struct
import fcntl
from typing import Optional

# =========================================================================
# ioctl number construction (mirrors <linux/ioctl.h> on Linux)
# =========================================================================

_IOC_NRBITS = 8
_IOC_TYPEBITS = 8
_IOC_SIZEBITS = 14
_IOC_DIRBITS = 2

_IOC_NRSHIFT = 0
_IOC_TYPESHIFT = _IOC_NRSHIFT + _IOC_NRBITS      # 8
_IOC_SIZESHIFT = _IOC_TYPESHIFT + _IOC_TYPEBITS   # 16
_IOC_DIRSHIFT = _IOC_SIZESHIFT + _IOC_SIZEBITS    # 30

_IOC_NONE = 0
_IOC_WRITE = 1
_IOC_READ = 2


def _IOC(dir_: int, type_: int, nr: int, size: int) -> int:
    return (dir_ << _IOC_DIRSHIFT) | (type_ << _IOC_TYPESHIFT) | \
           (nr << _IOC_NRSHIFT) | (size << _IOC_SIZESHIFT)


def _IOW(type_: int, nr: int, size: int) -> int:
    return _IOC(_IOC_WRITE, type_, nr, size)


def _IOR(type_: int, nr: int, size: int) -> int:
    return _IOC(_IOC_READ, type_, nr, size)


def _IOWR(type_: int, nr: int, size: int) -> int:
    return _IOC(_IOC_READ | _IOC_WRITE, type_, nr, size)


# =========================================================================
# ioctl command numbers  (must match include/uapi/atomik.h)
# =========================================================================

_ATOMIK_MAGIC = ord('A')  # 0x41

# struct sizes (natural alignment, matching kernel sizeof())
_SZ_CREATE_TABLE = 8   # u32 + u32
_SZ_DESTROY_TABLE = 4  # u32
_SZ_LOAD = 16          # u32 + u32 + u64
_SZ_ACCUM = 16         # u32 + u32 + u64
_SZ_READ = 16          # u32 + u32 + u64
_SZ_SWAP = 24          # u32 + u32 + u64 + u64
_SZ_INFO = 40          # u32 + u32 + u32 + u32 + pad(4) + u64 + u64 + u64

ATOMIK_IOC_CREATE_TABLE = _IOWR(_ATOMIK_MAGIC, 0x01, _SZ_CREATE_TABLE)
ATOMIK_IOC_DESTROY_TABLE = _IOW(_ATOMIK_MAGIC, 0x02, _SZ_DESTROY_TABLE)

ATOMIK_IOC_LOAD = _IOW(_ATOMIK_MAGIC, 0x10, _SZ_LOAD)
ATOMIK_IOC_ACCUM = _IOW(_ATOMIK_MAGIC, 0x11, _SZ_ACCUM)
ATOMIK_IOC_READ = _IOWR(_ATOMIK_MAGIC, 0x12, _SZ_READ)
ATOMIK_IOC_SWAP = _IOWR(_ATOMIK_MAGIC, 0x13, _SZ_SWAP)

ATOMIK_IOC_GET_INFO = _IOR(_ATOMIK_MAGIC, 0x40, _SZ_INFO)

# Struct format strings (little-endian, matching kernel C struct layout)
_FMT_CREATE_TABLE = "<II"       # num_contexts(u32), table_id(u32)
_FMT_DESTROY_TABLE = "<I"       # table_id(u32)
_FMT_LOAD = "<IIQ"              # table_id(u32), addr(u32), initial_state(u64)
_FMT_ACCUM = "<IIQ"             # table_id(u32), addr(u32), delta(u64)
_FMT_READ = "<IIQ"              # table_id(u32), addr(u32), state(u64)
_FMT_SWAP = "<IIQQ"             # table_id(u32), addr(u32), new_reference(u64), old_state(u64)
_FMT_INFO = "<IIIIQQQ"          # version, backend, hw_banks, fp_enabled, ops_total, fp_checks, fp_bytes_saved
                                # Note: natural alignment adds 4 bytes padding before first u64 → 40 bytes

# Limits (from UAPI header)
ATOMIK_MAX_CONTEXTS = 65536
ATOMIK_MAX_TABLES = 256

# Device path
ATOMIK_DEVICE = "/dev/atomik"


# =========================================================================
# DeviceInfo — result of GET_INFO ioctl
# =========================================================================

class DeviceInfo:
    """Information about the ATOMiK kernel module and hardware backend."""

    __slots__ = (
        "version_major", "version_minor", "version_patch",
        "backend", "hw_banks", "fp_enabled",
        "ops_total", "fp_checks", "fp_bytes_saved",
    )

    def __init__(self, raw: bytes):
        version, backend, hw_banks, fp_enabled, ops_total, fp_checks, fp_bytes_saved = \
            struct.unpack(_FMT_INFO, raw)
        self.version_major = (version >> 16) & 0xFF
        self.version_minor = (version >> 8) & 0xFF
        self.version_patch = version & 0xFF
        self.backend = "hardware" if backend == 1 else "software"
        self.hw_banks = hw_banks
        self.fp_enabled = bool(fp_enabled)
        self.ops_total = ops_total
        self.fp_checks = fp_checks
        self.fp_bytes_saved = fp_bytes_saved

    @property
    def version(self) -> str:
        return f"{self.version_major}.{self.version_minor}.{self.version_patch}"

    def __repr__(self) -> str:
        return (
            f"DeviceInfo(version={self.version!r}, backend={self.backend!r}, "
            f"hw_banks={self.hw_banks}, ops_total={self.ops_total})"
        )


# =========================================================================
# DeviceTable — multi-context table backed by /dev/atomik
# =========================================================================

class DeviceTable:
    """A kernel-backed ATOMiK context table.

    Wraps CREATE_TABLE / DESTROY_TABLE ioctls.  Provides load/accum/read/swap
    on any address within the table.

    Usage:
        with DeviceTable(num_contexts=256) as table:
            table.load(addr=0, initial_state=0xCAFEBABE)
            table.accum(addr=0, delta=0x00000001)
            assert table.read(addr=0) == 0xCAFEBABF
    """

    __slots__ = ("_fd", "_table_id", "_num_contexts", "_owns_fd")

    def __init__(
        self,
        num_contexts: int = 256,
        *,
        fd: Optional[int] = None,
        device: str = ATOMIK_DEVICE,
    ):
        """Create a kernel-backed context table.

        Args:
            num_contexts: Number of context slots (1 to 65536).
            fd: Existing file descriptor for /dev/atomik.  If None, opens the device.
            device: Path to the device node (default /dev/atomik).
        """
        # Set safe defaults before validation so __del__ works if __init__ raises
        self._fd = None
        self._owns_fd = False
        self._table_id = 0
        self._num_contexts = 0

        if num_contexts < 1 or num_contexts > ATOMIK_MAX_CONTEXTS:
            raise ValueError(
                f"num_contexts must be 1..{ATOMIK_MAX_CONTEXTS}, got {num_contexts}"
            )

        if fd is not None:
            self._fd = fd
            self._owns_fd = False
        else:
            self._fd = os.open(device, os.O_RDWR)
            self._owns_fd = True

        self._num_contexts = num_contexts
        self._table_id = 0

        # CREATE_TABLE ioctl
        buf = bytearray(struct.pack(_FMT_CREATE_TABLE, num_contexts, 0))
        fcntl.ioctl(self._fd, ATOMIK_IOC_CREATE_TABLE, buf)
        _, self._table_id = struct.unpack(_FMT_CREATE_TABLE, buf)

    def load(self, addr: int, initial_state: int) -> None:
        """LOAD: Set initial state at address, clear its accumulator."""
        buf = struct.pack(_FMT_LOAD, self._table_id, addr, initial_state & 0xFFFFFFFFFFFFFFFF)
        fcntl.ioctl(self._fd, ATOMIK_IOC_LOAD, buf)

    def accum(self, addr: int, delta: int) -> None:
        """ACCUM: XOR delta into the accumulator at address."""
        buf = struct.pack(_FMT_ACCUM, self._table_id, addr, delta & 0xFFFFFFFFFFFFFFFF)
        fcntl.ioctl(self._fd, ATOMIK_IOC_ACCUM, buf)

    def read(self, addr: int) -> int:
        """READ: Reconstruct current state at address."""
        buf = bytearray(struct.pack(_FMT_READ, self._table_id, addr, 0))
        fcntl.ioctl(self._fd, ATOMIK_IOC_READ, buf)
        _, _, state = struct.unpack(_FMT_READ, buf)
        return state

    def swap(self, addr: int, new_reference: int | None = None) -> int:
        """SWAP: Atomic snapshot + new epoch at address.

        Args:
            addr: Context address.
            new_reference: New reference state.  If None (or 0), uses current state.

        Returns:
            The state at the moment of the swap.
        """
        new_ref = 0 if new_reference is None else (new_reference & 0xFFFFFFFFFFFFFFFF)
        buf = bytearray(struct.pack(_FMT_SWAP, self._table_id, addr, new_ref, 0))
        fcntl.ioctl(self._fd, ATOMIK_IOC_SWAP, buf)
        _, _, _, old_state = struct.unpack(_FMT_SWAP, buf)
        return old_state

    def info(self) -> DeviceInfo:
        """Query device/driver information."""
        buf = bytearray(_SZ_INFO)
        fcntl.ioctl(self._fd, ATOMIK_IOC_GET_INFO, buf)
        return DeviceInfo(bytes(buf))

    @property
    def table_id(self) -> int:
        return self._table_id

    @property
    def num_contexts(self) -> int:
        return self._num_contexts

    def close(self) -> None:
        """Destroy the kernel table and close the device fd."""
        if self._fd is not None:
            try:
                buf = struct.pack(_FMT_DESTROY_TABLE, self._table_id)
                fcntl.ioctl(self._fd, ATOMIK_IOC_DESTROY_TABLE, buf)
            except OSError:
                pass  # best-effort on cleanup
            if self._owns_fd:
                try:
                    os.close(self._fd)
                except OSError:
                    pass
            self._fd = None

    def __del__(self) -> None:
        self.close()

    def __enter__(self) -> "DeviceTable":
        return self

    def __exit__(self, *_: object) -> None:
        self.close()

    def __repr__(self) -> str:
        return (
            f"DeviceTable(table_id={self._table_id}, "
            f"num_contexts={self._num_contexts})"
        )


# =========================================================================
# DeviceContext — single-context wrapper (drop-in for AtomikContext)
# =========================================================================

class DeviceContext:
    """ATOMiK context backed by the Linux kernel module.

    Drop-in replacement for AtomikContext that uses /dev/atomik ioctls.
    Same API — load(), accum(), read(), swap() — but operations go through
    the kernel module (software or hardware-accelerated).

    Usage:
        from atomik_core import DeviceContext
        ctx = DeviceContext()  # opens /dev/atomik
        ctx.load(0xDEADBEEF)
        ctx.accum(0x000000FF)
        state = ctx.read()    # kernel-accelerated
    """

    __slots__ = ("_table", "_addr")

    def __init__(
        self,
        *,
        device: str = ATOMIK_DEVICE,
        fd: Optional[int] = None,
        addr: int = 0,
    ):
        """Create a kernel-backed single context.

        Opens /dev/atomik, creates a 1-context table, and binds to address 0.

        Args:
            device: Path to the device node.
            fd: Existing fd (for sharing across multiple DeviceContext instances).
            addr: Address within the table (default 0).
        """
        self._table = DeviceTable(num_contexts=1, fd=fd, device=device)
        self._addr = addr

    def load(self, value: int) -> None:
        """LOAD: Set reference state, clear accumulator."""
        self._table.load(self._addr, value)

    def accum(self, delta: int) -> None:
        """ACCUM: XOR delta into the accumulator."""
        self._table.accum(self._addr, delta)

    def read(self) -> int:
        """READ: Reconstruct current state (reference ^ accumulator)."""
        return self._table.read(self._addr)

    def swap(self, new_reference: int | None = None) -> int:
        """SWAP: Atomic snapshot + new epoch.

        Args:
            new_reference: Optional new reference state.  If None, uses current state.

        Returns:
            The state at the moment of the swap.
        """
        return self._table.swap(self._addr, new_reference)

    def close(self) -> None:
        """Release the kernel table and device fd."""
        if self._table is not None:
            self._table.close()
            self._table = None

    def __del__(self) -> None:
        self.close()

    def __enter__(self) -> "DeviceContext":
        return self

    def __exit__(self, *_: object) -> None:
        self.close()

    def __repr__(self) -> str:
        return f"DeviceContext(table={self._table!r})"
