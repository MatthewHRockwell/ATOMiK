"""
Simple tests for ATOMiK SDK Generator Framework (no pytest required)
"""

import sys
from pathlib import Path

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from generator.core import GeneratorConfig, GeneratorEngine
from generator.namespace_mapper import NamespaceMapper
from generator.schema_validator import SchemaValidator


def test_schema_validator():
    """Test schema validation."""
    print("Testing SchemaValidator...")

    validator = SchemaValidator()

    # Test valid schema
    valid_schema = {
        "catalogue": {
            "vertical": "System",
            "field": "Test",
            "object": "TestModule",
            "version": "1.0.0"
        },
        "schema": {
            "delta_fields": {
                "test_delta": {
                    "type": "delta_stream",
                    "width": 64
                }
            },
            "operations": {
                "accumulate": {
                    "enabled": True
                }
            }
        }
    }

    result = validator.validate(valid_schema)
    assert result.valid, f"Valid schema failed validation: {result.errors}"
    print("  [PASS] Valid schema passes")

    # Test invalid schema (missing catalogue)
    invalid_schema = {"schema": {}}
    result = validator.validate(invalid_schema)
    assert not result.valid, "Invalid schema passed validation"
    print("  [PASS] Invalid schema correctly rejected")

    print("SchemaValidator: PASS\n")


def test_namespace_mapper():
    """Test namespace mapping."""
    print("Testing NamespaceMapper...")

    mapper = NamespaceMapper()

    catalogue = {
        "vertical": "Video",
        "field": "Stream",
        "object": "H264Delta"
    }

    mapping = mapper.map_catalogue(catalogue)

    assert mapping.vertical == "Video"
    assert mapping.python_import_statement == "from atomik.Video.Stream import H264Delta"
    assert "video::stream" in mapping.rust_path.lower()
    assert "h264_delta.h" in mapping.c_include_path
    assert mapping.verilog_module_name == "atomik_video_stream_h264_delta"

    print("  [PASS] Python namespace correct")
    print("  [PASS] Rust namespace correct")
    print("  [PASS] C namespace correct")
    print("  [PASS] Verilog namespace correct")

    # Test snake_case conversion
    result1 = NamespaceMapper._to_snake_case("TerminalIO")
    result2 = NamespaceMapper._to_snake_case("H264Delta")
    assert result1 == "terminal_io", f"Expected 'terminal_io', got '{result1}'"
    assert result2 == "h264_delta", f"Expected 'h264_delta', got '{result2}'"
    print("  [PASS] Snake case conversion correct")

    # Test identifier validation
    valid, _ = NamespaceMapper.validate_identifier("TerminalIO")
    assert valid, "Valid identifier rejected"
    print("  [PASS] Identifier validation correct")

    print("NamespaceMapper: PASS\n")


def test_generator_engine():
    """Test generator engine."""
    print("Testing GeneratorEngine...")

    # Get project root and test schema path
    project_root = Path(__file__).parent.parent.parent.parent
    schema_path = project_root / "sdk" / "schemas" / "examples" / "terminal-io.json"

    if not schema_path.exists():
        print(f"  [WARN] Schema file not found: {schema_path}")
        print("  Skipping GeneratorEngine test")
        return

    engine = GeneratorEngine(GeneratorConfig(verbose=False))

    # Test loading schema
    result = engine.load_schema(schema_path)
    assert result.valid, f"Schema validation failed: {result.errors}"
    print("  [PASS] Schema loaded successfully")

    # Test metadata extraction
    namespace = engine.extract_metadata()
    assert namespace is not None
    assert namespace.vertical == "System"
    assert namespace.field == "Terminal"
    assert namespace.object == "TerminalIO"
    print("  [PASS] Metadata extracted correctly")

    # Test schema summary
    summary = engine.get_schema_summary()
    assert 'catalogue' in summary
    assert 'delta_fields' in summary
    assert summary['catalogue']['vertical'] == 'System'
    print("  [PASS] Schema summary generated")

    print("GeneratorEngine: PASS\n")


def test_example_schemas():
    """Test all example schemas validate."""
    print("Testing example schemas...")

    project_root = Path(__file__).parent.parent.parent.parent
    examples_dir = project_root / "sdk" / "schemas" / "examples"

    if not examples_dir.exists():
        print(f"  [WARN] Examples directory not found: {examples_dir}")
        return

    validator = SchemaValidator()

    examples = list(examples_dir.glob("*.json"))
    if not examples:
        print(f"  [WARN] No example schemas found in {examples_dir}")
        return

    for example_path in examples:
        result = validator.validate_file(example_path)
        if result.valid:
            print(f"  [PASS] {example_path.name} validates")
        else:
            print(f"  [FAIL] {example_path.name} FAILED:")
            for error in result.errors:
                print(f"      {error}")
            raise AssertionError(f"{example_path.name} failed validation")

    print("Example schemas: PASS\n")


def main():
    """Run all tests."""
    print("=" * 70)
    print("ATOMiK SDK Generator Framework Tests")
    print("=" * 70)
    print()

    try:
        test_schema_validator()
        test_namespace_mapper()
        test_generator_engine()
        test_example_schemas()

        print("=" * 70)
        print("ALL TESTS PASSED")
        print("=" * 70)
        return 0

    except AssertionError as e:
        print()
        print("=" * 70)
        print(f"TEST FAILED: {e}")
        print("=" * 70)
        return 1
    except Exception as e:
        print()
        print("=" * 70)
        print(f"ERROR: {e}")
        import traceback
        traceback.print_exc()
        print("=" * 70)
        return 1


if __name__ == "__main__":
    sys.exit(main())
