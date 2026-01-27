"""
ATOMiK C SDK Generator

Generates C header and implementation files from ATOMiK schemas.
"""

from __future__ import annotations

from typing import Any

from .code_emitter import CodeEmitter, GeneratedFile, GenerationResult
from .namespace_mapper import NamespaceMapping


class CGenerator(CodeEmitter):
    """
    Generates C SDK code from ATOMiK schemas.

    Generates:
    - atomik/{vertical}/{field}/{object}.h - Header file
    - atomik/{vertical}/{field}/{object}.c - Implementation file
    - tests/test_{object}.c - Test program
    - Makefile - Build configuration
    """

    def __init__(self):
        """Initialize C code generator."""
        super().__init__('c')

    def generate(self, schema: dict[str, Any], namespace: NamespaceMapping) -> GenerationResult:
        """Generate C SDK code from schema."""
        try:
            files = []
            errors = []
            warnings = []

            # Extract schema components
            schema.get('catalogue', {})
            schema_def = schema.get('schema', {})
            delta_fields = schema_def.get('delta_fields', {})
            operations = schema_def.get('operations', {})
            hardware = schema.get('hardware', {})

            # Generate header file
            header_file = self._generate_header(namespace, delta_fields, operations)
            files.append(header_file)

            # Generate implementation file
            impl_file = self._generate_implementation(
                namespace, delta_fields, operations, hardware
            )
            files.append(impl_file)

            # Generate test file
            test_file = self._generate_test(namespace, delta_fields, operations)
            files.append(test_file)

            # Generate Makefile
            makefile = self._generate_makefile(namespace)
            files.append(makefile)

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
                errors=[f"C generation failed: {str(e)}"],
                warnings=[]
            )

    def _generate_header(
        self,
        namespace: NamespaceMapping,
        delta_fields: dict[str, Any],
        operations: dict[str, Any]
    ) -> GeneratedFile:
        """Generate C header file."""

        obj_snake = self._to_snake_case(namespace.object)
        vertical_lower = namespace.vertical.lower()
        field_lower = namespace.field.lower()

        # Header guard
        guard = f"ATOMIK_{vertical_lower.upper()}_{field_lower.upper()}_{obj_snake.upper()}_H"

        # Determine delta type
        delta_type = self._get_c_type(delta_fields)

        lines = []

        # Header guard start
        lines.append(f"#ifndef {guard}")
        lines.append(f"#define {guard}")
        lines.append("")

        # Includes
        lines.append("#include <stdint.h>")
        lines.append("#include <stdbool.h>")
        lines.append("")

        # Rollback support requires additional includes
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("/* History buffer for rollback */")
            history_depth = operations['rollback'].get('history_depth', 10)
            lines.append(f"#define ATOMIK_HISTORY_DEPTH {history_depth}")
            lines.append("")

        # Struct definition
        lines.append("/**")
        lines.append(f" * {namespace.object} - Delta-state manager")
        lines.append(" *")
        lines.append(" * Manages delta-state operations using XOR algebra.")
        lines.append(" */")
        lines.append("typedef struct {")
        lines.append(f"    {delta_type} initial_state;  /**< Initial state */")
        lines.append(f"    {delta_type} accumulator;    /**< Delta accumulator (XOR of all deltas) */")

        if operations.get('rollback', {}).get('enabled', False):
            lines.append(f"    {delta_type} history[ATOMIK_HISTORY_DEPTH];  /**< Delta history */")
            lines.append("    size_t history_count;      /**< Number of deltas in history */")
            lines.append("    size_t history_head;       /**< History buffer head index */")

        lines.append(f"}} atomik_{obj_snake}_t;")
        lines.append("")

        # Function prototypes
        lines.append("/**")
        lines.append(f" * Initialize a new {namespace.object} instance")
        lines.append(" */")
        lines.append(f"void atomik_{obj_snake}_init(atomik_{obj_snake}_t *manager);")
        lines.append("")

        lines.append("/**")
        lines.append(" * Load initial state (LOAD operation)")
        lines.append(" */")
        lines.append(f"void atomik_{obj_snake}_load(atomik_{obj_snake}_t *manager, {delta_type} initial_state);")
        lines.append("")

        lines.append("/**")
        lines.append(" * Accumulate delta (ACCUMULATE operation)")
        lines.append(" *")
        lines.append(" * XORs the delta into the accumulator.")
        lines.append(" */")
        lines.append(f"void atomik_{obj_snake}_accumulate(atomik_{obj_snake}_t *manager, {delta_type} delta);")
        lines.append("")

        lines.append("/**")
        lines.append(" * Reconstruct current state (READ operation)")
        lines.append(" *")
        lines.append(" * Returns current_state = initial_state XOR accumulator")
        lines.append(" */")
        lines.append(f"{delta_type} atomik_{obj_snake}_reconstruct(const atomik_{obj_snake}_t *manager);")
        lines.append("")

        lines.append("/**")
        lines.append(" * Check if accumulator is zero (STATUS operation)")
        lines.append(" */")
        lines.append(f"bool atomik_{obj_snake}_is_accumulator_zero(const atomik_{obj_snake}_t *manager);")
        lines.append("")

        if operations.get('rollback', {}).get('enabled', False):
            lines.append("/**")
            lines.append(" * Rollback the last N delta operations")
            lines.append(" *")
            lines.append(" * Returns the number of deltas actually rolled back.")
            lines.append(" */")
            lines.append(f"size_t atomik_{obj_snake}_rollback(atomik_{obj_snake}_t *manager, size_t count);")
            lines.append("")

        # Getter functions
        lines.append("/**")
        lines.append(" * Get the current accumulator value")
        lines.append(" */")
        lines.append(f"{delta_type} atomik_{obj_snake}_get_accumulator(const atomik_{obj_snake}_t *manager);")
        lines.append("")

        lines.append("/**")
        lines.append(" * Get the initial state")
        lines.append(" */")
        lines.append(f"{delta_type} atomik_{obj_snake}_get_initial_state(const atomik_{obj_snake}_t *manager);")
        lines.append("")

        # Header guard end
        lines.append(f"#endif /* {guard} */")
        lines.append("")

        content = "\n".join(lines)

        return GeneratedFile(
            relative_path=f"atomik/{vertical_lower}/{field_lower}/{obj_snake}.h",
            content=content,
            language='c',
            description=f"C header file for {namespace.object}"
        )

    def _generate_implementation(
        self,
        namespace: NamespaceMapping,
        delta_fields: dict[str, Any],
        operations: dict[str, Any],
        hardware: dict[str, Any]
    ) -> GeneratedFile:
        """Generate C implementation file."""

        obj_snake = self._to_snake_case(namespace.object)
        vertical_lower = namespace.vertical.lower()
        field_lower = namespace.field.lower()

        delta_type = self._get_c_type(delta_fields)

        lines = []

        # Includes
        lines.append(f"#include \"{obj_snake}.h\"")
        lines.append("#include <string.h>")
        lines.append("")

        # Init function
        lines.append(f"void atomik_{obj_snake}_init(atomik_{obj_snake}_t *manager) {{")
        lines.append("    memset(manager, 0, sizeof(atomik_{obj_snake}_t));")
        lines.append("}")
        lines.append("")

        # Load function
        lines.append(f"void atomik_{obj_snake}_load(atomik_{obj_snake}_t *manager, {delta_type} initial_state) {{")
        lines.append("    manager->initial_state = initial_state;")
        lines.append("    manager->accumulator = 0;")
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("    manager->history_count = 0;")
            lines.append("    manager->history_head = 0;")
        lines.append("}")
        lines.append("")

        # Accumulate function
        lines.append(f"void atomik_{obj_snake}_accumulate(atomik_{obj_snake}_t *manager, {delta_type} delta) {{")
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("    /* Save to history */")
            lines.append("    manager->history[manager->history_head] = delta;")
            lines.append("    manager->history_head = (manager->history_head + 1) % ATOMIK_HISTORY_DEPTH;")
            lines.append("    if (manager->history_count < ATOMIK_HISTORY_DEPTH) {")
            lines.append("        manager->history_count++;")
            lines.append("    }")
            lines.append("")
        lines.append("    /* XOR delta into accumulator */")
        lines.append("    manager->accumulator ^= delta;")
        lines.append("}")
        lines.append("")

        # Reconstruct function
        lines.append(f"{delta_type} atomik_{obj_snake}_reconstruct(const atomik_{obj_snake}_t *manager) {{")
        lines.append("    return manager->initial_state ^ manager->accumulator;")
        lines.append("}")
        lines.append("")

        # Status function
        lines.append(f"bool atomik_{obj_snake}_is_accumulator_zero(const atomik_{obj_snake}_t *manager) {{")
        lines.append("    return manager->accumulator == 0;")
        lines.append("}")
        lines.append("")

        # Rollback function
        if operations.get('rollback', {}).get('enabled', False):
            lines.append(f"size_t atomik_{obj_snake}_rollback(atomik_{obj_snake}_t *manager, size_t count) {{")
            lines.append("    size_t actual_count = (count < manager->history_count) ? count : manager->history_count;")
            lines.append("    ")
            lines.append("    for (size_t i = 0; i < actual_count; i++) {")
            lines.append("        /* Calculate index (going backwards from head) */")
            lines.append("        size_t index = (manager->history_head + ATOMIK_HISTORY_DEPTH - 1 - i) % ATOMIK_HISTORY_DEPTH;")
            lines.append("        ")
            lines.append("        /* XOR removes the delta (self-inverse property) */")
            lines.append("        manager->accumulator ^= manager->history[index];")
            lines.append("    }")
            lines.append("    ")
            lines.append("    /* Update history tracking */")
            lines.append("    manager->history_count -= actual_count;")
            lines.append("    manager->history_head = (manager->history_head + ATOMIK_HISTORY_DEPTH - actual_count) % ATOMIK_HISTORY_DEPTH;")
            lines.append("    ")
            lines.append("    return actual_count;")
            lines.append("}")
            lines.append("")

        # Getter functions
        lines.append(f"{delta_type} atomik_{obj_snake}_get_accumulator(const atomik_{obj_snake}_t *manager) {{")
        lines.append("    return manager->accumulator;")
        lines.append("}")
        lines.append("")

        lines.append(f"{delta_type} atomik_{obj_snake}_get_initial_state(const atomik_{obj_snake}_t *manager) {{")
        lines.append("    return manager->initial_state;")
        lines.append("}")
        lines.append("")

        content = "\n".join(lines)

        return GeneratedFile(
            relative_path=f"atomik/{vertical_lower}/{field_lower}/{obj_snake}.c",
            content=content,
            language='c',
            description=f"C implementation for {namespace.object}"
        )

    def _generate_test(
        self,
        namespace: NamespaceMapping,
        delta_fields: dict[str, Any],
        operations: dict[str, Any]
    ) -> GeneratedFile:
        """Generate C test file."""

        obj_snake = self._to_snake_case(namespace.object)
        vertical_lower = namespace.vertical.lower()
        field_lower = namespace.field.lower()

        delta_type = self._get_c_type(delta_fields)

        lines = []

        # Includes
        lines.append("#include <stdio.h>")
        lines.append("#include <assert.h>")
        lines.append(f"#include \"../atomik/{vertical_lower}/{field_lower}/{obj_snake}.h\"")
        lines.append("")

        # Test functions
        lines.append("void test_load() {")
        lines.append(f"    atomik_{obj_snake}_t manager;")
        lines.append(f"    atomik_{obj_snake}_init(&manager);")
        lines.append(f"    atomik_{obj_snake}_load(&manager, 0x1234567890ABCDEFULL);")
        lines.append("    ")
        lines.append(f"    assert(atomik_{obj_snake}_get_initial_state(&manager) == 0x1234567890ABCDEFULL);")
        lines.append(f"    assert(atomik_{obj_snake}_get_accumulator(&manager) == 0);")
        lines.append('    printf("[PASS] test_load\\n");')
        lines.append("}")
        lines.append("")

        lines.append("void test_accumulate() {")
        lines.append(f"    atomik_{obj_snake}_t manager;")
        lines.append(f"    atomik_{obj_snake}_init(&manager);")
        lines.append(f"    atomik_{obj_snake}_load(&manager, 0);")
        lines.append("    ")
        lines.append(f"    atomik_{obj_snake}_accumulate(&manager, 0x1111111111111111ULL);")
        lines.append(f"    assert(atomik_{obj_snake}_get_accumulator(&manager) == 0x1111111111111111ULL);")
        lines.append("    ")
        lines.append(f"    atomik_{obj_snake}_accumulate(&manager, 0x2222222222222222ULL);")
        lines.append(f"    assert(atomik_{obj_snake}_get_accumulator(&manager) == 0x3333333333333333ULL);")
        lines.append('    printf("[PASS] test_accumulate\\n");')
        lines.append("}")
        lines.append("")

        lines.append("void test_reconstruct() {")
        lines.append(f"    atomik_{obj_snake}_t manager;")
        lines.append(f"    atomik_{obj_snake}_init(&manager);")
        lines.append(f"    atomik_{obj_snake}_load(&manager, 0xAAAAAAAAAAAAAAAAULL);")
        lines.append(f"    atomik_{obj_snake}_accumulate(&manager, 0x5555555555555555ULL);")
        lines.append("    ")
        lines.append("    /* 0xAAAA XOR 0x5555 = 0xFFFF */")
        lines.append(f"    assert(atomik_{obj_snake}_reconstruct(&manager) == 0xFFFFFFFFFFFFFFFFULL);")
        lines.append('    printf("[PASS] test_reconstruct\\n");')
        lines.append("}")
        lines.append("")

        lines.append("void test_self_inverse() {")
        lines.append(f"    atomik_{obj_snake}_t manager;")
        lines.append(f"    atomik_{obj_snake}_init(&manager);")
        lines.append(f"    atomik_{obj_snake}_load(&manager, 0xAAAAAAAAAAAAAAAAULL);")
        lines.append("    ")
        lines.append(f"    {delta_type} delta = 0x1234567890ABCDEFULL;")
        lines.append(f"    atomik_{obj_snake}_accumulate(&manager, delta);")
        lines.append(f"    atomik_{obj_snake}_accumulate(&manager, delta);  /* Apply same delta twice */")
        lines.append("    ")
        lines.append("    /* Self-inverse: delta XOR delta = 0 */")
        lines.append(f"    assert(atomik_{obj_snake}_is_accumulator_zero(&manager));")
        lines.append(f"    assert(atomik_{obj_snake}_reconstruct(&manager) == 0xAAAAAAAAAAAAAAAAULL);")
        lines.append('    printf("[PASS] test_self_inverse\\n");')
        lines.append("}")
        lines.append("")

        if operations.get('rollback', {}).get('enabled', False):
            lines.append("void test_rollback() {")
            lines.append(f"    atomik_{obj_snake}_t manager;")
            lines.append(f"    atomik_{obj_snake}_init(&manager);")
            lines.append(f"    atomik_{obj_snake}_load(&manager, 0);")
            lines.append("    ")
            lines.append(f"    atomik_{obj_snake}_accumulate(&manager, 0x1111111111111111ULL);")
            lines.append(f"    atomik_{obj_snake}_accumulate(&manager, 0x2222222222222222ULL);")
            lines.append(f"    atomik_{obj_snake}_accumulate(&manager, 0x4444444444444444ULL);")
            lines.append(f"    assert(atomik_{obj_snake}_get_accumulator(&manager) == 0x7777777777777777ULL);")
            lines.append("    ")
            lines.append("    /* Rollback last 2 operations */")
            lines.append(f"    size_t count = atomik_{obj_snake}_rollback(&manager, 2);")
            lines.append("    assert(count == 2);")
            lines.append(f"    assert(atomik_{obj_snake}_get_accumulator(&manager) == 0x1111111111111111ULL);")
            lines.append('    printf("[PASS] test_rollback\\n");')
            lines.append("}")
            lines.append("")

        # Main function
        lines.append("int main() {")
        lines.append(f'    printf("Testing {namespace.object}...\\n");')
        lines.append("")
        lines.append("    test_load();")
        lines.append("    test_accumulate();")
        lines.append("    test_reconstruct();")
        lines.append("    test_self_inverse();")
        if operations.get('rollback', {}).get('enabled', False):
            lines.append("    test_rollback();")
        lines.append("")
        lines.append('    printf("\\nAll tests passed!\\n");')
        lines.append("    return 0;")
        lines.append("}")
        lines.append("")

        content = "\n".join(lines)

        return GeneratedFile(
            relative_path=f"tests/test_{obj_snake}.c",
            content=content,
            language='c',
            description=f"C tests for {namespace.object}"
        )

    def _generate_makefile(self, namespace: NamespaceMapping) -> GeneratedFile:
        """Generate Makefile for building."""

        obj_snake = self._to_snake_case(namespace.object)
        vertical_lower = namespace.vertical.lower()
        field_lower = namespace.field.lower()

        lines = []
        lines.append("# Makefile for ATOMiK C SDK")
        lines.append("")
        lines.append("CC = gcc")
        lines.append("CFLAGS = -Wall -Wextra -std=c99 -I.")
        lines.append("")
        lines.append(f"SRC = atomik/{vertical_lower}/{field_lower}/{obj_snake}.c")
        lines.append(f"TEST = tests/test_{obj_snake}.c")
        lines.append(f"TARGET = test_{obj_snake}")
        lines.append("")
        lines.append("all: $(TARGET)")
        lines.append("")
        lines.append("$(TARGET): $(SRC) $(TEST)")
        lines.append("\t$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(TEST)")
        lines.append("")
        lines.append("clean:")
        lines.append("\trm -f $(TARGET)")
        lines.append("")
        lines.append(".PHONY: all clean")
        lines.append("")

        content = "\n".join(lines)

        return GeneratedFile(
            relative_path="Makefile",
            content=content,
            language='c',
            description="Build configuration for C SDK"
        )

    @staticmethod
    def _get_c_type(delta_fields: dict[str, Any]) -> str:
        """Determine C type based on delta field widths."""
        max_width = 64
        for field_name, field_spec in delta_fields.items():
            width = field_spec.get('width', 64)
            max_width = max(max_width, width)

        if max_width <= 64:
            return "uint64_t"
        elif max_width <= 128:
            # Note: uint128_t is not standard C, would need __uint128_t (GCC) or custom implementation
            return "uint64_t"  # Simplified for now
        else:
            return "uint64_t"  # Simplified for now

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
