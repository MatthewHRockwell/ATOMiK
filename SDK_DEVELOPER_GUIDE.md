# ATOMiK SDK Developer Guide

**Version:** 2.1.0
**Last Updated:** February 14, 2026

## Table of Contents

1. [Introduction](#introduction)
2. [Architecture Overview](#architecture-overview)
3. [Generator Framework](#generator-framework)
4. [Creating Custom Generators](#creating-custom-generators)
5. [Pipeline Framework](#pipeline-framework)
6. [Schema Reference](#schema-reference)
7. [Testing](#testing)
8. [Contributing](#contributing)

---

## Introduction

The ATOMiK SDK is a multi-language code generation framework that produces delta-state computing primitives from JSON schema specifications. This guide is intended for developers who want to understand the SDK internals, extend the generator framework, or contribute new language targets.

### Key Features

- **Multi-Language Support**: Python, Rust, C, Verilog, JavaScript
- **Schema-Driven Generation**: Single JSON schema → 5 language implementations
- **Namespace Consistency**: Automatic mapping across languages
- **Hardware Integration**: Verilog RTL matching validated FPGA architecture
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

## Pipeline Framework

### Overview

The pipeline framework is a self-improving agentic orchestrator built on 25 modules across orchestration, routing, verification, feedback, and optimization subsystems. The full test suite includes 353 total tests validating all pipeline components end-to-end.

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

## Schema Reference

This section provides the complete reference for ATOMiK JSON schema specifications, which drive multi-language SDK generation from a single declarative source.

### Core Concepts

**Delta-State Computing**: Instead of storing full state, ATOMiK maintains:
- **Initial state** (S0): The base state at time t=0
- **Delta accumulator** (delta): XOR composition of all deltas
- **Current state**: S = S0 XOR delta (single XOR operation)

**Catalogue-Driven Namespaces**: The schema's catalogue position automatically determines import paths, file/module structure, and package naming conventions in all target languages.

| Benefit | Description |
|---------|-------------|
| **Write Once, Deploy Everywhere** | Define delta operations once, generate code for all platforms |
| **Type Safety** | JSON Schema validation catches errors before code generation |
| **Discoverability** | Hierarchical catalogue structure enables ecosystem browsing |
| **Hardware/Software Co-design** | Same schema generates both software SDK and hardware RTL |
| **Version Management** | Semantic versioning built into schema metadata |

### Schema Structure

An ATOMiK schema consists of three main sections: catalogue (required), schema (required), and hardware (optional).

#### Catalogue (Required)

Positioning metadata that determines API namespace and module identity.

```json
{
  "catalogue": {
    "vertical": "System",
    "field": "Terminal",
    "object": "TerminalIO",
    "version": "1.0.0",
    "author": "ATOMiK Project",
    "license": "MIT",
    "description": "Control primitive for terminal I/O operations"
  }
}
```

| Field | Description | Example |
|-------|-------------|---------|
| `vertical` | Top-level market vertical | "Video", "Network", "Edge" |
| `field` | Domain-specific field | "Stream", "P2P", "Sensor" |
| `object` | Specific component name | "H264Delta", "DeltaExchange" |
| `version` | Semantic version | "1.0.0", "2.1.3-beta" |
| `author` | Author/organization | "ATOMiK Project", "Acme Corp" |
| `license` | SPDX identifier | "MIT", "Apache-2.0" |
| `description` | Human-readable summary | "Efficient video frame delta encoding" |

**Naming Rules:**
- All identifiers must be PascalCase
- Alphanumeric characters only
- Avoid reserved keywords across all languages
- Length: 2-64 characters

#### Schema (Required)

Computational definition of delta fields, operations, and constraints.

```json
{
  "schema": {
    "delta_fields": {
      "command_delta": {
        "type": "parameter_delta",
        "width": 64,
        "encoding": "raw",
        "compression": "none"
      }
    },
    "operations": {
      "accumulate": {
        "enabled": true,
        "latency_cycles": 1
      },
      "reconstruct": {
        "enabled": true,
        "latency_cycles": 1
      }
    },
    "constraints": {
      "max_memory_mb": 1,
      "update_latency_ms": 1
    }
  }
}
```

#### Hardware (Optional)

Hardware mapping for Verilog RTL generation.

```json
{
  "hardware": {
    "target_device": "GW1NR-9",
    "rtl_params": {
      "DATA_WIDTH": 64,
      "ENABLE_PARALLEL": false
    },
    "synthesis_options": {
      "optimization_goal": "speed"
    }
  }
}
```

**Validation:**
- `DATA_WIDTH` must match maximum delta field width
- Verilog generator uses these parameters

### Field Type Reference

ATOMiK supports three fundamental delta field types.

#### `delta_stream`

**Purpose**: Continuous stream of deltas (e.g., video frames, sensor readings).

**Characteristics**:
- High-frequency updates
- Time-series data
- Typically compressed

**Use Cases**: Video frame deltas, audio sample deltas, network packet streams, sensor data streams.

```json
{
  "network_delta": {
    "type": "delta_stream",
    "width": 128,
    "compression": "xor"
  }
}
```

#### `bitmask_delta`

**Purpose**: Bit-level state changes (e.g., flags, status bits).

**Characteristics**:
- Sparse updates (few bits change)
- Boolean state tracking
- Efficient for large bitmaps

**Use Cases**: Device status flags, feature enable/disable, permission bits, configuration registers.

```json
{
  "status_flags": {
    "type": "bitmask_delta",
    "width": 32,
    "encoding": "raw"
  }
}
```

#### `parameter_delta`

**Purpose**: Configuration/parameter updates (e.g., settings, control values).

**Characteristics**:
- Infrequent updates
- Full-width values
- Human-readable intent

**Use Cases**: Configuration changes, control commands, parameter tuning, system settings.

```json
{
  "config_delta": {
    "type": "parameter_delta",
    "width": 64,
    "default_value": 0
  }
}
```

### Delta Field Properties

#### Width

**Required**: Yes
**Type**: Integer (enum)
**Values**: 8, 16, 32, 64, 128, 256

The bit width must be a power of 2 between 8 and 256 bits.

**Choosing Width**:
- **8-bit**: Boolean flags, byte-aligned data
- **16-bit**: Audio samples, small counters
- **32-bit**: Standard integers, addresses
- **64-bit**: Timestamps, large counters, pointers
- **128-bit**: UUID, IPv6 addresses, crypto keys
- **256-bit**: Hash values, wide data buses

**Hardware Impact**: Wider fields consume more FPGA resources (LUTs, registers).

#### Encoding

**Required**: No
**Type**: String (enum)
**Values**: `spatiotemporal_4x4x4`, `raw`, `rle`
**Default**: `raw`

| Encoding | Description | Use Case |
|----------|-------------|----------|
| `raw` | No encoding, direct bit representation | Default, maximum speed |
| `spatiotemporal_4x4x4` | 4x4x4 block encoding | Video/image deltas |
| `rle` | Run-length encoding | Sparse data, long zero runs |

```json
{
  "video_delta": {
    "type": "delta_stream",
    "width": 256,
    "encoding": "spatiotemporal_4x4x4"
  }
}
```

#### Compression

**Required**: No
**Type**: String (enum)
**Values**: `xor`, `rle`, `none`
**Default**: `none`

| Compression | Description | Ratio | Speed |
|-------------|-------------|-------|-------|
| `none` | No compression | 1:1 | Fastest |
| `xor` | XOR-based delta compression | 10-100:1 | Fast |
| `rle` | Run-length encoding | 5-50:1 | Medium |

**Trade-offs**:
- `none`: Maximum speed, no size reduction
- `xor`: Good balance, hardware-friendly (95% memory reduction validated)
- `rle`: Best for sparse data with long runs

#### Default Value

**Required**: No
**Type**: Integer (>= 0)
**Default**: 0

Initial value for the delta field at system startup.

### Operations Reference

#### Accumulate (Required)

XOR-based delta accumulation (the core operation).

**Mathematical Form**: `delta_accumulated <- delta_accumulated XOR delta_new`

**Properties** (proven in Lean4):
- **Commutative**: d1 XOR d2 = d2 XOR d1
- **Associative**: (d1 XOR d2) XOR d3 = d1 XOR (d2 XOR d3)
- **Self-Inverse**: d XOR d = 0
- **Identity**: d XOR 0 = d

**Hardware Performance**:
- **Latency**: 1 clock cycle @ 94.5 MHz
- **Throughput**: 94.5 million deltas/second
- **Resource**: ~160 LUTs (7% of GW1NR-9)

```json
{
  "operations": {
    "accumulate": {
      "enabled": true,
      "latency_cycles": 1
    }
  }
}
```

**Note**: `enabled` must always be `true` (accumulate is mandatory).

#### Reconstruct

State reconstruction from accumulated deltas.

**Mathematical Form**: `S_current = S_initial XOR delta_accumulated`

**Complexity**:
- **Software**: O(N) if maintaining delta history, O(1) if maintaining accumulator
- **Hardware**: O(1) - single XOR operation

**Performance**:
- **Latency**: 1 clock cycle (combinational + output register)

```json
{
  "operations": {
    "reconstruct": {
      "enabled": true,
      "latency_cycles": 1
    }
  }
}
```

**Use Cases**: Reading current state, checkpointing, state export.

#### Rollback (Optional)

Temporal state rollback via delta reversal.

**Mathematical Form**: `S_t-1 = S_t XOR delta_t` (reverse last delta)

**Key Property**: Self-inverse property (d XOR d = 0) enables perfect rollback.

**Requirements**:
- If `enabled` is `true`, `history_depth` must be specified
- History depth determines how many deltas to store

```json
{
  "operations": {
    "rollback": {
      "enabled": true,
      "history_depth": 256
    }
  }
}
```

**Memory Cost**: `memory_bytes = history_depth * (delta_width / 8)` (e.g., 256 entries x 128 bits / 8 = 4 KB)

**Use Cases**: Undo/redo functionality, distributed consensus (conflict resolution), time-travel debugging, checkpointing.

### Constraints

Constraints define resource and performance limits for the generated SDK.

#### Memory Constraints

**Field**: `max_memory_mb`
**Type**: Integer (1 - 65,536 MB)

**Guidance**:
- **Edge devices**: 1-16 MB
- **Mobile**: 16-256 MB
- **Server**: 256+ MB

#### Power Constraints

**Field**: `max_power_mw`
**Type**: Integer (1 - 100,000 mW)

**Guidance**:
- **Battery-powered**: 100-1,000 mW
- **USB-powered**: 1,000-5,000 mW
- **Mains-powered**: 5,000+ mW

#### Latency Constraints

**Field**: `update_latency_ms`
**Type**: Integer (0 - 10,000 ms)

**Guidance**:
- **Real-time control**: 0-10 ms
- **Interactive**: 10-100 ms
- **Batch processing**: 100+ ms

#### Target Frequency

**Field**: `target_frequency_mhz`
**Type**: Number (1.0 - 1,000.0 MHz)
**Default**: 94.5 MHz

### Schema Validation

All ATOMiK schemas must validate against `specs/atomik_schema_v1.json` (JSON Schema Draft 7).

**CLI Validation** (recommended):
```bash
# Validate a single schema
atomik-gen validate sdk/schemas/examples/terminal-io.json

# Show schema summary (namespace, fields, operations)
atomik-gen info sdk/schemas/domains/finance-price-tick.json
```

**VS Code**: Files matching `*.atomik.json` or in `**/schemas/**/*.json` are automatically validated with real-time error squiggles when the [ATOMiK VS Code extension](../sdk/vscode-extension/atomik-vscode/README.md) is installed.

**Python Example**:
```python
import json
import jsonschema

# Load schema specification
with open('specs/atomik_schema_v1.json') as f:
    schema_spec = json.load(f)

# Load instance schema
with open('sdk/schemas/examples/terminal-io.json') as f:
    instance = json.load(f)

# Validate
try:
    jsonschema.validate(instance=instance, schema=schema_spec)
    print("Schema is valid")
except jsonschema.ValidationError as e:
    print(f"Validation error: {e.message}")
```

See [`specs/schema_validation_rules.md`](../specs/schema_validation_rules.md) for detailed error descriptions and fixes.

**Validation Checklist**:
- Schema validates against `atomik_schema_v1.json`
- All required fields present (vertical, field, object, version, delta_fields, accumulate)
- Delta field widths are powers of 2 (8, 16, 32, 64, 128, 256)
- Object names are valid identifiers in all target languages
- If rollback enabled, history_depth is specified
- If hardware.rtl_params.DATA_WIDTH specified, matches delta field widths
- Semantic version follows semver format

### Schema Examples

#### Terminal I/O (Control Primitive)

**File**: [`sdk/schemas/examples/terminal-io.json`](../sdk/schemas/examples/terminal-io.json)
**Catalogue**: System / Terminal / TerminalIO

Two 64-bit delta fields (command, response) with minimal memory footprint (< 1 MB), low latency (< 1 ms), and hardware-ready GW1NR-9 target.

**Use Cases**: Command-line interfaces, serial terminal emulation, remote control protocols.

#### P2P Delta Exchange (Network Primitive)

**File**: [`sdk/schemas/examples/p2p-delta.json`](../sdk/schemas/examples/p2p-delta.json)
**Catalogue**: Network / P2P / DeltaExchange

128-bit delta stream with XOR compression, rollback capability (256 entry history), and conflict resolution support.

**Use Cases**: Distributed databases, collaborative editing, blockchain state sync, IoT mesh networks.

#### Matrix Operations (Compute Primitive)

**File**: [`sdk/schemas/examples/matrix-ops.json`](../sdk/schemas/examples/matrix-ops.json)
**Catalogue**: Compute / Linear / MatrixOps

256-bit wide delta field with spatiotemporal encoding (4x4x4 blocks), parallel hardware acceleration, optimized for sparse updates.

**Use Cases**: Machine learning (gradient updates), scientific computing (iterative solvers), computer graphics (transformation matrices), quantum simulation (state evolution).

#### Video H.264 Delta

**File**: [`sdk/schemas/domains/video-h264-delta.json`](../sdk/schemas/domains/video-h264-delta.json)
**Catalogue**: Video / Streaming / H264Delta

256-bit `frame_delta` with spatiotemporal 4x4x4 encoding and XOR compression, 256-bit `motion_vector` parameter delta, rollback support (512 frame history), hardware-optimized for speed at 150 MHz target.

#### Edge Sensor IMU Fusion

**File**: [`sdk/schemas/domains/edge-sensor-imu.json`](../sdk/schemas/domains/edge-sensor-imu.json)
**Catalogue**: Edge / Sensor / IMUFusion

64-bit `motion_delta` stream for accelerometer/gyroscope data, 64-bit `alert_flags` bitmask delta for anomaly detection, rollback support (1024 sample history), power-optimized hardware at 100 MHz target (500 mW budget).

#### Financial Price Tick

**File**: [`sdk/schemas/domains/finance-price-tick.json`](../sdk/schemas/domains/finance-price-tick.json)
**Catalogue**: Finance / Trading / PriceTick

64-bit `price_delta` (parameter delta for bid/ask changes), 64-bit `volume_delta` (delta stream with XOR compression), 64-bit `trade_flags` (bitmask delta for trade status), rollback support (4096 transaction history), speed-optimized hardware at 400 MHz target (1 ms latency).

### Vertical Catalog

Predefined verticals and common fields:

| Vertical | Common Fields | Examples |
|----------|---------------|----------|
| **Video** | Stream, Codec, Frame, Display | H264Delta, FrameDelta, DisplaySync |
| **Edge** | Sensor, Actuator, Gateway, Fusion | SensorFusion, ActuatorControl |
| **Network** | P2P, Packet, Protocol, Security | DeltaExchange, PacketAnalyzer |
| **Finance** | Trading, Risk, Settlement | TradeUpdate, RiskDelta |
| **Science** | Simulation, Analysis, Data | QuantumState, SimulationDelta |
| **Compute** | Linear, Transform, Neural | MatrixOps, FFTDelta, NeuralDelta |
| **System** | Terminal, Process, Memory | TerminalIO, ProcessState |
| **Storage** | Block, Object, Cache | BlockDelta, CacheLine |

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

### Pipeline Tests

The pipeline has 12 test files covering all subsystems. The full suite includes 353 tests across generator and pipeline components:

```bash
# Run the complete test suite (353 tests)
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
- [VS Code Extension](../sdk/vscode-extension/atomik-vscode/README.md)
- [Hardware Report](../archive/PHASE_3_COMPLETION_REPORT.md)
- [Mathematical Foundations](../specs/formal_model.md)
- [Schema Validation Rules](../specs/schema_validation_rules.md)
- [Lean4 Proofs](../math/proofs/ATOMiK/)
- [Performance Benchmarks](../math/benchmarks/results/PERFORMANCE_COMPARISON.md)

---

**Document Version:** 2.1.0
**Generator Framework Version:** 1.0.0
**Pipeline Framework Version:** 1.0.0
**Last Updated:** February 14, 2026
