"""
Shared interface dataclasses for cross-language consistency checking.

Separated from consistency.py to avoid circular imports with extractors.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any


@dataclass
class InterfaceField:
    """A single field/member extracted from generated code."""
    name: str
    type_name: str = ""
    bit_width: int = 0
    is_array: bool = False

    def to_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "type_name": self.type_name,
            "bit_width": self.bit_width,
            "is_array": self.is_array,
        }


@dataclass
class InterfaceOperation:
    """An operation/method extracted from generated code."""
    name: str
    return_type: str = ""
    parameters: list[str] = field(default_factory=list)

    def to_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "return_type": self.return_type,
            "parameters": self.parameters,
        }


@dataclass
class LanguageInterface:
    """Extracted interface for a single language."""
    language: str
    file_path: str = ""
    struct_name: str = ""
    fields: list[InterfaceField] = field(default_factory=list)
    operations: list[InterfaceOperation] = field(default_factory=list)
    constants: dict[str, Any] = field(default_factory=dict)

    @property
    def field_names(self) -> set[str]:
        return {f.name for f in self.fields}

    @property
    def operation_names(self) -> set[str]:
        return {op.name for op in self.operations}

    def to_dict(self) -> dict[str, Any]:
        return {
            "language": self.language,
            "file_path": self.file_path,
            "struct_name": self.struct_name,
            "fields": [f.to_dict() for f in self.fields],
            "operations": [op.to_dict() for op in self.operations],
            "constants": self.constants,
        }
