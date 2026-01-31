"""
Schema Inference Stage

Second stage of the source-mode pipeline: takes the extracted
LanguageInterface and produces an ATOMiK schema dict using
deterministic heuristics. Zero LLM tokens consumed.
"""

from __future__ import annotations

from typing import Any

from ..inference.schema_inferrer import InferenceHints, SchemaInferrer
from ..verification.interfaces import (
    InterfaceField,
    InterfaceOperation,
    LanguageInterface,
)
from . import BaseStage, StageManifest, StageStatus


class InferStage(BaseStage):
    """Pipeline stage that infers an ATOMiK schema from extracted interface."""

    name = "infer"

    def __init__(self) -> None:
        self._inferrer = SchemaInferrer()

    def run(
        self,
        schema: dict[str, Any],
        schema_path: str,
        previous_manifest: StageManifest | None,
        manifest: StageManifest,
        config: Any,
    ) -> None:
        # Read extracted interface from previous stage
        if previous_manifest is None:
            manifest.status = StageStatus.FAILED
            manifest.errors.append("infer: no previous manifest (extract stage missing)")
            return

        iface_dict = previous_manifest.metrics.get("extracted_interface")
        if iface_dict is None:
            manifest.status = StageStatus.FAILED
            manifest.errors.append("infer: no extracted_interface in previous manifest")
            return

        # Reconstruct LanguageInterface from dict
        iface = LanguageInterface(
            language=iface_dict.get("language", "unknown"),
            file_path=iface_dict.get("file_path", ""),
            struct_name=iface_dict.get("struct_name", ""),
            fields=[
                InterfaceField(**f) for f in iface_dict.get("fields", [])
            ],
            operations=[
                InterfaceOperation(**op) for op in iface_dict.get("operations", [])
            ],
            constants=iface_dict.get("constants", {}),
        )

        # Build hints from config overrides
        overrides = getattr(config, "inference_overrides", {}) or {}
        hints = InferenceHints(
            vertical=overrides.get("vertical"),
            version=overrides.get("version", "1.0.0"),
        )

        # Run inference
        inferred = self._inferrer.infer(iface, hints)

        # Mutate schema in-place so downstream stages see the inferred schema
        schema.update(inferred)

        # Store inference results in manifest
        manifest.metrics["inferred_schema"] = inferred
        manifest.metrics["vertical"] = inferred["catalogue"]["vertical"]
        manifest.metrics["object"] = inferred["catalogue"]["object"]
        manifest.metrics["delta_field_count"] = len(
            inferred["schema"]["delta_fields"]
        )
        manifest.metrics["operations"] = {
            k: v.get("enabled", False)
            for k, v in inferred["schema"]["operations"].items()
        }

        manifest.next_stage = "migrate_check"
        manifest.tokens_consumed = 0
