"""Verilog interface extractor using regex-based parsing."""

from __future__ import annotations

import re
from typing import Any

from ..interfaces import InterfaceField, LanguageInterface


class VerilogExtractor:
    """Extract interface definitions from Verilog module files."""

    MODULE_RE = re.compile(
        r"module\s+(\w+)\s*(?:#\s*\([^)]*\))?\s*\(",
    )
    PORT_RE = re.compile(
        r"(input|output|inout)\s+(?:(reg|wire)\s+)?(?:\[(\d+):(\d+)\]\s+)?(\w+)",
    )
    PARAM_RE = re.compile(
        r"parameter\s+(?:\[[\d:]+\]\s+)?(\w+)\s*=\s*([^;,\n]+)",
    )
    LOCALPARAM_RE = re.compile(
        r"localparam\s+(?:\[[\d:]+\]\s+)?(\w+)\s*=\s*([^;,\n]+)",
    )

    def extract(self, filepath: str) -> LanguageInterface:
        """Extract module ports and parameters from a Verilog file."""
        with open(filepath, encoding="utf-8") as f:
            source = f.read()

        iface = LanguageInterface(language="verilog", file_path=filepath)

        # Extract module name
        mod_match = self.MODULE_RE.search(source)
        if mod_match:
            iface.struct_name = mod_match.group(1)

        # Extract ports as fields
        iface.fields = self._extract_ports(source)

        # Extract parameters as constants
        iface.constants = self._extract_parameters(source)

        return iface

    def _extract_ports(self, source: str) -> list[InterfaceField]:
        """Extract module ports."""
        fields = []
        seen: set[str] = set()

        for match in self.PORT_RE.finditer(source):
            direction = match.group(1)
            msb_str = match.group(3)
            lsb_str = match.group(4)
            name = match.group(5)

            if name in seen:
                continue
            seen.add(name)

            if msb_str is not None and lsb_str is not None:
                msb = int(msb_str)
                lsb = int(lsb_str)
                bit_width = abs(msb - lsb) + 1
            else:
                bit_width = 1

            fields.append(InterfaceField(
                name=name,
                type_name=direction,
                bit_width=bit_width,
            ))

        return fields

    def _extract_parameters(self, source: str) -> dict[str, Any]:
        """Extract parameter and localparam declarations."""
        constants: dict[str, Any] = {}

        for pattern in (self.PARAM_RE, self.LOCALPARAM_RE):
            for match in pattern.finditer(source):
                name = match.group(1)
                value = match.group(2).strip().rstrip(",")
                try:
                    if value.startswith("'h") or value.startswith("'H"):
                        constants[name] = int(value[2:], 16)
                    elif "'d" in value:
                        constants[name] = int(value.split("'d")[-1])
                    elif "'b" in value:
                        constants[name] = int(value.split("'b")[-1], 2)
                    else:
                        constants[name] = int(value)
                except ValueError:
                    constants[name] = value

        return constants
