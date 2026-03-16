"""ATOMiK + Redis: Skip redundant writes using delta-state fingerprinting.

Usage:
    pip install atomik-core[redis]

    from atomik_core.integrations.redis_adapter import AtomikRedisCache

    cache = AtomikRedisCache(redis.Redis())
    written = cache.put("session:user42", data)  # False if unchanged
"""

from __future__ import annotations

from atomik_core.fingerprint import Fingerprint


class AtomikRedisCache:
    """Redis cache with ATOMiK change detection.

    Wraps any Redis-like client (must have ``get`` and ``set`` methods)
    and uses per-key fingerprints to skip redundant writes.  Only the
    keys that actually changed hit the network.

    Example::

        import redis
        from atomik_core.integrations.redis_adapter import AtomikRedisCache

        r = redis.Redis()
        cache = AtomikRedisCache(r)

        cache.put("session:user42", b'{"cart": 3}')   # True  (first write)
        cache.put("session:user42", b'{"cart": 3}')   # False (skipped)
        cache.put("session:user42", b'{"cart": 4}')   # True  (data changed)
    """

    __slots__ = ("_redis", "_fingerprints", "_checks", "_writes", "_skipped")

    def __init__(self, redis_client):
        """Create an ATOMiK-backed Redis cache wrapper.

        Args:
            redis_client: Any object with ``get(key)`` and
                ``set(key, value)`` methods.  Typically ``redis.Redis()``.

        Raises:
            ImportError: If the ``redis`` package is not installed and
                *redis_client* is ``None``.
        """
        if redis_client is None:
            # Give a helpful message when someone passes None expecting
            # auto-construction.
            try:
                import redis as _redis  # noqa: F811
                raise ValueError(
                    "redis_client must not be None. "
                    "Pass redis.Redis() or redis.StrictRedis()."
                )
            except ImportError:
                raise ImportError(
                    "The 'redis' package is required for AtomikRedisCache.\n"
                    "Install it with:  pip install atomik-core[redis]\n"
                    "  or:             pip install redis"
                ) from None

        # Duck-type check: must have get/set
        for method in ("get", "set"):
            if not callable(getattr(redis_client, method, None)):
                raise TypeError(
                    f"redis_client must have a callable '{method}' method"
                )

        self._redis = redis_client
        self._fingerprints: dict[str, Fingerprint] = {}
        self._checks = 0
        self._writes = 0
        self._skipped = 0

    def put(self, key: str, data: bytes | bytearray) -> bool:
        """Write *data* to Redis under *key*, but only if it changed.

        On the first call for a given key the data is always written.
        On subsequent calls the data is fingerprinted and compared
        against the previous value; the Redis write is skipped when the
        fingerprint matches.

        Args:
            key: Redis key.
            data: Value to store (bytes or bytearray).

        Returns:
            ``True`` if the write happened, ``False`` if skipped
            (data unchanged).
        """
        self._checks += 1

        if key not in self._fingerprints:
            fp = Fingerprint()
            fp.load(data)
            self._fingerprints[key] = fp
            self._redis.set(key, data)
            self._writes += 1
            return True

        fp = self._fingerprints[key]
        fp.update(data)

        if fp.changed:
            self._redis.set(key, data)
            fp.load(data)  # re-baseline the reference
            self._writes += 1
            return True
        else:
            self._skipped += 1
            return False

    def get(self, key: str) -> bytes | None:
        """Read a value from Redis.

        This is a plain pass-through; fingerprinting is only needed for
        writes.

        Args:
            key: Redis key.

        Returns:
            The stored value, or ``None`` if the key does not exist.
        """
        return self._redis.get(key)

    def stats(self) -> dict:
        """Return cumulative cache statistics.

        Returns:
            A dict with keys ``checks``, ``writes``, ``skipped``, and
            ``skip_rate`` (a formatted percentage string).
        """
        total = self._checks
        skip_rate = (self._skipped / total * 100) if total > 0 else 0.0
        return {
            "checks": total,
            "writes": self._writes,
            "skipped": self._skipped,
            "skip_rate": f"{skip_rate:.1f}%",
        }

    def __repr__(self) -> str:
        s = self.stats()
        return (
            f"AtomikRedisCache(checks={s['checks']}, "
            f"writes={s['writes']}, skip_rate={s['skip_rate']})"
        )
