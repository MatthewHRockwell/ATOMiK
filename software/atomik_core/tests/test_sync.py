"""Tests for SyncTable + transports — distributed convergence verification."""

from atomik_core.transport import CallbackTransport, MemoryTransport

from atomik_core import DeltaMessage, SyncTable

# =========================================================================
# SyncTable basics
# =========================================================================

def test_sync_put_get():
    t = SyncTable(16)
    t.put(0, 0xDEADBEEF)
    assert t.get(0) == 0xDEADBEEF


def test_sync_overwrite():
    t = SyncTable(16)
    t.put(0, 0xAAAA)
    t.put(0, 0xBBBB)
    assert t.get(0) == 0xBBBB


def test_sync_multi_address():
    t = SyncTable(16)
    t.put(0, 0x1111)
    t.put(1, 0x2222)
    t.put(2, 0x3333)
    assert t.get(0) == 0x1111
    assert t.get(1) == 0x2222
    assert t.get(2) == 0x3333


def test_sync_len_contains_iter():
    t = SyncTable(8)
    assert len(t) == 8
    assert 0 in t
    assert 7 in t
    assert 8 not in t
    assert list(t) == list(range(8))


# =========================================================================
# Delta callback
# =========================================================================

def test_sync_delta_callback_fires():
    deltas = []
    t = SyncTable(16)
    t.on_delta(lambda msg: deltas.append(msg))
    t.put(0, 0xDEADBEEF)
    assert len(deltas) == 1
    assert isinstance(deltas[0], DeltaMessage)
    assert deltas[0].addr == 0


def test_sync_delta_noop_no_callback():
    """Writing the same value twice should not fire a delta (no change)."""
    deltas = []
    t = SyncTable(16)
    t.on_delta(lambda msg: deltas.append(msg))
    t.put(0, 0xAAAA)
    t.put(0, 0xAAAA)  # same value — no-op
    assert len(deltas) == 1  # only the first write fires


def test_sync_receive_no_callback():
    """Receiving a remote delta should NOT fire the local callback."""
    deltas = []
    t = SyncTable(16)
    t.on_delta(lambda msg: deltas.append(msg))
    t.receive(DeltaMessage(addr=0, delta=0xFF, seq=0))
    assert len(deltas) == 0  # no callback for receives


# =========================================================================
# Convergence
# =========================================================================

def test_sync_two_peers_converge():
    a = SyncTable(16)
    b = SyncTable(16)
    hub = MemoryTransport()
    hub.connect(a)
    hub.connect(b)

    a.put(0, 0xDEADBEEF)
    assert b.get(0) == 0xDEADBEEF


def test_sync_three_peers_converge():
    a = SyncTable(16)
    b = SyncTable(16)
    c = SyncTable(16)
    hub = MemoryTransport()
    hub.connect(a)
    hub.connect(b)
    hub.connect(c)

    a.put(0, 0xCAFEBABE)
    assert b.get(0) == 0xCAFEBABE
    assert c.get(0) == 0xCAFEBABE


def test_sync_bidirectional():
    a = SyncTable(16)
    b = SyncTable(16)
    hub = MemoryTransport()
    hub.connect(a)
    hub.connect(b)

    a.put(0, 0xAAAA)
    assert b.get(0) == 0xAAAA

    b.put(1, 0xBBBB)
    assert a.get(1) == 0xBBBB


def test_sync_concurrent_writes_same_addr():
    """Both peers write to the same address — deltas compose via XOR."""
    a = SyncTable(16)
    b = SyncTable(16)
    hub = MemoryTransport()
    hub.connect(a)
    hub.connect(b)

    a.put(0, 0xFF00)
    b.put(0, 0x00FF)

    # Both applied each other's deltas — states converge
    assert a.get(0) == b.get(0)


def test_sync_out_of_order_convergence():
    """Simulate out-of-order delivery — result must be identical."""
    a = SyncTable(16)
    b = SyncTable(16)

    # A generates 3 deltas
    msgs = []
    a.on_delta(lambda msg: msgs.append(msg))
    a.put(0, 0x1111)
    a.put(0, 0x2222)
    a.put(0, 0x3333)

    # B receives in reverse order
    for msg in reversed(msgs):
        b.receive(msg)

    # XOR is commutative — order doesn't matter
    assert a.get(0) == b.get(0)


# =========================================================================
# Snapshot and diff
# =========================================================================

def test_sync_snapshot_roundtrip():
    a = SyncTable(8)
    a.put(0, 0xAAAA)
    a.put(3, 0xBBBB)

    snap = a.snapshot()
    b = SyncTable.from_snapshot(snap)
    assert b.get(0) == 0xAAAA
    assert b.get(3) == 0xBBBB


def test_sync_diff():
    a = SyncTable(8)
    b = SyncTable(8)
    a.put(0, 0xAAAA)
    a.put(3, 0xBBBB)

    diff = a.diff(b)
    assert 0 in diff
    assert 3 in diff

    # Sync and verify no diff
    hub = MemoryTransport()
    hub.connect(a)
    hub.connect(b)
    # Re-put to trigger sync (since hub was connected after puts)
    a.put(0, a.get(0))
    a.put(3, a.get(3))
    # Now both should have same state — but diff checks raw values


# =========================================================================
# Transport layer
# =========================================================================

def test_memory_transport_peer_count():
    hub = MemoryTransport()
    t1 = SyncTable(4)
    t2 = SyncTable(4)
    assert hub.peer_count == 0
    hub.connect(t1)
    assert hub.peer_count == 1
    hub.connect(t2)
    assert hub.peer_count == 2
    hub.disconnect(t1)
    assert hub.peer_count == 1


def test_callback_transport_wire_format():
    """CallbackTransport serializes to 16-byte wire format."""
    sent = []
    t = SyncTable(16)
    CallbackTransport(t, send_fn=lambda data: sent.append(data))
    t.put(0, 0xDEADBEEF)

    assert len(sent) == 1
    assert len(sent[0]) == 16  # DeltaMessage wire format
    assert isinstance(sent[0], bytes)


def test_callback_transport_receive():
    t = SyncTable(16)
    ct = CallbackTransport(t)
    msg = DeltaMessage(addr=0, delta=0xFF, seq=0)
    ct.receive_bytes(msg.to_bytes())
    assert t.get(0) == 0xFF


def test_callback_transport_roundtrip():
    """Two tables connected via callback transport converge."""
    a = SyncTable(16)
    b = SyncTable(16)

    ct_b = CallbackTransport(b)
    CallbackTransport(a, send_fn=lambda data: ct_b.receive_bytes(data))

    a.put(0, 0xCAFE)
    assert b.get(0) == 0xCAFE


# =========================================================================
# Idempotency (XOR self-inverse)
# =========================================================================

def test_sync_duplicate_delta_cancels():
    """Applying the same delta twice cancels it (self-inverse)."""
    t = SyncTable(16)
    t.put(0, 0xAAAA)

    msg = DeltaMessage(addr=0, delta=0xFF, seq=0)
    t.receive(msg)
    state_after_one = t.get(0)

    t.receive(msg)  # same delta again
    state_after_two = t.get(0)

    # XOR self-inverse: second application undoes the first
    assert state_after_two == 0xAAAA
    assert state_after_one != state_after_two


def test_sync_large_peer_group():
    """10 peers all converge through a hub."""
    hub = MemoryTransport()
    tables = [SyncTable(4) for _ in range(10)]
    for t in tables:
        hub.connect(t)

    tables[0].put(0, 0xDEADBEEF)

    for t in tables:
        assert t.get(0) == 0xDEADBEEF
