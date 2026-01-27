"""
Artifact Cache

Caches intermediate pipeline artifacts (generated code, checksums,
validation results) with content-based invalidation.
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Any


class ArtifactCache:
    """
    File-based artifact cache for pipeline intermediate results.

    Uses content hashes for cache keys. When a schema's content hash
    changes, all cached artifacts for that schema are invalidated.
    """

    def __init__(self, cache_dir: str | Path = ".atomik/cache"):
        self.cache_dir = Path(cache_dir)
        self.cache_dir.mkdir(parents=True, exist_ok=True)
        self._index: dict[str, dict[str, Any]] = {}
        self._load_index()

    def _index_path(self) -> Path:
        return self.cache_dir / "cache_index.json"

    def _load_index(self) -> None:
        """Load the cache index from disk."""
        idx = self._index_path()
        if idx.exists():
            with open(idx, encoding="utf-8") as f:
                self._index = json.load(f)

    def _save_index(self) -> None:
        """Persist the cache index to disk."""
        with open(self._index_path(), "w", encoding="utf-8") as f:
            json.dump(self._index, f, indent=2)

    @staticmethod
    def content_hash(content: str | bytes) -> str:
        """Compute SHA-256 hash of content."""
        if isinstance(content, str):
            content = content.encode("utf-8")
        return hashlib.sha256(content).hexdigest()

    @staticmethod
    def file_hash(path: str | Path) -> str:
        """Compute SHA-256 hash of a file."""
        h = hashlib.sha256()
        with open(path, "rb") as f:
            for chunk in iter(lambda: f.read(8192), b""):
                h.update(chunk)
        return h.hexdigest()

    def get(self, schema_name: str, key: str) -> dict[str, Any] | None:
        """
        Retrieve a cached artifact.

        Args:
            schema_name: Schema identifier.
            key: Artifact key (e.g., "diff_manifest", "generation_result").

        Returns:
            Cached data dict or None if not found.
        """
        schema_cache = self._index.get(schema_name, {})
        entry = schema_cache.get(key)
        if entry is None:
            return None

        # Load artifact data from file
        artifact_path = self.cache_dir / entry.get("file", "")
        if artifact_path.exists():
            with open(artifact_path, encoding="utf-8") as f:
                return json.load(f)
        return None

    def put(
        self,
        schema_name: str,
        key: str,
        data: dict[str, Any],
        schema_hash: str = "",
    ) -> None:
        """
        Store an artifact in the cache.

        Args:
            schema_name: Schema identifier.
            key: Artifact key.
            data: Data to cache.
            schema_hash: Content hash of the source schema.
        """
        if schema_name not in self._index:
            self._index[schema_name] = {}

        filename = f"{schema_name}_{key}.json"
        artifact_path = self.cache_dir / filename

        with open(artifact_path, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2)

        self._index[schema_name][key] = {
            "file": filename,
            "schema_hash": schema_hash,
        }
        self._save_index()

    def invalidate(self, schema_name: str) -> None:
        """Invalidate all cached artifacts for a schema."""
        if schema_name in self._index:
            for entry in self._index[schema_name].values():
                artifact_path = self.cache_dir / entry.get("file", "")
                if artifact_path.exists():
                    artifact_path.unlink()
            del self._index[schema_name]
            self._save_index()

    def is_valid(self, schema_name: str, current_hash: str) -> bool:
        """Check if cached artifacts are still valid for a schema."""
        schema_cache = self._index.get(schema_name, {})
        if not schema_cache:
            return False
        stored_hashes = {e.get("schema_hash") for e in schema_cache.values()}
        return all(h == current_hash for h in stored_hashes if h)

    def clear(self) -> None:
        """Clear the entire cache."""
        for schema_entries in self._index.values():
            for entry in schema_entries.values():
                artifact_path = self.cache_dir / entry.get("file", "")
                if artifact_path.exists():
                    artifact_path.unlink()
        self._index.clear()
        self._save_index()
