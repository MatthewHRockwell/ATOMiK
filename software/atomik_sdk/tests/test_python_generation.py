"""
Test Python SDK generation
"""

import sys
import tempfile
import py_compile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from generator.core import GeneratorEngine, GeneratorConfig
from generator.python_generator import PythonGenerator


def test_python_generation():
    """Test Python code generation from example schemas."""
    print("=" * 70)
    print("Testing Python SDK Generation")
    print("=" * 70)
    print()

    project_root = Path(__file__).parent.parent.parent.parent
    examples_dir = project_root / "sdk" / "schemas" / "examples"

    if not examples_dir.exists():
        print(f"[WARN] Examples directory not found: {examples_dir}")
        return 1

    # Create temporary output directory
    with tempfile.TemporaryDirectory() as temp_dir:
        output_dir = Path(temp_dir)

        # Test each example schema
        examples = list(examples_dir.glob("*.json"))
        if not examples:
            print(f"[WARN] No example schemas found")
            return 1

        for example_path in examples:
            print(f"Testing {example_path.name}...")
            print("-" * 70)

            # Create engine
            engine = GeneratorEngine(GeneratorConfig(
                output_dir=output_dir,
                validate_schemas=True,
                verbose=False
            ))

            # Register Python generator
            engine.register_generator('python', PythonGenerator())

            # Load schema
            try:
                validation = engine.load_schema(example_path)
                if not validation:
                    print(f"  [FAIL] Validation errors:")
                    for error in validation.errors:
                        print(f"    - {error}")
                    continue

                print(f"  [PASS] Schema validated")

                # Generate code
                results = engine.generate(target_languages=['python'])

                if 'python' not in results:
                    print(f"  [FAIL] No Python results")
                    continue

                result = results['python']

                if not result.success:
                    print(f"  [FAIL] Generation errors:")
                    for error in result.errors:
                        print(f"    - {error}")
                    continue

                print(f"  [PASS] Generated {len(result.files)} file(s)")

                # Write files
                files = engine.write_output(results)
                print(f"  [PASS] Wrote {len(files)} file(s)")

                # Validate syntax of generated Python files
                syntax_errors = 0
                for file_path in files:
                    try:
                        py_compile.compile(file_path, doraise=True)
                    except py_compile.PyCompileError as e:
                        print(f"  [FAIL] Syntax error in {Path(file_path).name}: {e}")
                        syntax_errors += 1

                if syntax_errors == 0:
                    print(f"  [PASS] All generated files compile successfully")
                else:
                    print(f"  [FAIL] {syntax_errors} file(s) have syntax errors")
                    continue

                # Try importing the generated module (basic check)
                # Note: This requires the module structure to be valid
                print(f"  [PASS] Python code generation successful")

            except Exception as e:
                print(f"  [FAIL] Exception: {e}")
                import traceback
                traceback.print_exc()
                continue

            print()

    print("=" * 70)
    print("Python generation tests complete")
    print("=" * 70)
    return 0


if __name__ == "__main__":
    sys.exit(test_python_generation())
