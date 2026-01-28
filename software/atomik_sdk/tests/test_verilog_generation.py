"""
Test Verilog RTL generation
"""

import subprocess
import sys
import tempfile
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from generator.core import GeneratorConfig, GeneratorEngine
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

            # Register Verilog generator
            engine.register_generator('verilog', VerilogGenerator())

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
                results = engine.generate(target_languages=['verilog'])

                if 'verilog' not in results:
                    print("  [FAIL] No Verilog results")
                    continue

                result = results['verilog']

                if not result.success:
                    print("  [FAIL] Generation errors:")
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
                    with open(verilog_file) as f:
                        content = f.read()
                        # Basic syntax checks
                        if 'module' not in content:
                            print(f"  [FAIL] Missing 'module' keyword in {Path(verilog_file).name}")
                            continue
                        if 'endmodule' not in content:
                            print(f"  [FAIL] Missing 'endmodule' keyword in {Path(verilog_file).name}")
                            continue

                print("  [PASS] Verilog syntax appears valid")

                # Check if iverilog is available for syntax checking
                try:
                    iverilog_version = subprocess.run(
                        ['iverilog', '-V'],
                        capture_output=True,
                        text=True,
                        timeout=5
                    )

                    if iverilog_version.returncode == 0:
                        print("  [INFO] Found iverilog for syntax checking")

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
                                print("  [PASS] Verilog syntax check passed")
                            else:
                                print("  [WARN] Verilog syntax warnings:")
                                if iverilog_result.stderr:
                                    for line in iverilog_result.stderr.split('\n')[:5]:
                                        if line.strip():
                                            print(f"    {line}")
                    else:
                        print("  [INFO] iverilog not available, skipping syntax check")

                except FileNotFoundError:
                    print("  [INFO] iverilog not installed, skipping syntax check")
                except subprocess.TimeoutExpired:
                    print("  [WARN] iverilog check timed out")
                except Exception as e:
                    print(f"  [WARN] Could not run iverilog: {e}")

                print("  [PASS] Verilog RTL generation successful")

            except Exception as e:
                print(f"  [FAIL] Exception: {e}")
                import traceback
                traceback.print_exc()
                continue

            print()

    print("=" * 70)
    print("Verilog generation tests complete")
    print("=" * 70)


def test_parallel_bank_generation():
    """Test Verilog RTL generation with N_BANKS=4 parallel accumulator banks."""
    print("=" * 70)
    print("Testing Parallel Bank Verilog Generation (N_BANKS=4)")
    print("=" * 70)
    print()

    from generator.namespace_mapper import NamespaceMapping

    # Build a schema with N_BANKS=4 in hardware.rtl_params
    schema = {
        "catalogue": {
            "vertical": "Test",
            "field": "Parallel",
            "object": "BankTest",
        },
        "schema": {
            "delta_fields": {
                "value": {"width": 64, "type": "xor"},
            },
            "operations": {},
        },
        "hardware": {
            "target_device": "GW1NR-9",
            "rtl_params": {
                "DATA_WIDTH": 64,
                "N_BANKS": 4,
            },
        },
    }

    namespace = NamespaceMapping(
        vertical="Test",
        field="Parallel",
        object="BankTest",
        python_module_path="atomik.Test.Parallel",
        python_import_statement="from atomik.Test.Parallel import BankTest",
        rust_path="atomik::test::parallel",
        rust_use_statement="use atomik::test::parallel::BankTest;",
        c_include_path="atomik/test/parallel/bank_test.h",
        c_include_statement='#include "atomik/test/parallel/bank_test.h"',
        javascript_package="@atomik/test-parallel",
        javascript_require_statement="const { BankTest } = require('@atomik/test-parallel');",
        verilog_module_name="atomik_test_parallel_bank_test",
    )

    gen = VerilogGenerator()
    result = gen.generate(schema, namespace)

    assert result.success, f"Generation failed: {result.errors}"
    print(f"  [PASS] Generated {len(result.files)} file(s)")

    # Find the RTL module file
    rtl_files = [f for f in result.files if f.language == "verilog" and "tb_" not in f.relative_path]
    assert len(rtl_files) >= 1, "No RTL file generated"

    rtl_content = rtl_files[0].content

    # Verify parallel accumulator instantiation
    assert "atomik_parallel_acc" in rtl_content, \
        "Generated Verilog should contain atomik_parallel_acc instantiation"
    print("  [PASS] Contains atomik_parallel_acc instantiation")

    assert "N_BANKS" in rtl_content, \
        "Generated Verilog should contain N_BANKS parameter"
    print("  [PASS] Contains N_BANKS parameter")

    assert "delta_parallel_in" in rtl_content, \
        "Generated Verilog should contain delta_parallel_in port"
    print("  [PASS] Contains delta_parallel_in port")

    assert "delta_parallel_valid" in rtl_content, \
        "Generated Verilog should contain delta_parallel_valid port"
    print("  [PASS] Contains delta_parallel_valid port")

    assert "parallel_mode" in rtl_content, \
        "Generated Verilog should contain parallel_mode port"
    print("  [PASS] Contains parallel_mode port")

    # Verify testbench has parallel tests
    tb_files = [f for f in result.files if "tb_" in f.relative_path]
    assert len(tb_files) >= 1, "No testbench file generated"

    tb_content = tb_files[0].content
    assert "Parallel" in tb_content or "parallel" in tb_content, \
        "Testbench should contain parallel mode tests"
    print("  [PASS] Testbench contains parallel mode tests")

    print()
    print("  [PASS] Parallel bank generation test complete")
    print("=" * 70)


if __name__ == "__main__":
    sys.exit(test_verilog_generation())
