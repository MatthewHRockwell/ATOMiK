#!/usr/bin/env python3
"""ATOMiK + Redis: Skip redundant writes using delta-state fingerprinting.

Problem: Your app writes to Redis on every request, even when the data
hasn't changed. Each write costs network RTT + Redis CPU + replication.

Solution: Use ATOMiK's Fingerprint to detect changes BEFORE writing.
Only write to Redis when the data actually changed. O(1) check per key.

Requires: pip install redis atomik-core

Usage: python redis_change_detection.py
"""

from atomik_core import Fingerprint

# --- Simulated Redis client (replace with real redis.Redis() in production) ---
class MockRedis:
    def __init__(self):
        self._store: dict[str, bytes] = {}
        self.writes = 0
        self.skipped = 0

    def set(self, key: str, value: bytes) -> None:
        self._store[key] = value
        self.writes += 1

    def get(self, key: str) -> bytes | None:
        return self._store.get(key)


class AtomikRedisCache:
    """Redis cache with ATOMiK change detection.

    Wraps a Redis client and uses per-key fingerprints to skip
    redundant writes. Only writes to Redis when data actually changed.
    """

    def __init__(self, redis_client):
        self.redis = redis_client
        self._fingerprints: dict[str, Fingerprint] = {}
        self.checks = 0
        self.writes = 0
        self.skipped = 0

    def put(self, key: str, data: bytes) -> bool:
        """Write data to Redis, but only if it changed.

        Returns True if the write happened, False if skipped (unchanged).
        """
        self.checks += 1

        # Get or create fingerprint for this key
        if key not in self._fingerprints:
            fp = Fingerprint()
            fp.load(data)
            self._fingerprints[key] = fp
            self.redis.set(key, data)
            self.writes += 1
            return True

        # Check if data changed
        fp = self._fingerprints[key]
        fp.update(data)

        if fp.changed:
            # Data changed — write to Redis and update fingerprint
            self.redis.set(key, data)
            fp.load(data)  # Update reference
            self.writes += 1
            return True
        else:
            # Data unchanged — skip the write entirely
            self.skipped += 1
            return False

    def get(self, key: str) -> bytes | None:
        """Read from Redis (fingerprint check not needed for reads)."""
        return self.redis.get(key)

    def stats(self) -> dict:
        """Return cache statistics."""
        total = self.checks
        hit_rate = (self.skipped / total * 100) if total > 0 else 0
        return {
            "checks": total,
            "writes": self.writes,
            "skipped": self.skipped,
            "skip_rate": f"{hit_rate:.1f}%",
        }


def main():
    """Demo: process 1000 API requests where 80% are unchanged."""
    import random
    random.seed(42)

    redis = MockRedis()
    cache = AtomikRedisCache(redis)

    # Simulate 1000 API requests updating user session data
    session_data = b"user_id=42&cart_items=3&locale=en_US&theme=dark"
    variants = [
        session_data,  # unchanged (80% of requests)
        b"user_id=42&cart_items=4&locale=en_US&theme=dark",  # +1 item
        b"user_id=42&cart_items=3&locale=fr_FR&theme=dark",  # locale change
        b"user_id=42&cart_items=3&locale=en_US&theme=light",  # theme change
    ]

    print("ATOMiK + Redis Change Detection Demo")
    print("=" * 50)
    print(f"Simulating 1000 API requests (80% unchanged)...\n")

    for i in range(1000):
        # 80% of requests have unchanged data
        if random.random() < 0.80:
            data = variants[0]
        else:
            data = random.choice(variants[1:])
        cache.put("session:user42", data)

    stats = cache.stats()
    print(f"  Total checks:    {stats['checks']}")
    print(f"  Redis writes:    {stats['writes']}")
    print(f"  Writes skipped:  {stats['skipped']} ({stats['skip_rate']} saved)")
    print(f"\n  Without ATOMiK:  1000 Redis writes")
    print(f"  With ATOMiK:     {stats['writes']} Redis writes")
    print(f"  Reduction:       {100 - (stats['writes'] / 10):.0f}%")
    print(f"\n  Each skipped write saves:")
    print(f"    - 1 network RTT (~0.5ms)")
    print(f"    - 1 Redis SET command (~0.1ms CPU)")
    print(f"    - 1 replication event to replicas")
    print(f"\n  At 10K req/s: ~{stats['skipped'] * 10} writes/sec eliminated")


if __name__ == "__main__":
    main()
