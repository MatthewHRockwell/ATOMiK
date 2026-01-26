# ATOMiK SDK Generator Framework

**Version**: 1.0.0
**Phase**: 4A.2 - Generator Framework
**Date**: January 26, 2026

---

## Overview

The ATOMiK SDK Generator is a multi-language code generation framework that transforms JSON schema definitions into production-ready code for Python, Rust, C, JavaScript, and Verilog.

### Key Features

- **Schema Validation**: JSON Schema Draft 7 validation with custom semantic rules
- **Namespace Mapping**: Automatic conversion of catalogue metadata to language-specific imports
- **Plugin Architecture**: Extensible code emitter system for adding new languages
- **Error Reporting**: Comprehensive validation and generation error messages
- **Multi-Language**: Single schema generates code for 5+ target languages

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    GeneratorEngine                          │
│  (Orchestrates the entire code generation pipeline)         │
└────────────────┬────────────────────────────────────────────┘
                 │
      ┌──────────┼──────────┐
      │          │          │
      ▼          ▼          ▼
┌──────────┐ ┌──────────┐ ┌──────────────────┐
│ Schema   │ │Namespace │ │Multi-Language    │
│Validator │ │ Mapper   │ │   Emitter        │
└──────────┘ └──────────┘ └─────────┬────────┘
                                     │
                        ┌────────────┴────────────┐
                        │                         │
                        ▼                         ▼
                 ┌────────────┐          ┌────────────┐
                 │  Python    │   ...    │  Verilog   │
                 │ Generator  │          │ Generator  │
                 └────────────┘          └────────────┘
```

---

## Module Descriptions

### `core.py` - Generator Engine

Main orchestrator that coordinates:
- Schema loading from JSON files
- Schema validation
- Metadata extraction
- Code generation across multiple languages
- File output management

**Key Classes**:
- `GeneratorEngine`: Main API entry point
- `GeneratorConfig`: Configuration settings

### `schema_validator.py` - Schema Validation

Validates ATOMiK schemas against JSON Schema Draft 7 specification and enforces semantic rules.

**Validation Levels**:
1. **Structural**: JSON Schema Draft 7 compliance
2. **Cross-field**: Inter-field dependency validation
3. **Semantic**: Best practices and warnings

**Key Classes**:
- `SchemaValidator`: Main validator
- `ValidationResult`: Validation outcome with errors/warnings

**Validation Rules**:
- Required fields (catalogue, delta_fields, operations.accumulate)
- Type constraints (field widths must be power of 2)
- Cross-field dependencies (hardware.DATA_WIDTH must match field widths)
- Namespace uniqueness checks

### `namespace_mapper.py` - Namespace Mapping

Converts catalogue metadata to language-specific namespaces and import paths.

**Key Classes**:
- `NamespaceMapper`: Maps catalogue to namespaces
- `NamespaceMapping`: Data class holding all namespace variants

**Supported Languages**:
| Language | Example |
|----------|---------|
| Python | `from atomik.Video.Stream import H264Delta` |
| Rust | `use atomik::video::stream::H264Delta;` |
| C | `#include <atomik/video/stream/h264_delta.h>` |
| JavaScript | `const {H264Delta} = require('@atomik/video/stream');` |
| Verilog | `module atomik_video_stream_h264_delta` |

**Identifier Validation**:
- Must start with uppercase letter (PascalCase)
- Alphanumeric characters only
- Not a reserved keyword in any target language
- Length: 2-64 characters

### `code_emitter.py` - Code Emission Base

Abstract base class and utilities for language-specific code generation.

**Key Classes**:
- `CodeEmitter`: Abstract base class for generators
- `GeneratedFile`: Represents a generated source file
- `GenerationResult`: Result of code generation
- `MultiLanguageEmitter`: Coordinates multiple generators

**Plugin Architecture**:
Language-specific generators subclass `CodeEmitter` and implement:
```python
def generate(self, schema: Dict, namespace: NamespaceMapping) -> GenerationResult:
    # Generate code for target language
    ...
```

