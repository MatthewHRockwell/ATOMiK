"""Tests for DeltaStream — network state synchronization."""

from atomik_core import DeltaStream, DeltaMessage


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
