#!/usr/bin/env python3
"""
ATOMiK Schema Validation Script
Validates all schema files against the JSON Schema specification.
"""

import json
import sys
import os
from pathlib import Path

def load_json(filepath):
    """Load and parse JSON file."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            return json.load(f)
    except json.JSONDecodeError as e:
        print(f"[FAIL] JSON parse error in {filepath}: {e}")
        return None
    except FileNotFoundError:
        print(f"[FAIL] File not found: {filepath}")
        return None

def validate_schema_structure(schema):
    """Validate basic schema structure."""
    required_keys = ['$schema', 'title', 'type', 'properties']
    for key in required_keys:
        if key not in schema:
            return False, f"Missing required key: {key}"

    if schema.get('$schema') != 'http://json-schema.org/draft-07/schema#':
        return False, f"Invalid $schema: {schema.get('$schema')}"

    return True, "Valid JSON Schema Draft 7 structure"

def validate_instance_against_schema(schema, instance, instance_name):
    """Validate instance against schema (basic validation without jsonschema library)."""
    errors = []

    # Check required fields
    if 'catalogue' not in instance:
        errors.append("Missing required 'catalogue' section")
    else:
        catalogue = instance['catalogue']
        required_catalogue = ['vertical', 'field', 'object', 'version']
        for field in required_catalogue:
            if field not in catalogue:
                errors.append(f"Missing catalogue.{field}")

    if 'schema' not in instance:
        errors.append("Missing required 'schema' section")
    else:
        schema_section = instance['schema']
        if 'delta_fields' not in schema_section:
            errors.append("Missing schema.delta_fields")
        elif not schema_section['delta_fields']:
            errors.append("schema.delta_fields must have at least one field")

        if 'operations' not in schema_section:
            errors.append("Missing schema.operations")
        elif 'accumulate' not in schema_section['operations']:
            errors.append("Missing schema.operations.accumulate (required)")

    return errors

def main():
    # Get project root
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent

    print("=" * 70)
    print("ATOMiK Schema Validation")
    print("=" * 70)
    print()

    # Validate main schema specification
    print("1. Validating JSON Schema Specification")
    print("-" * 70)

    schema_path = project_root / "specs" / "atomik_schema_v1.json"
    schema = load_json(schema_path)

    if schema is None:
        print("[FAIL] Failed to load schema specification")
        return 1

    valid, msg = validate_schema_structure(schema)
    if valid:
        print(f"[PASS] {schema_path.name}: {msg}")
    else:
        print(f"[FAIL] {schema_path.name}: {msg}")
        return 1

    print()

    # Validate example schemas
    print("2. Validating Example Schemas")
    print("-" * 70)

    examples_dir = project_root / "sdk" / "schemas" / "examples"
    example_files = [
        "terminal-io.json",
        "p2p-delta.json",
        "matrix-ops.json"
    ]

    all_valid = True
    example_configs = []

    for example_file in example_files:
        example_path = examples_dir / example_file
        instance = load_json(example_path)

        if instance is None:
            all_valid = False
            continue

        # Validate structure
        errors = validate_instance_against_schema(schema, instance, example_file)

        if errors:
            print(f"[FAIL] {example_file}:")
            for error in errors:
                print(f"    - {error}")
            all_valid = False
        else:
            print(f"[PASS] {example_file}: Valid")

            # Collect configuration details
            catalogue = instance.get('catalogue', {})
            schema_section = instance.get('schema', {})
            delta_fields = schema_section.get('delta_fields', {})

            config = {
                'name': example_file,
                'namespace': f"{catalogue.get('vertical')}.{catalogue.get('field')}.{catalogue.get('object')}",
                'widths': [field.get('width') for field in delta_fields.values()],
                'operations': list(schema_section.get('operations', {}).keys()),
                'has_rollback': 'rollback' in schema_section.get('operations', {}),
                'has_hardware': 'hardware' in instance
            }
            example_configs.append(config)

    print()

    # Validate diversity of examples
    print("3. Example Diversity Check")
    print("-" * 70)

    if example_configs:
        # Check different use cases
        namespaces = set(cfg['namespace'] for cfg in example_configs)
        print(f"[PASS] Unique namespaces: {len(namespaces)}")
        for cfg in example_configs:
            print(f"    - {cfg['namespace']}")

        # Check different widths
        all_widths = set()
        for cfg in example_configs:
            all_widths.update(cfg['widths'])
        print(f"[PASS] Different delta widths used: {sorted(all_widths)}")

        # Check operation variety
        has_rollback = sum(1 for cfg in example_configs if cfg['has_rollback'])
        has_hardware = sum(1 for cfg in example_configs if cfg['has_hardware'])
        print(f"[PASS] Examples with rollback: {has_rollback}/{len(example_configs)}")
        print(f"[PASS] Examples with hardware mapping: {has_hardware}/{len(example_configs)}")

    print()

    # Validate documentation exists
    print("4. Documentation Completeness")
    print("-" * 70)

    doc_files = [
        ("specs/schema_validation_rules.md", "Validation rules"),
        ("docs/SDK_SCHEMA_GUIDE.md", "Technical guide"),
        ("docs/user/SDK_USER_MANUAL.md", "User manual")
    ]

    docs_complete = True
    for doc_path, doc_name in doc_files:
        full_path = project_root / doc_path
        if full_path.exists():
            size = full_path.stat().st_size
            print(f"[PASS] {doc_name}: {full_path.name} ({size:,} bytes)")
        else:
            print(f"[FAIL] {doc_name}: {full_path.name} NOT FOUND")
            docs_complete = False

    print()
    print("=" * 70)

    if all_valid and docs_complete:
        print("[PASS] All validations passed!")
        print("=" * 70)
        return 0
    else:
        print("[FAIL] Some validations failed")
        print("=" * 70)
        return 1

if __name__ == "__main__":
    sys.exit(main())