---

## Usage Examples

### Basic Usage

```python
from atomik_sdk.generator import GeneratorEngine, GeneratorConfig

# Create engine with configuration
engine = GeneratorEngine(GeneratorConfig(
    output_dir="generated",
    validate_schemas=True,
    verbose=True
))

# Load schema
validation = engine.load_schema("sdk/schemas/examples/terminal-io.json")

if not validation:
    print(f"Validation errors: {validation.errors}")
    exit(1)

# Extract metadata
namespace = engine.extract_metadata()
print(f"Namespace: {namespace.python_import_statement}")

# Generate code (requires registered generators)
results = engine.generate(target_languages=['python'])

# Write output
files = engine.write_output(results)
print(f"Generated {len(files)} files")
```

### Validation Only

```python
from atomik_sdk.generator import SchemaValidator

validator = SchemaValidator()

# Validate single schema
result = validator.validate_file("my-schema.json")

if result:
    print("Schema is valid")
    if result.warnings:
        for warning in result.warnings:
            print(f"Warning: {warning}")
else:
    for error in result.errors:
        print(f"Error: {error}")
```

### Namespace Mapping

```python
from atomik_sdk.generator import NamespaceMapper

mapper = NamespaceMapper()

catalogue = {
    "vertical": "Video",
    "field": "Stream",
    "object": "H264Delta"
}

namespace = mapper.map_catalogue(catalogue)

print(namespace.python_import_statement)
# Output: from atomik.Video.Stream import H264Delta

print(namespace.rust_use_statement)
# Output: use atomik::video::stream::H264Delta;

print(namespace.verilog_module_name)
# Output: atomik_video_stream_h264_delta
```

### Implementing a Language Generator

```python
from atomik_sdk.generator import CodeEmitter, GeneratedFile, GenerationResult

class MyLanguageGenerator(CodeEmitter):
    def __init__(self):
        super().__init__('mylang')

    def generate(self, schema, namespace):
        # Extract information
        delta_fields = self._extract_delta_fields(schema)
        operations = self._extract_operations(schema)

        # Generate code
        code = self._generate_class(namespace, delta_fields, operations)

        # Create result
        file = GeneratedFile(
            relative_path=f"mylang/{namespace.object.lower()}.mylang",
            content=code,
            language='mylang',
            description=f"{namespace.object} implementation"
        )

        return GenerationResult(
            success=True,
            files=[file],
            errors=[],
            warnings=[]
        )

    def _generate_class(self, namespace, fields, ops):
        # Generate language-specific code
        return f"class {namespace.object} {{ ... }}"
```

---

## Testing

Run the test suite:

```bash
# Run all generator tests
python -m pytest software/atomik_sdk/tests/test_generator_core.py -v

# Run with coverage
python -m pytest software/atomik_sdk/tests/test_generator_core.py --cov=atomik_sdk.generator
```

---

## Phase 4 Integration

### Phase 4A.2 (Current) - Generator Framework ✓

Completed deliverables:
- `core.py` - Generator engine
- `schema_validator.py` - Schema validation
- `code_emitter.py` - Code emission base
- `namespace_mapper.py` - Namespace mapping
- `__init__.py` - Package initialization
- `README.md` - This documentation

### Phase 4A.3+ - Language Generators

Upcoming tasks will implement language-specific generators:

**T4A.3**: Python SDK Generator
- `generator/python_generator.py`
- Generates Python classes with type hints

**T4A.4**: Rust SDK Generator
- `generator/rust_generator.py`
- Generates Rust crates with proper ownership

**T4A.5**: C SDK Generator
- `generator/c_generator.py`
- Generates C header/implementation pairs

**T4A.6**: Verilog RTL Generator
- `generator/verilog_generator.py`
- Generates synthesis-ready Verilog modules

**T4A.7**: JavaScript SDK Generator
- `generator/javascript_generator.py`
- Generates NPM packages with TypeScript definitions

### Integration Pattern

