"""
ATOMiK Schema Validator

Validates ATOMiK JSON schemas against the specification and enforces
cross-field dependencies and semantic rules.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

try:
    import jsonschema  # noqa: F401
    HAS_JSONSCHEMA = True
except ImportError:
    HAS_JSONSCHEMA = False


class ValidationResult:
    """Result of schema validation."""

    def __init__(self, valid: bool, errors: list[str] = None, warnings: list[str] = None):
        self.valid = valid
        self.errors = errors or []
        self.warnings = warnings or []

    def __bool__(self) -> bool:
        return self.valid

    def __str__(self) -> str:
        if self.valid:
            status = "VALID"
            if self.warnings:
                status += f" (with {len(self.warnings)} warning(s))"
            return status
        else:
            return f"INVALID: {len(self.errors)} error(s)"


class SchemaValidator:
    """
    Validates ATOMiK schemas against specification.

    Performs JSON Schema Draft 7 validation and enforces additional
    semantic rules specific to ATOMiK schemas.

    Example:
        >>> validator = SchemaValidator()
        >>> result = validator.validate_file("terminal-io.json")
        >>> if result:
        >>>     print("Schema is valid!")
        >>> else:
        >>>     for error in result.errors:
        >>>         print(f"Error: {error}")
    """

    def __init__(self, schema_spec_path: str | Path = None):
        """
        Initialize validator with schema specification.

        Args:
            schema_spec_path: Path to atomik_schema_v1.json.
                            If None, uses default location.
        """
        if schema_spec_path is None:
            # Default to specs/atomik_schema_v1.json relative to project root
            project_root = Path(__file__).parent.parent.parent.parent
            schema_spec_path = project_root / "specs" / "atomik_schema_v1.json"

        self.schema_spec_path = Path(schema_spec_path)
        self.schema_spec = self._load_schema_spec()

        if HAS_JSONSCHEMA:
            self.validator = jsonschema.Draft7Validator(self.schema_spec)
        else:
            self.validator = None

    def _load_schema_spec(self) -> dict[str, Any]:
        """Load the ATOMiK schema specification."""
        try:
            with open(self.schema_spec_path, encoding='utf-8') as f:
                return json.load(f)
        except FileNotFoundError:
            raise FileNotFoundError(
                f"Schema specification not found: {self.schema_spec_path}"
            )
        except json.JSONDecodeError as e:
            raise ValueError(
                f"Invalid JSON in schema specification: {e}"
            )

    def validate_file(self, schema_path: str | Path) -> ValidationResult:
        """
        Validate a schema file.

        Args:
            schema_path: Path to the schema JSON file to validate.

        Returns:
            ValidationResult with validity status and any errors/warnings.
        """
        try:
            with open(schema_path, encoding='utf-8') as f:
                schema = json.load(f)
        except FileNotFoundError:
            return ValidationResult(False, [f"File not found: {schema_path}"])
        except json.JSONDecodeError as e:
            return ValidationResult(False, [f"Invalid JSON: {e}"])

        return self.validate(schema)

    def validate(self, schema: dict[str, Any]) -> ValidationResult:
        """
        Validate a schema dictionary.

        Args:
            schema: The schema dictionary to validate.

        Returns:
            ValidationResult with validity status and any errors/warnings.
        """
        errors = []
        warnings = []

        # JSON Schema validation (if library available)
        if self.validator:
            for error in self.validator.iter_errors(schema):
                path = ".".join(str(p) for p in error.path) if error.path else "root"
                errors.append(f"{path}: {error.message}")
        else:
            # Basic structural validation without jsonschema
            basic_errors = self._basic_validation(schema)
            errors.extend(basic_errors)

        # Cross-field dependency validation
        cross_field_errors = self._validate_cross_field_dependencies(schema)
        errors.extend(cross_field_errors)

        # Semantic validation
        semantic_warnings = self._validate_semantics(schema)
        warnings.extend(semantic_warnings)

        return ValidationResult(len(errors) == 0, errors, warnings)

    def _basic_validation(self, schema: dict[str, Any]) -> list[str]:
        """Basic validation without jsonschema library."""
        errors = []

        # Required top-level keys
        if 'catalogue' not in schema:
            errors.append("Missing required 'catalogue' section")
        if 'schema' not in schema:
            errors.append("Missing required 'schema' section")

        # Catalogue validation
        if 'catalogue' in schema:
            catalogue = schema['catalogue']
            required_catalogue = ['vertical', 'field', 'object', 'version']
            for field in required_catalogue:
                if field not in catalogue:
                    errors.append(f"Missing catalogue.{field}")

        # Schema validation
        if 'schema' in schema:
            schema_section = schema['schema']

            if 'delta_fields' not in schema_section:
                errors.append("Missing schema.delta_fields")
            elif not schema_section['delta_fields']:
                errors.append("schema.delta_fields must have at least one field")

            if 'operations' not in schema_section:
                errors.append("Missing schema.operations")
            elif 'accumulate' not in schema_section['operations']:
                errors.append("Missing schema.operations.accumulate (required)")
            else:
                accum = schema_section['operations']['accumulate']
                if not accum.get('enabled', False):
                    errors.append("schema.operations.accumulate.enabled must be true")

        return errors

    def _validate_cross_field_dependencies(self, schema: dict[str, Any]) -> list[str]:
        """
        Validate cross-field dependencies.

        Enforces rules like:
        - hardware.rtl_params.DATA_WIDTH must match delta field widths
        - rollback.enabled requires history_depth
        """
        errors = []

        schema_section = schema.get('schema', {})
        delta_fields = schema_section.get('delta_fields', {})
        operations = schema_section.get('operations', {})
        hardware = schema.get('hardware', {})

        # Rule 1: Hardware DATA_WIDTH must match all delta field widths
        if 'rtl_params' in hardware and 'DATA_WIDTH' in hardware['rtl_params']:
            data_width = hardware['rtl_params']['DATA_WIDTH']

            for field_name, field_spec in delta_fields.items():
                field_width = field_spec.get('width')
                if field_width != data_width:
                    errors.append(
                        f"hardware.rtl_params.DATA_WIDTH ({data_width}) "
                        f"does not match delta_fields.{field_name}.width ({field_width})"
                    )

        # Rule 2: Multiple delta fields with different widths require comment
        widths = [field.get('width') for field in delta_fields.values()]
        if len(set(widths)) > 1 and hardware:
            errors.append(
                "Multiple delta fields with different widths cannot have "
                "hardware mapping (RTL expects uniform width)"
            )

        # Rule 3: Rollback enabled requires history_depth
        if 'rollback' in operations:
            rollback = operations['rollback']
            if rollback.get('enabled') and 'history_depth' not in rollback:
                errors.append(
                    "operations.rollback.enabled=true requires history_depth"
                )

        return errors

    def _validate_semantics(self, schema: dict[str, Any]) -> list[str]:
        """
        Validate semantic rules and generate warnings.

        Checks for potential issues like:
        - Very large history depths
        - Unusual delta widths
        - Missing optional but recommended fields
        """
        warnings = []

        catalogue = schema.get('catalogue', {})
        schema_section = schema.get('schema', {})
        operations = schema_section.get('operations', {})
        constraints = schema_section.get('constraints', {})

        # Warn about missing optional metadata
        if 'author' not in catalogue:
            warnings.append("Recommended field 'catalogue.author' is missing")
        if 'license' not in catalogue:
            warnings.append("Recommended field 'catalogue.license' is missing")
        if 'description' not in catalogue:
            warnings.append("Recommended field 'catalogue.description' is missing")

        # Warn about very large history depths
        if 'rollback' in operations:
            rollback = operations['rollback']
            if rollback.get('enabled'):
                history = rollback.get('history_depth', 0)
                if history > 10000:
                    warnings.append(
                        f"Very large history_depth ({history}) may consume "
                        "significant memory"
                    )

        # Warn about missing constraints
        if not constraints:
            warnings.append(
                "No constraints specified - consider adding resource limits"
            )

        # Warn about reconstruct disabled
        if 'reconstruct' in operations:
            if not operations['reconstruct'].get('enabled', True):
                warnings.append(
                    "reconstruct operation disabled - reading state will not work"
                )

        return warnings

    def validate_namespace_uniqueness(
        self,
        schemas: list[dict[str, Any]]
    ) -> tuple[bool, list[str]]:
        """
        Validate that all schemas have unique namespaces.

        Args:
            schemas: List of schema dictionaries to check.

        Returns:
            Tuple of (all_unique: bool, conflicts: List[str])
        """
        namespaces = {}
        conflicts = []

        for i, schema in enumerate(schemas):
            catalogue = schema.get('catalogue', {})
            namespace = (
                f"{catalogue.get('vertical', 'Unknown')}."
                f"{catalogue.get('field', 'Unknown')}."
                f"{catalogue.get('object', 'Unknown')}"
            )

            if namespace in namespaces:
                conflicts.append(
                    f"Duplicate namespace '{namespace}' at indices "
                    f"{namespaces[namespace]} and {i}"
                )
            else:
                namespaces[namespace] = i

        return len(conflicts) == 0, conflicts
