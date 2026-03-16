"""Tests for DeltaStream — network state synchronization."""

from atomik_core import DeltaMessage, DeltaStream, SyncTable


def test_send_receive_converge():
    """Two streams with same reference converge after exchanging deltas."""
    sender = DeltaStream()
    receiver = DeltaStream()

    sender.load(0, 0xCAFEBABE)
    receiver.load(0, 0xCAFEBABE)

    msg = sender.accum(0, 0x000000FF)
    receiver.apply(msg)

    assert sender.read(0) == receiver.read(0)


def test_multiple_deltas_converge():
    sender = DeltaStream()
    receiver = DeltaStream()

    sender.load(0, 0x1000)
    receiver.load(0, 0x1000)

    msgs = []
    for delta in [0x0001, 0x0010, 0x0100]:
        msgs.append(sender.accum(0, delta))

    for msg in msgs:
        receiver.apply(msg)

    assert sender.read(0) == receiver.read(0)


def test_out_of_order_convergence():
    """Deltas applied in different order still converge (commutativity)."""
    sender = DeltaStream()
    receiver = DeltaStream()

    sender.load(0, 0xAAAA)
    receiver.load(0, 0xAAAA)

    m1 = sender.accum(0, 0x0011)
    m2 = sender.accum(0, 0x1100)
    m3 = sender.accum(0, 0x0F0F)

    # Apply in reverse order
    receiver.apply(m3)
    receiver.apply(m1)
    receiver.apply(m2)

    assert sender.read(0) == receiver.read(0)


def test_multi_address_stream():
    s = DeltaStream()
    s.load(0, 0x1111)
    s.load(1, 0x2222)
    s.accum(0, 0x00FF)
    s.accum(1, 0xFF00)
    assert s.read(0) == 0x1111 ^ 0x00FF
    assert s.read(1) == 0x2222 ^ 0xFF00


def test_pending_messages():
    s = DeltaStream()
    s.load(0, 0)
    s.accum(0, 0x11)
    s.accum(0, 0x22)
    msgs = s.pending()
    assert len(msgs) == 2
    assert s.pending() == []  # cleared


def test_delta_message_fields():
    s = DeltaStream()
    s.load(0, 0)
    msg = s.accum(0, 0xBEEF)
    assert msg.addr == 0
    assert msg.delta == 0xBEEF
    assert msg.seq > 0


def test_three_node_convergence():
    """Three nodes all converge to same state."""
    nodes = [DeltaStream() for _ in range(3)]
    for n in nodes:
        n.load(0, 0xAAAA)

    # Each node produces one delta
    m0 = nodes[0].accum(0, 0x0001)
    m1 = nodes[1].accum(0, 0x0010)
    m2 = nodes[2].accum(0, 0x0100)

    # All-to-all broadcast
    for i, node in enumerate(nodes):
        for j, msg in enumerate([m0, m1, m2]):
            if i != j:
                node.apply(msg)

    assert nodes[0].read(0) == nodes[1].read(0) == nodes[2].read(0)


def test_uninitialized_address_reads_zero():
    s = DeltaStream()
    assert s.read(42) == 0


# =========================================================================
# Serialization (to_dict / from_dict / to_bytes / from_bytes)
# =========================================================================

def test_delta_message_to_dict():
    """Round-trip to_dict / from_dict preserves all fields."""
    msg = DeltaMessage(addr=7, delta=0xDEADBEEFCAFEBABE, seq=42)
    d = msg.to_dict()
    restored = DeltaMessage.from_dict(d)
    assert restored.addr == msg.addr
    assert restored.delta == msg.delta
    assert restored.seq == msg.seq


def test_delta_message_to_bytes():
    """Round-trip to_bytes / from_bytes preserves all fields, exactly 16 bytes."""
    msg = DeltaMessage(addr=0x100, delta=0xCAFEBABEDEADBEEF, seq=99)
    raw = msg.to_bytes()
    assert len(raw) == 16
    restored = DeltaMessage.from_bytes(raw)
    assert restored.addr == msg.addr
    assert restored.delta == msg.delta
    assert restored.seq == msg.seq


def test_delta_message_bytes_invalid_length():
    """from_bytes with wrong length raises ValueError."""
    import pytest
    with pytest.raises(ValueError):
        DeltaMessage.from_bytes(b"\x00" * 10)
    with pytest.raises(ValueError):
        DeltaMessage.from_bytes(b"\x00" * 20)
    with pytest.raises(ValueError):
        DeltaMessage.from_bytes(b"")


