"""
Test domain SDK generation (Phase 4B)

Validates that all domain schemas in sdk/schemas/domains/ can be
loaded, validated, and used to generate code across all 5 target
languages (Python, Rust, C, JavaScript, Verilog).
"""

import sys
import tempfile
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from generator.c_generator import CGenerator
from generator.core import GeneratorConfig, GeneratorEngine
from generator.javascript_generator import JavaScriptGenerator
from generator.python_generator import PythonGenerator
from generator.rust_generator import RustGenerator
from generator.verilog_generator import VerilogGenerator

GENERATORS = {
    "python": PythonGenerator,
    "rust": RustGenerator,
    "c": CGenerator,
    "javascript": JavaScriptGenerator,
    "verilog": VerilogGenerator,
}


def test_domain_schema_validation():
    """All domain schemas pass validation."""
    project_root = Path(__file__).parent.parent.parent.parent
    domains_dir = project_root / "sdk" / "schemas" / "domains"

    if not domains_dir.exists():
        pytest.skip(f"Domains directory not found: {domains_dir}")

    schemas = list(domains_dir.glob("*.json"))
    if not schemas:
        pytest.skip("No domain schemas found")

    errors = []
    for schema_path in schemas:
        engine = GeneratorEngine(GeneratorConfig(validate_schemas=True, verbose=False))
        result = engine.load_schema(schema_path)
        if not result:
            errors.append(f"{schema_path.name}: {result.errors}")

    assert not errors, "Validation failures:\n" + "\n".join(str(e) for e in errors)


def test_domain_generation_all_languages():
    """All domain schemas generate code for all 5 languages."""
    project_root = Path(__file__).parent.parent.parent.parent
    domains_dir = project_root / "sdk" / "schemas" / "domains"

    if not domains_dir.exists():
        pytest.skip(f"Domains directory not found: {domains_dir}")

    schemas = list(domains_dir.glob("*.json"))
    if not schemas:
        pytest.skip("No domain schemas found")

    with tempfile.TemporaryDirectory() as temp_dir:
        output_dir = Path(temp_dir)
        errors = []

        for schema_path in schemas:
            engine = GeneratorEngine(GeneratorConfig(
                output_dir=output_dir,
                validate_schemas=True,
                verbose=False,
            ))

            for lang, gen_cls in GENERATORS.items():
                engine.register_generator(lang, gen_cls())

            validation = engine.load_schema(schema_path)
            if not validation:
                errors.append(f"{schema_path.name}: validation failed: {validation.errors}")
                continue

            results = engine.generate()
            for lang, result in results.items():
                if not result.success:
                    errors.append(
                        f"{schema_path.name}/{lang}: generation failed: {result.errors}"
                    )

            files = engine.write_output(results)
            if not files:
                errors.append(f"{schema_path.name}: no files written")

        assert not errors, "Generation failures:\n" + "\n".join(errors)


def test_domain_namespace_mapping():
    """Domain schemas produce correct namespace mappings."""
    project_root = Path(__file__).parent.parent.parent.parent
    domains_dir = project_root / "sdk" / "schemas" / "domains"

    if not domains_dir.exists():
        pytest.skip(f"Domains directory not found: {domains_dir}")

    schemas = list(domains_dir.glob("*.json"))
    if not schemas:
        pytest.skip("No domain schemas found")

    expected_namespaces = {
        "video-h264-delta.json": ("Video", "Streaming", "H264Delta"),
        "edge-sensor-imu.json": ("Edge", "Sensor", "IMUFusion"),
        "finance-price-tick.json": ("Finance", "Trading", "PriceTick"),
    }

    for schema_path in schemas:
        engine = GeneratorEngine(GeneratorConfig(validate_schemas=True, verbose=False))
        engine.load_schema(schema_path)
        ns = engine.extract_metadata()

        if schema_path.name in expected_namespaces:
            exp_v, exp_f, exp_o = expected_namespaces[schema_path.name]
            assert ns.vertical == exp_v, (
                f"{schema_path.name}: vertical {ns.vertical} != {exp_v}"
            )
            assert ns.field == exp_f, (
                f"{schema_path.name}: field {ns.field} != {exp_f}"
            )
            assert ns.object == exp_o, (
                f"{schema_path.name}: object {ns.object} != {exp_o}"
            )


def main():
    """Run domain generation tests."""
    print("Running domain generation tests...")
    try:
        test_domain_schema_validation()
        print("  PASS: Schema validation")
    except Exception as e:
        print(f"  FAIL: Schema validation - {e}")

    try:
        test_domain_generation_all_languages()
        print("  PASS: All-language generation")
    except Exception as e:
        print(f"  FAIL: All-language generation - {e}")

    try:
        test_domain_namespace_mapping()
        print("  PASS: Namespace mapping")
    except Exception as e:
        print(f"  FAIL: Namespace mapping - {e}")


if __name__ == "__main__":
    main()
