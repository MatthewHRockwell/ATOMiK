"""C interface extractor using regex-based parsing."""

from __future__ import annotations

import re
from typing import Any

from ..interfaces import LanguageInterface, InterfaceField, InterfaceOperation


# C type -> approximate bit width
C_TYPE_WIDTHS = {
    "uint8_t": 8, "int8_t": 8, "char": 8,
    "uint16_t": 16, "int16_t": 16, "short": 16,
    "uint32_t": 32, "int32_t": 32, "int": 32, "float": 32,
    "uint64_t": 64, "int64_t": 64, "long": 64, "double": 64,
    "bool": 1, "_Bool": 1,
    "size_t": 64,
}


class CExtractor:
    """Extract interface definitions from C header/source files."""

    STRUCT_RE = re.compile(
        r"typedef\s+struct\s*\w*\s*\{([^}]*)\}\s*(\w+)\s*;",
        re.DOTALL,
    )
    FIELD_RE = re.compile(
        r"(\w[\w\s\*]*?)\s+(\w+)\s*(?:\[(\d+)\])?\s*;",
    )
    FUNC_RE = re.compile(
        r"(\w+)\s+(\w+)\s*\(([^)]*)\)\s*;",
    )
    DEFINE_RE = re.compile(
        r"#define\s+(\w+)\s+(.+)",
    )

    def extract(self, filepath: str) -> LanguageInterface:
        """Extract struct members and function signatures from a C file."""
        with open(filepath, encoding="utf-8") as f:
            source = f.read()

        iface = LanguageInterface(language="c", file_path=filepath)

        # Extract struct
        struct_match = self.STRUCT_RE.search(source)
        if struct_match:
            body = struct_match.group(1)
            iface.struct_name = struct_match.group(2)
            iface.fields = self._extract_fields(body)

        # Extract function prototypes
        iface.operations = self._extract_functions(source)

        # Extract #define constants
        iface.constants = self._extract_defines(source)

        return iface

    def _extract_fields(self, struct_body: str) -> list[InterfaceField]:
        """Extract fields from a C struct body."""
        fields = []
        for match in self.FIELD_RE.finditer(struct_body):
            type_name = match.group(1).strip()
            name = match.group(2)
            array_size = match.group(3)

            # Clean up type name
            base_type = type_name.replace("*", "").strip()
            bit_width = C_TYPE_WIDTHS.get(base_type, 0)

            fields.append(InterfaceField(
                name=name,
                type_name=type_name,
                bit_width=bit_width,
                is_array=array_size is not None or "*" in type_name,
            ))
        return fields

    def _extract_functions(self, source: str) -> list[InterfaceOperation]:
        """Extract function prototypes."""
        ops = []
        for match in self.FUNC_RE.finditer(source):
            ret_type = match.group(1)
            name = match.group(2)
            params_str = match.group(3).strip()

            if ret_type in ("typedef", "struct", "enum", "union"):
                continue

            params = []
            if params_str and params_str != "void":
                for p in params_str.split(","):
                    p = p.strip()
                    parts = p.rsplit(None, 1)
                    if len(parts) == 2:
                        params.append(parts[1].lstrip("*"))

            ops.append(InterfaceOperation(
                name=name,
                return_type=ret_type,
                parameters=params,
            ))
        return ops

    def _extract_defines(self, source: str) -> dict[str, Any]:
        """Extract #define constants."""
        constants: dict[str, Any] = {}
        for match in self.DEFINE_RE.finditer(source):
            name = match.group(1)
            value = match.group(2).strip()
            if name.startswith("_") and name.endswith("_H"):
                continue
            try:
                if value.startswith("0x"):
                    constants[name] = int(value, 16)
                else:
                    constants[name] = int(value)
            except ValueError:
                try:
                    constants[name] = float(value)
                except ValueError:
                    constants[name] = value.strip('"')
        return constants
