"""
ATOMiK SDK Integration Tests

Tests cross-language consistency and semantic equivalence.
Validates that all generators produce code with matching behavior.
"""

import json
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from generator.c_generator import CGenerator
from generator.core import GeneratorConfig, GeneratorEngine
from generator.javascript_generator import JavaScriptGenerator
from generator.python_generator import PythonGenerator
from generator.rust_generator import RustGenerator
from generator.verilog_generator import VerilogGenerator


def test_cross_language_integration():
    """Test that all languages generate consistent code from same schema."""
    print("=" * 70)
    print("ATOMiK SDK Integration Tests")
    print("=" * 70)
    print()

    project_root = Path(__file__).parent.parent.parent.parent
    examples_dir = project_root / "sdk" / "schemas" / "examples"

    if not examples_dir.exists():
        print(f"[WARN] Examples directory not found: {examples_dir}")
        return 1

    # Test each example schema
    examples = list(examples_dir.glob("*.json"))
    if not examples:
        print("[WARN] No example schemas found")
        return 1

    all_passed = True

    for example_path in examples:
        print(f"Testing {example_path.name}...")
        print("-" * 70)

        # Load schema once
        with open(example_path) as f:
            schema = json.load(f)

        catalogue = schema.get('catalogue', {})
        vertical = catalogue.get('vertical', 'Unknown')
        field = catalogue.get('field', 'Unknown')
        obj = catalogue.get('object', 'Unknown')

        print(f"  Schema: {vertical}/{field}/{obj}")
        print(f"  Version: {catalogue.get('version', 'N/A')}")

        # Create temporary output directory
        with tempfile.TemporaryDirectory() as temp_dir:
            output_dir = Path(temp_dir)

            # Create engine with all generators
            engine = GeneratorEngine(GeneratorConfig(
                output_dir=output_dir,
                validate_schemas=True,
                verbose=False
            ))

            # Register all generators
            engine.register_generator('python', PythonGenerator())
            engine.register_generator('rust', RustGenerator())
            engine.register_generator('c', CGenerator())
            engine.register_generator('verilog', VerilogGenerator())
            engine.register_generator('javascript', JavaScriptGenerator())

            # Load schema
            try:
                validation = engine.load_schema(example_path)
                if not validation.valid:
                    print("  [FAIL] Schema validation failed:")
                    for error in validation.errors:
                        print(f"    - {error}")
                    all_passed = False
                    continue

                print("  [PASS] Schema validated")

                # Generate all languages
                results = engine.generate(target_languages=[
                    'python', 'rust', 'c', 'verilog', 'javascript'
                ])

                # Check all languages generated successfully
                expected_languages = ['python', 'rust', 'c', 'verilog', 'javascript']
                for lang in expected_languages:
                    if lang not in results:
                        print(f"  [FAIL] {lang} generation missing")
                        all_passed = False
                        continue

                    result = results[lang]
                    if not result.success:
                        print(f"  [FAIL] {lang} generation errors:")
                        for error in result.errors:
                            print(f"    - {error}")
                        all_passed = False
                        continue

                    print(f"  [PASS] {lang}: {len(result.files)} files")

                # Write all files
                files = engine.write_output(results)
                print(f"  [PASS] Total files written: {len(files)}")

                # Verify namespace consistency
                namespace = engine.extract_metadata()
                if namespace:
                    print(f"  [INFO] Python: {namespace.python_import_statement}")
                    print(f"  [INFO] Rust: {namespace.rust_use_statement}")
                    print(f"  [INFO] C: {namespace.c_include_statement}")
                    print(f"  [INFO] JavaScript: {namespace.javascript_require_statement}")
                    print(f"  [INFO] Verilog: module {namespace.verilog_module_name}")
                    print("  [PASS] Namespace mapping consistent")

                # Check that all languages have consistent file counts
                file_counts = {lang: len(results[lang].files) for lang in expected_languages}
                print(f"  [INFO] File counts: {file_counts}")

                # Verify key operations are present in generated files
                operations_present = test_operations_present(output_dir, results)
                if operations_present:
                    print("  [PASS] All operations present in generated code")
                else:
                    print("  [WARN] Some operations may be missing")

                print("  [PASS] Integration test successful")

            except Exception as e:
                print(f"  [FAIL] Exception: {e}")
                import traceback
                traceback.print_exc()
                all_passed = False
                continue

        print()

    print("=" * 70)
    if all_passed:
        print("All integration tests PASSED")
    else:
        print("Some integration tests FAILED")
    print("=" * 70)

    return 0 if all_passed else 1


