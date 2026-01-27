"""Rust interface extractor using regex-based parsing."""

from __future__ import annotations

import re
from typing import Any

from ..interfaces import LanguageInterface, InterfaceField, InterfaceOperation


# Rust type -> approximate bit width
RUST_TYPE_WIDTHS = {
    "u8": 8, "i8": 8,
    "u16": 16, "i16": 16,
    "u32": 32, "i32": 32, "f32": 32,
    "u64": 64, "i64": 64, "f64": 64,
    "u128": 128, "i128": 128,
    "bool": 1,
    "usize": 64, "isize": 64,
}


class RustExtractor:
    """Extract interface definitions from Rust source files."""

    # Regex patterns for Rust constructs
    STRUCT_RE = re.compile(
        r"pub\s+struct\s+(\w+)\s*\{([^}]*)\}",
        re.DOTALL,
    )
    FIELD_RE = re.compile(
        r"pub\s+(\w+)\s*:\s*([^,\n]+)",
    )
    IMPL_FN_RE = re.compile(
        r"pub\s+fn\s+(\w+)\s*\(([^)]*)\)\s*(?:->\s*(\S+))?",
    )
    CONST_RE = re.compile(
        r"pub\s+const\s+(\w+)\s*:\s*\w+\s*=\s*([^;]+);",
    )

    def extract(self, filepath: str) -> LanguageInterface:
        """Extract struct fields and impl methods from a Rust file."""
        with open(filepath, encoding="utf-8") as f:
            source = f.read()

        iface = LanguageInterface(language="rust", file_path=filepath)

        # Extract struct
        struct_match = self.STRUCT_RE.search(source)
        if struct_match:
            iface.struct_name = struct_match.group(1)
            body = struct_match.group(2)
            iface.fields = self._extract_fields(body)

        # Extract impl methods
        iface.operations = self._extract_methods(source)

        # Extract constants
        iface.constants = self._extract_constants(source)

        return iface

    def _extract_fields(self, struct_body: str) -> list[InterfaceField]:
        """Extract fields from a Rust struct body."""
        fields = []
        for match in self.FIELD_RE.finditer(struct_body):
            name = match.group(1)
            type_name = match.group(2).strip().rstrip(",")
            bit_width = RUST_TYPE_WIDTHS.get(type_name, 0)
            is_array = type_name.startswith("Vec<") or type_name.startswith("[")
            fields.append(InterfaceField(
                name=name,
                type_name=type_name,
                bit_width=bit_width,
                is_array=is_array,
            ))
        return fields

    def _extract_methods(self, source: str) -> list[InterfaceOperation]:
        """Extract public impl methods."""
        ops = []
        for match in self.IMPL_FN_RE.finditer(source):
            name = match.group(1)
            if name in ("new", "default"):
                continue
            params_str = match.group(2)
            params = [
                p.strip().split(":")[0].strip()
                for p in params_str.split(",")
                if p.strip() and "&self" not in p and "self" not in p.split(":")[0]
            ]
            ret_type = match.group(3) or ""
            ops.append(InterfaceOperation(
                name=name,
                return_type=ret_type.strip(),
                parameters=params,
            ))
        return ops

    def _extract_constants(self, source: str) -> dict[str, Any]:
        """Extract public constants."""
        constants: dict[str, Any] = {}
        for match in self.CONST_RE.finditer(source):
            name = match.group(1)
            value = match.group(2).strip()
            try:
                constants[name] = int(value)
            except ValueError:
                try:
                    constants[name] = float(value)
                except ValueError:
                    constants[name] = value.strip('"')
        return constants
