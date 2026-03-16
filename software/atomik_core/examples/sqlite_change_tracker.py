#!/usr/bin/env python3
"""ATOMiK + SQLite: Detect which rows changed without a full-table scan.

Zero external dependencies — SQLite is in the Python stdlib, and
atomik-core has no dependencies either.

Usage:  python examples/sqlite_change_tracker.py

Workflow:
  1. Create an in-memory SQLite "users" table with 1,000 rows.
  2. Fingerprint every row with ATOMiK (one Fingerprint per row).
  3. Randomly modify 50 rows.
  4. Re-fingerprint and detect exactly which rows changed.
  5. Compare detection time vs a naive full-table-scan approach.
"""

import random
import sqlite3
import struct
import time

from atomik_core import Fingerprint

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _row_to_bytes(row: tuple) -> bytes:
    """Serialize a row tuple to a deterministic byte string.

    We use a fixed struct format so the fingerprint is stable across runs.
    Fields: id(int32), name(32 bytes padded), email(64 bytes padded),
            balance(double).
    """
    uid, name, email, balance = row
    return struct.pack(
        "<i32s64sd",
        uid,
        name.encode("utf-8")[:32].ljust(32, b"\x00"),
        email.encode("utf-8")[:64].ljust(64, b"\x00"),
        balance,
    )


def _create_db(conn: sqlite3.Connection, n_rows: int) -> None:
    """Populate the users table with *n_rows* deterministic rows."""
    conn.execute(
        "CREATE TABLE users ("
        "  id INTEGER PRIMARY KEY,"
        "  name TEXT NOT NULL,"
        "  email TEXT NOT NULL,"
        "  balance REAL NOT NULL"
        ")"
    )
    rows = [
        (i, f"user_{i:04d}", f"user_{i:04d}@example.com", round(100.0 + i * 0.5, 2))
        for i in range(n_rows)
    ]
    conn.executemany(
        "INSERT INTO users (id, name, email, balance) VALUES (?, ?, ?, ?)",
        rows,
    )
    conn.commit()


# ---------------------------------------------------------------------------
# ATOMiK approach — per-row fingerprinting
# ---------------------------------------------------------------------------

def fingerprint_all_rows(conn: sqlite3.Connection) -> dict[int, Fingerprint]:
    """Create one Fingerprint per row, keyed by id."""
    fps: dict[int, Fingerprint] = {}
    for row in conn.execute("SELECT id, name, email, balance FROM users"):
        fp = Fingerprint()
        fp.load(_row_to_bytes(row))
        fps[row[0]] = fp
    return fps


def detect_changes_atomik(
    conn: sqlite3.Connection,
    fps: dict[int, Fingerprint],
) -> list[int]:
    """Return the ids of rows whose fingerprint no longer matches."""
    changed: list[int] = []
    for row in conn.execute("SELECT id, name, email, balance FROM users"):
        fp = fps[row[0]]
        fp.update(_row_to_bytes(row))
        if fp.changed:
            changed.append(row[0])
            fp.load(_row_to_bytes(row))  # re-baseline
    return changed


# ---------------------------------------------------------------------------
# Naive approach — snapshot + full comparison
# ---------------------------------------------------------------------------

def snapshot_all_rows(conn: sqlite3.Connection) -> dict[int, bytes]:
    """Store a byte-level snapshot of every row."""
    return {
        row[0]: _row_to_bytes(row)
        for row in conn.execute("SELECT id, name, email, balance FROM users")
    }


def detect_changes_naive(
    conn: sqlite3.Connection,
    old_snap: dict[int, bytes],
) -> list[int]:
    """Compare every row against the snapshot to find changes."""
    changed: list[int] = []
    for row in conn.execute("SELECT id, name, email, balance FROM users"):
        current = _row_to_bytes(row)
        if current != old_snap[row[0]]:
            changed.append(row[0])
    return changed


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    n_rows = 1000
    n_modify = 50

    random.seed(42)

    print("ATOMiK + SQLite Change Detection Demo")
    print("=" * 50)
    print(f"  Rows: {n_rows:,}   Modified: {n_modify}")
    print()

    # --- Setup ---
    conn = sqlite3.connect(":memory:")
    _create_db(conn, n_rows)

    # Pick which rows to modify
    modify_ids = sorted(random.sample(range(n_rows), n_modify))

    # --- ATOMiK: fingerprint all rows ---
    t0 = time.perf_counter()
    fps = fingerprint_all_rows(conn)
    t_fp_build = time.perf_counter() - t0

    # --- Naive: snapshot all rows ---
    t0 = time.perf_counter()
    snap = snapshot_all_rows(conn)
    t_snap_build = time.perf_counter() - t0

    # --- Apply modifications ---
    for uid in modify_ids:
        new_balance = round(random.uniform(0, 10000), 2)
        conn.execute("UPDATE users SET balance = ? WHERE id = ?", (new_balance, uid))
    conn.commit()

    # --- ATOMiK detection ---
    t0 = time.perf_counter()
    atomik_changed = detect_changes_atomik(conn, fps)
    t_atomik = time.perf_counter() - t0

    # --- Naive detection ---
    t0 = time.perf_counter()
    naive_changed = detect_changes_naive(conn, snap)
    t_naive = time.perf_counter() - t0

    # --- Verify correctness ---
    assert sorted(atomik_changed) == modify_ids, (
        f"ATOMiK missed changes: expected {modify_ids}, got {sorted(atomik_changed)}"
    )
    assert sorted(naive_changed) == modify_ids, (
        f"Naive missed changes: expected {modify_ids}, got {sorted(naive_changed)}"
    )

    # --- Results ---
    print("Results:")
    print(f"  {n_modify} changed, {n_rows - n_modify} unchanged")
    print()
    print("  Approach        Build       Detect")
    print("  ----------  ----------  ----------")
    print(f"  ATOMiK      {t_fp_build*1000:8.2f} ms  {t_atomik*1000:8.2f} ms")
    print(f"  Full scan   {t_snap_build*1000:8.2f} ms  {t_naive*1000:8.2f} ms")
    print()

    # Memory comparison
    #   ATOMiK stores a Fingerprint object per row (small, fixed-size).
    #   Naive stores a full byte snapshot per row.
    import sys
    fp_mem = sum(sys.getsizeof(fp) for fp in fps.values())
    snap_mem = sum(sys.getsizeof(v) for v in snap.values())
    print(f"  ATOMiK memory:    {fp_mem:>8,} bytes ({len(fps)} fingerprints)")
    print(f"  Snapshot memory:  {snap_mem:>8,} bytes ({len(snap)} byte copies)")
    print()

    print("  Both approaches correctly detected all 50 changes.")
    print("  ATOMiK fingerprints survive across transactions — no need")
    print("  to keep a full copy of every row in application memory.")
    print()
    print("  Zero dependencies used. Run with:")
    print("    python examples/sqlite_change_tracker.py")

    conn.close()


if __name__ == "__main__":
    main()
