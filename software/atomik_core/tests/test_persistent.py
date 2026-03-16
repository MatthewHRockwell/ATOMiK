"""Tests for PersistentSyncTable — crash-safe delta persistence."""

import os
import tempfile

import pytest

from atomik_core import DeltaMessage
from atomik_core.persistent import PersistentSyncTable, _RECORD_SIZE
from atomik_core.transport import MemoryTransport


@pytest.fixture
def wal_path():
    """Provide a temporary WAL path and clean up after the test."""
    path = tempfile.mktemp(suffix=".wal")
    yield path
    # Clean up WAL and any compaction temp files
    for p in (path, path + ".tmp"):
        try:
            os.unlink(p)
        except FileNotFoundError:
            pass


# =========================================================================
# Basic persistence
# =========================================================================

def test_persist_put_get(wal_path):
    """Write, close, reopen — state survives restart."""
    t = PersistentSyncTable(16, path=wal_path)
    t.put(0, 0xDEADBEEF)
    t.put(5, 0xCAFEBABE)
    t.close()

    # Reopen — WAL replay reconstructs state
    t2 = PersistentSyncTable(16, path=wal_path)
    assert t2.get(0) == 0xDEADBEEF
    assert t2.get(5) == 0xCAFEBABE
    t2.close()


def test_persist_crash_recovery(wal_path):
    """Simulate crash (no close) — WAL is still recoverable."""
    t = PersistentSyncTable(16, path=wal_path)
    t.put(0, 0x1111)
    t.put(1, 0x2222)
    t.put(2, 0x3333)
    # No close — simulates crash. OS flushes are already done
    # because put() calls flush() after each write.

    # Reopen — replays from the WAL file on disk
    t2 = PersistentSyncTable(16, path=wal_path)
    assert t2.get(0) == 0x1111
    assert t2.get(1) == 0x2222
    assert t2.get(2) == 0x3333
    t2.close()


def test_persist_receive_persists(wal_path):
    """Remote deltas received via receive() are also persisted."""
    t = PersistentSyncTable(16, path=wal_path)
    msg = DeltaMessage(addr=3, delta=0xBEEF, seq=42)
    t.receive(msg)
    assert t.get(3) == 0xBEEF
    t.close()

    # Reopen — the received delta was persisted
    t2 = PersistentSyncTable(16, path=wal_path)
    assert t2.get(3) == 0xBEEF
    t2.close()


# =========================================================================
# Compaction
# =========================================================================

def test_persist_compact(wal_path):
    """Compaction reduces WAL size while preserving state."""
    t = PersistentSyncTable(16, path=wal_path)

    # Write many deltas to the same address (lots of overwrites)
    for i in range(100):
        t.put(0, i)
    # Final state: addr 0 = 99
    assert t.get(0) == 99

    size_before = os.path.getsize(wal_path)

    t.compact()

    size_after = os.path.getsize(wal_path)
    # 100 delta records compacted to 1 snapshot record
    assert size_after < size_before
    assert size_after == _RECORD_SIZE  # exactly one non-zero context

    # State is still correct after compaction
    assert t.get(0) == 99

    # Close and reopen — snapshot record replays correctly
    t.close()
    t2 = PersistentSyncTable(16, path=wal_path)
    assert t2.get(0) == 99
    t2.close()


def test_persist_compact_multi_address(wal_path):
    """Compaction preserves state across multiple addresses."""
    t = PersistentSyncTable(16, path=wal_path)
    t.put(0, 0xAAAA)
    t.put(5, 0xBBBB)
    t.put(10, 0xCCCC)
    t.compact()
    t.close()

    t2 = PersistentSyncTable(16, path=wal_path)
    assert t2.get(0) == 0xAAAA
    assert t2.get(5) == 0xBBBB
    assert t2.get(10) == 0xCCCC
    t2.close()


# =========================================================================
# Context manager
# =========================================================================

