#!/usr/bin/env python3
"""
ATOMiK Schema Full Validation Script
Uses jsonschema library for complete JSON Schema Draft 7 validation.
"""

import json
import sys
from pathlib import Path

try:
    import jsonschema
    from jsonschema import validate, ValidationError, SchemaError
    HAS_JSONSCHEMA = True
except ImportError:
    HAS_JSONSCHEMA = False

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

def main():
    
    if not HAS_JSONSCHEMA:
        print("[WARN] jsonschema library not available, run: pip install jsonschema")
        print("[INFO] Using basic validation only")
        sys.exit(1)

    # Get project root
    script_dir = Path(__file__).parent
    project_root = script_dir.parent

    print("=" * 70)
    print("ATOMiK Schema Full Validation (JSON Schema Draft 7)")
    print("=" * 70)
    print()

    # Load the schema specification
    schema_path = project_root / "specs" / "atomik_schema_v1.json"
    schema_spec = load_json(schema_path)

    if schema_spec is None:
        return 1

    # Validate the schema specification itself
    print("1. Validating Schema Specification Structure")
    print("-" * 70)
    try:
        # Check it's a valid meta-schema
        jsonschema.Draft7Validator.check_schema(schema_spec)
        print(f"[PASS] {schema_path.name} is a valid JSON Schema Draft 7")
    except SchemaError as e:
        print(f"[FAIL] {schema_path.name} has schema errors: {e.message}")
        return 1

    print()

    # Validate example instances
    print("2. Validating Example Schemas Against Specification")
    print("-" * 70)

    examples_dir = project_root / "sdk" / "schemas" / "examples"
    example_files = [
        "terminal-io.json",
        "p2p-delta.json",
        "matrix-ops.json"
    ]

    all_valid = True
    validator = jsonschema.Draft7Validator(schema_spec)

    for example_file in example_files:
        example_path = examples_dir / example_file
        instance = load_json(example_path)

        if instance is None:
            all_valid = False
            continue

        # Validate against schema
        errors = list(validator.iter_errors(instance))

        if errors:
            print(f"[FAIL] {example_file}:")
            for error in errors:
                path = ".".join(str(p) for p in error.path) if error.path else "root"
                print(f"    - {path}: {error.message}")
            all_valid = False
        else:
            print(f"[PASS] {example_file}: Valid against schema specification")

    print()

    # Verify cross-field dependencies
    print("3. Cross-Field Dependency Validation")
    print("-" * 70)

    for example_file in example_files:
        example_path = examples_dir / example_file
        instance = load_json(example_path)

        if instance is None:
            continue

        errors = []

        # Rule 1: Hardware DATA_WIDTH must match delta field widths
        if 'hardware' in instance and 'rtl_params' in instance['hardware']:
            rtl_params = instance['hardware']['rtl_params']
            if 'DATA_WIDTH' in rtl_params:
                data_width = rtl_params['DATA_WIDTH']
                delta_fields = instance['schema']['delta_fields']

                for field_name, field_spec in delta_fields.items():
                    if field_spec['width'] != data_width:
                        errors.append(
                            f"hardware.rtl_params.DATA_WIDTH ({data_width}) "
                            f"!= delta_fields.{field_name}.width ({field_spec['width']})"
                        )

        # Rule 2: Rollback enabled requires history_depth
        operations = instance['schema'].get('operations', {})
        if 'rollback' in operations:
            rollback = operations['rollback']
            if rollback.get('enabled') and 'history_depth' not in rollback:
                errors.append("rollback.enabled=true requires history_depth")

        if errors:
            print(f"[FAIL] {example_file}:")
            for error in errors:
                print(f"    - {error}")
            all_valid = False
        else:
            print(f"[PASS] {example_file}: All cross-field dependencies satisfied")

    print()

    # Namespace uniqueness check
    print("4. Namespace Uniqueness Check")
    print("-" * 70)

    namespaces = {}
    for example_file in example_files:
        example_path = examples_dir / example_file
        instance = load_json(example_path)

        if instance is None:
            continue

        catalogue = instance['catalogue']
        namespace = f"{catalogue['vertical']}.{catalogue['field']}.{catalogue['object']}"

        if namespace in namespaces:
            print(f"[FAIL] Duplicate namespace: {namespace}")
            print(f"    - {namespaces[namespace]}")
            print(f"    - {example_file}")
            all_valid = False
        else:
            namespaces[namespace] = example_file

    if len(namespaces) == len(example_files):
        print(f"[PASS] All {len(namespaces)} namespaces are unique")
        for ns, file in namespaces.items():
            print(f"    - {ns} ({file})")

    print()
    print("=" * 70)

    if all_valid:
        print("[PASS] All validations passed!")
        print("=" * 70)
        return 0
    else:
        print("[FAIL] Some validations failed")
        print("=" * 70)
        return 1

if __name__ == "__main__":
    sys.exit(main())
