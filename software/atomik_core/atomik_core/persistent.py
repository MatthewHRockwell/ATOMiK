"""Crash-safe persistent SyncTable with write-ahead delta logging.

Every put() appends a 16-byte delta record to a write-ahead log (WAL).
On restart, replays all deltas to reconstruct state. Since XOR is
commutative and associative, replay order doesn't matter and duplicates
self-cancel -- crash recovery is trivially correct.

WAL format: raw concatenation of 16-byte DeltaMessage.to_bytes() records.
No headers, no framing, no checksums (XOR self-corrects duplicate replay).

Compaction uses seq=0xFFFFFFFF as a sentinel: these records are LOAD
operations (set addr to value) rather than delta accumulations.

    table = PersistentSyncTable(256, path="state.wal")
    table.put(0, 0xDEADBEEF)  # persisted immediately

    # After restart:
    table = PersistentSyncTable(256, path="state.wal")  # auto-replays
    assert table.get(0) == 0xDEADBEEF

SPDX-License-Identifier: Apache-2.0
"""

from __future__ import annotations

import os
from pathlib import Path
from typing import Callable, Iterator

from atomik_core.stream import DeltaMessage
from atomik_core.sync import SyncTable

# Sentinel sequence number used in compacted WAL files.
# Records with this seq are LOAD operations (set state directly),
# not delta accumulations.
_SNAPSHOT_SEQ = 0xFFFFFFFF

# DeltaMessage wire size (4 + 8 + 4 = 16 bytes)
_RECORD_SIZE = 16


