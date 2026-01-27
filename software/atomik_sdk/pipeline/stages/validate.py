"""
Schema Validation Stage

Stage 1 of the pipeline: loads a schema, validates it against the
ATOMiK specification, computes content hash, and extracts complexity
metrics.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

from atomik_sdk.generator.schema_validator import SchemaValidator

from . import BaseStage, StageManifest, StageStatus
from ..context.cache import ArtifactCache


class ValidateStage(BaseStage):
    """Pipeline stage for schema validation."""

    name = "validate"

    def __init__(self) -> None:
        self._validator = SchemaValidator()

    def run(
        self,
        schema: dict[str, Any],
        schema_path: str,
        previous_manifest: StageManifest | None,
        manifest: StageManifest,
        config: Any,
    ) -> None:
        # Validate against spec
        result = self._validator.validate(schema)

        if not result.valid:
            manifest.status = StageStatus.FAILED
            manifest.errors.extend(result.errors)
            return

        manifest.warnings.extend(result.warnings)

        # Compute content hash
        content_hash = ArtifactCache.file_hash(schema_path)
        manifest.metrics["content_hash"] = content_hash

        # Extract schema complexity metrics
        catalogue = schema.get("catalogue", {})
        schema_section = schema.get("schema", {})
        delta_fields = schema_section.get("delta_fields", {})
        operations = schema_section.get("operations", {})
        hardware = schema.get("hardware", {})

        manifest.metrics["namespace"] = (
            f"{catalogue.get('vertical', '')}"
            f".{catalogue.get('field', '')}"
            f".{catalogue.get('object', '')}"
        )
        manifest.metrics["field_count"] = len(delta_fields)
        manifest.metrics["operation_count"] = len(operations)
        manifest.metrics["has_hardware"] = bool(hardware)

        # Extract data width for hardware metrics
        widths = [f.get("width", 0) for f in delta_fields.values()]
        manifest.metrics["data_width"] = max(widths) if widths else 0

        # Track rollback depth
        rollback = operations.get("rollback", {})
        manifest.metrics["rollback_depth"] = rollback.get("history_depth", 0)

        manifest.artifacts.append({
            "path": schema_path,
            "type": "schema",
            "sha256": content_hash,
        })

        manifest.next_stage = "diff"
        manifest.tokens_consumed = 0  # All local
