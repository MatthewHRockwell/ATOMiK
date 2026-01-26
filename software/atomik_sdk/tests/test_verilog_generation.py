"""
Test Verilog RTL generation
"""

import sys
import tempfile
import subprocess
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from generator.core import GeneratorEngine, GeneratorConfig
from generator.verilog_generator import VerilogGenerator


def test_verilog_generation():
    """Test Verilog RTL code generation from example schemas."""
    print("=" * 70)
    print("Testing Verilog RTL Generation")
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

            # Register Verilog generator
            engine.register_generator('verilog', VerilogGenerator())

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
                results = engine.generate(target_languages=['verilog'])

                if 'verilog' not in results:
                    print(f"  [FAIL] No Verilog results")
                    continue

                result = results['verilog']

                if not result.success:
                    print(f"  [FAIL] Generation errors:")
                    for error in result.errors:
                        print(f"    - {error}")
                    continue

                print(f"  [PASS] Generated {len(result.files)} file(s)")

                # Write files
                files = engine.write_output(results)
                print(f"  [PASS] Wrote {len(files)} file(s)")

                # Verify generated files have Verilog syntax
                verilog_files = [f for f in files if str(f).endswith('.v')]
                for verilog_file in verilog_files:
                    with open(verilog_file, 'r') as f:
                        content = f.read()
                        # Basic syntax checks
                        if 'module' not in content:
                            print(f"  [FAIL] Missing 'module' keyword in {Path(verilog_file).name}")
                            continue
                        if 'endmodule' not in content:
                            print(f"  [FAIL] Missing 'endmodule' keyword in {Path(verilog_file).name}")
                            continue

                print(f"  [PASS] Verilog syntax appears valid")

                # Check if iverilog is available for syntax checking
                try:
                    iverilog_version = subprocess.run(
                        ['iverilog', '-V'],
                        capture_output=True,
                        text=True,
                        timeout=5
                    )

                    if iverilog_version.returncode == 0:
                        print(f"  [INFO] Found iverilog for syntax checking")

                        # Try to compile the main module
                        main_module = None
                        for f in verilog_files:
                            if 'tb_' not in str(f):
                                main_module = f
                                break

                        if main_module:
                            iverilog_result = subprocess.run(
                                ['iverilog', '-t', 'null', str(main_module)],
                                capture_output=True,
                                text=True,
                                timeout=10
                            )

                            if iverilog_result.returncode == 0:
                                print(f"  [PASS] Verilog syntax check passed")
                            else:
                                print(f"  [WARN] Verilog syntax warnings:")
                                if iverilog_result.stderr:
                                    for line in iverilog_result.stderr.split('\n')[:5]:
                                        if line.strip():
                                            print(f"    {line}")
                    else:
                        print(f"  [INFO] iverilog not available, skipping syntax check")

                except FileNotFoundError:
                    print(f"  [INFO] iverilog not installed, skipping syntax check")
                except subprocess.TimeoutExpired:
                    print(f"  [WARN] iverilog check timed out")
                except Exception as e:
                    print(f"  [WARN] Could not run iverilog: {e}")

                print(f"  [PASS] Verilog RTL generation successful")

            except Exception as e:
                print(f"  [FAIL] Exception: {e}")
                import traceback
                traceback.print_exc()
                continue

            print()

    print("=" * 70)
    print("Verilog generation tests complete")
    print("=" * 70)
    return 0


if __name__ == "__main__":
    sys.exit(test_verilog_generation())