def test_delta_message_dict_fields():
    """to_dict returns exactly {addr, delta, seq} keys."""
    msg = DeltaMessage(addr=1, delta=2, seq=3)
    d = msg.to_dict()
    assert set(d.keys()) == {"addr", "delta", "seq"}


# =========================================================================
# SyncTable — high-level distributed state synchronization
# =========================================================================

def test_sync_table_put_get():
    """put() sets state, get() reads it back."""
    t = SyncTable(num_contexts=16)
    t.put(addr=0, value=0xDEADBEEF)
    assert t.get(0) == 0xDEADBEEF

    # Overwrite
    t.put(addr=0, value=0xCAFEBABE)
    assert t.get(0) == 0xCAFEBABE

    # Multiple addresses
    t.put(addr=5, value=0x1234)
    assert t.get(5) == 0x1234
    assert t.get(0) == 0xCAFEBABE  # untouched

    # len, contains, iteration
    assert len(t) == 16
    assert 0 in t
    assert 15 in t
    assert 16 not in t
    assert list(t) == list(range(16))


def test_sync_table_delta_callback():
    """on_delta callback fires on local put(), not on receive()."""
    sent = []
    t = SyncTable(num_contexts=4)
    t.on_delta(lambda msg: sent.append(msg))

    t.put(addr=0, value=0xFF)
    assert len(sent) == 1
    assert sent[0].addr == 0
    assert sent[0].delta == 0xFF  # 0x00 ^ 0xFF

    # No-op write (same value) should NOT fire callback
    t.put(addr=0, value=0xFF)
    assert len(sent) == 1

    # receive() must NOT fire callback
    t.receive(DeltaMessage(addr=1, delta=0xAA, seq=99))
    assert len(sent) == 1  # still 1


def test_sync_table_receive_converges():
    """Applying a remote delta produces the correct state."""
    alice = SyncTable(num_contexts=4)
    bob = SyncTable(num_contexts=4)

    # Alice writes
    msg = alice.put(addr=0, value=0xBEEF)

    # Bob receives the delta
    bob.receive(msg)
    assert bob.get(0) == alice.get(0) == 0xBEEF


def test_sync_table_two_way_sync():
    """Full two-way sync: both peers write, both converge."""
    alice = SyncTable(num_contexts=8)
    bob = SyncTable(num_contexts=8)

    alice_outbox: list[DeltaMessage] = []
    bob_outbox: list[DeltaMessage] = []
    alice.on_delta(lambda msg: alice_outbox.append(msg))
    bob.on_delta(lambda msg: bob_outbox.append(msg))

    # Alice writes to addr 0, Bob writes to addr 1
    alice.put(addr=0, value=0xAAAA)
    bob.put(addr=1, value=0xBBBB)

    # Exchange deltas (order should not matter — commutativity)
    for msg in alice_outbox:
        bob.receive(msg)
    for msg in bob_outbox:
        alice.receive(msg)

    assert alice.get(0) == bob.get(0) == 0xAAAA
    assert alice.get(1) == bob.get(1) == 0xBBBB
    assert alice.diff(bob) == []

    # Both write to the SAME address in sequence
    msg_a = alice.put(addr=2, value=0x1111)
    bob.receive(msg_a)
    msg_b = bob.put(addr=2, value=0x2222)
    alice.receive(msg_b)

    # After full exchange, both see the latest value
    assert alice.get(2) == bob.get(2)
    assert alice.diff(bob) == []


def test_sync_table_diff():
    """diff() returns addresses where two tables disagree."""
    a = SyncTable(num_contexts=8)
    b = SyncTable(num_contexts=8)

    # Start identical — no diffs
    assert a.diff(b) == []

    # Diverge on addr 2 and 5
    a.put(addr=2, value=0xFF)
    a.put(addr=5, value=0xAA)

    assert a.diff(b) == [2, 5]

    # Sync addr 2 only
    b.put(addr=2, value=0xFF)
    assert a.diff(b) == [5]

    # Full sync
    b.put(addr=5, value=0xAA)
    assert a.diff(b) == []

    # Snapshot round-trip
    snap = a.snapshot()
    restored = SyncTable.from_snapshot(snap)
    assert restored.get(2) == 0xFF
    assert restored.get(5) == 0xAA
