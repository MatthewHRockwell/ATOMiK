"""
Python SDK Generator

Generates Python modules with type hints and docstrings from ATOMiK schemas.
"""

from __future__ import annotations

from typing import Any, Dict, List
from pathlib import Path

from .code_emitter import CodeEmitter, GeneratedFile, GenerationResult
from .namespace_mapper import NamespaceMapping


class PythonGenerator(CodeEmitter):
    """
    Generates Python SDK code from ATOMiK schemas.

    Produces:
    - Module structure following catalogue namespace
    - Delta operation classes with type hints
    - Docstrings for all public APIs
    - Unit test templates
    """

    def __init__(self):
        """Initialize Python generator."""
        super().__init__('python')

    def generate(
        self,
        schema: Dict[str, Any],
        namespace: NamespaceMapping
    ) -> GenerationResult:
        """
        Generate Python code from schema.

        Args:
            schema: Validated ATOMiK schema
            namespace: Namespace mapping

        Returns:
            GenerationResult with generated Python files
        """
        try:
            files = []
            errors = []
            warnings = []

            # Extract schema components
            catalogue = schema.get('catalogue', {})
            delta_fields = self._extract_delta_fields(schema)
            operations = self._extract_operations(schema)
            constraints = self._extract_constraints(schema)

            # Generate main module file
            module_content = self._generate_module(
                namespace, catalogue, delta_fields, operations, constraints
            )

            module_path = self._get_module_path(namespace)
            files.append(GeneratedFile(
                relative_path=module_path,
                content=module_content,
                language='python',
                description=f"Python module for {namespace.object}"
            ))

            # Generate __init__.py for package
            init_content = self._generate_init(namespace)
            init_path = str(Path(module_path).parent / "__init__.py")
            files.append(GeneratedFile(
                relative_path=init_path,
                content=init_content,
                language='python',
                description=f"Package initializer for {namespace.vertical}.{namespace.field}"
            ))

            # Generate test file
            test_content = self._generate_test(namespace, delta_fields, operations)
            test_path = self._get_test_path(namespace)
            files.append(GeneratedFile(
                relative_path=test_path,
                content=test_content,
                language='python',
                description=f"Unit tests for {namespace.object}"
            ))

            return GenerationResult(
                success=True,
                files=files,
                errors=errors,
                warnings=warnings
            )

        except Exception as e:
            return GenerationResult(
                success=False,
                files=[],
                errors=[f"Python generation failed: {str(e)}"],
                warnings=[]
            )

    def _generate_module(
        self,
        namespace: NamespaceMapping,
        catalogue: Dict[str, Any],
        delta_fields: Dict[str, Dict[str, Any]],
        operations: Dict[str, Dict[str, Any]],
        constraints: Dict[str, Any]
    ) -> str:
        """Generate main Python module content."""
        lines = []

        # Header
        header = self._generate_header(
            namespace.object,
            catalogue.get('description', 'ATOMiK delta-state module'),
            'hash'
        )
        lines.append(header)
        lines.append('')

        # Imports
        lines.append('from __future__ import annotations')
        lines.append('')
        lines.append('from typing import Optional')
        lines.append('')

        # Class definition
        lines.append(f'class {namespace.object}:')
        lines.append(self._indent('"""'))
        lines.append(self._indent(catalogue.get('description', f'{namespace.object} implementation.')))
        lines.append(self._indent(''))

        # Add field documentation
        if delta_fields:
            lines.append(self._indent('Delta Fields:'))
            for field_name, field_spec in delta_fields.items():
                field_type = field_spec.get('type', 'unknown')
                field_width = field_spec.get('width', 0)
                lines.append(self._indent(f'    {field_name}: {field_type} ({field_width}-bit)'))
            lines.append(self._indent(''))

        # Add operations documentation
        if operations:
            lines.append(self._indent('Operations:'))
            for op_name in operations.keys():
                lines.append(self._indent(f'    {op_name}: Available'))
            lines.append(self._indent(''))

        lines.append(self._indent('"""'))
        lines.append('')

        # __init__ method
        lines.append(self._indent('def __init__(self):'))
        lines.append(self._indent('"""Initialize delta-state module."""', 8))

        # Initialize fields
        for field_name, field_spec in delta_fields.items():
            width = field_spec.get('width', 64)
            default = field_spec.get('default_value', 0)
            lines.append(self._indent(f'self._{field_name}_initial: int = {default}', 8))
            lines.append(self._indent(f'self._{field_name}_accumulator: int = 0', 8))

        # Add rollback history if enabled
        if operations.get('rollback', {}).get('enabled'):
            history_depth = operations['rollback'].get('history_depth', 100)
            for field_name in delta_fields.keys():
                lines.append(self._indent(f'self._{field_name}_history: list[int] = []', 8))
                lines.append(self._indent(f'self._history_depth = {history_depth}', 8))

        lines.append('')

        # Generate methods for each delta field
        for field_name, field_spec in delta_fields.items():
            width = field_spec.get('width', 64)
            mask = (1 << width) - 1

            # Load initial state
            lines.extend(self._generate_load_method(field_name, width, mask))
            lines.append('')

            # Accumulate delta
            lines.extend(self._generate_accumulate_method(
                field_name, width, mask,
                operations.get('rollback', {}).get('enabled', False)
            ))
            lines.append('')

            # Reconstruct state
            if operations.get('reconstruct', {}).get('enabled', True):
                lines.extend(self._generate_reconstruct_method(field_name, mask))
                lines.append('')

        # Rollback method if enabled
        if operations.get('rollback', {}).get('enabled'):
            lines.extend(self._generate_rollback_method(list(delta_fields.keys())))
            lines.append('')

        # Status methods
        lines.extend(self._generate_status_methods(list(delta_fields.keys())))

        return '\n'.join(lines)

    def _generate_load_method(self, field_name: str, width: int, mask: int) -> List[str]:
        """Generate load_initial_state method."""
        method_name = f'load_{field_name}'
        lines = []
        lines.append(self._indent(f'def {method_name}(self, initial_state: int) -> None:'))
        lines.append(self._indent('"""', 8))
        lines.append(self._indent(f'Load initial state for {field_name}.', 8))
        lines.append(self._indent('', 8))
        lines.append(self._indent('Args:', 8))
        lines.append(self._indent(f'    initial_state: Initial {width}-bit state value', 8))
        lines.append(self._indent('"""', 8))
        lines.append(self._indent(f'self._{field_name}_initial = initial_state & {hex(mask)}', 8))
        return lines

    def _generate_accumulate_method(
        self, field_name: str, width: int, mask: int, has_rollback: bool
    ) -> List[str]:
        """Generate accumulate_delta method."""
        method_name = f'accumulate_{field_name}'
        lines = []
        lines.append(self._indent(f'def {method_name}(self, delta: int) -> int:'))
        lines.append(self._indent('"""', 8))
        lines.append(self._indent(f'Accumulate delta for {field_name} via XOR.', 8))
        lines.append(self._indent('', 8))
        lines.append(self._indent('Args:', 8))
        lines.append(self._indent(f'    delta: {width}-bit delta value', 8))
        lines.append(self._indent('', 8))
        lines.append(self._indent('Returns:', 8))
        lines.append(self._indent('    Updated accumulator value', 8))
        lines.append(self._indent('"""', 8))

        if has_rollback:
            lines.append(self._indent(f'# Save to history', 8))
            lines.append(self._indent(f'self._{field_name}_history.append(delta)', 8))
            lines.append(self._indent(f'if len(self._{field_name}_history) > self._history_depth:', 8))
            lines.append(self._indent(f'    self._{field_name}_history.pop(0)', 12))
            lines.append(self._indent('', 8))

        lines.append(self._indent(f'self._{field_name}_accumulator ^= (delta & {hex(mask)})', 8))
        lines.append(self._indent(f'return self._{field_name}_accumulator', 8))
        return lines

    def _generate_reconstruct_method(self, field_name: str, mask: int) -> List[str]:
        """Generate reconstruct_state method."""
        method_name = f'reconstruct_{field_name}'
        lines = []
        lines.append(self._indent(f'def {method_name}(self) -> int:'))
        lines.append(self._indent('"""', 8))
        lines.append(self._indent(f'Reconstruct current state for {field_name}.', 8))
        lines.append(self._indent('', 8))
        lines.append(self._indent('Returns:', 8))
        lines.append(self._indent('    Current state value', 8))
        lines.append(self._indent('"""', 8))
        lines.append(self._indent(
            f'return (self._{field_name}_initial ^ self._{field_name}_accumulator) & {hex(mask)}',
            8
        ))
        return lines

    def _generate_rollback_method(self, field_names: List[str]) -> List[str]:
        """Generate rollback method."""
        lines = []
        lines.append(self._indent('def rollback(self, steps: int = 1) -> bool:'))
        lines.append(self._indent('"""', 8))
        lines.append(self._indent('Rollback state by reversing recent deltas.', 8))
        lines.append(self._indent('', 8))
        lines.append(self._indent('Args:', 8))
        lines.append(self._indent('    steps: Number of deltas to rollback', 8))
        lines.append(self._indent('', 8))
        lines.append(self._indent('Returns:', 8))
        lines.append(self._indent('    True if rollback successful', 8))
        lines.append(self._indent('"""', 8))

        # Use first field for history check (all fields should have same history)
        first_field = field_names[0]
        lines.append(self._indent(f'if steps > len(self._{first_field}_history):', 8))
        lines.append(self._indent('return False', 12))
        lines.append(self._indent('', 8))

        lines.append(self._indent('for _ in range(steps):', 8))
        for field_name in field_names:
            lines.append(self._indent(f'delta = self._{field_name}_history.pop()', 12))
            lines.append(self._indent(f'self._{field_name}_accumulator ^= delta', 12))
        lines.append(self._indent('', 8))
        lines.append(self._indent('return True', 8))
        return lines

    def _generate_status_methods(self, field_names: List[str]) -> List[str]:
        """Generate status check methods."""
        lines = []

        # is_accumulator_zero method
        lines.append(self._indent('def is_accumulator_zero(self, field_name: Optional[str] = None) -> bool:'))
        lines.append(self._indent('"""', 8))
        lines.append(self._indent('Check if accumulator is zero (no pending deltas).', 8))
        lines.append(self._indent('', 8))
        lines.append(self._indent('Args:', 8))
        lines.append(self._indent('    field_name: Optional field name to check (None = check all)', 8))
        lines.append(self._indent('', 8))
        lines.append(self._indent('Returns:', 8))
        lines.append(self._indent('    True if accumulator(s) are zero', 8))
        lines.append(self._indent('"""', 8))
        lines.append(self._indent('if field_name:', 8))
        lines.append(self._indent(f'    return getattr(self, f"_{{field_name}}_accumulator") == 0', 8))
        lines.append(self._indent('', 8))

        # Check all fields
        checks = ' and '.join([f'self._{fn}_accumulator == 0' for fn in field_names])
        lines.append(self._indent(f'return {checks}', 8))

        return lines

    def _generate_init(self, namespace: NamespaceMapping) -> str:
        """Generate __init__.py for package."""
        lines = []
        lines.append(f'"""')
        lines.append(f'{namespace.vertical}.{namespace.field} package.')
        lines.append(f'"""')
        lines.append('')
        lines.append(f'from .{namespace.object.lower()} import {namespace.object}')
        lines.append('')
        lines.append(f'__all__ = ["{namespace.object}"]')
        lines.append('')
        return '\n'.join(lines)

    def _generate_test(
        self,
        namespace: NamespaceMapping,
        delta_fields: Dict[str, Dict[str, Any]],
        operations: Dict[str, Dict[str, Any]]
    ) -> str:
        """Generate unit test file."""
        lines = []

        # Header
        lines.append(f'"""')
        lines.append(f'Tests for {namespace.object}')
        lines.append(f'"""')
        lines.append('')
        lines.append('import unittest')
        lines.append('')
        lines.append(f'from atomik.{namespace.vertical}.{namespace.field} import {namespace.object}')
        lines.append('')
        lines.append('')

        # Test class
        lines.append(f'class Test{namespace.object}(unittest.TestCase):')
        lines.append(self._indent(f'"""Test cases for {namespace.object}."""'))
        lines.append('')

        # Test initialization
        lines.append(self._indent('def test_initialization(self):'))
        lines.append(self._indent('"""Test module initialization."""', 8))
        lines.append(self._indent(f'module = {namespace.object}()', 8))
        lines.append(self._indent('self.assertIsNotNone(module)', 8))
        lines.append('')

        # Test each field
        for field_name, field_spec in delta_fields.items():
            width = field_spec.get('width', 64)

            # Test load and reconstruct
            lines.append(self._indent(f'def test_{field_name}_load_reconstruct(self):'))
            lines.append(self._indent(f'"""Test load and reconstruct for {field_name}."""', 8))
            lines.append(self._indent(f'module = {namespace.object}()', 8))
            lines.append(self._indent(f'initial = 0x{"1" * (width // 4)}', 8))
            lines.append(self._indent(f'module.load_{field_name}(initial)', 8))
            lines.append(self._indent(f'state = module.reconstruct_{field_name}()', 8))
            lines.append(self._indent('self.assertEqual(state, initial)', 8))
            lines.append('')

            # Test accumulate
            lines.append(self._indent(f'def test_{field_name}_accumulate(self):'))
            lines.append(self._indent(f'"""Test delta accumulation for {field_name}."""', 8))
            lines.append(self._indent(f'module = {namespace.object}()', 8))
            lines.append(self._indent(f'module.load_{field_name}(0)', 8))
            lines.append(self._indent('delta1 = 0xFF', 8))
            lines.append(self._indent('delta2 = 0xAA', 8))
            lines.append(self._indent(f'module.accumulate_{field_name}(delta1)', 8))
            lines.append(self._indent(f'module.accumulate_{field_name}(delta2)', 8))
            lines.append(self._indent(f'state = module.reconstruct_{field_name}()', 8))
            lines.append(self._indent('self.assertEqual(state, delta1 ^ delta2)', 8))
            lines.append('')

            # Test self-inverse property
            lines.append(self._indent(f'def test_{field_name}_self_inverse(self):'))
            lines.append(self._indent(f'"""Test delta self-inverse property for {field_name}."""', 8))
            lines.append(self._indent(f'module = {namespace.object}()', 8))
            lines.append(self._indent('initial = 0x123456', 8))
            lines.append(self._indent(f'module.load_{field_name}(initial)', 8))
            lines.append(self._indent('delta = 0xABCDEF', 8))
            lines.append(self._indent(f'module.accumulate_{field_name}(delta)', 8))
            lines.append(self._indent(f'module.accumulate_{field_name}(delta)  # Apply same delta twice', 8))
            lines.append(self._indent(f'state = module.reconstruct_{field_name}()', 8))
            lines.append(self._indent('self.assertEqual(state, initial)  # Should cancel out', 8))
            lines.append('')

        # Test accumulator zero
        lines.append(self._indent('def test_accumulator_zero(self):'))
        lines.append(self._indent('"""Test accumulator zero detection."""', 8))
        lines.append(self._indent(f'module = {namespace.object}()', 8))
        lines.append(self._indent('self.assertTrue(module.is_accumulator_zero())', 8))
        if delta_fields:
            first_field = list(delta_fields.keys())[0]
            lines.append(self._indent(f'module.accumulate_{first_field}(0x100)', 8))
            lines.append(self._indent('self.assertFalse(module.is_accumulator_zero())', 8))
        lines.append('')

        # Test rollback if enabled
        if operations.get('rollback', {}).get('enabled'):
            lines.append(self._indent('def test_rollback(self):'))
            lines.append(self._indent('"""Test rollback functionality."""', 8))
            lines.append(self._indent(f'module = {namespace.object}()', 8))
            if delta_fields:
                first_field = list(delta_fields.keys())[0]
                lines.append(self._indent(f'module.load_{first_field}(0)', 8))
                lines.append(self._indent(f'module.accumulate_{first_field}(0x10)', 8))
                lines.append(self._indent(f'module.accumulate_{first_field}(0x20)', 8))
                lines.append(self._indent(f'state_before = module.reconstruct_{first_field}()', 8))
                lines.append(self._indent('success = module.rollback(1)', 8))
                lines.append(self._indent('self.assertTrue(success)', 8))
                lines.append(self._indent(f'state_after = module.reconstruct_{first_field}()', 8))
                lines.append(self._indent('self.assertNotEqual(state_before, state_after)', 8))
            lines.append('')

        # Main
        lines.append('')
        lines.append('if __name__ == "__main__":')
        lines.append(self._indent('unittest.main()'))
        lines.append('')

        return '\n'.join(lines)

    def _get_module_path(self, namespace: NamespaceMapping) -> str:
        """Get relative path for module file."""
        return f"atomik/{namespace.vertical}/{namespace.field}/{namespace.object.lower()}.py"

    def _get_test_path(self, namespace: NamespaceMapping) -> str:
        """Get relative path for test file."""
        return f"tests/test_{namespace.object.lower()}.py"
