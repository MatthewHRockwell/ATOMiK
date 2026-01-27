"""
Prompt Cache

Caches prompt content with TTL-based and schema-aware invalidation.
Reduces input tokens for repeated schema processing.
"""

from __future__ import annotations

import hashlib
import time
from dataclasses import dataclass, field
from typing import Any


@dataclass
class CacheEntry:
    """A cached prompt entry."""
    key: str
    content: str
    schema_hash: str
    created_at: float
    ttl_seconds: float
    hit_count: int = 0

    @property
    def expired(self) -> bool:
        return time.time() > self.created_at + self.ttl_seconds

    def to_dict(self) -> dict[str, Any]:
        return {
            "key": self.key,
            "schema_hash": self.schema_hash,
            "created_at": self.created_at,
            "ttl_seconds": self.ttl_seconds,
            "hit_count": self.hit_count,
            "expired": self.expired,
            "content_size": len(self.content),
        }


class PromptCache:
    """
    TTL-based prompt cache with schema-aware invalidation.

    Caches prompt content keyed by (task, schema_hash). Entries
    expire after a configurable TTL or when the schema hash changes.

    Example:
        >>> cache = PromptCache(default_ttl=900)
        >>> cache.put("gen_python", "abc123", "prompt content here")
        >>> hit = cache.get("gen_python", "abc123")
        >>> assert hit == "prompt content here"
    """

    def __init__(self, default_ttl: float = 900.0) -> None:
        self._entries: dict[str, CacheEntry] = {}
        self._default_ttl = default_ttl
        self._stats = {"hits": 0, "misses": 0}

    def get(self, task: str, schema_hash: str) -> str | None:
        """
        Look up cached prompt content.

        Returns None on miss, expiration, or schema hash mismatch.
        """
        key = self._make_key(task, schema_hash)
        entry = self._entries.get(key)

        if entry is None:
            self._stats["misses"] += 1
            return None

        if entry.expired:
            del self._entries[key]
            self._stats["misses"] += 1
            return None

        if entry.schema_hash != schema_hash:
            del self._entries[key]
            self._stats["misses"] += 1
            return None

        entry.hit_count += 1
        self._stats["hits"] += 1
        return entry.content

    def put(
        self,
        task: str,
        schema_hash: str,
        content: str,
        ttl: float | None = None,
    ) -> None:
        """Store prompt content in cache."""
        key = self._make_key(task, schema_hash)
        self._entries[key] = CacheEntry(
            key=key,
            content=content,
            schema_hash=schema_hash,
            created_at=time.time(),
            ttl_seconds=ttl or self._default_ttl,
        )

    def invalidate_schema(self, schema_hash: str) -> int:
        """Invalidate all entries for a schema. Returns count removed."""
        to_remove = [
            k for k, v in self._entries.items()
            if v.schema_hash == schema_hash
        ]
        for k in to_remove:
            del self._entries[k]
        return len(to_remove)

    def clear(self) -> None:
        """Clear all cached entries."""
        self._entries.clear()

    def hit_rate(self) -> float:
        """Compute cache hit rate percentage."""
        total = self._stats["hits"] + self._stats["misses"]
        if total == 0:
            return 0.0
        return 100.0 * self._stats["hits"] / total

    def get_stats(self) -> dict[str, Any]:
        """Get cache statistics."""
        self._evict_expired()
        return {
            "entries": len(self._entries),
            "hits": self._stats["hits"],
            "misses": self._stats["misses"],
            "hit_rate_pct": round(self.hit_rate(), 1),
            "total_content_bytes": sum(
                len(e.content) for e in self._entries.values()
            ),
        }

    def _make_key(self, task: str, schema_hash: str) -> str:
        """Create a cache key from task and schema hash."""
        return f"{task}:{schema_hash[:16]}"

    def _evict_expired(self) -> int:
        """Remove expired entries. Returns count removed."""
        expired = [k for k, v in self._entries.items() if v.expired]
        for k in expired:
            del self._entries[k]
        return len(expired)
