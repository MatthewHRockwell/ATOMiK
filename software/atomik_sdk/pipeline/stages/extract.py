"""
Source Code Extraction Stage

First stage of the source-mode pipeline: reads a source file, selects
the appropriate language extractor, and produces a LanguageInterface
for the inference stage.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

from ..verification.extractors import (
    CExtractor,
    JavaScriptExtractor,
    PythonExtractor,
    RustExtractor,
    VerilogExtractor,
)
from . import BaseStage, StageManifest, StageStatus

# File extension → (extractor class, language name)
EXTRACTOR_MAP: dict[str, tuple[type, str]] = {
    ".py": (PythonExtractor, "python"),
    ".rs": (RustExtractor, "rust"),
    ".c": (CExtractor, "c"),
    ".h": (CExtractor, "c"),
    ".js": (JavaScriptExtractor, "javascript"),
    ".v": (VerilogExtractor, "verilog"),
}


class ExtractStage(BaseStage):
    """Pipeline stage that extracts interfaces from source code."""

    name = "extract"

    def run(
        self,
        schema: dict[str, Any],
        schema_path: str,
        previous_manifest: StageManifest | None,
        manifest: StageManifest,
        config: Any,
    ) -> None:
        source_path = getattr(config, "source_path", None) or schema_path
        path = Path(source_path)

        if not path.exists():
            manifest.status = StageStatus.FAILED
            manifest.errors.append(f"Source file not found: {source_path}")
            return

        # Detect language
        source_lang = getattr(config, "source_language", "auto")
        ext = path.suffix.lower()

        if source_lang != "auto":
            # Manual override — find matching extension
            for e, (_, lang) in EXTRACTOR_MAP.items():
                if lang == source_lang:
                    ext = e
                    break

        if ext not in EXTRACTOR_MAP:
            manifest.status = StageStatus.FAILED
            manifest.errors.append(
                f"Unsupported file extension: {ext} "
                f"(supported: {', '.join(sorted(EXTRACTOR_MAP))})"
            )
            return

        extractor_cls, language = EXTRACTOR_MAP[ext]
        extractor = extractor_cls()
        iface = extractor.extract(str(path))

        # Store results for next stage
        manifest.metrics["extracted_interface"] = iface.to_dict()
        manifest.metrics["source_language"] = language
        manifest.metrics["struct_name"] = iface.struct_name
        manifest.metrics["field_count"] = len(iface.fields)
        manifest.metrics["operation_count"] = len(iface.operations)
        manifest.metrics["constant_count"] = len(iface.constants)

        manifest.artifacts.append({
            "path": str(path),
            "type": "source",
            "language": language,
        })

        manifest.next_stage = "infer"
        manifest.tokens_consumed = 0
