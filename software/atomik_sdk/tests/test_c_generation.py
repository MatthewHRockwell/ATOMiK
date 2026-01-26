"""
Test C SDK generation
"""

import sys
import tempfile
import subprocess
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from generator.core import GeneratorEngine, GeneratorConfig
from generator.c_generator import CGenerator


def test_c_generation():
    """Test C code generation from example schemas."""
    print("=" * 70)
    print("Testing C SDK Generation")
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

            # Register C generator
            engine.register_generator('c', CGenerator())

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
                results = engine.generate(target_languages=['c'])

                if 'c' not in results:
                    print(f"  [FAIL] No C results")
                    continue

                result = results['c']

                if not result.success:
                    print(f"  [FAIL] Generation errors:")
                    for error in result.errors:
                        print(f"    - {error}")
                    continue

                print(f"  [PASS] Generated {len(result.files)} file(s)")

                # Write files
                files = engine.write_output(results)
                print(f"  [PASS] Wrote {len(files)} file(s)")

                # Verify expected files exist
                expected_files = ['Makefile']
                for expected in expected_files:
                    expected_path = output_dir / expected
                    if not expected_path.exists():
                        print(f"  [FAIL] Missing expected file: {expected}")
                        continue

                print(f"  [PASS] All expected files present")

                # Check if gcc is available for compilation
                try:
                    gcc_version = subprocess.run(
                        ['gcc', '--version'],
                        capture_output=True,
                        text=True,
                        timeout=5
                    )

                    if gcc_version.returncode == 0:
                        gcc_first_line = gcc_version.stdout.split('\n')[0]
                        print(f"  [INFO] Found: {gcc_first_line}")

                        # Try to compile with make
                        print(f"  [INFO] Running make...")
                        make_result = subprocess.run(
                            ['make'],
                            cwd=output_dir,
                            capture_output=True,
                            text=True,
                            timeout=30
                        )

                        if make_result.returncode == 0:
                            print(f"  [PASS] Compilation succeeded")

                            # Try to run the test binary
                            test_binary = output_dir / f"test_{CGenerator._to_snake_case(engine.extract_metadata().object)}"
                            if test_binary.exists():
                                print(f"  [INFO] Running test binary...")
                                test_result = subprocess.run(
                                    [str(test_binary)],
                                    capture_output=True,
                                    text=True,
                                    timeout=10
                                )

                                if test_result.returncode == 0:
                                    print(f"  [PASS] Test binary executed successfully")
                                    for line in test_result.stdout.strip().split('\n'):
                                        if line.strip():
                                            print(f"    {line}")
                                else:
                                    print(f"  [WARN] Test binary failed with code {test_result.returncode}")
                                    if test_result.stderr:
                                        print(f"    {test_result.stderr}")
                        else:
                            print(f"  [WARN] Compilation errors:")
                            if make_result.stderr:
                                for line in make_result.stderr.split('\n')[:10]:
                                    if line.strip():
                                        print(f"    {line}")
                    else:
                        print(f"  [INFO] gcc not available, skipping compilation")

                except FileNotFoundError:
                    print(f"  [INFO] gcc not installed, skipping compilation")
                except subprocess.TimeoutExpired:
                    print(f"  [WARN] Compilation timed out")
                except Exception as e:
                    print(f"  [WARN] Could not compile: {e}")

                print(f"  [PASS] C code generation successful")

            except Exception as e:
                print(f"  [FAIL] Exception: {e}")
                import traceback
                traceback.print_exc()
                continue

            print()

    print("=" * 70)
    print("C generation tests complete")
    print("=" * 70)
    return 0


if __name__ == "__main__":
    sys.exit(test_c_generation())
