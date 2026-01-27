"""
Differential Change Detection Stage

Stage 2 of the pipeline: detects what changed between a schema revision
and the previously generated output. Produces a diff manifest that tells
the generation stage which languages/files need regeneration.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

from ..context.checkpoint import Checkpoint
from . import BaseStage, StageManifest, StageStatus


class DiffStage(BaseStage):
    """Pipeline stage for differential change detection."""

    name = "diff"

    # Change type -> affected generators
    CHANGE_IMPACT = {
        "delta_fields": {"python", "rust", "c", "javascript", "verilog"},
        "operations": {"python", "rust", "c", "javascript", "verilog"},
        "namespace": {"python", "rust", "c", "javascript", "verilog"},
        "hardware": {"verilog"},
        "constraints": {"verilog"},
        "metadata": {"python", "javascript"},
    }

    def run(
        self,
        schema: dict[str, Any],
        schema_path: str,
        previous_manifest: StageManifest | None,
        manifest: StageManifest,
        config: Any,
    ) -> None:
        content_hash = ""
        if previous_manifest:
            content_hash = previous_manifest.metrics.get("content_hash", "")

        checkpoint_dir = getattr(config, "checkpoint_dir", ".atomik")
        checkpoint = Checkpoint(checkpoint_dir)

        # Check if schema has changed
        if content_hash and checkpoint.is_current(
            Path(schema_path).stem, content_hash
        ):
            manifest.status = StageStatus.SKIPPED
            manifest.metrics["skip_reason"] = "up-to-date"
            manifest.metrics["diff_type"] = "none"
            manifest.metrics["affected_generators"] = []
            manifest.tokens_consumed = 0
            return

        # Schema is new or changed -- compute structural diff
        changes = self._detect_changes(schema, schema_path, checkpoint)
        affected = set()
        for change_type in changes:
            affected |= self.CHANGE_IMPACT.get(change_type, set())

        # Filter by requested languages
        requested = getattr(config, "languages", None)
        if requested:
            affected &= set(requested)

        manifest.metrics["diff_type"] = (
            "full" if not checkpoint.get_schema_hash(Path(schema_path).stem)
            else ",".join(sorted(changes)) if changes
            else "none"
        )
        manifest.metrics["affected_generators"] = sorted(affected)
        manifest.metrics["change_types"] = sorted(changes)
        manifest.metrics["content_hash"] = content_hash
        manifest.metrics["re_synthesize"] = bool(
            changes & {"hardware", "constraints", "delta_fields", "operations", "namespace"}
        )

        manifest.next_stage = "generate"
        manifest.tokens_consumed = 0  # All local

    def _detect_changes(
        self,
        schema: dict[str, Any],
        schema_path: str,
        checkpoint: Checkpoint,
    ) -> set[str]:
        """Detect which sections of the schema changed."""
        schema_name = Path(schema_path).stem
        stored_hash = checkpoint.get_schema_hash(schema_name)

        if stored_hash is None:
            # No previous version -- everything is new
            return {"delta_fields", "operations", "namespace", "hardware", "constraints", "metadata"}

        # We have a previous version but content hash differs.
        # Without storing the full previous schema, we flag a full change.
        # Future optimization: store and diff the previous schema structurally.
        changes = set()

        # For now, compare section presence. The content_hash check already
        # confirmed the schema is different, so we flag all sections present.
        if schema.get("schema", {}).get("delta_fields"):
            changes.add("delta_fields")
        if schema.get("schema", {}).get("operations"):
            changes.add("operations")
        if schema.get("catalogue"):
            changes.add("namespace")
        if schema.get("hardware"):
            changes.add("hardware")
        if schema.get("schema", {}).get("constraints"):
            changes.add("constraints")

        # Always include metadata as it's cheap to regenerate
        changes.add("metadata")

        return changes