```python
from atomik_sdk.generator import GeneratorEngine
from atomik_sdk.generator.python_generator import PythonGenerator
from atomik_sdk.generator.rust_generator import RustGenerator

engine = GeneratorEngine()

# Register generators as they become available
engine.register_generator('python', PythonGenerator())
engine.register_generator('rust', RustGenerator())

# Generate for all registered languages
results = engine.generate_and_write("schema.json")
```

---

## API Reference

### GeneratorEngine

| Method | Description |
|--------|-------------|
| `load_schema(path)` | Load and validate schema file |
| `extract_metadata()` | Extract namespace mapping from catalogue |
| `register_generator(lang, gen)` | Register language generator |
| `generate(languages=None)` | Generate code for specified languages |
| `write_output(results)` | Write generated files to disk |
| `generate_and_write(path, langs)` | Complete pipeline in one call |
| `get_schema_summary()` | Get schema information summary |

### SchemaValidator

| Method | Description |
|--------|-------------|
| `validate_file(path)` | Validate schema from file |
| `validate(schema_dict)` | Validate schema dictionary |
| `validate_namespace_uniqueness(schemas)` | Check for duplicate namespaces |

### NamespaceMapper

| Method | Description |
|--------|-------------|
| `map_catalogue(catalogue)` | Map catalogue to all namespaces |
| `validate_identifier(name)` | Check if identifier is valid |
| `generate_directory_structure(mapping)` | Get directory structure for languages |

---

## Error Handling

### Validation Errors

Schema validation errors are categorized:

1. **Structural Errors**: Missing required fields, invalid types
2. **Cross-Field Errors**: Conflicting specifications
3. **Semantic Warnings**: Best practice violations

Example:
```python
result = validator.validate_file("bad-schema.json")

if not result:
    for error in result.errors:
        print(f"Error: {error}")
        # Error: Missing catalogue.version
        # Error: schema.operations.accumulate.enabled must be true
```

### Generation Errors

Code generation errors include file creation failures, template errors, etc.

```python
results = engine.generate()

for lang, result in results.items():
    if not result.success:
        print(f"{lang} failed:")
        for error in result.errors:
            print(f"  - {error}")
```

---

## Best Practices

### 1. Always Validate

Enable schema validation in production:
```python
config = GeneratorConfig(validate_schemas=True)
```

### 2. Handle Errors Gracefully

Check results before proceeding:
```python
validation = engine.load_schema("schema.json")
if not validation:
    # Handle validation errors
    return

results = engine.generate()
for lang, result in results.items():
    if not result.success:
        # Handle generation errors
        continue
```

### 3. Use Verbose Mode During Development

```python
config = GeneratorConfig(verbose=True)
engine = GeneratorEngine(config)
```

### 4. Organize Output by Language

Generated files are automatically organized:
```
generated/
├── python/
│   └── atomik/System/Terminal/terminal_io.py
├── rust/
│   └── src/system/terminal/terminal_io.rs
└── ...
```

---

## Troubleshooting

### "Schema specification not found"

Ensure `specs/atomik_schema_v1.json` exists in project root:
```python
validator = SchemaValidator(schema_spec_path="path/to/atomik_schema_v1.json")
```

### "No generators registered"

Register at least one language generator:
```python
from atomik_sdk.generator.python_generator import PythonGenerator
engine.register_generator('python', PythonGenerator())
```

### "Validation failed: Missing required field"

Check your schema has all required fields:
- `catalogue.vertical`
- `catalogue.field`
- `catalogue.object`
- `catalogue.version`
- `schema.delta_fields`
- `schema.operations.accumulate`

---

## Contributing

To add a new language generator:

1. Create `generator/{language}_generator.py`
2. Subclass `CodeEmitter`
3. Implement `generate(schema, namespace)` method
4. Add tests in `tests/test_{language}_generator.py`
5. Update this README

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2026-01-26 | Initial framework (Phase 4A.2) |

---

*ATOMiK SDK Generator Framework - Phase 4A.2 Complete*
