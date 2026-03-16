"""Tests for async transport layer — AsyncCallbackTransport and AsyncSyncTable.

Uses asyncio.run() directly (no pytest-asyncio dependency required).
"""

import asyncio

from atomik_core.async_transport import AsyncCallbackTransport, AsyncSyncTable
from atomik_core.stream import DeltaMessage
from atomik_core.sync import SyncTable

# =========================================================================
# AsyncCallbackTransport
# =========================================================================

def test_async_callback_sends():
    """Verify async send_fn is called with serialized delta bytes."""
    sent = []

    async def fake_send(data: bytes):
        sent.append(data)

    async def run():
        table = SyncTable(16)
        transport = AsyncCallbackTransport(table, send_fn=fake_send)
        table.put(0, 0xDEADBEEF)
        assert transport.pending_count == 1
        count = await transport.send_pending()
        assert count == 1
        assert transport.pending_count == 0
        assert len(sent) == 1
        assert isinstance(sent[0], bytes)
        assert len(sent[0]) == 16  # DeltaMessage wire format

    asyncio.run(run())


def test_async_callback_no_send_fn():
    """Transport without send_fn queues but drains silently."""
    async def run():
        table = SyncTable(16)
        transport = AsyncCallbackTransport(table, send_fn=None)
        table.put(0, 0xAAAA)
        assert transport.pending_count == 1
        count = await transport.send_pending()
        assert count == 1
        assert transport.pending_count == 0

    asyncio.run(run())


def test_async_callback_multiple_deltas():
    """Multiple puts queue multiple deltas, all sent in one flush."""
    sent = []

    async def fake_send(data: bytes):
        sent.append(data)

    async def run():
        table = SyncTable(16)
        transport = AsyncCallbackTransport(table, send_fn=fake_send)
        table.put(0, 0x1111)
        table.put(1, 0x2222)
        table.put(2, 0x3333)
        assert transport.pending_count == 3
        count = await transport.send_pending()
        assert count == 3
        assert len(sent) == 3

    asyncio.run(run())


def test_async_receive():
    """Verify receive_bytes applies delta synchronously."""
    table = SyncTable(16)
    transport = AsyncCallbackTransport(table)

    msg = DeltaMessage(addr=0, delta=0xFF, seq=0)
    transport.receive_bytes(msg.to_bytes())
    assert table.get(0) == 0xFF


def test_async_receive_multiple():
    """Multiple receives accumulate correctly (XOR algebra)."""
    table = SyncTable(16)
    transport = AsyncCallbackTransport(table)

    transport.receive_bytes(DeltaMessage(addr=0, delta=0xFF00, seq=0).to_bytes())
    transport.receive_bytes(DeltaMessage(addr=0, delta=0x00FF, seq=1).to_bytes())
    assert table.get(0) == 0xFFFF


def test_async_roundtrip():
    """Two AsyncSyncTables exchanging deltas converge."""
    sent_a_to_b = []
    sent_b_to_a = []

    async def send_to_b(data: bytes):
        sent_a_to_b.append(data)

    async def send_to_a(data: bytes):
        sent_b_to_a.append(data)

    async def run():
        table_a = AsyncSyncTable(16, send_fn=send_to_b)
        table_b = AsyncSyncTable(16, send_fn=send_to_a)

        # A writes, sends delta to B
        await table_a.put(0, 0xDEADBEEF)
        assert len(sent_a_to_b) == 1

        # B receives A's delta
        table_b.receive_bytes(sent_a_to_b[0])
        assert table_b.get(0) == 0xDEADBEEF

        # B writes, sends delta to A
        await table_b.put(1, 0xCAFEBABE)
        assert len(sent_b_to_a) == 1

        # A receives B's delta
        table_a.receive_bytes(sent_b_to_a[0])
        assert table_a.get(1) == 0xCAFEBABE

        # Both tables have converged
        assert table_a.get(0) == table_b.get(0)
        assert table_a.get(1) == table_b.get(1)

    asyncio.run(run())


def test_async_roundtrip_same_address():
    """Both peers write to the same address — deltas compose via XOR."""
    wire_a = []
    wire_b = []

    async def send_a(data: bytes):
        wire_a.append(data)

    async def send_b(data: bytes):
        wire_b.append(data)

    async def run():
        a = AsyncSyncTable(16, send_fn=send_a)
        b = AsyncSyncTable(16, send_fn=send_b)

        await a.put(0, 0xFF00)
        b.receive_bytes(wire_a[0])

        await b.put(0, 0x00FF)
        a.receive_bytes(wire_b[0])

        # Both see the same converged state
        assert a.get(0) == b.get(0)

    asyncio.run(run())


# =========================================================================
# AsyncSyncTable properties
# =========================================================================

def test_async_sync_table_properties():
    """AsyncSyncTable exposes num_contexts, width, snapshot."""
    async def run():
        t = AsyncSyncTable(32, width=64)
        assert t.num_contexts == 32
        assert t.width == 64

        await t.put(0, 0xAAAA)
        snap = t.snapshot()
        assert snap[0] == 0xAAAA

    asyncio.run(run())


def test_async_sync_table_get_is_sync():
    """get() is synchronous — no await needed."""
    async def run():
        t = AsyncSyncTable(16)
        await t.put(0, 0x1234)
        # get is sync, called without await
        assert t.get(0) == 0x1234

    asyncio.run(run())


def test_async_sync_table_receive_is_sync():
    """receive() is synchronous — no await needed."""
    t = AsyncSyncTable(16)
    msg = DeltaMessage(addr=0, delta=0xBEEF, seq=1)
    t.receive(msg)
    assert t.get(0) == 0xBEEF


def test_async_sync_table_noop_put():
    """Writing the same value twice does not send a delta."""
    sent = []

    async def fake_send(data: bytes):
        sent.append(data)

    async def run():
        t = AsyncSyncTable(16, send_fn=fake_send)
        await t.put(0, 0xAAAA)
        await t.put(0, 0xAAAA)  # same value — no delta
        assert len(sent) == 1  # only the first write sends

    asyncio.run(run())


def test_async_sync_table_diff():
    """diff() works between two AsyncSyncTables."""
    async def run():
        a = AsyncSyncTable(8)
        b = AsyncSyncTable(8)

        await a.put(0, 0x1111)
        await a.put(3, 0x3333)

        diffs = a.diff(b)
        assert 0 in diffs
        assert 3 in diffs

    asyncio.run(run())
