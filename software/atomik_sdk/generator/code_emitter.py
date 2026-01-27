"""
ATOMiK Code Emitter

Base class and utilities for language-specific code generation.
"""

from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .namespace_mapper import NamespaceMapping


@dataclass
class GeneratedFile:
    """Represents a generated source code file."""

    relative_path: str  # Relative path from output root
    content: str  # File content
    language: str  # Target language (python, rust, c, javascript, verilog)
    description: str  # Human-readable description


@dataclass
class GenerationResult:
    """Result of code generation."""

    success: bool
    files: list[GeneratedFile]
    errors: list[str]
    warnings: list[str]

    def __bool__(self) -> bool:
        return self.success


class CodeEmitter(ABC):
    """
    Abstract base class for language-specific code generation.

    Subclasses implement generate() method to produce code for their
    target language.

    Example subclass:
        class PythonGenerator(CodeEmitter):
            def generate(self, schema, namespace):
                # Generate Python code
                return GenerationResult(...)
    """

    def __init__(self, language: str):
        """
        Initialize code emitter.

        Args:
            language: Target language name (python, rust, c, javascript, verilog)
        """
        self.language = language

    @abstractmethod
    def generate(
        self,
        schema: dict[str, Any],
        namespace: NamespaceMapping
    ) -> GenerationResult:
        """
        Generate code from schema.

        Args:
            schema: Validated ATOMiK schema dictionary
            namespace: Namespace mapping for the schema

        Returns:
            GenerationResult with generated files and status
        """
        pass

    def _extract_delta_fields(self, schema: dict[str, Any]) -> dict[str, dict[str, Any]]:
        """
        Extract delta fields from schema.

        Args:
            schema: Schema dictionary

        Returns:
            Dictionary of field_name -> field_spec
        """
        return schema.get('schema', {}).get('delta_fields', {})

    def _extract_operations(self, schema: dict[str, Any]) -> dict[str, dict[str, Any]]:
        """
        Extract operations from schema.

        Args:
            schema: Schema dictionary

        Returns:
            Dictionary of operation_name -> operation_spec
        """
        return schema.get('schema', {}).get('operations', {})

    def _extract_constraints(self, schema: dict[str, Any]) -> dict[str, Any]:
        """
        Extract constraints from schema.

        Args:
            schema: Schema dictionary

        Returns:
            Dictionary of constraint specifications
        """
        return schema.get('schema', {}).get('constraints', {})

    def _extract_hardware(self, schema: dict[str, Any]) -> dict[str, Any]:
        """
        Extract hardware mapping from schema.

        Args:
            schema: Schema dictionary

        Returns:
            Dictionary of hardware specifications (empty if not present)
        """
        return schema.get('hardware', {})

    def _indent(self, text: str, spaces: int = 4) -> str:
        """
        Indent text by specified number of spaces.

        Args:
            text: Text to indent
            spaces: Number of spaces per indent level

        Returns:
            Indented text
        """
        indent_str = ' ' * spaces
        lines = text.split('\n')
        return '\n'.join(indent_str + line if line.strip() else line for line in lines)

    def _comment_block(self, text: str, style: str = 'hash') -> str:
        """
        Generate a comment block in the target language style.

        Args:
            text: Comment text
            style: Comment style ('hash' for #, 'slash' for //, 'c' for /* */)

        Returns:
            Formatted comment block
        """
        lines = text.strip().split('\n')

        if style == 'hash':
            # Python, Ruby, shell
            return '\n'.join(f"# {line}" for line in lines)
        elif style == 'slash':
            # C++, Rust, JavaScript
            return '\n'.join(f"// {line}" for line in lines)
        elif style == 'c':
            # C, Verilog multi-line
            result = "/*\n"
            result += '\n'.join(f" * {line}" for line in lines)
            result += "\n */"
            return result
        else:
            raise ValueError(f"Unknown comment style: {style}")

    def _generate_header(
        self,
        title: str,
        description: str,
        style: str = 'hash'
    ) -> str:
        """
        Generate file header with title and description.

        Args:
            title: File title
            description: File description
            style: Comment style

        Returns:
            Formatted header
        """
        header_text = f"{title}\n\n{description}\n\nGenerated by ATOMiK SDK Generator"
        return self._comment_block(header_text, style)


class MultiLanguageEmitter:
    """
    Coordinates code generation across multiple languages.

    Example:
        >>> emitter = MultiLanguageEmitter()
        >>> emitter.register('python', PythonGenerator())
        >>> emitter.register('rust', RustGenerator())
        >>> results = emitter.generate_all(schema, namespace)
    """

    def __init__(self):
        """Initialize multi-language emitter."""
        self.generators: dict[str, CodeEmitter] = {}

    def register(self, language: str, generator: CodeEmitter) -> None:
        """
        Register a code generator for a language.

        Args:
            language: Target language name
            generator: CodeEmitter instance
        """
        self.generators[language] = generator

    def generate_all(
        self,
        schema: dict[str, Any],
        namespace: NamespaceMapping
    ) -> dict[str, GenerationResult]:
        """
        Generate code for all registered languages.

        Args:
            schema: Validated schema dictionary
            namespace: Namespace mapping

        Returns:
            Dictionary mapping language to GenerationResult
        """
        results = {}

        for language, generator in self.generators.items():
            try:
                result = generator.generate(schema, namespace)
                results[language] = result
            except Exception as e:
                results[language] = GenerationResult(
                    success=False,
                    files=[],
                    errors=[f"Exception during generation: {str(e)}"],
                    warnings=[]
                )

        return results

    def write_generated_files(
        self,
        results: dict[str, GenerationResult],
        output_dir: str | Path
    ) -> list[str]:
        """
        Write generated files to disk.

        Args:
            results: Dictionary of GenerationResults by language
            output_dir: Output directory root

        Returns:
            List of written file paths
        """
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)

        written_files = []

        for language, result in results.items():
            if not result.success:
                continue

            for gen_file in result.files:
                file_path = output_path / gen_file.relative_path

                # Create parent directories
                file_path.parent.mkdir(parents=True, exist_ok=True)

                # Write file
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(gen_file.content)

                written_files.append(str(file_path))

        return written_files
