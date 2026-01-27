"""
Pipeline Checkpoints

Provides cross-session state persistence for the pipeline. Checkpoints
capture schema hashes, artifact state, and metrics history to enable
cold-start resumption and differential regeneration.
"""

from __future__ import annotations

import json
import shutil
import time
from pathlib import Path
from typing import Any


class Checkpoint:
    """
    Pipeline checkpoint for cross-session state.

    Stores schema content hashes, generated artifact checksums, and
    metrics history. Enables differential regeneration by tracking
    what was previously generated and whether it is still current.
    """

    def __init__(self, checkpoint_dir: str | Path = ".atomik"):
        self.checkpoint_dir = Path(checkpoint_dir)
        self.checkpoint_dir.mkdir(parents=True, exist_ok=True)
        self._state: dict[str, Any] = self._load()

    def _checkpoint_path(self) -> Path:
        return self.checkpoint_dir / "checkpoint.json"

    def _backup_path(self) -> Path:
        return self.checkpoint_dir / "checkpoint.json.bak"

    def _load(self) -> dict[str, Any]:
        """Load checkpoint from disk."""
        path = self._checkpoint_path()
        if path.exists():
            with open(path, encoding="utf-8") as f:
                return json.load(f)
        return {
            "version": "2.0",
            "created": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "schemas": {},
            "metrics_history": [],
        }

    def save(self) -> None:
        """Persist checkpoint to disk with atomic write and backup."""
        path = self._checkpoint_path()
        backup = self._backup_path()

        # Backup existing checkpoint
        if path.exists():
            shutil.copy2(path, backup)

        self._state["last_updated"] = time.strftime(
            "%Y-%m-%dT%H:%M:%SZ", time.gmtime()
        )

        # Atomic write via temp file
        tmp = path.with_suffix(".tmp")
        with open(tmp, "w", encoding="utf-8") as f:
            json.dump(self._state, f, indent=2)
        tmp.replace(path)

    def get_schema_hash(self, schema_name: str) -> str | None:
        """Get the stored content hash for a schema."""
        entry = self._state.get("schemas", {}).get(schema_name)
        if entry:
            return entry.get("content_hash")
        return None

    def update_schema(
        self,
        schema_name: str,
        content_hash: str,
        artifact_hashes: dict[str, str] | None = None,
        metrics: dict[str, Any] | None = None,
    ) -> None:
        """
        Update checkpoint with schema generation results.

        Args:
            schema_name: Schema identifier.
            content_hash: SHA-256 of the schema file content.
            artifact_hashes: Map of language -> artifact content hash.
            metrics: Pipeline metrics from this run.
        """
        if "schemas" not in self._state:
            self._state["schemas"] = {}

        self._state["schemas"][schema_name] = {
            "content_hash": content_hash,
            "last_generated": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "artifact_hashes": artifact_hashes or {},
        }

        if metrics:
            self.append_metrics(schema_name, metrics)

        self.save()

    def append_metrics(self, schema_name: str, metrics: dict[str, Any]) -> None:
        """Append a metrics entry to the history."""
        if "metrics_history" not in self._state:
            self._state["metrics_history"] = []

        entry = {
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "schema": schema_name,
            **metrics,
        }
        self._state["metrics_history"].append(entry)

    def get_metrics_history(
        self, schema_name: str | None = None
    ) -> list[dict[str, Any]]:
        """Get metrics history, optionally filtered by schema."""
        history = self._state.get("metrics_history", [])
        if schema_name:
            return [e for e in history if e.get("schema") == schema_name]
        return history

    def is_current(self, schema_name: str, content_hash: str) -> bool:
        """Check if the checkpoint for a schema matches the current content hash."""
        stored = self.get_schema_hash(schema_name)
        return stored == content_hash

    def to_dict(self) -> dict[str, Any]:
        return dict(self._state)
