"""
Structured Context Manifests

Provides compact, structured context for pipeline resumption across
sessions. Designed for <2K token cold-start loading.
"""

from __future__ import annotations

import json
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass
class ArtifactEntry:
    """Tracks a generated artifact with its checksum and status."""
    path: str
    sha256: str
    language: str = ""
    status: str = "generated"
    last_generated: str = ""

    def to_dict(self) -> dict[str, Any]:
        return {
            "path": self.path,
            "sha256": self.sha256,
            "language": self.language,
            "status": self.status,
            "last_generated": self.last_generated,
        }


@dataclass
class SchemaEntry:
    """Tracks a schema with its content hash and generation state."""
    name: str
    sha256: str
    path: str
    namespace: str = ""
    last_generated: str = ""
    artifacts: dict[str, ArtifactEntry] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "sha256": self.sha256,
            "path": self.path,
            "namespace": self.namespace,
            "last_generated": self.last_generated,
            "artifacts": {k: v.to_dict() for k, v in self.artifacts.items()},
        }


@dataclass
class SegmentMetadata:
    """Metadata for a context segment to support intelligent loading."""
    segment_type: str = ""        # "schema", "kb_entry", "output", "error"
    task_affinity: list[str] = field(default_factory=list)
    token_estimate: int = 0
    priority: float = 1.0         # Higher = more important

    def to_dict(self) -> dict[str, Any]:
        return {
            "segment_type": self.segment_type,
            "task_affinity": self.task_affinity,
            "token_estimate": self.token_estimate,
            "priority": round(self.priority, 2),
        }


@dataclass
class PipelineManifest:
    """
    Top-level pipeline manifest for cross-session state tracking.

    Designed to be serializable to <2K tokens of JSON for compact
    context loading.
    """
    version: str = "2.1"
    phase: str = "5"
    schemas_registered: int = 0
    languages_supported: list[str] = field(
        default_factory=lambda: ["python", "rust", "c", "javascript", "verilog"]
    )
    last_pipeline_run: str = ""
    schemas: dict[str, SchemaEntry] = field(default_factory=dict)
    hardware_state: dict[str, Any] = field(default_factory=dict)
    metrics_summary: dict[str, Any] = field(default_factory=dict)
    token_ledger: dict[str, Any] = field(default_factory=lambda: {
        "session_total": 0,
        "budget_remaining": 130.0,
    })
    pending_actions: list[str] = field(default_factory=list)
    segment_metadata: dict[str, SegmentMetadata] = field(default_factory=dict)

    def register_schema(self, name: str, sha256: str, path: str, namespace: str = "") -> None:
        """Register or update a schema in the manifest."""
        if name in self.schemas:
            self.schemas[name].sha256 = sha256
            self.schemas[name].path = path
            if namespace:
                self.schemas[name].namespace = namespace
        else:
            self.schemas[name] = SchemaEntry(
                name=name, sha256=sha256, path=path, namespace=namespace
            )
        self.schemas_registered = len(self.schemas)

    def register_segment(
        self,
        segment_id: str,
        segment_type: str,
        task_affinity: list[str] | None = None,
        token_estimate: int = 0,
        priority: float = 1.0,
    ) -> None:
        """Register segment-level metadata for intelligent context loading."""
        self.segment_metadata[segment_id] = SegmentMetadata(
            segment_type=segment_type,
            task_affinity=task_affinity or [],
            token_estimate=token_estimate,
            priority=priority,
        )

    def record_run(self, tokens_consumed: int = 0) -> None:
        """Record a pipeline run in the manifest."""
        self.last_pipeline_run = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
        self.token_ledger["session_total"] += tokens_consumed

    def to_dict(self) -> dict[str, Any]:
        return {
            "version": self.version,
            "project_state": {
                "phase": self.phase,
                "schemas_registered": self.schemas_registered,
                "languages_supported": self.languages_supported,
                "last_pipeline_run": self.last_pipeline_run,
            },
            "artifact_index": {
                "schemas": {k: v.to_dict() for k, v in self.schemas.items()},
            },
            "hardware_state": self.hardware_state,
            "metrics_summary": self.metrics_summary,
            "token_ledger": self.token_ledger,
            "pending_actions": self.pending_actions,
            "segment_metadata": {
                k: v.to_dict() for k, v in self.segment_metadata.items()
            },
        }

    def save(self, path: str | Path) -> None:
        """Persist manifest to disk."""
        path = Path(path)
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            json.dump(self.to_dict(), f, indent=2)

    @classmethod
    def load(cls, path: str | Path) -> PipelineManifest:
        """Load manifest from disk."""
        path = Path(path)
        with open(path, encoding="utf-8") as f:
            data = json.load(f)

        manifest = cls()
        ps = data.get("project_state", {})
        manifest.version = data.get("version", "2.0")
        manifest.phase = ps.get("phase", "4C")
        manifest.last_pipeline_run = ps.get("last_pipeline_run", "")
        manifest.languages_supported = ps.get(
            "languages_supported", manifest.languages_supported
        )

        # Restore schemas
        for name, sdata in data.get("artifact_index", {}).get("schemas", {}).items():
            entry = SchemaEntry(
                name=sdata.get("name", name),
                sha256=sdata.get("sha256", ""),
                path=sdata.get("path", ""),
                namespace=sdata.get("namespace", ""),
                last_generated=sdata.get("last_generated", ""),
            )
            manifest.schemas[name] = entry

        manifest.schemas_registered = len(manifest.schemas)
        manifest.hardware_state = data.get("hardware_state", {})
        manifest.metrics_summary = data.get("metrics_summary", {})
        manifest.token_ledger = data.get("token_ledger", manifest.token_ledger)
        manifest.pending_actions = data.get("pending_actions", [])

        return manifest
