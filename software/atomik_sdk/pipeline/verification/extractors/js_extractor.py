"""JavaScript interface extractor using regex-based parsing."""

from __future__ import annotations

import re
from typing import Any

from ..interfaces import InterfaceField, InterfaceOperation, LanguageInterface


class JavaScriptExtractor:
    """Extract interface definitions from JavaScript source files."""

    CLASS_RE = re.compile(
        r"class\s+(\w+)\s*(?:extends\s+\w+\s*)?\{",
    )
    CONSTRUCTOR_FIELD_RE = re.compile(
        r"this\.(\w+)\s*=",
    )
    METHOD_RE = re.compile(
        r"(?:^|\n)\s+(\w+)\s*\(([^)]*)\)\s*\{",
    )
    JSDOC_TYPE_RE = re.compile(
        r"@(?:type|param)\s*\{(\w+)\}\s*(\w+)?",
    )
    CONST_RE = re.compile(
        r"(?:const|var|let)\s+([A-Z][A-Z_0-9]+)\s*=\s*([^;\n]+)",
    )

    def extract(self, filepath: str) -> LanguageInterface:
        """Extract class properties and methods from a JavaScript file."""
        with open(filepath, encoding="utf-8") as f:
            source = f.read()

        iface = LanguageInterface(language="javascript", file_path=filepath)

        # Extract class name
        class_match = self.CLASS_RE.search(source)
        if class_match:
            iface.struct_name = class_match.group(1)

        # Extract constructor fields
        iface.fields = self._extract_fields(source)

        # Extract methods
        iface.operations = self._extract_methods(source)

        # Extract constants
        iface.constants = self._extract_constants(source)

        return iface

    def _extract_fields(self, source: str) -> list[InterfaceField]:
        """Extract this.field assignments from constructor."""
        fields = []
        seen: set[str] = set()

        for match in self.CONSTRUCTOR_FIELD_RE.finditer(source):
            name = match.group(1)
            if name not in seen:
                seen.add(name)
                fields.append(InterfaceField(name=name))

        return fields

    def _extract_methods(self, source: str) -> list[InterfaceOperation]:
        """Extract class methods."""
        ops = []
        for match in self.METHOD_RE.finditer(source):
            name = match.group(1)
            if name in ("constructor", "if", "for", "while", "switch"):
                continue
            params_str = match.group(2).strip()
            params = [
                p.strip() for p in params_str.split(",")
                if p.strip()
            ]
            ops.append(InterfaceOperation(
                name=name,
                parameters=params,
            ))
        return ops

    def _extract_constants(self, source: str) -> dict[str, Any]:
        """Extract module-level constants."""
        constants: dict[str, Any] = {}
        for match in self.CONST_RE.finditer(source):
            name = match.group(1)
            value = match.group(2).strip().rstrip(";")
            try:
                constants[name] = int(value)
            except ValueError:
                try:
                    constants[name] = float(value)
                except ValueError:
                    constants[name] = value.strip("'\"")
        return constants