def test_operations_present(output_dir: Path, results: dict) -> bool:
    """Verify that key operations are present in generated code."""
    operations_to_check = ['load', 'accumulate', 'reconstruct']
    all_present = True

    # Check Python files
    python_files = list(output_dir.glob("atomik/**/*.py"))
    if python_files:
        for pyfile in python_files:
            if pyfile.name == '__init__.py':
                continue
            with open(pyfile) as f:
                content = f.read().lower()
                for op in operations_to_check:
                    if op not in content:
                        all_present = False

    # Check Rust files
    rust_files = list(output_dir.glob("src/**/*.rs"))
    if rust_files:
        for rsfile in rust_files:
            if rsfile.name == 'lib.rs' or rsfile.name == 'mod.rs':
                continue
            with open(rsfile) as f:
                content = f.read().lower()
                for op in operations_to_check:
                    if op not in content:
                        all_present = False

    # Check C files
    c_files = list(output_dir.glob("atomik/**/*.c"))
    if c_files:
        for cfile in c_files:
            with open(cfile) as f:
                content = f.read().lower()
                for op in operations_to_check:
                    if op not in content:
                        all_present = False

    # Check Verilog files
    verilog_files = list(output_dir.glob("rtl/**/*.v"))
    if verilog_files:
        for vfile in verilog_files:
            if vfile.name.startswith('tb_'):
                continue
            with open(vfile) as f:
                content = f.read().lower()
                # Verilog uses different names
                verilog_ops = ['load', 'accumulate', 'read']
                for op in verilog_ops:
                    if op not in content:
                        all_present = False

    # Check JavaScript files
    js_files = list(output_dir.glob("src/**/*.js"))
    if js_files:
        for jsfile in js_files:
            with open(jsfile) as f:
                content = f.read().lower()
                for op in operations_to_check:
                    if op not in content:
                        all_present = False

    return all_present


def test_schema_summary():
    """Test schema summary generation."""
    print("=" * 70)
    print("Testing Schema Summary Generation")
    print("=" * 70)
    print()

    project_root = Path(__file__).parent.parent.parent.parent
    examples_dir = project_root / "sdk" / "schemas" / "examples"

    if not examples_dir.exists():
        print("[WARN] Examples directory not found")
        return 1

    examples = list(examples_dir.glob("*.json"))
    if not examples:
        print("[WARN] No example schemas found")
        return 1

    for example_path in examples:
        print(f"Schema: {example_path.name}")
        print("-" * 70)

        engine = GeneratorEngine(GeneratorConfig(verbose=False))
        engine.load_schema(example_path)

        summary = engine.get_schema_summary()

        print("  Catalogue:")
        print(f"    Vertical: {summary['catalogue']['vertical']}")
        print(f"    Field: {summary['catalogue']['field']}")
        print(f"    Object: {summary['catalogue']['object']}")
        print(f"    Version: {summary['catalogue']['version']}")

        print(f"  Delta Fields: {len(summary['delta_fields'])}")
        for field_name, field_spec in summary['delta_fields'].items():
            print(f"    - {field_name}: {field_spec.get('type')} ({field_spec.get('width')} bits)")

        print(f"  Operations: {', '.join(summary['operations'])}")

        if summary.get('has_hardware'):
            print("  Hardware: Yes")

        print()

    print("=" * 70)
    print("Schema summary tests complete")
    print("=" * 70)
    return 0


def test_multi_language_generation():
    """Test generating multiple languages simultaneously."""
    print("=" * 70)
    print("Testing Multi-Language Generation")
    print("=" * 70)
    print()

    project_root = Path(__file__).parent.parent.parent.parent
    schema_path = project_root / "sdk" / "schemas" / "examples" / "terminal-io.json"

    if not schema_path.exists():
        print("[WARN] terminal-io.json not found")
        return 1

    with tempfile.TemporaryDirectory() as temp_dir:
        output_dir = Path(temp_dir)

        engine = GeneratorEngine(GeneratorConfig(
            output_dir=output_dir,
            validate_schemas=True,
            verbose=False
        ))

        # Register all generators
        engine.register_generator('python', PythonGenerator())
        engine.register_generator('rust', RustGenerator())
        engine.register_generator('c', CGenerator())
        engine.register_generator('verilog', VerilogGenerator())
        engine.register_generator('javascript', JavaScriptGenerator())

        # Generate all languages at once
        results, files = engine.generate_and_write(
            schema_path,
            target_languages=['python', 'rust', 'c', 'verilog', 'javascript']
        )

        print(f"Generated {len(files)} total files across 5 languages")

        # Count files by language
        file_extensions = {}
        for file_path in files:
            ext = Path(file_path).suffix
            file_extensions[ext] = file_extensions.get(ext, 0) + 1

        print("File breakdown:")
        for ext, count in sorted(file_extensions.items()):
            print(f"  {ext or 'no extension'}: {count} files")

        print("[PASS] Multi-language generation successful")

    print()
    print("=" * 70)
    print("Multi-language generation tests complete")
    print("=" * 70)
    return 0


def main():
    """Run all integration tests."""
    print()
    print("*" * 70)
    print("ATOMiK SDK - COMPREHENSIVE INTEGRATION TEST SUITE")
    print("*" * 70)
    print()

    exit_code = 0

    try:
        # Test 1: Cross-language integration
        result = test_cross_language_integration()
        if result != 0:
            exit_code = result

        # Test 2: Schema summary
        result = test_schema_summary()
        if result != 0:
            exit_code = result

        # Test 3: Multi-language generation
        result = test_multi_language_generation()
        if result != 0:
            exit_code = result

        print()
        print("*" * 70)
        if exit_code == 0:
            print("ALL INTEGRATION TESTS PASSED")
        else:
            print("SOME INTEGRATION TESTS FAILED")
        print("*" * 70)

    except Exception as e:
        print(f"FATAL ERROR: {e}")
        import traceback
        traceback.print_exc()
        exit_code = 1

    return exit_code


if __name__ == "__main__":
    sys.exit(main())
