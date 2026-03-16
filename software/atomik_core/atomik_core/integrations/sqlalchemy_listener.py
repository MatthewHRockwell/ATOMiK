"""ATOMiK SQLAlchemy listener — fingerprint rows before flush, skip unchanged writes.

Usage:
    pip install atomik-core[sqlalchemy]

    from atomik_core.integrations.sqlalchemy_listener import AtomikSessionTracker
    tracker = AtomikSessionTracker(session)
    # Automatically fingerprints dirty objects before flush
    # Reports which objects actually changed vs were just marked dirty
"""

from __future__ import annotations

from atomik_core.fingerprint import Fingerprint


def _require_sqlalchemy():
    """Import and return the ``sqlalchemy`` module, raising a helpful error if absent."""
    try:
        import sqlalchemy
    except ImportError:
        raise ImportError(
            "The 'sqlalchemy' package (>= 2.0) is required for AtomikSessionTracker.\n"
            "Install it with:  pip install atomik-core[sqlalchemy]\n"
            "  or:             pip install sqlalchemy"
        ) from None
    return sqlalchemy


def _column_snapshot(obj) -> bytes:
    """Serialize an ORM instance's mapped column values to bytes for fingerprinting.

    Uses SQLAlchemy's ``inspect()`` to iterate over column attributes
    without requiring any model modifications.
    """
    sa = _require_sqlalchemy()
    insp = sa.inspect(obj)
    parts: list[bytes] = []
    for attr in insp.mapper.column_attrs:
        val = getattr(obj, attr.key)
        parts.append(repr(val).encode("utf-8"))
    return b"\x00".join(parts)


class AtomikSessionTracker:
    """Track a SQLAlchemy session and detect false-dirty objects before flush.

    Registers a ``before_flush`` listener on the given session.  Each time
    SQLAlchemy is about to flush, every object in ``session.dirty`` is
    fingerprinted and compared against its previous fingerprint.  Objects
    whose column values have not actually changed are counted as
    *false dirty* — they were marked dirty (e.g. by attribute access) but
    carry no real modification.

    Args:
        session: A SQLAlchemy ``Session`` instance.
        fingerprint_width: Bit width of per-object fingerprints (default 64).

    Example::

        from sqlalchemy.orm import Session
        from atomik_core.integrations.sqlalchemy_listener import AtomikSessionTracker

        session = Session(engine)
        tracker = AtomikSessionTracker(session)

        user = session.get(User, 1)
        user.name = user.name          # no-op write
        session.flush()
        print(tracker.stats())          # false_dirty: 1
    """

    __slots__ = (
        "_session",
        "_fingerprints",
        "_fp_width",
        "_flushes",
        "_objects_checked",
        "_truly_changed",
        "_false_dirty",
    )

    def __init__(self, session, fingerprint_width: int = 64):
        sa = _require_sqlalchemy()

        self._session = session
        self._fingerprints: dict[int, Fingerprint] = {}
        self._fp_width = fingerprint_width
        self._flushes = 0
        self._objects_checked = 0
        self._truly_changed = 0
        self._false_dirty = 0

        sa.event.listen(session, "before_flush", self._before_flush)

    # ------------------------------------------------------------------
    # Event handler
    # ------------------------------------------------------------------

    def _before_flush(self, session, flush_context, instances):
        """``before_flush`` callback — fingerprint every dirty object."""
        self._flushes += 1

        for obj in list(session.dirty):
            self._objects_checked += 1
            obj_id = id(obj)
            snapshot = _column_snapshot(obj)

            fp = self._fingerprints.get(obj_id)
            if fp is not None:
                fp.update(snapshot)
                if fp.changed:
                    self._truly_changed += 1
                    fp.load(snapshot)  # re-baseline
                else:
                    self._false_dirty += 1
            else:
                # First time seeing this object -- assume it is truly changed
                fp = Fingerprint(width=self._fp_width)
                fp.load(snapshot)
                self._fingerprints[obj_id] = fp
                self._truly_changed += 1

        # Also baseline any newly-added objects so we can track them on
        # subsequent flushes.
        for obj in list(session.new):
            obj_id = id(obj)
            if obj_id not in self._fingerprints:
                snapshot = _column_snapshot(obj)
                fp = Fingerprint(width=self._fp_width)
                fp.load(snapshot)
                self._fingerprints[obj_id] = fp

    # ------------------------------------------------------------------
    # Stats
    # ------------------------------------------------------------------

    def stats(self) -> dict:
        """Return cumulative tracking statistics.

        Returns:
            A dict with keys ``flushes``, ``objects_checked``,
            ``truly_changed``, ``false_dirty``, and ``skip_rate``
            (percentage of dirty objects that were actually unchanged).
        """
        total = self._objects_checked
        rate = (self._false_dirty / total * 100) if total > 0 else 0.0
        return {
            "flushes": self._flushes,
            "objects_checked": total,
            "truly_changed": self._truly_changed,
            "false_dirty": self._false_dirty,
            "skip_rate": f"{rate:.1f}%",
        }

    def detach(self) -> None:
        """Remove the event listener from the session."""
        sa = _require_sqlalchemy()
        sa.event.remove(self._session, "before_flush", self._before_flush)

    def __repr__(self) -> str:
        s = self.stats()
        return (
            f"AtomikSessionTracker(flushes={s['flushes']}, "
            f"checked={s['objects_checked']}, "
            f"false_dirty={s['false_dirty']}, skip_rate={s['skip_rate']})"
        )