def test_persist_context_manager(wal_path):
    """PersistentSyncTable works as a context manager."""
    with PersistentSyncTable(16, path=wal_path) as t:
        t.put(0, 0xDEAD)
        assert t.get(0) == 0xDEAD

    # WAL was flushed on __exit__ — reopen works
    with PersistentSyncTable(16, path=wal_path) as t2:
        assert t2.get(0) == 0xDEAD


# =========================================================================
# Two-peer persistence
# =========================================================================

def test_persist_two_peers(wal_path):
    """Two PersistentSyncTables connected via MemoryTransport both persist."""
    path_a = wal_path
    path_b = wal_path + ".peer_b"

    try:
        hub = MemoryTransport()
        a = PersistentSyncTable(16, path=path_a)
        b = PersistentSyncTable(16, path=path_b)
        hub.connect(a)
        hub.connect(b)

        a.put(0, 0xDEADBEEF)
        assert b.get(0) == 0xDEADBEEF

        b.put(1, 0xCAFEBABE)
        assert a.get(1) == 0xCAFEBABE

        a.close()
        b.close()

        # Reopen both — each has the full converged state
        a2 = PersistentSyncTable(16, path=path_a)
        b2 = PersistentSyncTable(16, path=path_b)
        assert a2.get(0) == 0xDEADBEEF
        assert a2.get(1) == 0xCAFEBABE
        assert b2.get(0) == 0xDEADBEEF
        assert b2.get(1) == 0xCAFEBABE
        a2.close()
        b2.close()
    finally:
        try:
            os.unlink(path_b)
        except FileNotFoundError:
            pass


# =========================================================================
# Delegation / container protocol
# =========================================================================

def test_persist_len_contains_iter(wal_path):
    """Container protocol delegates correctly to SyncTable."""
    with PersistentSyncTable(8, path=wal_path) as t:
        assert len(t) == 8
        assert 0 in t
        assert 7 in t
        assert 8 not in t
        assert list(t) == list(range(8))


def test_persist_snapshot_diff(wal_path):
    """snapshot() and diff() delegate correctly."""
    path_a = wal_path
    path_b = wal_path + ".peer_b"

    try:
        a = PersistentSyncTable(8, path=path_a)
        b = PersistentSyncTable(8, path=path_b)
        a.put(0, 0xAAAA)
        a.put(3, 0xBBBB)

        snap = a.snapshot()
        assert snap[0] == 0xAAAA
        assert snap[3] == 0xBBBB

        diff = a.diff(b)
        assert 0 in diff
        assert 3 in diff

        a.close()
        b.close()
    finally:
        try:
            os.unlink(path_b)
        except FileNotFoundError:
            pass


def test_persist_on_delta_callback(wal_path):
    """on_delta callback fires for local puts."""
    deltas = []
    with PersistentSyncTable(8, path=wal_path, on_delta=lambda m: deltas.append(m)) as t:
        t.put(0, 0xFF)
        assert len(deltas) == 1
        assert deltas[0].addr == 0


def test_persist_overwrite_same_address(wal_path):
    """Multiple writes to same address — final state survives restart."""
    with PersistentSyncTable(8, path=wal_path) as t:
        t.put(0, 0xAAAA)
        t.put(0, 0xBBBB)
        t.put(0, 0xCCCC)
        assert t.get(0) == 0xCCCC

    with PersistentSyncTable(8, path=wal_path) as t2:
        assert t2.get(0) == 0xCCCC


def test_persist_no_wal_fresh_start(wal_path):
    """When no WAL exists, table starts empty."""
    with PersistentSyncTable(8, path=wal_path) as t:
        for addr in t:
            assert t.get(addr) == 0


def test_persist_partial_record_ignored(wal_path):
    """A truncated trailing record (crash mid-write) is silently ignored."""
    with PersistentSyncTable(8, path=wal_path) as t:
        t.put(0, 0xDEAD)

    # Append a partial record to simulate crash mid-write
    with open(wal_path, "ab") as f:
        f.write(b"\x00" * 7)  # 7 bytes < 16-byte record

    # Reopen — the good record is replayed, partial is ignored
    with PersistentSyncTable(8, path=wal_path) as t2:
        assert t2.get(0) == 0xDEAD
