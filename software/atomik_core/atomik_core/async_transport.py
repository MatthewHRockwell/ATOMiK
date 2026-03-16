"""Async transport layer for SyncTable delta propagation.

Provides async-compatible transports for use with asyncio, aiohttp,
FastAPI, WebSocket servers, and any other async framework.

Because XOR accumulation is instant (no I/O), only the network send
needs to be async. Receives and reads remain synchronous.

Usage:
    from atomik_core.async_transport import AsyncCallbackTransport, AsyncSyncTable

    # Low-level: async callback transport
    async def send_ws(data: bytes):
        await websocket.send(data)

    transport = AsyncCallbackTransport(table, send_fn=send_ws)
    await transport.send_pending()  # flush any queued deltas

    # High-level: AsyncSyncTable wraps SyncTable with async put()
    async_table = AsyncSyncTable(256, send_fn=send_ws)
    await async_table.put(0, 0xDEADBEEF)  # sends delta immediately

SPDX-License-Identifier: Apache-2.0
"""

from __future__ import annotations

from atomik_core.stream import DeltaMessage
from atomik_core.sync import SyncTable


class AsyncCallbackTransport:
    """Async-compatible transport for SyncTable.

    Accepts an async send function for use with asyncio, aiohttp, FastAPI, etc.
    Since SyncTable's on_delta callback is synchronous, deltas are queued
    internally and flushed via the async send_pending() method.

    Usage:
        async def send_ws(data: bytes):
            await websocket.send(data)

        transport = AsyncCallbackTransport(table, send_fn=send_ws)
        await transport.send_pending()  # flush any queued deltas
    """

    __slots__ = ("_table", "_send_fn", "_queue")

    def __init__(self, table: SyncTable, send_fn=None) -> None:
        """Create an async callback transport.

        Args:
            table: The SyncTable to wrap.
            send_fn: An async callable that accepts bytes. Called with
                     serialized DeltaMessages when send_pending() is awaited.
        """
        self._table = table
        self._send_fn = send_fn
        self._queue: list[bytes] = []
        table.on_delta(self._on_local_delta)

    def _on_local_delta(self, msg: DeltaMessage) -> None:
        """Called synchronously when local table generates a delta.

        Queues the serialized message for later async sending.
        """
        self._queue.append(msg.to_bytes())

    async def send_pending(self) -> int:
        """Send all queued deltas via the async send function.

        Returns:
            Number of deltas sent.
        """
        if self._send_fn is None or not self._queue:
            count = len(self._queue)
            self._queue.clear()
            return count
        pending = self._queue
        self._queue = []
        for data in pending:
            await self._send_fn(data)
        return len(pending)

    def receive_bytes(self, data: bytes) -> None:
        """Process incoming delta bytes from the network.

        Synchronous — XOR accumulation is instant, no I/O needed.

        Args:
            data: Exactly 16 bytes in network byte order (DeltaMessage wire format).
        """
        msg = DeltaMessage.from_bytes(data)
        self._table.receive(msg)

    @property
    def pending_count(self) -> int:
        """Number of deltas queued for sending."""
        return len(self._queue)


class AsyncSyncTable:
    """Async wrapper around SyncTable with automatic delta sending.

    Provides an async put() that writes locally and immediately sends
    the delta via the configured async send function. Reads and receives
    remain synchronous since they involve no I/O.

    Usage:
        async def send_ws(data: bytes):
            await websocket.send(data)

        table = AsyncSyncTable(256, send_fn=send_ws)
        await table.put(0, 0xDEADBEEF)  # write + send in one await
        value = table.get(0)             # sync read (instant)
    """

    __slots__ = ("_table", "_transport")

    def __init__(self, num_contexts: int = 256, width: int = 64, send_fn=None) -> None:
        """Create an async-capable synchronized state table.

        Args:
            num_contexts: Number of addressable context slots (default 256).
            width: Bit width per context (default 64).
            send_fn: An async callable that accepts bytes. Called with
                     serialized DeltaMessages on each put().
        """
        self._table = SyncTable(num_contexts=num_contexts, width=width)
        self._transport = AsyncCallbackTransport(self._table, send_fn=send_fn)

    async def put(self, addr: int, value: int) -> DeltaMessage:
        """Write a value and send the delta to peers.

        Args:
            addr: Context address to write.
            value: Desired new state value.

        Returns:
            The DeltaMessage that was sent (or would be sent if no send_fn).
        """
        msg = self._table.put(addr, value)
        await self._transport.send_pending()
        return msg

    def get(self, addr: int) -> int:
        """Read the current state at an address.

        Synchronous — reads are instant (reference XOR accumulator).

        Args:
            addr: Context address to read.

        Returns:
            Current state value.
        """
        return self._table.get(addr)

    def receive(self, msg: DeltaMessage) -> None:
        """Apply a delta received from a remote peer.

        Synchronous — XOR accumulation is instant. Does NOT trigger
        sending (prevents echo loops).

        Args:
            msg: DeltaMessage from a remote peer.
        """
        self._table.receive(msg)

    def receive_bytes(self, data: bytes) -> None:
        """Apply delta bytes received from the network.

        Synchronous — XOR accumulation is instant.

        Args:
            data: Exactly 16 bytes in network byte order.
        """
        self._transport.receive_bytes(data)

    @property
    def num_contexts(self) -> int:
        """Number of context slots in this table."""
        return self._table.num_contexts

    @property
    def width(self) -> int:
        """Bit width per context."""
        return self._table.width

    def snapshot(self) -> dict[int, int]:
        """Serialize the full state of all contexts."""
        return self._table.snapshot()

    def diff(self, other: AsyncSyncTable) -> list[int]:
        """Return addresses where this table and another disagree."""
        return self._table.diff(other._table)

    def __repr__(self) -> str:
        return f"AsyncSyncTable({self._table!r})"
