"""
Migration Safety Check Stage

Third stage of the source-mode pipeline: optionally compares the
inferred schema against an existing schema JSON to detect breaking
changes before regeneration.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from ..analysis.diff_impact import DiffImpactMapper
from ..analysis.field_diff import FieldDiff
from . import BaseStage, StageManifest, StageStatus


class MigrateCheckStage(BaseStage):
    """Pipeline stage for migration safety checking."""

    name = "migrate_check"

    def __init__(self) -> None:
        self._differ = FieldDiff()
        self._impact_mapper = DiffImpactMapper()

    def run(
        self,
        schema: dict[str, Any],
        schema_path: str,
        previous_manifest: StageManifest | None,
        manifest: StageManifest,
        config: Any,
    ) -> None:
        existing_path = getattr(config, "existing_schema_path", None)

        if not existing_path:
            manifest.status = StageStatus.SKIPPED
            manifest.metrics["skip_reason"] = "no existing schema"
            manifest.next_stage = "validate"
            return

        path = Path(existing_path)
        if not path.exists():
            manifest.status = StageStatus.FAILED
            manifest.errors.append(f"Existing schema not found: {existing_path}")
            return

        # Load existing schema
        try:
            with open(path, encoding="utf-8") as f:
                existing_schema = json.load(f)
        except (json.JSONDecodeError, OSError) as e:
            manifest.status = StageStatus.FAILED
            manifest.errors.append(f"Failed to load existing schema: {e}")
            return

        # Compare using existing FieldDiff
        diff_result = self._differ.compare(existing_schema, schema)
        impact = self._impact_mapper.analyze(diff_result)

        manifest.metrics["migration_diff"] = diff_result.to_dict()
        manifest.metrics["is_identical"] = diff_result.is_identical
        manifest.metrics["change_count"] = diff_result.change_count
        manifest.metrics["sections_changed"] = sorted(diff_result.sections_changed)
        manifest.metrics["affected_languages"] = sorted(impact.affected_languages)
        manifest.metrics["requires_resynthesis"] = impact.requires_resynthesis

        # Add warnings for breaking changes
        breaking_sections = {"delta_fields", "operations"}
        breaking = diff_result.sections_changed & breaking_sections
        if breaking:
            for section in sorted(breaking):
                changes = diff_result.changes_in_section(section)
                for change in changes:
                    manifest.warnings.append(
                        f"Breaking change: {change.path} ({change.change_type})"
                    )

        # In strict mode, warnings become errors
        strict = getattr(config, "fail_on_regression", False)
        if strict and manifest.warnings:
            manifest.status = StageStatus.FAILED
            manifest.errors.append(
                f"Strict mode: {len(manifest.warnings)} breaking change(s) detected"
            )
            return

        manifest.next_stage = "validate"
        manifest.tokens_consumed = 0
