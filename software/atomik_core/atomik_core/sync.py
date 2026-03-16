"""High-level distributed state synchronization — the one-liner API.

SyncTable wraps AtomikTable + DeltaStream into a callback-driven sync
interface. Peers exchange 16-byte deltas over any transport (TCP, UDP,
WebSocket, UART, carrier pigeon) — the table converges automatically
regardless of message ordering.

    table = SyncTable(num_contexts=256)
    table.on_delta(lambda msg: send_to_peers(msg.to_bytes()))

    # Local writes auto-broadcast
    table.put(addr=0, value=0xDEADBEEF)

    # Remote deltas auto-apply
    table.receive(DeltaMessage.from_bytes(wire_data))

    # All peers converge — math guarantees it
    assert table.get(0) == 0xDEADBEEF

SPDX-License-Identifier: Apache-2.0
"""

from __future__ import annotations

from typing import Callable, Iterator

from atomik_core.table import AtomikTable
from atomik_core.stream import DeltaMessage


class SyncTable:
    """Distributed state table with automatic delta propagation.

    Wraps AtomikTable + DeltaStream with a callback-based sync interface.
    Peers exchange 16-byte deltas over any transport -- the table converges
    automatically regardless of message ordering.

    Usage:
        table = SyncTable(num_contexts=256)
        table.on_delta(lambda msg: send_to_peers(msg.to_bytes()))

        # Local writes auto-broadcast
        table.put(addr=0, value=0xDEADBEEF)

        # Remote deltas auto-apply
        table.receive(DeltaMessage.from_bytes(wire_data))

        # All peers converge
        assert table.get(0) == 0xDEADBEEF
    """

    __slots__ = ("_table", "_on_delta_cb", "_seq")

    def __init__(self, num_contexts: int = 256, width: int = 64):
        """Create a synchronized state table.

        Args:
            num_contexts: Number of addressable context slots (default 256).
            width: Bit width per context (default 64).
        """
        self._table = AtomikTable(num_contexts=num_contexts, width=width)
        self._on_delta_cb: Callable[[DeltaMessage], None] | None = None
        self._seq = 0

    def on_delta(self, callback: Callable[[DeltaMessage], None]) -> None:
        """Register a callback fired whenever a local write generates a delta.

        The callback receives a DeltaMessage that should be sent to all peers.
        Only one callback is active at a time; calling again replaces it.

        Args:
            callback: Function called with a DeltaMessage on each local put().
        """
        self._on_delta_cb = callback

    def put(self, addr: int, value: int) -> DeltaMessage:
        """Write a value to an address, computing and broadcasting the delta.

        Computes delta = current_state ^ new_value, applies it locally,
        and fires the on_delta callback so peers can converge.

        Args:
            addr: Context address to write.
            value: Desired new state value.

        Returns:
            The DeltaMessage that was (or would be) sent to peers.
        """
        current = self._table.read(addr)
        delta = current ^ value
        if delta == 0:
            # No change -- still produce a message for the caller, but
            # skip the callback since there is nothing to propagate.
            return DeltaMessage(addr=addr, delta=0, seq=self._seq)
        self._table.accum(addr, delta)
        self._seq += 1
        msg = DeltaMessage(addr=addr, delta=delta, seq=self._seq)
        if self._on_delta_cb is not None:
            self._on_delta_cb(msg)
        return msg

    def get(self, addr: int) -> int:
        """Read the current state at an address.

        Args:
            addr: Context address to read.

        Returns:
            Current state value (reference XOR accumulator).
        """
        return self._table.read(addr)

    def receive(self, msg: DeltaMessage) -> None:
        """Apply a delta received from a remote peer.

        This does NOT fire the on_delta callback -- prevents echo loops
        in broadcast topologies.

        Args:
            msg: DeltaMessage from a remote peer.
        """
        self._table.accum(msg.addr, msg.delta)

    def snapshot(self) -> dict[int, int]:
        """Serialize the full state of all contexts.

        Returns:
            Dict of {address: current_state} for every context slot
            (including zeros, so the snapshot is complete).
        """
        return {i: self._table.read(i) for i in range(self._table.num_contexts)}

    @classmethod
    def from_snapshot(cls, snap: dict[int, int], width: int = 64) -> SyncTable:
        """Restore a SyncTable from a snapshot dict.

        Args:
            snap: Dict of {address: state_value} as returned by snapshot().
            width: Bit width per context (default 64).

        Returns:
            A new SyncTable with contexts loaded to match the snapshot.
        """
        if not snap:
            return cls(num_contexts=256, width=width)
        num_contexts = max(snap.keys()) + 1
        table = cls(num_contexts=num_contexts, width=width)
        for addr, value in snap.items():
            table._table.load(addr, value)
        return table

    def diff(self, other: SyncTable) -> list[int]:
        """Return addresses where this table and another disagree.

        Useful for debugging convergence -- after sync, diff should be empty.

        Args:
            other: Another SyncTable to compare against.

        Returns:
            Sorted list of addresses with differing state values.
        """
        size = min(self._table.num_contexts, other._table.num_contexts)
        return sorted(
            addr
            for addr in range(size)
            if self._table.read(addr) != other._table.read(addr)
        )

    @property
    def num_contexts(self) -> int:
        """Number of context slots in this table."""
        return self._table.num_contexts

    @property
    def width(self) -> int:
        """Bit width per context."""
        return self._table.width

    def __len__(self) -> int:
        """Number of context slots (same as num_contexts)."""
        return self._table.num_contexts

    def __iter__(self) -> Iterator[int]:
        """Iterate over all valid addresses (0 .. num_contexts-1)."""
        return iter(range(self._table.num_contexts))

    def __contains__(self, addr: object) -> bool:
        """Check if an address is within the valid range."""
        if not isinstance(addr, int):
            return False
        return 0 <= addr < self._table.num_contexts

    def __repr__(self) -> str:
        active = sum(1 for i in range(self._table.num_contexts) if self._table.read(i) != 0)
        return (
            f"SyncTable(contexts={self._table.num_contexts}, "
            f"width={self._table.width}, active={active})"
        )
