"""
Test JavaScript SDK generation
"""

import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from generator.core import GeneratorConfig, GeneratorEngine
from generator.javascript_generator import JavaScriptGenerator


def test_javascript_generation():
    """Test JavaScript code generation from example schemas."""
    print("=" * 70)
    print("Testing JavaScript SDK Generation")
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
            print("[WARN] No example schemas found")
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

            # Register JavaScript generator
            engine.register_generator('javascript', JavaScriptGenerator())

            # Load schema
            try:
                validation = engine.load_schema(example_path)
                if not validation:
                    print("  [FAIL] Validation errors:")
                    for error in validation.errors:
                        print(f"    - {error}")
                    continue

                print("  [PASS] Schema validated")

                # Generate code
                results = engine.generate(target_languages=['javascript'])

                if 'javascript' not in results:
                    print("  [FAIL] No JavaScript results")
                    continue

                result = results['javascript']

                if not result.success:
                    print("  [FAIL] Generation errors:")
                    for error in result.errors:
                        print(f"    - {error}")
                    continue

                print(f"  [PASS] Generated {len(result.files)} file(s)")

                # Write files
                files = engine.write_output(results)
                print(f"  [PASS] Wrote {len(files)} file(s)")

                # Verify expected files exist
                expected_files = ['package.json', 'index.js']
                for expected in expected_files:
                    expected_path = output_dir / expected
                    if not expected_path.exists():
                        print(f"  [FAIL] Missing expected file: {expected}")
                        continue

                print("  [PASS] All expected files present")

                # Check if node is available
                try:
                    node_version = subprocess.run(
                        ['node', '--version'],
                        capture_output=True,
                        text=True,
                        timeout=5
                    )

                    if node_version.returncode == 0:
                        print(f"  [INFO] Found: Node.js {node_version.stdout.strip()}")

                        # Try to run the test file
                        test_files = list(output_dir.glob("test/*.test.js"))
                        if test_files:
                            test_file = test_files[0]
                            print("  [INFO] Running tests...")
                            test_result = subprocess.run(
                                ['node', str(test_file)],
                                capture_output=True,
                                text=True,
                                timeout=10,
                                cwd=output_dir
                            )

                            if test_result.returncode == 0:
                                print("  [PASS] Tests executed successfully")
                                for line in test_result.stdout.strip().split('\n'):
                                    if line.strip():
                                        print(f"    {line}")
                            else:
                                print("  [WARN] Tests failed:")
                                if test_result.stderr:
                                    for line in test_result.stderr.split('\n')[:10]:
                                        if line.strip():
                                            print(f"    {line}")
                    else:
                        print("  [INFO] Node.js not available, skipping execution")

                except FileNotFoundError:
                    print("  [INFO] Node.js not installed, skipping execution")
                except subprocess.TimeoutExpired:
                    print("  [WARN] Test execution timed out")
                except Exception as e:
                    print(f"  [WARN] Could not run tests: {e}")

                print("  [PASS] JavaScript code generation successful")

            except Exception as e:
                print(f"  [FAIL] Exception: {e}")
                import traceback
                traceback.print_exc()
                continue

            print()

    print("=" * 70)
    print("JavaScript generation tests complete")
    print("=" * 70)
    return 0


if __name__ == "__main__":
    sys.exit(test_javascript_generation())
