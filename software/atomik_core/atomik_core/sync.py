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

import asyncio
import threading
from collections.abc import Iterator
from typing import Any, Callable

from atomik_core.stream import DeltaMessage
from atomik_core.table import AtomikTable


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

    __slots__ = ("_table", "_on_delta_cb", "_seq", "_ws_handle")

    def __init__(self, num_contexts: int = 256, width: int = 64):
        """Create a synchronized state table.

        Args:
            num_contexts: Number of addressable context slots (default 256).
            width: Bit width per context (default 64).
        """
        self._table = AtomikTable(num_contexts=num_contexts, width=width)
        self._on_delta_cb: Callable[[DeltaMessage], None] | None = None
        self._seq = 0
        self._ws_handle: WebSocketHandle | None = None

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

    def connect_websocket(self, url: str) -> WebSocketHandle:
        """Connect to a WebSocket server for real-time delta synchronization.

        Creates a background thread running an asyncio event loop that:
        - Sends local deltas (16 bytes each) to the server via on_delta callback
        - Receives remote deltas and applies them via self.receive()

        Requires the ``websockets`` package::

            pip install atomik-core[ws]

        Args:
            url: WebSocket URL to connect to (e.g. ``ws://localhost:8765``).

        Returns:
            A WebSocketHandle that can be used to close the connection.

        Raises:
            ImportError: If the ``websockets`` package is not installed.
        """
        try:
            import websockets  # noqa: F401
        except ImportError:
            raise ImportError(
                "WebSocket support requires the 'websockets' package.\n"
                "Install it with:  pip install atomik-core[ws]"
            ) from None

        handle = WebSocketHandle(url=url)
        self._ws_handle = handle

        async def _ws_loop() -> None:
            import websockets

            async with websockets.connect(url) as ws:
                handle._ws = ws

                # Register on_delta callback to send deltas over the wire
                prev_cb = self._on_delta_cb

                def _send_delta(msg: DeltaMessage) -> None:
                    if prev_cb is not None:
                        prev_cb(msg)
                    try:
                        asyncio.run_coroutine_threadsafe(
                            ws.send(msg.to_bytes()), handle._loop
                        )
                    except Exception:
                        pass  # Connection closed

                self._on_delta_cb = _send_delta

                # Receive loop
                try:
                    async for data in ws:
                        if isinstance(data, bytes) and len(data) == 16:
                            self.receive(DeltaMessage.from_bytes(data))
                except Exception:
                    pass  # Connection closed or error
                finally:
                    # Restore previous callback
                    self._on_delta_cb = prev_cb

        def _run_loop() -> None:
            loop = asyncio.new_event_loop()
            handle._loop = loop
            try:
                loop.run_until_complete(_ws_loop())
            finally:
                loop.close()
                handle._closed = True

        thread = threading.Thread(target=_run_loop, daemon=True)
        handle._thread = thread
        thread.start()
        return handle

    def __repr__(self) -> str:
        active = sum(1 for i in range(self._table.num_contexts) if self._table.read(i) != 0)
        return (
            f"SyncTable(contexts={self._table.num_contexts}, "
            f"width={self._table.width}, active={active})"
        )


class WebSocketHandle:
    """Handle returned by SyncTable.connect_websocket().

    Use close() to disconnect, or use as a context manager::

        handle = table.connect_websocket("ws://localhost:8765")
        # ... do work ...
        handle.close()

        # Or:
        with table.connect_websocket("ws://...") as handle:
            ...
    """

    def __init__(self, url: str):
        self.url = url
        self._ws: Any = None
        self._loop: Any = None
        self._thread: threading.Thread | None = None
        self._closed = False

    @property
    def connected(self) -> bool:
        """True if the WebSocket connection is still active."""
        return not self._closed and self._ws is not None

    def close(self) -> None:
        """Close the WebSocket connection and stop the background thread."""
        if self._closed:
            return
        self._closed = True
        if self._ws is not None and self._loop is not None:
            try:
                asyncio.run_coroutine_threadsafe(
                    self._ws.close(), self._loop
                )
            except Exception:
                pass
        if self._thread is not None:
            self._thread.join(timeout=5.0)

    def __enter__(self) -> WebSocketHandle:
        return self

    def __exit__(self, *args: Any) -> None:
        self.close()

    def __repr__(self) -> str:
        status = "connected" if self.connected else "closed"
        return f"WebSocketHandle(url={self.url!r}, {status})"
