"""
ATOMiK Namespace Mapper

Maps catalogue metadata to language-specific namespaces and import paths.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any


@dataclass
class NamespaceMapping:
    """Namespace mapping for different target languages."""

    vertical: str
    field: str
    object: str

    # Python: from atomik.Video.Stream import H264Delta
    python_module_path: str
    python_import_statement: str

    # Rust: use atomik::video::stream::H264Delta;
    rust_path: str
    rust_use_statement: str

    # C: #include <atomik/video/stream/h264_delta.h>
    c_include_path: str
    c_include_statement: str

    # JavaScript: const {H264Delta} = require('@atomik/video/stream');
    javascript_package: str
    javascript_require_statement: str

    # Verilog: module atomik_video_stream_h264_delta
    verilog_module_name: str


class NamespaceMapper:
    """
    Maps ATOMiK catalogue metadata to target language namespaces.

    Example:
        >>> mapper = NamespaceMapper()
        >>> catalogue = {
        ...     "vertical": "Video",
        ...     "field": "Stream",
        ...     "object": "H264Delta"
        ... }
        >>> mapping = mapper.map_catalogue(catalogue)
        >>> print(mapping.python_import_statement)
        from atomik.Video.Stream import H264Delta
    """

    @staticmethod
    def map_catalogue(catalogue: dict[str, Any]) -> NamespaceMapping:
        """
        Map catalogue metadata to language-specific namespaces.

        Args:
            catalogue: Dictionary containing vertical, field, object keys.

        Returns:
            NamespaceMapping with paths for all target languages.
        """
        vertical = catalogue.get('vertical', 'Unknown')
        field = catalogue.get('field', 'Unknown')
        obj = catalogue.get('object', 'Unknown')

        # Convert to lowercase for case-sensitive languages
        vertical_lower = vertical.lower()
        field_lower = field.lower()
        obj.lower()

        # Convert PascalCase to snake_case for C
        obj_snake = NamespaceMapper._to_snake_case(obj)

        # Python (preserves PascalCase for classes)
        python_module_path = f"atomik.{vertical}.{field}"
        python_import = f"from atomik.{vertical}.{field} import {obj}"

        # Rust (uses lowercase with :: separators)
        rust_path = f"atomik::{vertical_lower}::{field_lower}::{obj}"
        rust_use = f"use atomik::{vertical_lower}::{field_lower}::{obj};"

        # C (uses lowercase with / separators and snake_case for filenames)
        c_include_path = f"atomik/{vertical_lower}/{field_lower}/{obj_snake}.h"
        c_include = f"#include <atomik/{vertical_lower}/{field_lower}/{obj_snake}.h>"

        # JavaScript (uses lowercase with / separators, scoped package)
        javascript_package = f"@atomik/{vertical_lower}/{field_lower}"
        javascript_require = (
            f"const {{{obj}}} = require('@atomik/{vertical_lower}/{field_lower}');"
        )

        # Verilog (uses snake_case with underscores)
        verilog_module = f"atomik_{vertical_lower}_{field_lower}_{obj_snake}"

        return NamespaceMapping(
            vertical=vertical,
            field=field,
            object=obj,
            python_module_path=python_module_path,
            python_import_statement=python_import,
            rust_path=rust_path,
            rust_use_statement=rust_use,
            c_include_path=c_include_path,
            c_include_statement=c_include,
            javascript_package=javascript_package,
            javascript_require_statement=javascript_require,
            verilog_module_name=verilog_module
        )

    @staticmethod
    def _to_snake_case(name: str) -> str:
        """
        Convert PascalCase or camelCase to snake_case.

        Args:
            name: PascalCase string.

        Returns:
            snake_case string.

        Example:
            >>> NamespaceMapper._to_snake_case("H264Delta")
            'h264_delta'
            >>> NamespaceMapper._to_snake_case("TerminalIO")
            'terminal_io'
        """
        result = []
        for i, char in enumerate(name):
            if char.isupper() and i > 0:
                # Check if previous char is lowercase or current is followed by lowercase
                # This handles cases like "IO" staying together
                prev_is_lower = name[i-1].islower()
                next_is_lower = i + 1 < len(name) and name[i+1].islower()

                # Add underscore only if transitioning from lowercase to uppercase
                # or if this uppercase is the start of a new word (followed by lowercase)
                if prev_is_lower or next_is_lower:
                    if result and result[-1] != '_':
                        result.append('_')

            result.append(char.lower())

        return ''.join(result)

    @staticmethod
    def validate_identifier(name: str) -> tuple[bool, str | None]:
        """
        Validate that an identifier is valid across all target languages.

        Args:
            name: Identifier name to validate.

        Returns:
            Tuple of (valid: bool, error_message: str | None)
        """
        # Reserved keywords across all languages
        reserved_keywords = {
            # Python
            'False', 'None', 'True', 'and', 'as', 'assert', 'async', 'await',
            'break', 'class', 'continue', 'def', 'del', 'elif', 'else', 'except',
            'finally', 'for', 'from', 'global', 'if', 'import', 'in', 'is',
            'lambda', 'nonlocal', 'not', 'or', 'pass', 'raise', 'return', 'try',
            'while', 'with', 'yield',
            # Rust
            'as', 'break', 'const', 'continue', 'crate', 'else', 'enum', 'extern',
            'false', 'fn', 'for', 'if', 'impl', 'in', 'let', 'loop', 'match',
            'mod', 'move', 'mut', 'pub', 'ref', 'return', 'self', 'Self',
            'static', 'struct', 'super', 'trait', 'true', 'type', 'unsafe',
            'use', 'where', 'while',
            # C
            'auto', 'break', 'case', 'char', 'const', 'continue', 'default',
            'do', 'double', 'else', 'enum', 'extern', 'float', 'for', 'goto',
            'if', 'int', 'long', 'register', 'return', 'short', 'signed',
            'sizeof', 'static', 'struct', 'switch', 'typedef', 'union',
            'unsigned', 'void', 'volatile', 'while',
            # JavaScript
            'abstract', 'arguments', 'boolean', 'break', 'byte', 'case', 'catch',
            'char', 'class', 'const', 'continue', 'debugger', 'default', 'delete',
            'do', 'double', 'else', 'enum', 'eval', 'export', 'extends', 'false',
            'final', 'finally', 'float', 'for', 'function', 'goto', 'if',
            'implements', 'import', 'in', 'instanceof', 'int', 'interface', 'let',
            'long', 'native', 'new', 'null', 'package', 'private', 'protected',
            'public', 'return', 'short', 'static', 'super', 'switch',
            'synchronized', 'this', 'throw', 'throws', 'transient', 'true', 'try',
            'typeof', 'var', 'void', 'volatile', 'while', 'with', 'yield',
            # Verilog
            'module', 'endmodule', 'input', 'output', 'inout', 'wire', 'reg',
            'parameter', 'localparam', 'assign', 'always', 'initial', 'begin',
            'end', 'if', 'else', 'case', 'endcase', 'for', 'while', 'repeat',
            'forever', 'function', 'endfunction', 'task', 'endtask'
        }

        # Must start with uppercase letter (PascalCase)
        if not name[0].isupper():
            return False, "Identifier must start with an uppercase letter (PascalCase)"

        # Must be alphanumeric only
        if not name.isalnum():
            return False, "Identifier must contain only alphanumeric characters"

        # Check for reserved keywords
        if name in reserved_keywords or name.lower() in reserved_keywords:
            return False, f"'{name}' is a reserved keyword in one or more target languages"

        # Length check
        if len(name) < 2:
            return False, "Identifier must be at least 2 characters long"
        if len(name) > 64:
            return False, "Identifier must be 64 characters or less"

        return True, None

    @staticmethod
    def generate_directory_structure(mapping: NamespaceMapping) -> dict[str, str]:
        """
        Generate directory structure for each language target.

        Args:
            mapping: NamespaceMapping instance.

        Returns:
            Dictionary mapping language to relative directory path.
        """
        return {
            'python': f"atomik/{mapping.vertical}/{mapping.field}",
            'rust': f"src/{mapping.vertical.lower()}/{mapping.field.lower()}",
            'c': f"atomik/{mapping.vertical.lower()}/{mapping.field.lower()}",
            'javascript': f"packages/{mapping.vertical.lower()}/{mapping.field.lower()}",
            'verilog': f"rtl/{mapping.vertical.lower()}/{mapping.field.lower()}"
        }
