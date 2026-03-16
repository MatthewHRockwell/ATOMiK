"""Pluggable transport layer for SyncTable delta propagation.

Provides transport backends that SyncTable can use to send/receive
deltas over the network. Start with in-memory (for testing) and
callback-based (for custom integration). WebSocket transport is
available when the 'websockets' package is installed.

Usage:
    from atomik_core.transport import MemoryTransport

    # In-memory hub for testing multi-peer convergence
    hub = MemoryTransport()
    table_a = SyncTable(256)
    table_b = SyncTable(256)
    hub.connect(table_a)
    hub.connect(table_b)

    table_a.put(0, 0xDEADBEEF)  # auto-propagates to table_b
"""

from __future__ import annotations

from atomik_core.stream import DeltaMessage


class MemoryTransport:
    """In-memory transport that connects multiple SyncTables.

    Deltas from any connected table are broadcast to all others.
    No network, no serialization — pure in-process relay.
    Ideal for testing and single-process multi-table scenarios.
    """

    def __init__(self) -> None:
        self._tables: list = []

    def connect(self, table: object) -> None:
        """Connect a SyncTable to this transport hub.

        The table's on_delta callback is set to broadcast to all
        other connected tables.
        """
        self._tables.append(table)
        table.on_delta(lambda msg, src=table: self._relay(src, msg))  # type: ignore[attr-defined]

    def disconnect(self, table: object) -> None:
        """Disconnect a SyncTable from this hub."""
        self._tables.remove(table)
        table.on_delta(None)  # type: ignore[attr-defined]

    def _relay(self, source: object, msg: DeltaMessage) -> None:
        """Relay a delta to all connected tables except the source."""
        for table in self._tables:
            if table is not source:
                table.receive(msg)  # type: ignore[attr-defined]

    @property
    def peer_count(self) -> int:
        """Number of connected tables."""
        return len(self._tables)


class CallbackTransport:
    """Callback-based transport for custom network integration.

    Wraps a SyncTable with a send callback. Received bytes are
    deserialized and applied automatically.

    Usage:
        transport = CallbackTransport(table, send_fn=my_socket.send)
        # When data arrives from network:
        transport.receive_bytes(data)
    """

    def __init__(self, table: object, send_fn: object = None) -> None:
        self._table = table
        self._send_fn = send_fn
        table.on_delta(self._on_local_delta)  # type: ignore[attr-defined]

    def _on_local_delta(self, msg: DeltaMessage) -> None:
        """Called when local table generates a delta."""
        if self._send_fn is not None:
            self._send_fn(msg.to_bytes())  # type: ignore[operator]

    def receive_bytes(self, data: bytes) -> None:
        """Process incoming delta bytes from the network."""
        msg = DeltaMessage.from_bytes(data)
        self._table.receive(msg)  # type: ignore[attr-defined]
