"""
Tests for ATOMiK SDK Generator Framework

Tests cover:
- Schema validation
- Namespace mapping
- Code emitter base classes
- Generator engine
"""

import json
import sys
from pathlib import Path

import pytest

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from generator.code_emitter import (
    CodeEmitter,
    GeneratedFile,
    GenerationResult,
    MultiLanguageEmitter,
)
from generator.core import GeneratorConfig, GeneratorEngine
from generator.namespace_mapper import NamespaceMapper
from generator.schema_validator import SchemaValidator

# Test fixtures

@pytest.fixture
def project_root():
    """Get project root directory."""
    return Path(__file__).parent.parent.parent.parent


@pytest.fixture
def schema_spec_path(project_root):
    """Path to atomik_schema_v1.json"""
    return project_root / "specs" / "atomik_schema_v1.json"


@pytest.fixture
def terminal_io_path(project_root):
    """Path to terminal-io.json example."""
    return project_root / "sdk" / "schemas" / "examples" / "terminal-io.json"


@pytest.fixture
def p2p_delta_path(project_root):
    """Path to p2p-delta.json example."""
    return project_root / "sdk" / "schemas" / "examples" / "p2p-delta.json"


@pytest.fixture
def valid_schema():
    """A valid minimal schema for testing."""
    return {
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


# Schema Validator Tests

class TestSchemaValidator:
    """Test schema validation functionality."""

    def test_validator_initialization(self, schema_spec_path):
        """Test validator can be initialized."""
        validator = SchemaValidator(schema_spec_path)
        assert validator.schema_spec is not None
        assert '$schema' in validator.schema_spec

    def test_validate_valid_schema(self, valid_schema):
        """Test validation of a valid schema."""
        validator = SchemaValidator()
        result = validator.validate(valid_schema)
        assert result.valid is True
        assert len(result.errors) == 0

    def test_validate_missing_catalogue(self):
        """Test validation fails without catalogue."""
        validator = SchemaValidator()
        invalid_schema = {"schema": {}}
        result = validator.validate(invalid_schema)
        assert result.valid is False
        assert any("catalogue" in error.lower() for error in result.errors)

    def test_validate_missing_delta_fields(self):
        """Test validation fails without delta_fields."""
        validator = SchemaValidator()
        invalid_schema = {
            "catalogue": {
                "vertical": "Test",
                "field": "Test",
                "object": "Test",
                "version": "1.0.0"
            },
            "schema": {
                "operations": {
                    "accumulate": {"enabled": True}
                }
            }
        }
        result = validator.validate(invalid_schema)
        assert result.valid is False
        assert any("delta_fields" in error for error in result.errors)

    def test_validate_accumulate_not_enabled(self):
        """Test validation fails if accumulate not enabled."""
        validator = SchemaValidator()
        invalid_schema = {
            "catalogue": {
                "vertical": "Test",
                "field": "Test",
                "object": "Test",
                "version": "1.0.0"
            },
            "schema": {
                "delta_fields": {
                    "test": {"type": "delta_stream", "width": 64}
                },
                "operations": {
                    "accumulate": {"enabled": False}
                }
            }
        }
        result = validator.validate(invalid_schema)
        assert result.valid is False

    def test_validate_file(self, terminal_io_path):
        """Test validation of actual schema file."""
        validator = SchemaValidator()
        result = validator.validate_file(terminal_io_path)
        assert result.valid is True

    def test_cross_field_validation_hardware_width_mismatch(self):
        """Test hardware DATA_WIDTH must match field widths."""
        validator = SchemaValidator()
        schema = {
            "catalogue": {
                "vertical": "Test",
                "field": "Test",
                "object": "Test",
                "version": "1.0.0"
            },
            "schema": {
                "delta_fields": {
                    "test": {"type": "delta_stream", "width": 64}
                },
                "operations": {
                    "accumulate": {"enabled": True}
                }
            },
            "hardware": {
                "rtl_params": {
                    "DATA_WIDTH": 32  # Mismatch!
                }
            }
        }
        result = validator.validate(schema)
        assert result.valid is False
        assert any("DATA_WIDTH" in error for error in result.errors)

    def test_cross_field_validation_rollback_needs_history(self):
        """Test rollback enabled requires history_depth."""
        validator = SchemaValidator()
        schema = {
            "catalogue": {
                "vertical": "Test",
                "field": "Test",
                "object": "Test",
                "version": "1.0.0"
            },
            "schema": {
                "delta_fields": {
                    "test": {"type": "delta_stream", "width": 64}
                },
                "operations": {
                    "accumulate": {"enabled": True},
                    "rollback": {"enabled": True}  # Missing history_depth
                }
            }
        }
        result = validator.validate(schema)
        assert result.valid is False
        assert any("history_depth" in error for error in result.errors)

    def test_namespace_uniqueness(self, terminal_io_path, p2p_delta_path):
        """Test namespace uniqueness validation."""
        validator = SchemaValidator()

        with open(terminal_io_path) as f:
            schema1 = json.load(f)
        with open(p2p_delta_path) as f:
            schema2 = json.load(f)

        all_unique, conflicts = validator.validate_namespace_uniqueness([schema1, schema2])
        assert all_unique is True
        assert len(conflicts) == 0


# Namespace Mapper Tests

class TestNamespaceMapper:
    """Test namespace mapping functionality."""

    def test_map_catalogue(self):
        """Test basic catalogue mapping."""
        mapper = NamespaceMapper()
        catalogue = {
            "vertical": "Video",
            "field": "Stream",
            "object": "H264Delta"
        }
        mapping = mapper.map_catalogue(catalogue)

        assert mapping.vertical == "Video"
        assert mapping.field == "Stream"
        assert mapping.object == "H264Delta"

    def test_python_namespace(self):
        """Test Python namespace generation."""
        mapper = NamespaceMapper()
        catalogue = {
            "vertical": "Video",
            "field": "Stream",
            "object": "H264Delta"
        }
        mapping = mapper.map_catalogue(catalogue)

        assert mapping.python_import_statement == "from atomik.Video.Stream import H264Delta"
        assert mapping.python_module_path == "atomik.Video.Stream"

    def test_rust_namespace(self):
        """Test Rust namespace generation."""
        mapper = NamespaceMapper()
        catalogue = {
            "vertical": "Video",
            "field": "Stream",
            "object": "H264Delta"
        }
        mapping = mapper.map_catalogue(catalogue)

        assert mapping.rust_use_statement == "use atomik::video::stream::H264Delta;"
        assert "video::stream" in mapping.rust_path

    def test_c_namespace(self):
        """Test C namespace generation."""
        mapper = NamespaceMapper()
        catalogue = {
            "vertical": "Video",
            "field": "Stream",
            "object": "H264Delta"
        }
        mapping = mapper.map_catalogue(catalogue)

        assert "#include <atomik/video/stream/h264_delta.h>" in mapping.c_include_statement
        assert "h264_delta.h" in mapping.c_include_path

    def test_javascript_namespace(self):
        """Test JavaScript namespace generation."""
        mapper = NamespaceMapper()
        catalogue = {
            "vertical": "Video",
            "field": "Stream",
            "object": "H264Delta"
        }
        mapping = mapper.map_catalogue(catalogue)

        assert "@atomik/video/stream" in mapping.javascript_package
        assert "H264Delta" in mapping.javascript_require_statement

    def test_verilog_namespace(self):
        """Test Verilog namespace generation."""
        mapper = NamespaceMapper()
        catalogue = {
            "vertical": "Video",
            "field": "Stream",
            "object": "H264Delta"
        }
        mapping = mapper.map_catalogue(catalogue)

        assert mapping.verilog_module_name == "atomik_video_stream_h264_delta"

    def test_snake_case_conversion(self):
        """Test PascalCase to snake_case conversion."""
        assert NamespaceMapper._to_snake_case("TerminalIO") == "terminal_io"
        assert NamespaceMapper._to_snake_case("H264Delta") == "h264_delta"
        assert NamespaceMapper._to_snake_case("Simple") == "simple"

    def test_validate_identifier_valid(self):
        """Test identifier validation for valid names."""
        valid, error = NamespaceMapper.validate_identifier("TerminalIO")
        assert valid is True
        assert error is None

        valid, error = NamespaceMapper.validate_identifier("H264Delta")
        assert valid is True

    def test_validate_identifier_lowercase_start(self):
        """Test identifier must start with uppercase."""
        valid, error = NamespaceMapper.validate_identifier("terminalIO")
        assert valid is False
        assert "uppercase" in error.lower()

    def test_validate_identifier_reserved_keyword(self):
        """Test reserved keyword detection."""
        valid, error = NamespaceMapper.validate_identifier("Class")
        assert valid is False
        assert "reserved" in error.lower()

    def test_directory_structure_generation(self):
        """Test directory structure generation."""
        mapper = NamespaceMapper()
        catalogue = {
            "vertical": "System",
            "field": "Terminal",
            "object": "TerminalIO"
        }
        mapping = mapper.map_catalogue(catalogue)
        dirs = mapper.generate_directory_structure(mapping)

        assert "python" in dirs
        assert "rust" in dirs
        assert "c" in dirs
        assert "System/Terminal" in dirs['python']


# Code Emitter Tests

class TestCodeEmitter:
    """Test code emitter base functionality."""

    class MockGenerator(CodeEmitter):
        """Mock generator for testing."""

        def __init__(self):
            super().__init__('mock')

        def generate(self, schema, namespace):
            files = [
                GeneratedFile(
                    relative_path=f"mock/{namespace.object.lower()}.mock",
                    content=f"// Mock code for {namespace.object}",
                    language="mock",
                    description="Mock generated file"
                )
            ]
            return GenerationResult(
                success=True,
                files=files,
                errors=[],
                warnings=[]
            )

    def test_generated_file_creation(self):
        """Test GeneratedFile data class."""
        file = GeneratedFile(
            relative_path="test.py",
            content="print('hello')",
            language="python",
            description="Test file"
        )
        assert file.relative_path == "test.py"
        assert file.content == "print('hello')"

    def test_generation_result_success(self):
        """Test GenerationResult success case."""
        result = GenerationResult(
            success=True,
            files=[],
            errors=[],
            warnings=[]
        )
        assert result.success is True
        assert bool(result) is True

    def test_generation_result_failure(self):
        """Test GenerationResult failure case."""
        result = GenerationResult(
            success=False,
            files=[],
            errors=["Test error"],
            warnings=[]
        )
        assert result.success is False
        assert bool(result) is False

    def test_mock_generator(self, valid_schema):
        """Test mock generator implementation."""
        generator = self.MockGenerator()
        mapper = NamespaceMapper()
        namespace = mapper.map_catalogue(valid_schema['catalogue'])

        result = generator.generate(valid_schema, namespace)

        assert result.success is True
        assert len(result.files) == 1
        assert result.files[0].language == "mock"

    def test_multi_language_emitter(self, valid_schema):
        """Test multi-language code emission."""
        emitter = MultiLanguageEmitter()
        generator1 = self.MockGenerator()
        generator2 = self.MockGenerator()

        emitter.register('lang1', generator1)
        emitter.register('lang2', generator2)

        mapper = NamespaceMapper()
        namespace = mapper.map_catalogue(valid_schema['catalogue'])

        results = emitter.generate_all(valid_schema, namespace)

        assert len(results) == 2
        assert 'lang1' in results
        assert 'lang2' in results
        assert results['lang1'].success is True
        assert results['lang2'].success is True


# Generator Engine Tests

class TestGeneratorEngine:
    """Test main generator engine."""

    def test_engine_initialization(self):
        """Test engine can be initialized."""
        engine = GeneratorEngine()
        assert engine.config is not None
        assert engine.validator is not None

    def test_load_schema(self, terminal_io_path):
        """Test schema loading."""
        engine = GeneratorEngine()
        result = engine.load_schema(terminal_io_path)
        assert result.valid is True
        assert engine.schema is not None

    def test_extract_metadata(self, terminal_io_path):
        """Test metadata extraction."""
        engine = GeneratorEngine()
        engine.load_schema(terminal_io_path)
        namespace = engine.extract_metadata()

        assert namespace is not None
        assert namespace.vertical == "System"
        assert namespace.field == "Terminal"
        assert namespace.object == "TerminalIO"

    def test_get_schema_summary(self, terminal_io_path):
        """Test schema summary generation."""
        engine = GeneratorEngine()
        engine.load_schema(terminal_io_path)
        summary = engine.get_schema_summary()

        assert 'catalogue' in summary
        assert summary['catalogue']['vertical'] == 'System'
        assert 'delta_fields' in summary
        assert 'operations' in summary

    def test_config_verbose(self):
        """Test verbose configuration."""
        config = GeneratorConfig(verbose=True)
        engine = GeneratorEngine(config)
        assert engine.config.verbose is True

    def test_config_output_dir(self):
        """Test output directory configuration."""
        config = GeneratorConfig(output_dir="custom_output")
        engine = GeneratorEngine(config)
        assert engine.config.output_dir == Path("custom_output")


# Integration Tests

class TestIntegration:
    """Integration tests for the complete pipeline."""

    def test_complete_pipeline_terminal_io(self, terminal_io_path):
        """Test complete pipeline with terminal-io.json."""
        engine = GeneratorEngine(GeneratorConfig(
            output_dir="test_output",
            verbose=False
        ))

        # Load schema
        result = engine.load_schema(terminal_io_path)
        assert result.valid is True

        # Extract metadata
        namespace = engine.extract_metadata()
        assert namespace.vertical == "System"

        # Get summary
        summary = engine.get_schema_summary()
        assert len(summary['delta_fields']) == 2  # command_delta, response_delta

    def test_complete_pipeline_p2p_delta(self, p2p_delta_path):
        """Test complete pipeline with p2p-delta.json."""
        engine = GeneratorEngine()
        result = engine.load_schema(p2p_delta_path)
        assert result.valid is True

        namespace = engine.extract_metadata()
        assert namespace.vertical == "Network"
        assert namespace.field == "P2P"

        summary = engine.get_schema_summary()
        assert 'rollback' in summary['operations']


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
