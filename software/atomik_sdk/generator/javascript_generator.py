"""
ATOMiK JavaScript SDK Generator

Generates JavaScript/Node.js modules from ATOMiK schemas.
Supports both CommonJS and ES6 modules.
"""

from __future__ import annotations

from typing import Dict, Any
from pathlib import Path

from .code_emitter import CodeEmitter, GeneratedFile, GenerationResult
from .namespace_mapper import NamespaceMapping


class JavaScriptGenerator(CodeEmitter):
    """
    Generates JavaScript SDK code from ATOMiK schemas.

    Generates:
    - src/{object}.js - Main module (ES6)
    - package.json - NPM package metadata
    - test/{object}.test.js - Test file
    """

    def __init__(self):
        """Initialize JavaScript code generator."""
        super().__init__('javascript')

    def generate(self, schema: Dict[str, Any], namespace: NamespaceMapping) -> GenerationResult:
        """Generate JavaScript SDK code from schema."""
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

            # Generate main module
            module_file = self._generate_module(
                namespace, delta_fields, operations, hardware
            )
            files.append(module_file)

            # Generate package.json
            package_file = self._generate_package_json(catalogue, namespace)
            files.append(package_file)

            # Generate test file
            test_file = self._generate_test(namespace, delta_fields, operations)
            files.append(test_file)

            # Generate index.js (entry point)
            index_file = self._generate_index(namespace)
            files.append(index_file)

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
                errors=[f"JavaScript generation failed: {str(e)}"],
                warnings=[]
            )

    def _generate_module(
        self,
        namespace: NamespaceMapping,
        delta_fields: Dict[str, Any],
        operations: Dict[str, Any],
        hardware: Dict[str, Any]
    ) -> GeneratedFile:
        """Generate JavaScript module file."""

        obj_name = namespace.object
        vertical_lower = namespace.vertical.lower()
        field_lower = namespace.field.lower()

        # Determine if we need BigInt (>53 bits)
        use_bigint = self._should_use_bigint(delta_fields)

        lines = []

        # Module header
        lines.append("/**")
        lines.append(f" * {obj_name} - Delta-state module")
        lines.append(f" * ")
        lines.append(f" * Generated from ATOMiK schema")
        lines.append(f" * Vertical: {namespace.vertical}")
        lines.append(f" * Field: {namespace.field}")
        lines.append(" * ")
        lines.append(" * @module @atomik/%s/%s" % (vertical_lower, field_lower))
        lines.append(" */")
        lines.append("")

        # Class definition
        lines.append("/**")
        lines.append(f" * {obj_name} delta-state manager")
        lines.append(" * ")
        lines.append(" * Manages delta-state operations using XOR algebra.")
        lines.append(" */")
        lines.append(f"export class {obj_name} {{")

        # Constructor
        lines.append("    /**")
        lines.append(f"     * Create a new {obj_name} instance")
        lines.append("     */")
        lines.append("    constructor() {")
        if use_bigint:
            lines.append("        /** @type {BigInt} */")
            lines.append("        this.initialState = 0n;")
            lines.append("        /** @type {BigInt} */")
            lines.append("        this.accumulator = 0n;")
        else:
            lines.append("        /** @type {number} */")
            lines.append("        this.initialState = 0;")
            lines.append("        /** @type {number} */")
            lines.append("        this.accumulator = 0;")

        if operations.get('rollback', {}).get('enabled', False):
            history_depth = operations['rollback'].get('history_depth', 10)
            if use_bigint:
                lines.append("        /** @type {BigInt[]} */")
            else:
                lines.append("        /** @type {number[]} */")
            lines.append("        this.history = [];")
            lines.append("        /** @type {number} */")
            lines.append(f"        this.maxHistory = {history_depth};")

        lines.append("    }")
        lines.append("")

        # Load method
        lines.append("    /**")
        lines.append("     * Load initial state (LOAD operation)")
        lines.append("     * ")
        if use_bigint:
            lines.append("     * @param {BigInt} initialState - Initial state value")
        else:
            lines.append("     * @param {number} initialState - Initial state value")
        lines.append("     */")
        lines.append("    load(initialState) {")
        lines.append("        this.initialState = initialState;")
        if use_bigint:
            lines.append("        this.accumulator = 0n;")
        else:
            lines.append("        this.accumulator = 0;")
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("        this.history = [];")
        lines.append("    }")
        lines.append("")

        # Accumulate method
        lines.append("    /**")
        lines.append("     * Accumulate delta (ACCUMULATE operation)")
        lines.append("     * ")
        lines.append("     * XORs the delta into the accumulator.")
        lines.append("     * ")
        if use_bigint:
            lines.append("     * @param {BigInt} delta - Delta value to accumulate")
        else:
            lines.append("     * @param {number} delta - Delta value to accumulate")
        lines.append("     */")
        lines.append("    accumulate(delta) {")
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("        // Save to history")
            lines.append("        this.history.push(delta);")
            lines.append("        if (this.history.length > this.maxHistory) {")
            lines.append("            this.history.shift();")
            lines.append("        }")
        lines.append("        // XOR delta into accumulator")
        lines.append("        this.accumulator ^= delta;")
        lines.append("    }")
        lines.append("")

        # Reconstruct method
        lines.append("    /**")
        lines.append("     * Reconstruct current state (READ operation)")
        lines.append("     * ")
        lines.append("     * Returns current_state = initial_state XOR accumulator")
        lines.append("     * ")
        if use_bigint:
            lines.append("     * @returns {BigInt} Current state")
        else:
            lines.append("     * @returns {number} Current state")
        lines.append("     */")
        lines.append("    reconstruct() {")
        lines.append("        return this.initialState ^ this.accumulator;")
        lines.append("    }")
        lines.append("")

        # Status method
        lines.append("    /**")
        lines.append("     * Check if accumulator is zero (STATUS operation)")
        lines.append("     * ")
        lines.append("     * @returns {boolean} True if accumulator is zero")
        lines.append("     */")
        lines.append("    isAccumulatorZero() {")
        if use_bigint:
            lines.append("        return this.accumulator === 0n;")
        else:
            lines.append("        return this.accumulator === 0;")
        lines.append("    }")
        lines.append("")

        # Rollback method
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("    /**")
            lines.append("     * Rollback the last N delta operations")
            lines.append("     * ")
            lines.append("     * @param {number} count - Number of deltas to rollback")
            lines.append("     * @returns {number} Number of deltas actually rolled back")
            lines.append("     */")
            lines.append("    rollback(count) {")
            lines.append("        const actualCount = Math.min(count, this.history.length);")
            lines.append("        for (let i = 0; i < actualCount; i++) {")
            lines.append("            const delta = this.history.pop();")
            lines.append("            // XOR removes the delta (self-inverse property)")
            lines.append("            this.accumulator ^= delta;")
            lines.append("        }")
            lines.append("        return actualCount;")
            lines.append("    }")
            lines.append("")

        # Getter methods
        lines.append("    /**")
        lines.append("     * Get the current accumulator value")
        lines.append("     * ")
        if use_bigint:
            lines.append("     * @returns {BigInt} Accumulator value")
        else:
            lines.append("     * @returns {number} Accumulator value")
        lines.append("     */")
        lines.append("    getAccumulator() {")
        lines.append("        return this.accumulator;")
        lines.append("    }")
        lines.append("")

        lines.append("    /**")
        lines.append("     * Get the initial state")
        lines.append("     * ")
        if use_bigint:
            lines.append("     * @returns {BigInt} Initial state")
        else:
            lines.append("     * @returns {number} Initial state")
        lines.append("     */")
        lines.append("    getInitialState() {")
        lines.append("        return this.initialState;")
        lines.append("    }")
        lines.append("")

        if operations.get('rollback', {}).get('enabled', False):
            lines.append("    /**")
            lines.append("     * Get the number of deltas in history")
            lines.append("     * ")
            lines.append("     * @returns {number} History size")
            lines.append("     */")
            lines.append("    historySize() {")
            lines.append("        return this.history.length;")
            lines.append("    }")
            lines.append("")

        lines.append("}")
        lines.append("")

        # Default export
        lines.append(f"export default {obj_name};")
        lines.append("")

        content = "\n".join(lines)

        return GeneratedFile(
            relative_path=f"src/{obj_name}.js",
            content=content,
            language='javascript',
            description=f"JavaScript module for {namespace.object}"
        )

    def _generate_index(self, namespace: NamespaceMapping) -> GeneratedFile:
        """Generate index.js entry point."""

        obj_name = namespace.object

        lines = []
        lines.append("/**")
        lines.append(f" * ATOMiK {obj_name} SDK")
        lines.append(" */")
        lines.append("")
        lines.append(f"export {{ {obj_name} }} from './src/{obj_name}.js';")
        lines.append(f"export {{ default as default }} from './src/{obj_name}.js';")
        lines.append("")

        content = "\n".join(lines)

        return GeneratedFile(
            relative_path="index.js",
            content=content,
            language='javascript',
            description="Package entry point"
        )

    def _generate_package_json(
        self,
        catalogue: Dict[str, Any],
        namespace: NamespaceMapping
    ) -> GeneratedFile:
        """Generate package.json for NPM."""

        vertical_lower = namespace.vertical.lower()
        field_lower = namespace.field.lower()
        obj_name = namespace.object

        version = catalogue.get('version', '1.0.0')
        description = catalogue.get('description', f'{obj_name} delta-state module')

        package = {
            "name": f"@atomik/{vertical_lower}-{field_lower}",
            "version": version,
            "description": description,
            "main": "index.js",
            "type": "module",
            "scripts": {
                "test": "node test/%s.test.js" % obj_name
            },
            "keywords": [
                "atomik",
                "delta-state",
                "xor",
                vertical_lower,
                field_lower
            ],
            "author": "ATOMiK SDK Generator",
            "license": "MIT",
            "engines": {
                "node": ">=14.0.0"
            }
        }

        import json
        content = json.dumps(package, indent=2) + "\n"

        return GeneratedFile(
            relative_path="package.json",
            content=content,
            language='javascript',
            description="NPM package manifest"
        )

    def _generate_test(
        self,
        namespace: NamespaceMapping,
        delta_fields: Dict[str, Any],
        operations: Dict[str, Any]
    ) -> GeneratedFile:
        """Generate test file."""

        obj_name = namespace.object
        use_bigint = self._should_use_bigint(delta_fields)

        lines = []

        # Imports
        lines.append(f"import {{ {obj_name} }} from '../src/{obj_name}.js';")
        lines.append("import assert from 'assert';")
        lines.append("")

        # Test header
        lines.append(f"console.log('Testing {obj_name}...');")
        lines.append("console.log('='.repeat(70));")
        lines.append("")

        # Test load
        lines.append("// Test 1: LOAD operation")
        lines.append(f"const manager = new {obj_name}();")
        if use_bigint:
            lines.append("manager.load(0x1234567890ABCDEFn);")
            lines.append("assert.strictEqual(manager.getInitialState(), 0x1234567890ABCDEFn);")
            lines.append("assert.strictEqual(manager.getAccumulator(), 0n);")
        else:
            lines.append("manager.load(0x12345678);")
            lines.append("assert.strictEqual(manager.getInitialState(), 0x12345678);")
            lines.append("assert.strictEqual(manager.getAccumulator(), 0);")
        lines.append("console.log('[PASS] test_load');")
        lines.append("")

        # Test accumulate
        lines.append("// Test 2: ACCUMULATE operation")
        if use_bigint:
            lines.append("manager.load(0n);")
            lines.append("manager.accumulate(0x1111111111111111n);")
            lines.append("assert.strictEqual(manager.getAccumulator(), 0x1111111111111111n);")
            lines.append("manager.accumulate(0x2222222222222222n);")
            lines.append("assert.strictEqual(manager.getAccumulator(), 0x3333333333333333n);")
        else:
            lines.append("manager.load(0);")
            lines.append("manager.accumulate(0x11111111);")
            lines.append("assert.strictEqual(manager.getAccumulator(), 0x11111111);")
            lines.append("manager.accumulate(0x22222222);")
            lines.append("assert.strictEqual(manager.getAccumulator(), 0x33333333);")
        lines.append("console.log('[PASS] test_accumulate');")
        lines.append("")

        # Test reconstruct
        lines.append("// Test 3: RECONSTRUCT operation")
        if use_bigint:
            lines.append("manager.load(0xAAAAAAAAAAAAAAAAn);")
            lines.append("manager.accumulate(0x5555555555555555n);")
            lines.append("// 0xAAAA XOR 0x5555 = 0xFFFF")
            lines.append("assert.strictEqual(manager.reconstruct(), 0xFFFFFFFFFFFFFFFFn);")
        else:
            lines.append("manager.load(0xAAAAAAAA);")
            lines.append("manager.accumulate(0x55555555);")
            lines.append("// 0xAAAA XOR 0x5555 = 0xFFFF")
            lines.append("assert.strictEqual(manager.reconstruct(), 0xFFFFFFFF);")
        lines.append("console.log('[PASS] test_reconstruct');")
        lines.append("")

        # Test self-inverse
        lines.append("// Test 4: Self-inverse property")
        if use_bigint:
            lines.append("manager.load(0xAAAAAAAAAAAAAAAAn);")
            lines.append("const delta = 0x1234567890ABCDEFn;")
        else:
            lines.append("manager.load(0xAAAAAAAA);")
            lines.append("const delta = 0x12345678;")
        lines.append("manager.accumulate(delta);")
        lines.append("manager.accumulate(delta);  // Apply same delta twice")
        lines.append("// Self-inverse: delta XOR delta = 0")
        lines.append("assert.strictEqual(manager.isAccumulatorZero(), true);")
        if use_bigint:
            lines.append("assert.strictEqual(manager.reconstruct(), 0xAAAAAAAAAAAAAAAAn);")
        else:
            lines.append("assert.strictEqual(manager.reconstruct(), 0xAAAAAAAA);")
        lines.append("console.log('[PASS] test_self_inverse');")
        lines.append("")

        # Test rollback
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("// Test 5: Rollback operation")
            if use_bigint:
                lines.append("manager.load(0n);")
                lines.append("manager.accumulate(0x1111111111111111n);")
                lines.append("manager.accumulate(0x2222222222222222n);")
                lines.append("manager.accumulate(0x4444444444444444n);")
                lines.append("assert.strictEqual(manager.getAccumulator(), 0x7777777777777777n);")
            else:
                lines.append("manager.load(0);")
                lines.append("manager.accumulate(0x11111111);")
                lines.append("manager.accumulate(0x22222222);")
                lines.append("manager.accumulate(0x44444444);")
                lines.append("assert.strictEqual(manager.getAccumulator(), 0x77777777);")
            lines.append("// Rollback last 2 operations")
            lines.append("const count = manager.rollback(2);")
            lines.append("assert.strictEqual(count, 2);")
            if use_bigint:
                lines.append("assert.strictEqual(manager.getAccumulator(), 0x1111111111111111n);")
            else:
                lines.append("assert.strictEqual(manager.getAccumulator(), 0x11111111);")
            lines.append("console.log('[PASS] test_rollback');")
            lines.append("")

        # Test summary
        lines.append("console.log('='.repeat(70));")
        lines.append("console.log('All tests passed!');")
        lines.append("")

        content = "\n".join(lines)

        return GeneratedFile(
            relative_path=f"test/{obj_name}.test.js",
            content=content,
            language='javascript',
            description=f"Tests for {namespace.object}"
        )

    @staticmethod
    def _should_use_bigint(delta_fields: Dict[str, Any]) -> bool:
        """Determine if BigInt should be used (>53 bits)."""
        max_width = 64
        for field_name, field_spec in delta_fields.items():
            width = field_spec.get('width', 64)
            max_width = max(max_width, width)
        # JavaScript Number can safely represent integers up to 53 bits
        return max_width > 53
