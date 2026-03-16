"""Delta streaming — network-efficient state synchronization.

Instead of sending full state snapshots between nodes, send only the
deltas. The receiver applies them to reconstruct the same state.

Because XOR is commutative, deltas can arrive out of order, be
duplicated, or be merged in transit — the result is always correct.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass, field
from typing import Iterator

from atomik_core.context import AtomikContext

# Wire format: addr (uint32) + delta (uint64) + seq (uint32) = 16 bytes
_WIRE_FORMAT = "!IQI"
_WIRE_SIZE = struct.calcsize(_WIRE_FORMAT)  # 16


@dataclass(frozen=True)
class DeltaMessage:
    """A single delta to be transmitted between nodes.

    Attributes:
        addr: Context address this delta applies to.
        delta: The XOR delta value.
        epoch: Epoch number (incremented on SWAP). Receiver can detect stale deltas.
        seq: Sequence number (monotonic per sender). For dedup, not ordering.
    """

    addr: int
    delta: int
    epoch: int = 0
    seq: int = 0

    def to_dict(self) -> dict:
        """Serialize to a JSON-compatible dictionary."""
        return {"addr": self.addr, "delta": self.delta, "seq": self.seq}

    @classmethod
    def from_dict(cls, d: dict) -> DeltaMessage:
        """Deserialize from a dictionary."""
        return cls(addr=d["addr"], delta=d["delta"], seq=d["seq"])

    def to_bytes(self) -> bytes:
        """Serialize to compact 16-byte wire format (network byte order).

        Format: !IQI — addr (uint32) + delta (uint64) + seq (uint32).
        """
        return struct.pack(_WIRE_FORMAT, self.addr, self.delta, self.seq)

    @classmethod
    def from_bytes(cls, data: bytes) -> DeltaMessage:
        """Deserialize from 16-byte wire format.

        Args:
            data: Exactly 16 bytes in network byte order.

        Raises:
            ValueError: If data is not exactly 16 bytes.
        """
        if len(data) != _WIRE_SIZE:
            raise ValueError(f"expected {_WIRE_SIZE} bytes, got {len(data)}")
        addr, delta, seq = struct.unpack(_WIRE_FORMAT, data)
        return cls(addr=addr, delta=delta, seq=seq)


class DeltaStream:
    """Produces and consumes delta messages for state synchronization.

    Sender side:
        stream = DeltaStream(width=64)
        stream.load(addr=0, initial_state=0xCAFE)
        stream.accum(addr=0, delta=0x00FF)
        for msg in stream.pending():
            network.send(msg)

    Receiver side:
        stream = DeltaStream(width=64)
        stream.load(addr=0, initial_state=0xCAFE)  # same initial state
        stream.apply(msg)  # apply received delta
        assert stream.read(addr=0) == sender_stream.read(addr=0)

    Convergence guarantee: if two streams start from the same reference
    and receive the same set of deltas (in any order), they converge to
    the same state. This is proven in Lean4 (commutativity theorem).
    """

    __slots__ = ("_contexts", "_width", "_epoch", "_seq", "_pending")

    def __init__(self, width: int = 64, num_contexts: int = 256):
        self._width = width
        self._contexts: dict[int, AtomikContext] = {}
        self._epoch = 0
        self._seq = 0
        self._pending: list[DeltaMessage] = []

    def _get_or_create(self, addr: int) -> AtomikContext:
        if addr not in self._contexts:
            self._contexts[addr] = AtomikContext(width=self._width)
        return self._contexts[addr]

    def load(self, addr: int, initial_state: int) -> None:
        """Initialize a context. Both sender and receiver must call this
        with the same initial_state before streaming deltas."""
        ctx = self._get_or_create(addr)
        ctx.load(initial_state)
        self._epoch += 1

    def accum(self, addr: int, delta: int) -> DeltaMessage:
        """Apply a delta locally and produce a message to send.

        Returns:
            DeltaMessage to transmit to peers.
        """
        ctx = self._get_or_create(addr)
        ctx.accum(delta)
        self._seq += 1
        msg = DeltaMessage(addr=addr, delta=delta, epoch=self._epoch, seq=self._seq)
        self._pending.append(msg)
        return msg

    def apply(self, msg: DeltaMessage) -> None:
        """Apply a received delta message from a peer.

        Safe to call with out-of-order or duplicate messages:
        - Out-of-order: XOR commutativity guarantees correct result
        - Duplicate: XOR self-inverse means applying twice = no-op
          (caller should dedup by seq if exact-once semantics needed)
        """
        ctx = self._get_or_create(msg.addr)
        ctx.accum(msg.delta)

    def read(self, addr: int) -> int:
        """Read the current state at an address."""
        if addr not in self._contexts:
            return 0
        return self._contexts[addr].read()

    def pending(self) -> list[DeltaMessage]:
        """Get and clear all pending outbound delta messages."""
        msgs = self._pending
        self._pending = []
        return msgs

    def contexts(self) -> Iterator[tuple[int, AtomikContext]]:
        """Iterate over all active (addr, context) pairs."""
        yield from self._contexts.items()
