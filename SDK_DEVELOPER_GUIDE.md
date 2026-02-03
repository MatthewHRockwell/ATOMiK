# ATOMiK SDK Developer Guide

**Version:** 2.0.0
**Last Updated:** January 27, 2026
**Phase:** 5 - Agentic Orchestration

## Table of Contents

1. [Introduction](#introduction)
2. [Architecture Overview](#architecture-overview)
3. [Generator Framework](#generator-framework)
4. [Creating Custom Generators](#creating-custom-generators)
5. [Pipeline Framework (Phase 5)](#pipeline-framework-phase-5)
6. [Schema Design Guidelines](#schema-design-guidelines)
7. [Testing](#testing)
8. [Contributing](#contributing)

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
├── pipeline/
│   ├── orchestrator.py          # DAG-based task orchestrator
│   ├── dag.py                   # Task DAG with cycle detection
│   ├── event_bus.py             # Pub/sub event system
│   ├── feedback.py              # Generate→Verify→Fix loop
│   ├── coordinator.py           # Multi-agent coordinator
│   ├── consensus.py             # Consensus resolution
│   ├── agents/
│   │   ├── router.py            # Static model routing
│   │   ├── adaptive_router.py   # Adaptive model routing
│   │   ├── registry.py          # Agent registry
│   │   ├── specialist.py        # Specialist base class
│   │   ├── token_predictor.py   # Token usage prediction
│   │   ├── prompt_cache.py      # Prompt caching
│   │   ├── context_compressor.py # Context compression
│   │   └── complexity_scorer.py # Schema complexity scoring
│   ├── parallel/
│   │   ├── decomposer.py        # Task decomposition
│   │   ├── executor.py          # Parallel execution
│   │   └── worker.py            # Worker thread management
│   ├── verification/
│   │   ├── deep_verify.py       # Deep verification engine
│   │   ├── consistency.py       # Cross-language consistency
│   │   ├── interfaces.py        # Shared interface types
│   │   └── extractors/          # 5 language extractors
│   ├── analysis/
│   │   ├── field_diff.py        # Field-level diff
│   │   ├── metrics_analyzer.py  # Cross-run metrics
│   │   └── regression_detector.py # Regression detection
│   ├── knowledge/
│   │   ├── error_kb.py          # Error pattern knowledge base
│   │   └── fuzzy_match.py       # Fuzzy matching utilities
│   ├── context/
│   │   ├── manifest.py          # Pipeline manifest
│   │   ├── cache.py             # Artifact caching
│   │   ├── segment_tracker.py   # Context segment tracking
│   │   └── intelligent_manager.py # Intelligent context manager
│   ├── regression/
│   │   ├── baseline.py          # Baseline management
│   │   └── detector.py          # Regression gate
│   ├── optimization/
│   │   ├── tuner.py             # Config auto-tuning
│   │   └── self_optimizer.py    # Self-optimization engine
│   └── reports/
│       └── pipeline_report.py   # Pipeline reports
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
DAG Orchestrator (event-driven)
    ↓
Validate → Diff → Generate ×5 → Verify → Report
                     ↑       ↓
                 [Adaptive  [Deep Verify]
                  Router]       ↓
                          [Feedback Loop]
                              ↓
                          [Error KB]
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

The [VS Code extension](../sdk/vscode-extension/atomik-vscode/README.md) provides:
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

## Pipeline Framework (Phase 5)

### Overview

Phase 5 transforms the Phase 4C linear pipeline into a self-improving agentic orchestrator. The implementation spans 16 tasks (T5.1-T5.16), introducing 25 new modules across orchestration, routing, verification, feedback, and optimization subsystems. The full test suite includes 242 total tests validating all pipeline components end-to-end.

### DAG Orchestrator

The pipeline execution model is built on a directed acyclic graph (DAG) of tasks. `TaskDAG` manages task nodes and their dependencies, performs cycle detection on insertion, and computes a topological execution order. The `PipelineOrchestrator` consumes the DAG, groups independent tasks into parallel stages, and drives execution through the event bus.

```python
from pipeline.orchestrator import PipelineOrchestrator
from pipeline.dag import TaskDAG

dag = TaskDAG()
dag.add_task("validate", "validation")
dag.add_task("generate", "generation", dependencies=["validate"])
order = dag.topological_order()
```

### Event Bus

The `EventBus` provides a publish/subscribe event system that decouples pipeline stages. Components subscribe to typed events and react asynchronously, enabling extensible instrumentation, logging, and side-effect handling without modifying core pipeline logic.

```python
from pipeline.event_bus import EventBus, Event, EventType

bus = EventBus()
bus.subscribe(EventType.TASK_COMPLETED, lambda e: print(f"Done: {e.data}"))
bus.emit(Event(EventType.TASK_COMPLETED, {"stage": "generate"}))
```

### Feedback Loop

The feedback module implements a Generate, Verify, Diagnose, Fix, Retry cycle. When verification detects errors in generated code, the system first consults the Error Knowledge Base for a known fix pattern (KB-first diagnosis). If no matching pattern is found, it escalates to an LLM for diagnosis and repair. The loop depth is configurable via `max_depth`, preventing infinite retry spirals while maximizing autonomous recovery.

### Adaptive Model Router

The `AdaptiveModelRouter` extends the static `ModelRouter` with multi-signal routing decisions. It considers schema complexity scoring, recent error history, budget pressure (cumulative token spend vs. ceiling), and prompt cache hit rates to select the optimal model tier for each generation request. Four tiers are available: LOCAL (offline/fast), HAIKU (low-cost), SONNET (balanced), and OPUS (highest capability). The router continuously adjusts tier selection as pipeline state evolves across runs.

### Token Efficiency

Three modules work together to minimize token consumption:

- **TokenPredictor**: Predicts token usage for a given schema and model tier based on historical generation data. Predictions inform the adaptive router's budget pressure calculations.
- **PromptCache**: Maintains a schema-keyed cache of prompt fragments, avoiding redundant prompt construction for previously seen schemas or schema components.
- **ContextCompressor**: Applies progressive compression at three pressure levels (low, medium, high) to reduce context window usage when approaching token limits, preserving the most relevant information while discarding lower-priority segments.

### Error Knowledge Base

The `ErrorKB` stores `ErrorPattern` records that map error signatures to known fix strategies. Pattern matching uses a combination of edit distance and token overlap (fuzzy matching) to find the closest known error, even when exact matches are unavailable. The knowledge base auto-learns from successful fixes: when the feedback loop resolves an error, the pattern and its resolution are persisted for future reuse. Seed patterns for common errors (missing imports, type mismatches, syntax violations) are loaded at initialization.

### Parallel Execution

The `TaskDecomposer` analyzes the DAG to create up to 5 parallel groups of independent tasks that can execute concurrently. The `ParallelExecutor` dispatches these groups using a `ThreadPoolExecutor`, managing concurrency within configured limits. Individual `Worker` threads track their own state (idle, running, completed, failed), enabling the orchestrator to monitor progress and handle partial failures without blocking the entire pipeline.

### Deep Verification

The deep verification engine runs generated code through native toolchain checks for each target language:

- **Python**: `pytest` execution of generated test files
- **Rust**: `cargo check` for type and borrow checking
- **C**: `gcc` compilation with warnings-as-errors
- **JavaScript**: `node --check` for syntax verification
- **Verilog**: `iverilog` compilation followed by `vvp` simulation

The system uses a pluggable runner architecture, allowing new verification backends to be added without modifying the core verification logic.

### Multi-Agent Coordination

The `Coordinator` dispatches generation and verification tasks to specialist agents based on their registered capabilities. Each specialist agent handles a specific language or verification domain. The `ConsensusResolver` handles conflicts when multiple agents produce differing results for the same task, using majority voting to select the canonical output. The `AgentRegistry` tracks agent capabilities, current load, and health status, enabling the coordinator to make informed dispatch decisions.

### Context Management

Four modules collaborate to manage pipeline context efficiently:

- **PipelineManifest**: Tracks all schemas processed, run metadata, and artifact locations across pipeline invocations.
- **ArtifactCache**: Caches generated artifacts with content-hash-based invalidation, skipping regeneration when inputs have not changed.
- **SegmentTracker**: Assigns relevance scores to context segments, enabling intelligent pruning when context budgets are constrained.
- **IntelligentContextManager**: Enforces budget limits on total context size, coordinating with the segment tracker and context compressor to maintain the most valuable information within available token budgets.

### Self-Optimization

The `ConfigTuner` auto-tunes pipeline configuration parameters including worker thread count, feedback loop retry depth, and model routing thresholds based on observed performance metrics. The `SelfOptimizer` generates periodic optimization reports with bottleneck analysis, identifying stages that consume disproportionate time or tokens and recommending configuration adjustments to improve throughput.

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

### Phase 5 Pipeline Tests

Phase 5 introduces 12 new test files covering all pipeline subsystems. The full suite now includes 242 tests across generator and pipeline components:

```bash
# Run the complete test suite (242 tests)
pytest tests/ atomik_sdk/tests/ -v
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
- DAG orchestration and cycle detection
- Event bus pub/sub delivery
- Feedback loop convergence
- Adaptive model routing decisions
- Token prediction and prompt caching
- Error knowledge base fuzzy matching
- Parallel execution and worker management
- Deep verification across all toolchains
- Multi-agent coordination and consensus
- Context management and budget enforcement
- Self-optimization and config tuning

---

## Contributing

### Development Setup

1. Clone the repository:
```bash
git clone https://github.com/MatthewHRockwell/ATOMiK.git
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
cd ../sdk/vscode-extension/atomik-vscode
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
- [VS Code Extension](../sdk/vscode-extension/atomik-vscode/README.md)
- [Phase 3 Hardware Report](../archive/PHASE_3_COMPLETION_REPORT.md)
- [Mathematical Foundations](../specs/formal_model.md)

---

**Document Version:** 2.0.0
**Generator Framework Version:** 1.0.0
**Pipeline Framework Version:** 1.0.0
**ATOMiK Phase:** 5 - Agentic Orchestration
**Last Updated:** January 27, 2026
