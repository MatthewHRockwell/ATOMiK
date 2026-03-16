"""Fingerprinting — fast change detection using delta-state algebra.

Instead of comparing two buffers byte-by-byte (O(n)), maintain a running
XOR fingerprint. Any change flips bits in the accumulator; if the
accumulator is zero, the data matches the reference.

    fp = Fingerprint()
    fp.load(original_data)
    fp.update(possibly_changed_data)
    if fp.changed:
        print("Data was modified")

This is NOT cryptographic hashing — it's algebraic integrity checking.
Two different changes CAN produce the same fingerprint (collision).
For integrity, not authentication. Deterministic latency, no timing
side channels.
"""

from __future__ import annotations

from atomik_core.context import AtomikContext


class Fingerprint:
    """XOR-based change detection fingerprint.

    Reduces an arbitrary-length buffer to a fixed-width accumulator.
    Any modification to the buffer is detected as a non-zero accumulator.

    Width determines collision probability:
    - 64-bit:  1 in 2^64 false negative rate per random change
    - 128-bit: 1 in 2^128 (practically zero)
    """

    __slots__ = ("_ctx", "_chunk_size")

    def __init__(self, width: int = 64):
        """Create a fingerprint tracker.

        Args:
            width: Bit width of the fingerprint (default 64).
        """
        self._ctx = AtomikContext(width=width)
        self._chunk_size = width // 8

    def load(self, data: bytes | bytearray | memoryview) -> int:
        """Set the reference fingerprint from data.

        Computes XOR-reduce over all width-sized chunks of data.

        Args:
            data: Reference data to fingerprint.

        Returns:
            The computed reference fingerprint value.
        """
        ref = self._reduce(data)
        self._ctx.load(ref)
        return ref

    def update(self, data: bytes | bytearray | memoryview) -> int:
        """Compute fingerprint of new data and accumulate the delta.

        Call this with potentially-changed data. If the data matches the
        reference, the accumulator stays zero. If it differs, the
        accumulator captures the XOR difference.

        Args:
            data: New data to compare against reference.

        Returns:
            The fingerprint of the new data.
        """
        current = self._reduce(data)
        # Delta = reference XOR current. We want the accumulator to equal
        # (reference XOR current) so that read() = reference XOR acc = current.
        # Use the internal setter to avoid inflating delta_count.
        self._ctx._set_accumulator(self._ctx.reference ^ current)
        return current

    def accumulate_delta(self, old_chunk: int, new_chunk: int) -> None:
        """Incrementally update the fingerprint when a single chunk changes.

        Instead of re-scanning the full buffer, XOR in the change:
        delta = old_chunk XOR new_chunk.

        Args:
            old_chunk: Previous value of the changed chunk.
            new_chunk: New value of the changed chunk.
        """
        delta = old_chunk ^ new_chunk
        self._ctx.accum(delta)

    @property
    def changed(self) -> bool:
        """True if the data has been modified since load()."""
        return not self._ctx.is_clean

    @property
    def value(self) -> int:
        """Current fingerprint value (reference XOR accumulator)."""
        return self._ctx.read()

    @property
    def delta(self) -> int:
        """Raw accumulator — the XOR difference between reference and current."""
        return self._ctx.accumulator

    def reset(self) -> None:
        """Clear the accumulator (mark data as matching reference)."""
        self._ctx._set_accumulator(0)

    def _reduce(self, data: bytes | bytearray | memoryview) -> int:
        """XOR-reduce data into a single width-bit value."""
        chunk_size = self._chunk_size
        result = 0
        mv = memoryview(data) if not isinstance(data, memoryview) else data
        i = 0
        length = len(mv)
        while i + chunk_size <= length:
            chunk = int.from_bytes(mv[i:i + chunk_size], "little")
            result ^= chunk
            i += chunk_size
        # Handle remaining bytes (pad with zeros)
        if i < length:
            remaining = bytes(mv[i:]) + b"\x00" * (chunk_size - (length - i))
            chunk = int.from_bytes(remaining, "little")
            result ^= chunk
        return result

    def __repr__(self) -> str:
        hex_w = (self._ctx.width + 3) // 4
        return (
            f"Fingerprint(value=0x{self.value:0{hex_w}x}, "
            f"changed={self.changed}, width={self._ctx.width})"
        )
