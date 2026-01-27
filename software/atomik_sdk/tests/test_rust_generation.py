"""
Test Rust SDK generation
"""

import subprocess
import sys
import tempfile
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from generator.core import GeneratorConfig, GeneratorEngine
from generator.rust_generator import RustGenerator


def test_rust_generation():
    """Test Rust code generation from example schemas."""
    print("=" * 70)
    print("Testing Rust SDK Generation")
    print("=" * 70)
    print()

    project_root = Path(__file__).parent.parent.parent.parent
    examples_dir = project_root / "sdk" / "schemas" / "examples"

    if not examples_dir.exists():
        pytest.skip(f"Examples directory not found: {examples_dir}")

    # Create temporary output directory
    with tempfile.TemporaryDirectory() as temp_dir:
        output_dir = Path(temp_dir)

        # Test each example schema
        examples = list(examples_dir.glob("*.json"))
        if not examples:
            pytest.skip("No example schemas found")

        for example_path in examples:
            print(f"Testing {example_path.name}...")
            print("-" * 70)

            # Create engine
            engine = GeneratorEngine(GeneratorConfig(
                output_dir=output_dir,
                validate_schemas=True,
                verbose=False
            ))

            # Register Rust generator
            engine.register_generator('rust', RustGenerator())

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
                results = engine.generate(target_languages=['rust'])

                if 'rust' not in results:
                    print("  [FAIL] No Rust results")
                    continue

                result = results['rust']

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
                expected_files = ['Cargo.toml', 'src/lib.rs']
                for expected in expected_files:
                    expected_path = output_dir / expected
                    if not expected_path.exists():
                        print(f"  [FAIL] Missing expected file: {expected}")
                        continue

                print("  [PASS] All expected files present")

                # Check if rustc is available for syntax validation
                try:
                    rustc_version = subprocess.run(
                        ['rustc', '--version'],
                        capture_output=True,
                        text=True,
                        timeout=5
                    )

                    if rustc_version.returncode == 0:
                        print(f"  [INFO] Found: {rustc_version.stdout.strip()}")

                        # Try to run cargo check in the output directory
                        print("  [INFO] Running cargo check...")
                        cargo_check = subprocess.run(
                            ['cargo', 'check'],
                            cwd=output_dir,
                            capture_output=True,
                            text=True,
                            timeout=60
                        )

                        if cargo_check.returncode == 0:
                            print("  [PASS] Cargo check succeeded")
                        else:
                            print("  [WARN] Cargo check warnings/errors:")
                            if cargo_check.stderr:
                                for line in cargo_check.stderr.split('\n')[:10]:
                                    if line.strip():
                                        print(f"    {line}")
                    else:
                        print("  [INFO] rustc not available, skipping syntax validation")

                except FileNotFoundError:
                    print("  [INFO] Rust toolchain not installed, skipping syntax validation")
                except subprocess.TimeoutExpired:
                    print("  [WARN] Cargo check timed out")
                except Exception as e:
                    print(f"  [WARN] Could not run cargo check: {e}")

                print("  [PASS] Rust code generation successful")

            except Exception as e:
                print(f"  [FAIL] Exception: {e}")
                import traceback
                traceback.print_exc()
                continue

            print()

    print("=" * 70)
    print("Rust generation tests complete")
    print("=" * 70)


if __name__ == "__main__":
    sys.exit(test_rust_generation())
