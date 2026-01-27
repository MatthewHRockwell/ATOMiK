"""
Pipeline Stage Protocol

Defines the base interface for all pipeline stages. Each stage receives
a manifest from the previous stage, performs its work, and produces
a manifest for the next stage.
"""

from __future__ import annotations

import time
from dataclasses import dataclass, field
from enum import Enum
from typing import Any, Protocol


class StageStatus(Enum):
    """Status of a pipeline stage execution."""
    PENDING = "pending"
    RUNNING = "running"
    SUCCESS = "success"
    SKIPPED = "skipped"
    FAILED = "failed"


@dataclass
class StageManifest:
    """Artifact manifest produced by each pipeline stage."""
    stage: str
    status: StageStatus = StageStatus.PENDING
    timestamp: str = ""
    duration_ms: float = 0.0
    tokens_consumed: int = 0
    artifacts: list[dict[str, Any]] = field(default_factory=list)
    metrics: dict[str, Any] = field(default_factory=dict)
    errors: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)
    next_stage: str | None = None
    validation_level: str = "none"

    @property
    def success(self) -> bool:
        return self.status in (StageStatus.SUCCESS, StageStatus.SKIPPED)

    def to_dict(self) -> dict[str, Any]:
        return {
            "stage": self.stage,
            "status": self.status.value,
            "timestamp": self.timestamp,
            "duration_ms": self.duration_ms,
            "tokens_consumed": self.tokens_consumed,
            "artifacts": self.artifacts,
            "metrics": self.metrics,
            "errors": self.errors,
            "warnings": self.warnings,
            "next_stage": self.next_stage,
            "validation_level": self.validation_level,
        }


class PipelineStage(Protocol):
    """Protocol for pipeline stages."""

    name: str

    def execute(
        self,
        schema: dict[str, Any],
        schema_path: str,
        previous_manifest: StageManifest | None,
        config: Any,
    ) -> StageManifest:
        """Execute this pipeline stage."""
        ...


class BaseStage:
    """Base implementation for pipeline stages with timing and error handling."""

    name: str = "base"

    def execute(
        self,
        schema: dict[str, Any],
        schema_path: str,
        previous_manifest: StageManifest | None,
        config: Any,
    ) -> StageManifest:
        manifest = StageManifest(stage=self.name)
        manifest.status = StageStatus.RUNNING
        manifest.timestamp = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
        start = time.perf_counter()

        try:
            self.run(schema, schema_path, previous_manifest, manifest, config)
            if manifest.status == StageStatus.RUNNING:
                manifest.status = StageStatus.SUCCESS
        except Exception as e:
            manifest.status = StageStatus.FAILED
            manifest.errors.append(f"{self.name}: {e}")

        manifest.duration_ms = (time.perf_counter() - start) * 1000
        return manifest

    def run(
        self,
        schema: dict[str, Any],
        schema_path: str,
        previous_manifest: StageManifest | None,
        manifest: StageManifest,
        config: Any,
    ) -> None:
        raise NotImplementedError
