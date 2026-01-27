"""
ATOMiK SDK Generator Core

Main generator engine that orchestrates schema loading, validation,
and multi-language code generation.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from .code_emitter import CodeEmitter, GenerationResult, MultiLanguageEmitter
from .namespace_mapper import NamespaceMapper, NamespaceMapping
from .schema_validator import SchemaValidator, ValidationResult


class GeneratorConfig:
    """Configuration for the generator."""

    def __init__(
        self,
        output_dir: str | Path = "generated",
        validate_schemas: bool = True,
        verbose: bool = False
    ):
        """
        Initialize generator configuration.

        Args:
            output_dir: Directory for generated code
            validate_schemas: Whether to validate schemas before generation
            verbose: Enable verbose logging
        """
        self.output_dir = Path(output_dir)
        self.validate_schemas = validate_schemas
        self.verbose = verbose


class GeneratorEngine:
    """
    Main SDK generator engine.

    Coordinates the entire code generation pipeline:
    1. Load schema
    2. Validate schema
    3. Extract metadata
    4. Generate code for target languages
    5. Write output files

    Example:
        >>> engine = GeneratorEngine()
        >>> engine.load_schema("terminal-io.json")
        >>> result = engine.generate(target_languages=['python', 'rust'])
        >>> if result:
        >>>     print(f"Generated {len(result.files)} files")
    """

    def __init__(self, config: GeneratorConfig | None = None):
        """
        Initialize generator engine.

        Args:
            config: Generator configuration (uses defaults if None)
        """
        self.config = config or GeneratorConfig()
        self.validator = SchemaValidator()
        self.mapper = NamespaceMapper()
        self.emitter = MultiLanguageEmitter()

        self.schema: dict[str, Any] | None = None
        self.schema_path: Path | None = None
        self.namespace: NamespaceMapping | None = None

    def load_schema(self, schema_path: str | Path) -> ValidationResult:
        """
        Load and optionally validate a schema file.

        Args:
            schema_path: Path to the schema JSON file

        Returns:
            ValidationResult with validity status

        Raises:
            FileNotFoundError: If schema file doesn't exist
            ValueError: If schema JSON is invalid
        """
        self.schema_path = Path(schema_path)

        # Load JSON
        try:
            with open(self.schema_path, encoding='utf-8') as f:
                self.schema = json.load(f)
        except FileNotFoundError:
            raise FileNotFoundError(f"Schema file not found: {schema_path}")
        except json.JSONDecodeError as e:
            raise ValueError(f"Invalid JSON in schema: {e}")

        if self.config.verbose:
            print(f"Loaded schema: {self.schema_path}")

        # Validate if enabled
        if self.config.validate_schemas:
            result = self.validator.validate(self.schema)

            if result.errors and self.config.verbose:
                print("Validation errors:")
                for error in result.errors:
                    print(f"  - {error}")

            if result.warnings and self.config.verbose:
                print("Validation warnings:")
                for warning in result.warnings:
                    print(f"  - {warning}")

            return result

        return ValidationResult(valid=True)

    def extract_metadata(self) -> NamespaceMapping:
        """
        Extract catalogue metadata and generate namespace mapping.

        Returns:
            NamespaceMapping for the loaded schema

        Raises:
            ValueError: If no schema is loaded
        """
        if self.schema is None:
            raise ValueError("No schema loaded. Call load_schema() first.")

        catalogue = self.schema.get('catalogue', {})
        self.namespace = self.mapper.map_catalogue(catalogue)

        if self.config.verbose:
            print(f"Extracted namespace: {self.namespace.vertical}."
                  f"{self.namespace.field}.{self.namespace.object}")

        return self.namespace

    def register_generator(self, language: str, generator: CodeEmitter) -> None:
        """
        Register a code generator for a target language.

        Args:
            language: Target language name
            generator: CodeEmitter instance for the language
        """
        self.emitter.register(language, generator)

        if self.config.verbose:
            print(f"Registered generator for: {language}")

    def generate(
        self,
        target_languages: list[str] | None = None
    ) -> dict[str, GenerationResult]:
        """
        Generate code for target languages.

        Args:
            target_languages: List of languages to generate (None = all registered)

        Returns:
            Dictionary mapping language to GenerationResult

        Raises:
            ValueError: If no schema is loaded or no generators registered
        """
        if self.schema is None:
            raise ValueError("No schema loaded. Call load_schema() first.")

        if self.namespace is None:
            self.extract_metadata()

        if not self.emitter.generators:
            raise ValueError("No generators registered. Call register_generator() first.")

        # Filter generators if specific languages requested
        if target_languages:
            generators_to_use = {
                lang: gen
                for lang, gen in self.emitter.generators.items()
                if lang in target_languages
            }

            if not generators_to_use:
                raise ValueError(f"None of the requested languages are registered: "
                                 f"{target_languages}")

            # Temporarily replace generators
            original_generators = self.emitter.generators
            self.emitter.generators = generators_to_use

        if self.config.verbose:
            print(f"Generating code for: {', '.join(self.emitter.generators.keys())}")

        # Generate code
        results = self.emitter.generate_all(self.schema, self.namespace)

        # Restore original generators if filtered
        if target_languages:
            self.emitter.generators = original_generators

        # Report results
        if self.config.verbose:
            for language, result in results.items():
                if result.success:
                    print(f"  {language}: Generated {len(result.files)} file(s)")
                else:
                    print(f"  {language}: FAILED with {len(result.errors)} error(s)")

        return results

    def write_output(self, results: dict[str, GenerationResult]) -> list[str]:
        """
        Write generated files to disk.

        Args:
            results: Dictionary of generation results by language

        Returns:
            List of written file paths
        """
        written_files = self.emitter.write_generated_files(
            results,
            self.config.output_dir
        )

        if self.config.verbose:
            print(f"Wrote {len(written_files)} file(s) to {self.config.output_dir}")

        return written_files

    def generate_and_write(
        self,
        schema_path: str | Path,
        target_languages: list[str] | None = None
    ) -> tuple[dict[str, GenerationResult], list[str]]:
        """
        Complete pipeline: load, validate, generate, and write.

        Args:
            schema_path: Path to schema file
            target_languages: List of target languages (None = all registered)

        Returns:
            Tuple of (results dict, written file paths list)
        """
        # Load and validate
        validation = self.load_schema(schema_path)
        if not validation:
            raise ValueError(f"Schema validation failed: {validation.errors}")

        # Generate
        results = self.generate(target_languages)

        # Write
        files = self.write_output(results)

        return results, files

    def get_schema_summary(self) -> dict[str, Any]:
        """
        Get summary information about the loaded schema.

        Returns:
            Dictionary with schema summary

        Raises:
            ValueError: If no schema is loaded
        """
        if self.schema is None:
            raise ValueError("No schema loaded")

        catalogue = self.schema.get('catalogue', {})
        schema_section = self.schema.get('schema', {})
        delta_fields = schema_section.get('delta_fields', {})
        operations = schema_section.get('operations', {})

        return {
            'catalogue': {
                'vertical': catalogue.get('vertical'),
                'field': catalogue.get('field'),
                'object': catalogue.get('object'),
                'version': catalogue.get('version')
            },
            'delta_fields': {
                name: {
                    'type': spec.get('type'),
                    'width': spec.get('width')
                }
                for name, spec in delta_fields.items()
            },
            'operations': list(operations.keys()),
            'has_hardware': 'hardware' in self.schema
        }
