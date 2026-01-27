"""
ATOMiK Rust SDK Generator

Generates Rust modules from ATOMiK schemas with proper Rust idioms.
"""

from __future__ import annotations

from typing import Any

from .code_emitter import CodeEmitter, GeneratedFile, GenerationResult
from .namespace_mapper import NamespaceMapping


class RustGenerator(CodeEmitter):
    """
    Generates Rust SDK code from ATOMiK schemas.

    Generates:
    - src/{vertical}/{field}/mod.rs - Module definition
    - src/{vertical}/{field}/{object}.rs - Main implementation
    - Cargo.toml metadata
    - tests/{object}_test.rs - Integration tests
    """

    def __init__(self):
        """Initialize Rust code generator."""
        super().__init__('rust')

    def generate(self, schema: dict[str, Any], namespace: NamespaceMapping) -> GenerationResult:
        """Generate Rust SDK code from schema."""
        try:
            files = []
            errors = []
            warnings = []

            # Extract schema components
            catalogue = schema.get('catalogue', {})
            schema_def = schema.get('schema', {})
            delta_fields = schema_def.get('delta_fields', {})
            operations = schema_def.get('operations', {})
            hardware = schema.get('hardware', {})

            # Generate main module file
            module_file = self._generate_module(
                namespace, delta_fields, operations, hardware
            )
            files.append(module_file)

            # Generate mod.rs for the field module
            mod_file = self._generate_mod_file(namespace)
            files.append(mod_file)

            # Generate lib.rs entry point
            lib_file = self._generate_lib_file(namespace)
            files.append(lib_file)

            # Generate Cargo.toml
            cargo_file = self._generate_cargo_toml(catalogue, namespace)
            files.append(cargo_file)

            # Generate integration tests
            test_file = self._generate_tests(namespace, delta_fields, operations)
            files.append(test_file)

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
                errors=[f"Rust generation failed: {str(e)}"],
                warnings=[]
            )

    def _generate_module(
        self,
        namespace: NamespaceMapping,
        delta_fields: dict[str, Any],
        operations: dict[str, Any],
        hardware: dict[str, Any]
    ) -> GeneratedFile:
        """Generate the main Rust module implementation."""

        obj_snake = self._to_snake_case(namespace.object)
        vertical_lower = namespace.vertical.lower()
        field_lower = namespace.field.lower()

        # Determine delta type based on width
        delta_type = self._get_rust_type(delta_fields)

        lines = []

        # Module header
        lines.append("//! ATOMiK Delta-State Module")
        lines.append(f"//! Generated from schema: {namespace.vertical}/{namespace.field}/{namespace.object}")
        lines.append("//!")
        lines.append("//! This module provides delta-state operations based on XOR algebra.")
        lines.append("")
        lines.append("use std::collections::VecDeque;")
        lines.append("")

        # Generate struct
        lines.append(f"/// {namespace.object} delta-state manager")
        lines.append("#[derive(Debug, Clone)]")
        lines.append(f"pub struct {namespace.object} {{")
        lines.append("    /// Initial state")
        lines.append(f"    initial_state: {delta_type},")
        lines.append("    /// Delta accumulator (XOR of all deltas)")
        lines.append(f"    accumulator: {delta_type},")

        # Add history for rollback if enabled
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("    /// Delta history for rollback")
            lines.append(f"    history: VecDeque<{delta_type}>,")
            history_depth = operations['rollback'].get('history_depth', 10)
            lines.append("    /// Maximum history depth")
            lines.append("    max_history: usize,")

        lines.append("}")
        lines.append("")

        # Implement methods
        lines.append(f"impl {namespace.object} {{")

        # Constructor
        lines.append("    /// Create a new delta-state manager")
        lines.append("    pub fn new() -> Self {")
        lines.append("        Self {")
        lines.append("            initial_state: 0,")
        lines.append("            accumulator: 0,")
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("            history: VecDeque::new(),")
            lines.append(f"            max_history: {history_depth},")
        lines.append("        }")
        lines.append("    }")
        lines.append("")

        # Load operation
        lines.append("    /// Load initial state (LOAD operation)")
        lines.append(f"    pub fn load(&mut self, initial_state: {delta_type}) {{")
        lines.append("        self.initial_state = initial_state;")
        lines.append("        self.accumulator = 0;")
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("        self.history.clear();")
        lines.append("    }")
        lines.append("")

        # Accumulate operation
        lines.append("    /// Accumulate delta (ACCUMULATE operation)")
        lines.append("    ///")
        lines.append("    /// XORs the delta into the accumulator.")
        lines.append(f"    pub fn accumulate(&mut self, delta: {delta_type}) {{")
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("        // Save to history")
            lines.append("        self.history.push_back(delta);")
            lines.append("        if self.history.len() > self.max_history {")
            lines.append("            self.history.pop_front();")
            lines.append("        }")
        lines.append("        // XOR delta into accumulator")
        lines.append("        self.accumulator ^= delta;")
        lines.append("    }")
        lines.append("")

        # Reconstruct operation
        lines.append("    /// Reconstruct current state (READ operation)")
        lines.append("    ///")
        lines.append("    /// Returns current_state = initial_state XOR accumulator")
        lines.append(f"    pub fn reconstruct(&self) -> {delta_type} {{")
        lines.append("        self.initial_state ^ self.accumulator")
        lines.append("    }")
        lines.append("")

        # Status operation
        lines.append("    /// Check if accumulator is zero (STATUS operation)")
        lines.append("    pub fn is_accumulator_zero(&self) -> bool {")
        lines.append("        self.accumulator == 0")
        lines.append("    }")
        lines.append("")

        # Rollback operation if enabled
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("    /// Rollback the last N delta operations")
            lines.append("    ///")
            lines.append("    /// Returns the number of deltas actually rolled back.")
            lines.append("    pub fn rollback(&mut self, count: usize) -> usize {")
            lines.append("        let actual_count = count.min(self.history.len());")
            lines.append("        for _ in 0..actual_count {")
            lines.append("            if let Some(delta) = self.history.pop_back() {")
            lines.append("                // XOR removes the delta (self-inverse property)")
            lines.append("                self.accumulator ^= delta;")
            lines.append("            }")
            lines.append("        }")
            lines.append("        actual_count")
            lines.append("    }")
            lines.append("")

        # Getter methods
        lines.append("    /// Get the current accumulator value")
        lines.append(f"    pub fn get_accumulator(&self) -> {delta_type} {{")
        lines.append("        self.accumulator")
        lines.append("    }")
        lines.append("")

        lines.append("    /// Get the initial state")
        lines.append(f"    pub fn get_initial_state(&self) -> {delta_type} {{")
        lines.append("        self.initial_state")
        lines.append("    }")
        lines.append("")

        if operations.get('rollback', {}).get('enabled', False):
            lines.append("    /// Get the number of deltas in history")
            lines.append("    pub fn history_size(&self) -> usize {")
            lines.append("        self.history.len()")
            lines.append("    }")
            lines.append("")

        lines.append("}")
        lines.append("")

        # Default trait
        lines.append(f"impl Default for {namespace.object} {{")
        lines.append("    fn default() -> Self {")
        lines.append("        Self::new()")
        lines.append("    }")
        lines.append("}")
        lines.append("")

        content = "\n".join(lines)

        return GeneratedFile(
            relative_path=f"src/{vertical_lower}/{field_lower}/{obj_snake}.rs",
            content=content,
            language='rust',
            description=f"Rust module for {namespace.object}"
        )

    def _generate_mod_file(self, namespace: NamespaceMapping) -> GeneratedFile:
        """Generate mod.rs for the field module."""

        obj_snake = self._to_snake_case(namespace.object)
        vertical_lower = namespace.vertical.lower()
        field_lower = namespace.field.lower()

        lines = []
        lines.append(f"//! {namespace.field} module")
        lines.append("")
        lines.append(f"pub mod {obj_snake};")
        lines.append("")
        lines.append(f"pub use {obj_snake}::{namespace.object};")
        lines.append("")

        content = "\n".join(lines)

        return GeneratedFile(
            relative_path=f"src/{vertical_lower}/{field_lower}/mod.rs",
            content=content,
            language='rust',
            description=f"Module definition for {namespace.field}"
        )

    def _generate_lib_file(self, namespace: NamespaceMapping) -> GeneratedFile:
        """Generate lib.rs entry point."""

        vertical_lower = namespace.vertical.lower()
        field_lower = namespace.field.lower()

        lines = []
        lines.append("//! ATOMiK Rust SDK")
        lines.append("//!")
        lines.append("//! Delta-state computing primitives based on XOR algebra.")
        lines.append("")
        lines.append(f"pub mod {vertical_lower} {{")
        lines.append(f"    pub mod {field_lower};")
        lines.append("}")
        lines.append("")
        lines.append(f"pub use {vertical_lower}::{field_lower}::{namespace.object};")
        lines.append("")

        content = "\n".join(lines)

        return GeneratedFile(
            relative_path="src/lib.rs",
            content=content,
            language='rust',
            description="Rust library entry point"
        )

    def _generate_cargo_toml(
        self,
        catalogue: dict[str, Any],
        namespace: NamespaceMapping
    ) -> GeneratedFile:
        """Generate Cargo.toml manifest."""

        vertical_lower = namespace.vertical.lower()
        field_lower = namespace.field.lower()
        self._to_snake_case(namespace.object)

        version = catalogue.get('version', '0.1.0')
        description = catalogue.get('description', f'{namespace.object} delta-state module')

        lines = []
        lines.append("[package]")
        lines.append(f'name = "atomik-{vertical_lower}-{field_lower}"')
        lines.append(f'version = "{version}"')
        lines.append('edition = "2021"')
        lines.append(f'description = "{description}"')
        lines.append('license = "MIT"')
        lines.append("")
        lines.append("[dependencies]")
        lines.append("")
        lines.append("[dev-dependencies]")
        lines.append("")

        content = "\n".join(lines)

        return GeneratedFile(
            relative_path="Cargo.toml",
            content=content,
            language='rust',
            description="Cargo package manifest"
        )

    def _generate_tests(
        self,
        namespace: NamespaceMapping,
        delta_fields: dict[str, Any],
        operations: dict[str, Any]
    ) -> GeneratedFile:
        """Generate integration tests."""

        obj_snake = self._to_snake_case(namespace.object)
        vertical_lower = namespace.vertical.lower()
        field_lower = namespace.field.lower()

        lines = []
        lines.append(f"//! Integration tests for {namespace.object}")
        lines.append("")
        lines.append(f"use atomik_{vertical_lower}_{field_lower}::{namespace.object};")
        lines.append("")

        # Test load operation
        lines.append("#[test]")
        lines.append("fn test_load() {")
        lines.append(f"    let mut manager = {namespace.object}::new();")
        lines.append("    manager.load(0x1234567890ABCDEF);")
        lines.append("    assert_eq!(manager.get_initial_state(), 0x1234567890ABCDEF);")
        lines.append("    assert_eq!(manager.get_accumulator(), 0);")
        lines.append("}")
        lines.append("")

        # Test accumulate operation
        lines.append("#[test]")
        lines.append("fn test_accumulate() {")
        lines.append(f"    let mut manager = {namespace.object}::new();")
        lines.append("    manager.load(0);")
        lines.append("    manager.accumulate(0x1111111111111111);")
        lines.append("    assert_eq!(manager.get_accumulator(), 0x1111111111111111);")
        lines.append("    manager.accumulate(0x2222222222222222);")
        lines.append("    assert_eq!(manager.get_accumulator(), 0x3333333333333333);")
        lines.append("}")
        lines.append("")

        # Test reconstruct operation
        lines.append("#[test]")
        lines.append("fn test_reconstruct() {")
        lines.append(f"    let mut manager = {namespace.object}::new();")
        lines.append("    manager.load(0xAAAAAAAAAAAAAAAA);")
        lines.append("    manager.accumulate(0x5555555555555555);")
        lines.append("    // 0xAAAA XOR 0x5555 = 0xFFFF")
        lines.append("    assert_eq!(manager.reconstruct(), 0xFFFFFFFFFFFFFFFF);")
        lines.append("}")
        lines.append("")

        # Test self-inverse property
        lines.append("#[test]")
        lines.append("fn test_self_inverse() {")
        lines.append(f"    let mut manager = {namespace.object}::new();")
        lines.append("    manager.load(0xAAAAAAAAAAAAAAAA);")
        lines.append("    let delta = 0x1234567890ABCDEF;")
        lines.append("    manager.accumulate(delta);")
        lines.append("    manager.accumulate(delta);  // Apply same delta twice")
        lines.append("    // Self-inverse: delta XOR delta = 0")
        lines.append("    assert!(manager.is_accumulator_zero());")
        lines.append("    assert_eq!(manager.reconstruct(), 0xAAAAAAAAAAAAAAAA);")
        lines.append("}")
        lines.append("")

        # Test rollback if enabled
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("#[test]")
            lines.append("fn test_rollback() {")
            lines.append(f"    let mut manager = {namespace.object}::new();")
            lines.append("    manager.load(0);")
            lines.append("    manager.accumulate(0x1111111111111111);")
            lines.append("    manager.accumulate(0x2222222222222222);")
            lines.append("    manager.accumulate(0x4444444444444444);")
            lines.append("    assert_eq!(manager.get_accumulator(), 0x7777777777777777);")
            lines.append("    ")
            lines.append("    // Rollback last 2 operations")
            lines.append("    let count = manager.rollback(2);")
            lines.append("    assert_eq!(count, 2);")
            lines.append("    assert_eq!(manager.get_accumulator(), 0x1111111111111111);")
            lines.append("}")
            lines.append("")

        content = "\n".join(lines)

        return GeneratedFile(
            relative_path=f"tests/{obj_snake}_test.rs",
            content=content,
            language='rust',
            description=f"Integration tests for {namespace.object}"
        )

    @staticmethod
    def _get_rust_type(delta_fields: dict[str, Any]) -> str:
        """Determine Rust type based on delta field widths."""
        max_width = 64
        for field_name, field_spec in delta_fields.items():
            width = field_spec.get('width', 64)
            max_width = max(max_width, width)

        if max_width <= 64:
            return "u64"
        elif max_width <= 128:
            return "u128"
        else:
            # For >128 bits, would need external crate like num-bigint
            return "u128"  # Simplified for now

    @staticmethod
    def _to_snake_case(name: str) -> str:
        """Convert PascalCase to snake_case."""
        result = []
        for i, char in enumerate(name):
            if char.isupper() and i > 0:
                prev_is_lower = name[i-1].islower()
                next_is_lower = i + 1 < len(name) and name[i+1].islower()
                if prev_is_lower or next_is_lower:
                    if result and result[-1] != '_':
                        result.append('_')
            result.append(char.lower())
        return ''.join(result)
