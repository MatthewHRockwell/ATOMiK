"""
Selective Code Generation Stage

Stage 3 of the pipeline: wraps GeneratorEngine with selective execution
based on the diff manifest. Tracks per-file generation status and
produces an artifact manifest with checksums and metrics.
"""

from __future__ import annotations

import hashlib
import time
from pathlib import Path
from typing import Any

from atomik_sdk.generator.c_generator import CGenerator
from atomik_sdk.generator.core import GeneratorConfig, GeneratorEngine
from atomik_sdk.generator.javascript_generator import JavaScriptGenerator
from atomik_sdk.generator.python_generator import PythonGenerator
from atomik_sdk.generator.rust_generator import RustGenerator
from atomik_sdk.generator.verilog_generator import VerilogGenerator

from . import BaseStage, StageManifest, StageStatus

ALL_GENERATORS = {
    "python": PythonGenerator,
    "rust": RustGenerator,
    "c": CGenerator,
    "javascript": JavaScriptGenerator,
    "verilog": VerilogGenerator,
}


class GenerateStage(BaseStage):
    """Pipeline stage for selective code generation."""

    name = "generate"

    def run(
        self,
        schema: dict[str, Any],
        schema_path: str,
        previous_manifest: StageManifest | None,
        manifest: StageManifest,
        config: Any,
    ) -> None:
        output_dir = getattr(config, "output_dir", "generated")
        verbose = getattr(config, "verbose", False)

        # Determine which generators to invoke from diff manifest
        affected = None
        if previous_manifest and previous_manifest.metrics.get("affected_generators"):
            affected = previous_manifest.metrics["affected_generators"]

        # Fall back to config languages or all
        if not affected:
            affected = getattr(config, "languages", None) or list(ALL_GENERATORS.keys())

        # Create engine
        engine = GeneratorEngine(GeneratorConfig(
            output_dir=output_dir,
            validate_schemas=False,  # Already validated in stage 1
            verbose=verbose,
        ))

        # Register only affected generators
        for lang in affected:
            if lang in ALL_GENERATORS:
                engine.register_generator(lang, ALL_GENERATORS[lang]())

        # Load schema into engine
        engine.schema = schema
        engine.schema_path = Path(schema_path)
        engine.extract_metadata()

        # Generate
        gen_start = time.perf_counter()
        results = engine.generate(affected)
        gen_time = (time.perf_counter() - gen_start) * 1000

        # Write output
        written_files = engine.write_output(results)

        # Collect metrics per language
        total_files = 0
        total_lines = 0
        errors = []

        for lang, result in results.items():
            file_count = len(result.files)
            total_files += file_count

            # Count lines in generated files
            lang_lines = 0
            for file_entry in result.files:
                content = file_entry.get("content", "")
                lang_lines += content.count("\n") + (1 if content else 0)

            total_lines += lang_lines

            manifest.metrics[f"{lang}_files"] = file_count
            manifest.metrics[f"{lang}_lines"] = lang_lines

            if not result.success:
                errors.extend(result.errors)

            # Record artifacts with checksums
            for file_entry in result.files:
                content = file_entry.get("content", "")
                sha = hashlib.sha256(content.encode("utf-8")).hexdigest()
                manifest.artifacts.append({
                    "path": file_entry.get("path", ""),
                    "language": lang,
                    "sha256": sha,
                    "action": "generated",
                    "lines": content.count("\n") + (1 if content else 0),
                })

        manifest.metrics["files_generated"] = total_files
        manifest.metrics["lines_generated"] = total_lines
        manifest.metrics["generation_time_ms"] = round(gen_time, 1)
        manifest.metrics["languages_generated"] = sorted(results.keys())
        manifest.metrics["written_files"] = written_files

        if errors:
            manifest.status = StageStatus.FAILED
            manifest.errors.extend(errors)

        manifest.next_stage = "verify"
        manifest.tokens_consumed = 0  # GeneratorEngine is local
