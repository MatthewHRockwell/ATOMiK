# ATOMiK SDK Developer Guide

**Version:** 1.0.0
**Last Updated:** January 26, 2026
**Phase:** 4A - SDK Development

## Table of Contents

1. [Introduction](#introduction)
2. [Architecture Overview](#architecture-overview)
3. [Generator Framework](#generator-framework)
4. [Creating Custom Generators](#creating-custom-generators)
5. [Schema Design Guidelines](#schema-design-guidelines)
6. [Testing](#testing)
7. [Contributing](#contributing)

---

## Introduction

The ATOMiK SDK is a multi-language code generation framework that produces delta-state computing primitives from JSON schema specifications. This guide is intended for developers who want to understand the SDK internals, extend the generator framework, or contribute new language targets.

### Key Features

- **Multi-Language Support**: Python, Rust, C, Verilog, JavaScript
- **Schema-Driven Generation**: Single JSON schema → 5 language implementations
- **Namespace Consistency**: Automatic mapping across languages
- **Hardware Integration**: Verilog RTL matching Phase 3 FPGA architecture
- **Extensible Architecture**: Plugin-based generator system

---

## Architecture Overview

### Component Structure

```
software/atomik_sdk/
├── cli.py                    # atomik-gen CLI tool (pip-installable entry point)
├── generator/
│   ├── core.py               # GeneratorEngine orchestrator
│   ├── schema_validator.py   # JSON Schema validation
│   ├── namespace_mapper.py   # Cross-language namespace mapping
│   ├── code_emitter.py       # Base classes for code generation
│   ├── python_generator.py   # Python SDK generator
│   ├── rust_generator.py     # Rust SDK generator
│   ├── c_generator.py         # C SDK generator
│   ├── verilog_generator.py  # Verilog RTL generator
│   └── javascript_generator.py # JavaScript SDK generator
├── tests/
│   ├── test_generator_simple.py
│   ├── test_python_generation.py
│   ├── test_rust_generation.py
│   ├── test_c_generation.py
│   ├── test_verilog_generation.py
│   ├── test_javascript_generation.py
│   └── test_integration.py
└── README.md
```

### Data Flow

```
JSON Schema
    ↓
atomik-gen CLI  (user-facing entry point)
    ↓
SchemaValidator (validates schema)
    ↓
NamespaceMapper (extracts catalogue metadata)
    ↓
GeneratorEngine (orchestrates generation)
    ↓
CodeEmitter plugins (generate language-specific code)
    ↓
Generated Files (written to output directory)
```

### CLI Tool

The `atomik-gen` CLI (`atomik_sdk/cli.py`) is the primary user-facing interface. It wraps `GeneratorEngine` with argparse subcommands:

```bash
atomik-gen generate <schema> [--output-dir DIR] [--languages LANG ...]
atomik-gen validate <schema>
atomik-gen info <schema>
atomik-gen batch <directory> [--output-dir DIR] [--report FILE]
atomik-gen list
```

Installed via `pip install -e ./software` (entry point defined in `pyproject.toml`).

### VS Code Extension

The [VS Code extension](../vscode-extension/atomik-vscode/README.md) provides:
- JSON Schema intellisense for `*.atomik.json` and `**/schemas/**/*.json` files
- Schema snippets (`atomik-schema`, `atomik-field`, `atomik-hardware`)
- Command palette integration invoking `atomik-gen` CLI commands

---

## Generator Framework

### Core Components

#### 1. GeneratorEngine

The central orchestrator for the SDK generation pipeline.

**Key Methods:**

```python
class GeneratorEngine:
    def __init__(self, config: GeneratorConfig)
    def register_generator(self, language: str, emitter: CodeEmitter)
    def load_schema(self, schema_path: Path) -> ValidationResult
    def extract_metadata(self) -> NamespaceMapping
    def generate(self, target_languages: List[str] = None) -> Dict[str, GenerationResult]
    def write_output(self, results: Dict[str, GenerationResult]) -> List[str]
    def generate_and_write(self, schema_path, target_languages=None) -> Tuple[Dict, List]
```

**Usage Example:**

```python
from generator.core import GeneratorEngine, GeneratorConfig
from generator.python_generator import PythonGenerator

# Create engine
engine = GeneratorEngine(GeneratorConfig(
    output_dir=Path("./output"),
    validate_schemas=True,
    verbose=True
))

# Register generators
engine.register_generator('python', PythonGenerator())

# Load schema and generate
engine.load_schema(Path("schema.json"))
results = engine.generate(target_languages=['python'])
files = engine.write_output(results)
```

#### 2. SchemaValidator

Validates JSON schemas against the ATOMiK schema specification (Draft 7).

**Features:**
- JSON Schema Draft 7 validation
- Cross-field dependency checking
- Hardware-schema consistency validation
- Detailed error reporting

**Key Methods:**

```python
class SchemaValidator:
    def validate(self, schema: Dict[str, Any]) -> ValidationResult
    def validate_file(self, schema_path: Path) -> ValidationResult
```

#### 3. NamespaceMapper

Maps catalogue metadata to language-specific namespaces.

**Features:**
- Consistent naming across 5 languages
- PascalCase → snake_case conversion
- Reserved keyword checking
- Directory structure generation

**Key Methods:**

```python
class NamespaceMapper:
    @staticmethod
    def map_catalogue(catalogue: Dict[str, Any]) -> NamespaceMapping

    @staticmethod
    def validate_identifier(name: str) -> tuple[bool, str | None]

    @staticmethod
    def generate_directory_structure(mapping: NamespaceMapping) -> Dict[str, str]
```

**Example:**

```python
mapper = NamespaceMapper()
catalogue = {
    "vertical": "Video",
    "field": "Stream",
    "object": "H264Delta"
}

mapping = mapper.map_catalogue(catalogue)
print(mapping.python_import_statement)
# Output: from atomik.Video.Stream import H264Delta

print(mapping.rust_use_statement)
# Output: use atomik::video::stream::H264Delta;
```

#### 4. CodeEmitter

Abstract base class for language-specific code generators.

**Interface:**

```python
class CodeEmitter(ABC):
    def __init__(self, language: str):
        self.language = language

    @abstractmethod
    def generate(
        self,
        schema: Dict[str, Any],
        namespace: NamespaceMapping
    ) -> GenerationResult:
        pass
```

---

## Creating Custom Generators

### Step 1: Create Generator Class

```python
from generator.code_emitter import CodeEmitter, GeneratedFile, GenerationResult
from generator.namespace_mapper import NamespaceMapping
from typing import Dict, Any

class MyLanguageGenerator(CodeEmitter):
    """Generator for MyLanguage."""

    def __init__(self):
        super().__init__('mylanguage')

    def generate(
        self,
        schema: Dict[str, Any],
        namespace: NamespaceMapping
    ) -> GenerationResult:
        """Generate code for MyLanguage."""
        try:
            files = []
            errors = []
            warnings = []

            # Extract schema components
            catalogue = schema.get('catalogue', {})
            delta_fields = schema.get('schema', {}).get('delta_fields', {})
            operations = schema.get('schema', {}).get('operations', {})

            # Generate files
            main_file = self._generate_main_file(namespace, delta_fields, operations)
            files.append(main_file)

            return GenerationResult(
                success=True,
                files=files,
                errors=errors,
                warnings=warnings
            )

        except Exception as e:
            return GenerationResult(
                success=False,
                files=[],
                errors=[f"Generation failed: {str(e)}"],
                warnings=[]
            )

    def _generate_main_file(self, namespace, delta_fields, operations):
        """Generate main implementation file."""
        lines = []

        # Add your code generation logic here
        lines.append(f"// Generated code for {namespace.object}")
        lines.append("")

        # ... generate delta operations ...

        content = "\n".join(lines)

        return GeneratedFile(
            relative_path=f"src/{namespace.object}.mylang",
            content=content,
            language='mylanguage',
            description=f"MyLanguage module for {namespace.object}"
        )
```

### Step 2: Register Generator

```python
from generator.core import GeneratorEngine, GeneratorConfig
from my_generator import MyLanguageGenerator

engine = GeneratorEngine(GeneratorConfig(output_dir=Path("./output")))
engine.register_generator('mylanguage', MyLanguageGenerator())
```

### Step 3: Create Tests

```python
def test_mylanguage_generation():
    """Test MyLanguage code generation."""
    engine = GeneratorEngine(GeneratorConfig(
        output_dir=Path(tempfile.mkdtemp()),
        validate_schemas=True
    ))

    engine.register_generator('mylanguage', MyLanguageGenerator())
    engine.load_schema(Path("test_schema.json"))

    results = engine.generate(target_languages=['mylanguage'])

    assert 'mylanguage' in results
    assert results['mylanguage'].success
    assert len(results['mylanguage'].files) > 0
```

---

## Schema Design Guidelines

### Catalogue Metadata

The catalogue section defines the namespace position:

```json
{
  "catalogue": {
    "vertical": "Compute",     // Industry vertical (Video, Network, etc.)
    "field": "Linear",          // Field within vertical
    "object": "MatrixOps",      // Specific object/module name
    "version": "1.0.0",         // Semantic versioning
    "description": "Matrix operations using delta encoding"
  }
}
```

**Naming Rules:**
- All identifiers must be PascalCase
- Alphanumeric characters only
- Avoid reserved keywords across all languages
- Length: 2-64 characters

### Delta Fields

Define the delta state fields:

```json
{
  "schema": {
    "delta_fields": {
      "matrix_delta": {
        "type": "delta_stream",
        "width": 256,
        "description": "Spatiotemporal delta encoding"
      }
    }
  }
}
```

**Delta Types:**
- `delta_stream`: Continuous delta stream
- `parameter_delta`: Parameter-based deltas
- `event_delta`: Event-driven deltas

**Width Constraints:**
- Must be power of 2
- Range: 8 to 512 bits
- Must match hardware DATA_WIDTH if hardware section present

### Operations

Configure which operations are enabled:

```json
{
  "schema": {
    "operations": {
      "accumulate": {
        "enabled": true
      },
      "reconstruct": {
        "enabled": true
      },
      "rollback": {
        "enabled": true,
        "history_depth": 10
      }
    }
  }
}
```

**Standard Operations:**
- `accumulate`: XOR delta into accumulator (always enabled)
- `reconstruct`: Compute current state (always enabled)
- `rollback`: Undo N previous deltas (optional)

### Hardware Mapping

Optional hardware configuration for Verilog generation:

```json
{
  "hardware": {
    "target": "FPGA",
    "device": "GW1NR-LV9QN88PC6/I5",
    "clock_mhz": 94.5,
    "interface": "UART",
    "data_width": 64
  }
}
```

**Validation:**
- `data_width` must match maximum delta field width
- Verilog generator uses these parameters

---

## Testing

### Unit Tests

Each generator has dedicated unit tests:

```bash
# Test individual generators
python tests/test_python_generation.py
python tests/test_rust_generation.py
python tests/test_c_generation.py
python tests/test_verilog_generation.py
python tests/test_javascript_generation.py
```

### Integration Tests

Cross-language consistency validation:

```bash
# Test all generators together
python tests/test_integration.py
```

### Test Coverage

The test suite validates:
- Schema validation (JSON Schema Draft 7)
- Namespace mapping consistency
- Code generation for all languages
- Syntax validation (py_compile, cargo check, gcc, iverilog, node)
- Semantic equivalence across languages
- Cross-field dependencies
- Hardware constraints

---

## Contributing

### Development Setup

1. Clone the repository:
```bash
git clone https://github.com/mopore/ATOMiK.git
cd ATOMiK/software
```

2. Install SDK with development dependencies:
```bash
pip install -e ".[dev]"
```

This installs the `atomik-gen` CLI tool and all dependencies including `jsonschema`.

3. Verify CLI:
```bash
atomik-gen --version
atomik-gen list
```

4. Run tests:
```bash
pytest tests/ atomik_sdk/tests/ -v
```

5. (Optional) Build the VS Code extension:
```bash
cd ../vscode-extension/atomik-vscode
npm install && npm run compile
```

### Adding a New Language Generator

1. Create `generator/your_language_generator.py`
2. Implement `CodeEmitter` interface
3. Add test file `tests/test_your_language_generation.py`
4. Update `test_integration.py` to include new language
5. Document language-specific features in SDK User Manual
6. Submit pull request with:
   - Generator implementation
   - Tests (100% pass rate required)
   - Documentation updates
   - Example generated code

### Code Style

- Follow PEP 8 for Python code
- Use type hints for all public APIs
- Document all classes and methods with docstrings
- Keep functions focused and < 50 lines
- Use descriptive variable names

### Pull Request Process

1. Create feature branch: `git checkout -b feature/your-language-generator`
2. Implement changes with tests
3. Run full test suite
4. Update documentation
5. Submit PR with description of changes
6. Ensure CI passes (all tests, linting)

---

## Appendix

### Namespace Mapping Table

| Language   | Pattern | Example |
|------------|---------|---------|
| Python     | `from atomik.{Vertical}.{Field} import {Object}` | `from atomik.Video.Stream import H264Delta` |
| Rust       | `use atomik::{vertical}::{field}::{Object};` | `use atomik::video::stream::H264Delta;` |
| C          | `#include <atomik/{vertical}/{field}/{object}.h>` | `#include <atomik/video/stream/h264_delta.h>` |
| JavaScript | `const {{Object}} = require('@atomik/{vertical}/{field}');` | `const {H264Delta} = require('@atomik/video/stream');` |
| Verilog    | `module atomik_{vertical}_{field}_{object}` | `module atomik_video_stream_h264_delta` |

### File Generation Summary

| Language   | Files Generated | Description |
|------------|-----------------|-------------|
| Python     | 3 | module.py, __init__.py, test_module.py |
| Rust       | 5 | lib.rs, mod.rs, module.rs, Cargo.toml, tests |
| C          | 4 | module.h, module.c, test_module.c, Makefile |
| Verilog    | 3 | module.v, tb_module.v, module.cst |
| JavaScript | 4 | module.js, index.js, package.json, test.js |

**Total:** 19 files per schema

### References

- [ATOMiK Schema Specification](../specs/atomik_schema_v1.json)
- [SDK User Manual](./user/SDK_USER_MANUAL.md)
- [Schema Guide](./SDK_SCHEMA_GUIDE.md)
- [VS Code Extension](../vscode-extension/atomik-vscode/README.md)
- [Phase 3 Hardware Report](../archive/PHASE_3_COMPLETION_REPORT.md)
- [Mathematical Foundations](../specs/formal_model.md)

---

**Document Version:** 1.0.0
**Generator Framework Version:** 1.0.0
**ATOMiK Phase:** 4A - SDK Development
**Last Updated:** January 26, 2026