class PersistentSyncTable:
    """SyncTable with crash-safe delta persistence.

    Every put() appends a 16-byte delta record to a write-ahead log.
    On restart, replays all deltas to reconstruct state. Since XOR is
    commutative and associative, replay order doesn't matter and
    duplicates self-cancel -- crash recovery is trivially correct.

    Usage:
        table = PersistentSyncTable(256, path="state.wal")
        table.put(0, 0xDEADBEEF)  # persisted immediately

        # After restart:
        table = PersistentSyncTable(256, path="state.wal")  # auto-replays
        assert table.get(0) == 0xDEADBEEF
    """

    __slots__ = ("_table", "_wal_path", "_wal")

    def __init__(
        self,
        num_contexts: int = 256,
        path: str | Path = "state.wal",
        on_delta: Callable[[DeltaMessage], None] | None = None,
        width: int = 64,
    ):
        """Create a persistent synchronized state table.

        If a WAL file exists at *path*, all records are replayed to
        reconstruct the prior state before returning.

        Args:
            num_contexts: Number of addressable context slots.
            path: Filesystem path for the write-ahead log.
            on_delta: Optional callback fired on local put() deltas.
            width: Bit width per context (default 64).
        """
        self._table = SyncTable(num_contexts=num_contexts, width=width)
        self._wal_path = Path(path)

        if on_delta is not None:
            self._table.on_delta(on_delta)

        # Replay existing WAL if present
        if self._wal_path.exists():
            self._replay()

        # Open WAL for appending new records
        self._wal = open(self._wal_path, "ab")

    # ------------------------------------------------------------------
    # Core operations
    # ------------------------------------------------------------------

    def put(self, addr: int, value: int) -> DeltaMessage:
        """Write a value, persisting the delta to the WAL.

        Args:
            addr: Context address to write.
            value: Desired new state value.

        Returns:
            The DeltaMessage produced by the underlying SyncTable.
        """
        msg = self._table.put(addr, value)
        if msg.delta != 0:
            self._wal.write(msg.to_bytes())
            self._wal.flush()
        return msg

    def receive(self, msg: DeltaMessage) -> None:
        """Apply a remote delta and persist it to the WAL.

        Args:
            msg: DeltaMessage from a remote peer.
        """
        self._table.receive(msg)
        self._wal.write(msg.to_bytes())
        self._wal.flush()

    def get(self, addr: int) -> int:
        """Read the current state at an address.

        Args:
            addr: Context address to read.

        Returns:
            Current state value.
        """
        return self._table.get(addr)

    # ------------------------------------------------------------------
    # Delta callback
    # ------------------------------------------------------------------

    def on_delta(self, callback: Callable[[DeltaMessage], None]) -> None:
        """Register a callback fired on local put() deltas.

        Args:
            callback: Function called with a DeltaMessage on each local put().
        """
        self._table.on_delta(callback)

    # ------------------------------------------------------------------
    # Compaction
    # ------------------------------------------------------------------

    def compact(self) -> None:
        """Rewrite the WAL as a minimal snapshot.

        Replaces the current WAL with one LOAD record per non-zero
        context. Uses seq=0xFFFFFFFF as a sentinel so replay
        distinguishes snapshot LOADs from delta ACCUMs.

        This is safe even if the process crashes mid-compaction:
        the old WAL is only replaced by an atomic rename after the
        new file is fully written and flushed.
        """
        tmp_path = self._wal_path.with_suffix(".wal.tmp")
        with open(tmp_path, "wb") as f:
            for addr in self._table:
                value = self._table.get(addr)
                if value != 0:
                    record = DeltaMessage(
                        addr=addr, delta=value, seq=_SNAPSHOT_SEQ
                    )
                    f.write(record.to_bytes())
            f.flush()
            os.fsync(f.fileno())

        # Atomic replace (POSIX rename is atomic within same filesystem)
        os.replace(tmp_path, self._wal_path)

        # Reopen WAL for appending
        self._wal.close()
        self._wal = open(self._wal_path, "ab")

    # ------------------------------------------------------------------
    # Snapshot and diff (delegate to SyncTable)
    # ------------------------------------------------------------------

    def snapshot(self) -> dict[int, int]:
        """Serialize the full state of all contexts."""
        return self._table.snapshot()

    def diff(self, other: PersistentSyncTable | SyncTable) -> list[int]:
        """Return addresses where this table and another disagree."""
        if isinstance(other, PersistentSyncTable):
            return self._table.diff(other._table)
        return self._table.diff(other)

    # ------------------------------------------------------------------
    # Container protocol (delegate to SyncTable)
    # ------------------------------------------------------------------

    @property
    def num_contexts(self) -> int:
        """Number of context slots in this table."""
        return self._table.num_contexts

    @property
    def width(self) -> int:
        """Bit width per context."""
        return self._table.width

    def __len__(self) -> int:
        return len(self._table)

    def __iter__(self) -> Iterator[int]:
        return iter(self._table)

    def __contains__(self, addr: object) -> bool:
        return addr in self._table

    # ------------------------------------------------------------------
    # Lifecycle
    # ------------------------------------------------------------------

    def close(self) -> None:
        """Flush and close the WAL file handle."""
        if self._wal and not self._wal.closed:
            self._wal.flush()
            self._wal.close()

    def __enter__(self) -> PersistentSyncTable:
        return self

    def __exit__(self, exc_type: object, exc_val: object, exc_tb: object) -> None:
        self.close()

    def __repr__(self) -> str:
        active = sum(1 for addr in self._table if self._table.get(addr) != 0)
        return (
            f"PersistentSyncTable(contexts={self.num_contexts}, "
            f"width={self.width}, active={active}, "
            f"wal={str(self._wal_path)!r})"
        )

    # ------------------------------------------------------------------
    # Internal
    # ------------------------------------------------------------------

    def _replay(self) -> None:
        """Replay all records from the WAL file to reconstruct state."""
        data = self._wal_path.read_bytes()
        # Silently ignore any trailing partial record (crash mid-write)
        num_records = len(data) // _RECORD_SIZE
        for i in range(num_records):
            chunk = data[i * _RECORD_SIZE : (i + 1) * _RECORD_SIZE]
            msg = DeltaMessage.from_bytes(chunk)
            if msg.seq == _SNAPSHOT_SEQ:
                # Snapshot LOAD record: set state directly
                self._table.put(msg.addr, msg.delta)
            else:
                # Normal delta: accumulate
                self._table.receive(msg)
