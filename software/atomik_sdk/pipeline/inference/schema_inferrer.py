"""
Core inference engine: LanguageInterface → ATOMiK schema dict.

All inference is deterministic Python — zero LLM tokens consumed.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from ..verification.interfaces import LanguageInterface
from .heuristics import (
    RECONSTRUCT_KEYWORDS,
    ROLLBACK_KEYWORDS,
    classify_delta_type,
    classify_vertical,
    is_delta_candidate,
    snap_width,
)


@dataclass
class InferenceHints:
    """Optional overrides for schema inference."""
    vertical: str | None = None
    version: str = "1.0.0"
    target_frequency_mhz: float = 94.5
    max_memory_mb: int = 64
    extra: dict[str, Any] = field(default_factory=dict)


class SchemaInferrer:
    """
    Convert a LanguageInterface into a valid ATOMiK schema dict.

    The algorithm is entirely deterministic and rule-based:
    1. Catalogue from struct name, module path, and keyword heuristics.
    2. Delta fields from interface fields (type classification, width snapping).
    3. Operations from method name pattern matching.
    4. Constraints from sensible defaults (overridable via hints).
    """

    def infer(
        self,
        iface: LanguageInterface,
        hints: InferenceHints | None = None,
    ) -> dict[str, Any]:
        """
        Produce a valid ATOMiK schema dict from an extracted interface.

        Returns:
            Schema dict with "catalogue" and "schema" sections,
            ready for the downstream validate stage.
        """
        hints = hints or InferenceHints()

        catalogue = self._infer_catalogue(iface, hints)
        delta_fields = self._infer_delta_fields(iface)
        operations = self._infer_operations(iface)
        constraints = self._infer_constraints(hints)

        return {
            "catalogue": catalogue,
            "schema": {
                "delta_fields": delta_fields,
                "operations": operations,
                "constraints": constraints,
            },
        }

    def _infer_catalogue(
        self,
        iface: LanguageInterface,
        hints: InferenceHints,
    ) -> dict[str, Any]:
        """Infer the catalogue section from interface metadata."""
        # Object name from struct/class name (already PascalCase from extractor)
        object_name = iface.struct_name or "UnknownObject"

        # Field from module path or directory
        field_name = self._derive_field(iface.file_path)

        # Vertical from keyword heuristics (overridable)
        if hints.vertical:
            vertical = hints.vertical
        else:
            field_names = [f.name for f in iface.fields]
            vertical = classify_vertical(object_name, field_names)

        return {
            "vertical": vertical,
            "field": field_name,
            "object": object_name,
            "version": hints.version,
        }

    def _derive_field(self, file_path: str) -> str:
        """Derive the 'field' catalogue entry from file path."""
        if not file_path:
            return "General"

        from pathlib import PurePosixPath, PureWindowsPath

        # Handle both path separators
        try:
            path = PureWindowsPath(file_path)
        except Exception:
            path = PurePosixPath(file_path)

        # Use parent directory name, title-cased
        parent = path.parent.name
        if parent and parent not in (".", "src", "lib", "generated"):
            return parent.replace("_", " ").replace("-", " ").title().replace(" ", "")

        # Fall back to stem
        stem = path.stem
        return stem.replace("_", " ").title().replace(" ", "")

    def _infer_delta_fields(
        self, iface: LanguageInterface
    ) -> dict[str, Any]:
        """Infer delta_fields from interface fields."""
        delta_fields: dict[str, Any] = {}

        for f in iface.fields:
            if not is_delta_candidate(f.name):
                continue

            width = snap_width(f.bit_width)
            delta_type = classify_delta_type(f.name, f.type_name)

            # Encoding: spatiotemporal for wide stream fields
            if width >= 128 and delta_type == "delta_stream":
                encoding = "spatiotemporal_4x4x4"
            else:
                encoding = "raw"

            # Compression: xor for streams, none otherwise
            compression = "xor" if delta_type == "delta_stream" else "none"

            delta_fields[f.name] = {
                "type": delta_type,
                "width": width,
                "encoding": encoding,
                "compression": compression,
                "default_value": 0,
            }

        return delta_fields

    def _infer_operations(self, iface: LanguageInterface) -> dict[str, Any]:
        """Infer operations from interface method names."""
        op_names = {op.name.lower() for op in iface.operations}

        # Accumulate is always enabled (core ATOMiK requirement)
        operations: dict[str, Any] = {
            "accumulate": {
                "enabled": True,
                "latency_cycles": 1,
            },
        }

        # Reconstruct if any method matches keywords
        has_reconstruct = any(
            kw in name
            for name in op_names
            for kw in RECONSTRUCT_KEYWORDS
        )
        if has_reconstruct:
            operations["reconstruct"] = {
                "enabled": True,
                "latency_cycles": 1,
            }

        # Rollback if any method matches keywords
        has_rollback = any(
            kw in name
            for name in op_names
            for kw in ROLLBACK_KEYWORDS
        )
        if has_rollback:
            # Try to find history depth from constants
            depth = 256  # default
            for key, val in iface.constants.items():
                key_lower = key.lower()
                if ("history" in key_lower or "depth" in key_lower
                        or "rollback" in key_lower):
                    if isinstance(val, int) and val > 0:
                        depth = val
                        break

            operations["rollback"] = {
                "enabled": True,
                "history_depth": depth,
            }

        return operations

    def _infer_constraints(self, hints: InferenceHints) -> dict[str, Any]:
        """Build constraints section from hints/defaults."""
        return {
            "target_frequency_mhz": hints.target_frequency_mhz,
            "max_memory_mb": hints.max_memory_mb,
        }
