"""Python interface extractor using AST parsing."""

from __future__ import annotations

import ast
import re
from typing import Any

from ..interfaces import LanguageInterface, InterfaceField, InterfaceOperation


# Python type -> approximate bit width
PYTHON_TYPE_WIDTHS = {
    "int": 64,
    "float": 64,
    "bool": 1,
    "str": 0,
    "bytes": 0,
}


class PythonExtractor:
    """Extract interface definitions from Python source files."""

    def extract(self, filepath: str) -> LanguageInterface:
        """
        Extract fields, methods, and constants from a Python file.

        Uses AST parsing to find class definitions, their attributes
        (from __init__ or dataclass fields), and methods.
        """
        with open(filepath, encoding="utf-8") as f:
            source = f.read()

        iface = LanguageInterface(language="python", file_path=filepath)

        try:
            tree = ast.parse(source)
        except SyntaxError:
            return iface

        for node in ast.walk(tree):
            if isinstance(node, ast.ClassDef):
                iface.struct_name = node.name
                iface.fields = self._extract_fields(node)
                iface.operations = self._extract_methods(node)
                iface.constants = self._extract_constants(tree)
                break

        return iface

    def _extract_fields(self, cls_node: ast.ClassDef) -> list[InterfaceField]:
        """Extract fields from class (dataclass or __init__)."""
        fields = []

        # Check for dataclass-style annotations
        for node in cls_node.body:
            if isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name):
                field = InterfaceField(name=node.target.id)
                if isinstance(node.annotation, ast.Name):
                    field.type_name = node.annotation.id
                    field.bit_width = PYTHON_TYPE_WIDTHS.get(field.type_name, 0)
                elif isinstance(node.annotation, ast.Constant):
                    field.type_name = str(node.annotation.value)
                fields.append(field)

        # Check __init__ if no annotations found
        if not fields:
            for node in cls_node.body:
                if (isinstance(node, ast.FunctionDef) and node.name == "__init__"):
                    for stmt in ast.walk(node):
                        if (isinstance(stmt, ast.Assign) and
                                len(stmt.targets) == 1 and
                                isinstance(stmt.targets[0], ast.Attribute) and
                                isinstance(stmt.targets[0].value, ast.Name) and
                                stmt.targets[0].value.id == "self"):
                            field = InterfaceField(
                                name=stmt.targets[0].attr,
                            )
                            fields.append(field)

        return fields

    def _extract_methods(self, cls_node: ast.ClassDef) -> list[InterfaceOperation]:
        """Extract public methods from class."""
        ops = []
        for node in cls_node.body:
            if isinstance(node, ast.FunctionDef) and not node.name.startswith("_"):
                params = [
                    arg.arg for arg in node.args.args
                    if arg.arg != "self"
                ]
                ret_type = ""
                if node.returns and isinstance(node.returns, ast.Name):
                    ret_type = node.returns.id
                ops.append(InterfaceOperation(
                    name=node.name,
                    return_type=ret_type,
                    parameters=params,
                ))
        return ops

    def _extract_constants(self, tree: ast.Module) -> dict[str, Any]:
        """Extract module-level constants (UPPER_CASE assignments)."""
        constants: dict[str, Any] = {}
        for node in tree.body:
            if isinstance(node, ast.Assign):
                for target in node.targets:
                    if isinstance(target, ast.Name) and target.id.isupper():
                        if isinstance(node.value, ast.Constant):
                            constants[target.id] = node.value.value
        return constants
