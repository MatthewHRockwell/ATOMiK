# ATOMiK SDK API Reference

**Version:** 2.0.0
**Last Updated:** January 27, 2026

## Table of Contents

1. [Overview](#overview)
2. [CLI Tool Reference](#cli-tool-reference)
3. [Python SDK API](#python-sdk-api)
4. [Rust SDK API](#rust-sdk-api)
5. [C SDK API](#c-sdk-api)
6. [JavaScript SDK API](#javascript-sdk-api)
7. [Verilog RTL Interface](#verilog-rtl-interface)
8. [Pipeline API (Phase 5)](#pipeline-api-phase-5)

---

## Overview

This document provides API reference for the `atomik-gen` CLI tool and all generated SDKs. Each language implementation provides semantically equivalent operations based on ATOMiK delta algebra:

**Core Operations:**
- **LOAD**: Set initial state
- **ACCUMULATE**: XOR delta into accumulator
- **RECONSTRUCT**: Compute current_state = initial_state ⊕ accumulator
- **STATUS**: Check if accumulator is zero
- **ROLLBACK** (optional): Undo N previous delta operations

---

## CLI Tool Reference

The `atomik-gen` CLI tool is the primary interface for schema validation and code generation.

**Installation**: `pip install -e ./software`

### Commands

| Command | Description |
|---------|-------------|
| `atomik-gen generate <schema>` | Generate SDK code from a schema file |
| `atomik-gen validate <schema>` | Validate a schema (no generation) |
| `atomik-gen info <schema>` | Show schema summary (namespace, fields, operations) |
| `atomik-gen batch <directory>` | Batch generate from a directory of schemas |
| `atomik-gen list` | List available target languages |
| `atomik-gen pipeline run <target>` | Execute full autonomous pipeline |
| `atomik-gen pipeline diff <target>` | Show what would change (dry run) |
| `atomik-gen pipeline status` | Show pipeline state from checkpoint |
| `atomik-gen metrics show` | Show metrics for last run |
| `atomik-gen metrics compare` | Compare metrics across schemas |
| `atomik-gen metrics export --output FILE` | Export metrics to CSV |
| `atomik-gen demo <domain>` | Run domain hardware demonstrator (video, sensor, finance) |
| `atomik-gen --version` | Show version |

### Options (generate/batch)

| Option | Description | Default |
|--------|-------------|---------|
| `--output-dir DIR` | Output directory | `generated` |
| `--languages LANG [LANG ...]` | Target languages | all 5 |
| `--report FILE` | Write JSON report | none |
| `-v`, `--verbose` | Verbose output | off |

### Options (pipeline run)

| Option | Description | Default |
|--------|-------------|---------|
| `--output-dir DIR` | Output directory | `generated` |
| `--languages LANG [...]` | Target languages | all 5 |
| `--report FILE` | Write pipeline report | none |
| `--checkpoint DIR` | Checkpoint directory | `.atomik` |
| `--metrics-csv FILE` | Metrics CSV path | `.atomik/metrics.csv` |
| `--com-port PORT` | Serial port for hardware validation | none |
| `--token-budget N` | Maximum tokens for this run | unlimited |
| `--sim-only` | RTL simulation only (no FPGA) | off |
| `--skip-synthesis` | Skip synthesis stage | off |
| `--batch` | Process directory of schemas | off |
| `-v`, `--verbose` | Verbose output | off |

### Options (demo)

| Option | Description | Default |
|--------|-------------|---------|
| `--com-port PORT` | Serial port for FPGA | none |
| `--sim-only` | Simulation only | off |
| `--report FILE` | Write demo report | none |
| `-v`, `--verbose` | Verbose output | off |

### Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Validation failure |
| 2 | Generation failure |
| 3 | File or configuration error |

### Examples

```bash
# Generate Python + Rust from a schema
atomik-gen generate sdk/schemas/examples/terminal-io.json --languages python rust

# Validate a domain schema
atomik-gen validate sdk/schemas/domains/video-h264-delta.json

# Batch process all domain schemas with report
atomik-gen batch sdk/schemas/domains/ --output-dir generated_sdks --report report.json

# Show schema metadata
atomik-gen info sdk/schemas/domains/finance-price-tick.json

# Run autonomous pipeline on a schema
atomik-gen pipeline run sdk/schemas/examples/matrix-ops.json --sim-only

# Dry-run: show what the pipeline would change
atomik-gen pipeline diff sdk/schemas/examples/matrix-ops.json

# Show metrics from last pipeline run
atomik-gen metrics show

# Export metrics to CSV
atomik-gen metrics export --output metrics.csv

# Run the video domain hardware demonstrator (simulation)
atomik-gen demo video --sim-only
```

---

## Python SDK API

### Class: `{Object}`

Delta-state manager for Python.

#### Constructor

```python
def __init__(self) -> None
```

Creates a new delta-state manager instance.

**Example:**
```python
from atomik.System.Terminal import TerminalIO

manager = TerminalIO()
```

#### Methods

##### `load(initial_state: int) -> None`

Load initial state (LOAD operation).

**Parameters:**
- `initial_state` (int): Initial state value

**Example:**
```python
manager.load(0x1234567890ABCDEF)
```

##### `accumulate(delta: int) -> None`

Accumulate delta (ACCUMULATE operation).

XORs the delta into the accumulator.

**Parameters:**
- `delta` (int): Delta value to accumulate

**Example:**
```python
manager.accumulate(0x1111111111111111)
manager.accumulate(0x2222222222222222)
```

##### `reconstruct() -> int`

Reconstruct current state (READ operation).

Returns current_state = initial_state ⊕ accumulator

**Returns:**
- int: Current state

**Example:**
```python
current_state = manager.reconstruct()
print(f"Current state: 0x{current_state:016X}")
```

##### `is_accumulator_zero() -> bool`

Check if accumulator is zero (STATUS operation).

**Returns:**
- bool: True if accumulator is zero

**Example:**
```python
if manager.is_accumulator_zero():
    print("No pending deltas")
```

##### `rollback(count: int) -> int` (optional)

Rollback the last N delta operations.

**Parameters:**
- `count` (int): Number of deltas to rollback

**Returns:**
- int: Number of deltas actually rolled back

**Example:**
```python
rolled_back = manager.rollback(3)
print(f"Rolled back {rolled_back} deltas")
```

##### `get_accumulator() -> int`

Get the current accumulator value.

**Returns:**
- int: Accumulator value

##### `get_initial_state() -> int`

Get the initial state.

**Returns:**
- int: Initial state value

##### `history_size() -> int` (optional)

Get the number of deltas in history.

**Returns:**
- int: History size

---

## Rust SDK API

### Struct: `{Object}`

Delta-state manager for Rust.

#### Methods

##### `new() -> Self`

Create a new delta-state manager.

**Example:**
```rust
use atomik::system::terminal::TerminalIO;

let mut manager = TerminalIO::new();
```

##### `load(&mut self, initial_state: u64)`

Load initial state (LOAD operation).

**Parameters:**
- `initial_state`: Initial state value

**Example:**
```rust
manager.load(0x1234567890ABCDEF);
```

##### `accumulate(&mut self, delta: u64)`

Accumulate delta (ACCUMULATE operation).

XORs the delta into the accumulator.

**Parameters:**
- `delta`: Delta value to accumulate

**Example:**
```rust
manager.accumulate(0x1111111111111111);
manager.accumulate(0x2222222222222222);
```

##### `reconstruct(&self) -> u64`

Reconstruct current state (READ operation).

Returns current_state = initial_state ⊕ accumulator

**Returns:**
- Current state

**Example:**
```rust
let current_state = manager.reconstruct();
println!("Current state: 0x{:016X}", current_state);
```

##### `is_accumulator_zero(&self) -> bool`

Check if accumulator is zero (STATUS operation).

**Returns:**
- `true` if accumulator is zero

**Example:**
```rust
if manager.is_accumulator_zero() {
    println!("No pending deltas");
}
```

##### `rollback(&mut self, count: usize) -> usize` (optional)

Rollback the last N delta operations.

**Parameters:**
- `count`: Number of deltas to rollback

**Returns:**
- Number of deltas actually rolled back

**Example:**
```rust
let rolled_back = manager.rollback(3);
println!("Rolled back {} deltas", rolled_back);
```

##### `get_accumulator(&self) -> u64`

Get the current accumulator value.

**Returns:**
- Accumulator value

##### `get_initial_state(&self) -> u64`

Get the initial state.

**Returns:**
- Initial state value

##### `history_size(&self) -> usize` (optional)

Get the number of deltas in history.

**Returns:**
- History size

#### Traits

Implements `Default` trait (equivalent to `new()`).

---

## C SDK API

### Type: `atomik_{object}_t`

Delta-state manager struct.

#### Functions

##### `void atomik_{object}_init(atomik_{object}_t *manager)`

Initialize a new delta-state manager instance.

**Parameters:**
- `manager`: Pointer to manager struct

**Example:**
```c
#include <atomik/system/terminal/terminal_io.h>

atomik_terminal_io_t manager;
atomik_terminal_io_init(&manager);
```

##### `void atomik_{object}_load(atomik_{object}_t *manager, uint64_t initial_state)`

Load initial state (LOAD operation).

**Parameters:**
- `manager`: Pointer to manager struct
- `initial_state`: Initial state value

**Example:**
```c
atomik_terminal_io_load(&manager, 0x1234567890ABCDEFULL);
```

##### `void atomik_{object}_accumulate(atomik_{object}_t *manager, uint64_t delta)`

Accumulate delta (ACCUMULATE operation).

**Parameters:**
- `manager`: Pointer to manager struct
- `delta`: Delta value to accumulate

**Example:**
```c
atomik_terminal_io_accumulate(&manager, 0x1111111111111111ULL);
atomik_terminal_io_accumulate(&manager, 0x2222222222222222ULL);
```

##### `uint64_t atomik_{object}_reconstruct(const atomik_{object}_t *manager)`

Reconstruct current state (READ operation).

**Parameters:**
- `manager`: Pointer to manager struct (const)

**Returns:**
- Current state value

**Example:**
```c
uint64_t current_state = atomik_terminal_io_reconstruct(&manager);
printf("Current state: 0x%016llX\n", current_state);
```

##### `bool atomik_{object}_is_accumulator_zero(const atomik_{object}_t *manager)`

Check if accumulator is zero (STATUS operation).

**Parameters:**
- `manager`: Pointer to manager struct (const)

**Returns:**
- `true` if accumulator is zero

**Example:**
```c
if (atomik_terminal_io_is_accumulator_zero(&manager)) {
    printf("No pending deltas\n");
}
```

##### `size_t atomik_{object}_rollback(atomik_{object}_t *manager, size_t count)` (optional)

Rollback the last N delta operations.

**Parameters:**
- `manager`: Pointer to manager struct
- `count`: Number of deltas to rollback

**Returns:**
- Number of deltas actually rolled back

**Example:**
```c
size_t rolled_back = atomik_terminal_io_rollback(&manager, 3);
printf("Rolled back %zu deltas\n", rolled_back);
```

##### `uint64_t atomik_{object}_get_accumulator(const atomik_{object}_t *manager)`

Get the current accumulator value.

**Parameters:**
- `manager`: Pointer to manager struct (const)

**Returns:**
- Accumulator value

##### `uint64_t atomik_{object}_get_initial_state(const atomik_{object}_t *manager)`

Get the initial state.

**Parameters:**
- `manager`: Pointer to manager struct (const)

**Returns:**
- Initial state value

---

## JavaScript SDK API

### Class: `{Object}`

Delta-state manager for JavaScript/Node.js.

#### Constructor

```javascript
constructor()
```

Creates a new delta-state manager instance.

**Example:**
```javascript
import { TerminalIO } from '@atomik/system/terminal';

const manager = new TerminalIO();
```

#### Methods

##### `load(initialState)`

Load initial state (LOAD operation).

**Parameters:**
- `initialState` (Number | BigInt): Initial state value

**Example:**
```javascript
manager.load(0x1234567890ABCDEFn);
```

##### `accumulate(delta)`

Accumulate delta (ACCUMULATE operation).

XORs the delta into the accumulator.

**Parameters:**
- `delta` (Number | BigInt): Delta value to accumulate

**Example:**
```javascript
manager.accumulate(0x1111111111111111n);
manager.accumulate(0x2222222222222222n);
```

##### `reconstruct()`

Reconstruct current state (READ operation).

Returns current_state = initial_state ⊕ accumulator

**Returns:**
- Number | BigInt: Current state

**Example:**
```javascript
const currentState = manager.reconstruct();
console.log(`Current state: 0x${currentState.toString(16)}`);
```

##### `isAccumulatorZero()`

Check if accumulator is zero (STATUS operation).

**Returns:**
- boolean: True if accumulator is zero

**Example:**
```javascript
if (manager.isAccumulatorZero()) {
    console.log('No pending deltas');
}
```

##### `rollback(count)` (optional)

Rollback the last N delta operations.

**Parameters:**
- `count` (number): Number of deltas to rollback

**Returns:**
- number: Number of deltas actually rolled back

**Example:**
```javascript
const rolledBack = manager.rollback(3);
console.log(`Rolled back ${rolledBack} deltas`);
```

##### `getAccumulator()`

Get the current accumulator value.

**Returns:**
- Number | BigInt: Accumulator value

##### `getInitialState()`

Get the initial state.

**Returns:**
- Number | BigInt: Initial state value

##### `historySize()` (optional)

Get the number of deltas in history.

**Returns:**
- number: History size

---

## Verilog RTL Interface

### Module: `atomik_{vertical}_{field}_{object}`

Synthesizable Verilog RTL module.

#### Parameters

```verilog
parameter DATA_WIDTH = 64
```

Data width in bits (matches schema delta field widths).

#### Ports

##### Clock and Reset

```verilog
input wire clk         // Clock input
input wire rst_n       // Active-low reset
```

##### Control Signals

```verilog
input wire load_en        // LOAD operation enable
input wire accumulate_en  // ACCUMULATE operation enable
input wire read_en        // READ operation enable
input wire rollback_en    // ROLLBACK operation enable (optional)
```

##### Data Signals

```verilog
input wire [DATA_WIDTH-1:0] data_in   // Input data bus
output reg [DATA_WIDTH-1:0] data_out  // Output data bus (registered)
```

##### Status Signals

```verilog
output wire accumulator_zero  // High when accumulator is zero
```

#### Operation Timing

All operations are synchronous to the rising edge of `clk`.

##### LOAD Operation

```verilog
// Set load_en high for one clock cycle
// data_in is loaded as initial_state
// accumulator is reset to 0
```

##### ACCUMULATE Operation

```verilog
// Set accumulate_en high for one clock cycle
// data_in is XORed into accumulator
// If rollback enabled, delta is saved to history
```

##### READ Operation

```verilog
// Set read_en high for one clock cycle
// data_out = initial_state XOR accumulator on next clock edge
```

##### ROLLBACK Operation (optional)

```verilog
// Set rollback_en high for one clock cycle
// Last delta is XORed out of accumulator
// History pointer is decremented
```

#### Example Usage

```verilog
// Instantiate module
atomik_system_terminal_terminal_io #(
    .DATA_WIDTH(64)
) dut (
    .clk(clk),
    .rst_n(rst_n),
    .load_en(load_en),
    .accumulate_en(accumulate_en),
    .read_en(read_en),
    .data_in(data_in),
    .data_out(data_out),
    .accumulator_zero(accumulator_zero)
);

// LOAD operation
data_in = 64'hAAAAAAAAAAAAAAAA;
load_en = 1'b1;
@(posedge clk);
load_en = 1'b0;

// ACCUMULATE operation
data_in = 64'h5555555555555555;
accumulate_en = 1'b1;
@(posedge clk);
accumulate_en = 1'b0;

// READ operation
read_en = 1'b1;
@(posedge clk);
read_en = 1'b0;
// data_out now contains initial_state XOR accumulator
```

#### Internal Registers

```verilog
reg [DATA_WIDTH-1:0] initial_state;  // Initial state register
reg [DATA_WIDTH-1:0] accumulator;    // Delta accumulator
```

For rollback-enabled modules:

```verilog
reg [DATA_WIDTH-1:0] history [0:HISTORY_DEPTH-1];  // Delta history buffer
reg [$clog2(HISTORY_DEPTH):0] history_count;       // Number of deltas in history
reg [$clog2(HISTORY_DEPTH)-1:0] history_head;      // History buffer head pointer
```

#### Synthesis Notes

- All operations complete in 1 clock cycle
- ACCUMULATE latency: 1 cycle
- RECONSTRUCT latency: 0 cycles (combinatorial) + 1 cycle register
- Designed for 94.5 MHz operation (matches Phase 3 FPGA)
- LUT utilization: ~7% on GW1NR-9
- FF utilization: ~9% on GW1NR-9

---

## Pipeline API (Phase 5)

The Pipeline API provides the autonomous generation pipeline infrastructure, including orchestration, feedback loops, adaptive routing, token efficiency, parallel execution, verification, analysis, context management, and self-optimization.

### Orchestrator

Manages task dependency graphs and multi-stage pipeline execution.

#### Class: `TaskDAG`

Directed acyclic graph for tracking task dependencies and execution state.

##### `add_task(task_id: str, stage: str, dependencies: list[str] = None) -> None`

Register a task with its stage and optional dependencies.

**Parameters:**
- `task_id` (str): Unique identifier for the task
- `stage` (str): Pipeline stage the task belongs to
- `dependencies` (list[str], optional): List of task IDs that must complete before this task

##### `topological_order() -> list[str]`

Return all task IDs in a valid topological execution order.

**Returns:**
- list[str]: Task IDs sorted so that dependencies precede dependents

##### `ready_tasks() -> list[str]`

Return task IDs whose dependencies are all completed and that are not yet running or done.

**Returns:**
- list[str]: Task IDs eligible for immediate execution

##### `mark_running(task_id: str) -> None`

Transition a task to the running state.

**Parameters:**
- `task_id` (str): Task to mark as running

##### `mark_completed(task_id: str) -> None`

Transition a task to the completed state, unblocking dependent tasks.

**Parameters:**
- `task_id` (str): Task to mark as completed

##### `mark_failed(task_id: str) -> None`

Transition a task to the failed state.

**Parameters:**
- `task_id` (str): Task to mark as failed

#### Class: `PipelineOrchestrator`

Top-level orchestrator that coordinates pipeline stages end to end.

##### `__init__(event_bus: EventBus = None)`

Create a new orchestrator, optionally wired to an event bus for lifecycle events.

**Parameters:**
- `event_bus` (EventBus, optional): Event bus for emitting pipeline events

##### `add_stage(name: str, dependencies: list[str] = None) -> None`

Register a pipeline stage with optional stage-level dependencies.

**Parameters:**
- `name` (str): Stage name (e.g., "generate", "verify")
- `dependencies` (list[str], optional): Stages that must complete before this stage runs

##### `execute(runner: Callable) -> dict`

Execute the full pipeline by invoking the runner for each stage in dependency order.

**Parameters:**
- `runner` (Callable): Function called for each stage to perform its work

**Returns:**
- dict: Execution summary including per-stage results, timing, and status

##### `set_coordinator(coordinator: Coordinator) -> None`

Attach a coordinator to manage cross-stage resource sharing and scheduling.

**Parameters:**
- `coordinator` (Coordinator): Coordinator instance

##### `apply_tuning(tuning_result: TuningResult) -> None`

Apply an optimization tuning result to adjust pipeline parameters at runtime.

**Parameters:**
- `tuning_result` (TuningResult): Tuning configuration produced by ConfigTuner

---

### Event Bus

Publish-subscribe event system for decoupled communication between pipeline components.

#### Class: `EventBus`

Central event dispatcher supporting typed subscriptions and event history.

##### `subscribe(event_type: EventType, handler: Callable) -> str`

Register a handler to be called when events of the given type are emitted.

**Parameters:**
- `event_type` (EventType): The event type to listen for
- `handler` (Callable): Callback invoked with the Event object

**Returns:**
- str: Subscription ID for later unsubscription

##### `unsubscribe(subscription_id: str) -> None`

Remove a previously registered subscription.

**Parameters:**
- `subscription_id` (str): ID returned by `subscribe()`

##### `emit(event: Event) -> None`

Dispatch an event to all matching subscribers and record it in history.

**Parameters:**
- `event` (Event): Event object to emit

##### `get_history() -> list[Event]`

Retrieve the full ordered list of events emitted so far.

**Returns:**
- list[Event]: All emitted events in chronological order

---

### Feedback Loop

Iterative error diagnosis and repair loop combining knowledge base lookups and LLM-driven fixes.

#### Class: `FeedbackLoop`

Runs multiple correction iterations to resolve generation errors.

##### `__init__(max_depth: int = 3, event_bus: EventBus = None)`

Create a feedback loop with a maximum retry depth.

**Parameters:**
- `max_depth` (int): Maximum number of correction iterations (default 3)
- `event_bus` (EventBus, optional): Event bus for emitting feedback events

##### `run(language, initial_errors, classify, kb_lookup, apply_fix, llm_diagnose, verify, kb_record=None) -> FeedbackResult`

Execute the feedback loop until errors are resolved or retries are exhausted.

**Parameters:**
- `language`: Target language being corrected
- `initial_errors`: Error list from the initial generation attempt
- `classify` (Callable): Function to classify an error into an error class
- `kb_lookup` (Callable): Function to look up a known fix in the knowledge base
- `apply_fix` (Callable): Function to apply a fix to the generated code
- `llm_diagnose` (Callable): Function to invoke the LLM for diagnosis when KB has no match
- `verify` (Callable): Function to verify the corrected code
- `kb_record` (Callable, optional): Function to record a newly discovered fix in the KB

**Returns:**
- FeedbackResult: Outcome, iteration details, and token usage

#### Class: `FeedbackResult`

Result of a completed feedback loop execution.

**Fields:**
- `outcome` (FeedbackOutcome): Resolution status -- one of `FIXED_BY_KB`, `FIXED_BY_LLM`, `RETRY_EXHAUSTED`, or `IDENTICAL_ERROR`
- `resolved` (bool): True if all errors were resolved
- `iterations` (list[FeedbackIteration]): Per-iteration details including errors and fixes applied
- `total_tokens` (int): Total LLM tokens consumed across all iterations

---

### Adaptive Router

Selects the appropriate model tier for each pipeline stage based on schema complexity, budget pressure, and historical success rates.

#### Class: `AdaptiveRouter`

Routes pipeline tasks to model tiers dynamically.

##### `route(stage_name: str, schema: dict = None, schema_hash: str = "", budget_pressure: float = 0.0, cache_hit: bool = False) -> ModelTier`

Determine the model tier for a given stage and context.

**Parameters:**
- `stage_name` (str): Pipeline stage requesting a model
- `schema` (dict, optional): Schema being processed
- `schema_hash` (str): Hash of the schema for cache and history lookups
- `budget_pressure` (float): Value between 0.0 (no pressure) and 1.0 (maximum pressure)
- `cache_hit` (bool): Whether a prompt cache hit was found for this task

**Returns:**
- ModelTier: Selected model tier (e.g., FAST, STANDARD, ADVANCED)

##### `record_failure(schema_hash: str) -> None`

Record that a generation attempt failed for the given schema, influencing future routing.

**Parameters:**
- `schema_hash` (str): Hash of the schema that failed

##### `record_success(schema_hash: str) -> None`

Record that a generation attempt succeeded for the given schema.

**Parameters:**
- `schema_hash` (str): Hash of the schema that succeeded

##### `get_decisions() -> list[dict]`

Retrieve the log of all routing decisions made during the current session.

**Returns:**
- list[dict]: List of decision records with stage, schema hash, tier, and rationale

---

### Token Efficiency

Tools for predicting, caching, and compressing token usage across pipeline stages.

#### Class: `TokenPredictor`

Predicts token consumption for pipeline tasks based on historical data.

##### `predict(task_type: str) -> int`

Return the predicted token count for the given task type.

**Parameters:**
- `task_type` (str): Type of task (e.g., "generate_python", "verify_rust")

**Returns:**
- int: Predicted token count

##### `record_actual(task_type: str, tokens: int) -> None`

Record the actual token count observed for a completed task, updating the prediction model.

**Parameters:**
- `task_type` (str): Type of task
- `tokens` (int): Actual tokens consumed

##### `predict_and_track(task_type: str) -> int`

Predict token count and register a tracking entry for later reconciliation.

**Parameters:**
- `task_type` (str): Type of task

**Returns:**
- int: Predicted token count

##### `predict_remaining(remaining_tasks: list[str]) -> int`

Estimate the total tokens needed for all remaining tasks in the pipeline.

**Parameters:**
- `remaining_tasks` (list[str]): List of task types still to execute

**Returns:**
- int: Aggregate predicted token count

#### Class: `PromptCache`

In-memory cache for prompt responses keyed by task and schema hash.

##### `get(task_key: str, schema_hash: str) -> str | None`

Look up a cached response for the given task and schema.

**Parameters:**
- `task_key` (str): Task identifier
- `schema_hash` (str): Hash of the schema

**Returns:**
- str | None: Cached response string, or None on cache miss

##### `put(task_key: str, schema_hash: str, response: str) -> None`

Store a response in the cache.

**Parameters:**
- `task_key` (str): Task identifier
- `schema_hash` (str): Hash of the schema
- `response` (str): Response string to cache

##### `hit_rate() -> float`

Return the cache hit rate as a percentage.

**Returns:**
- float: Hit rate from 0 to 100

##### `invalidate_schema(schema_hash: str) -> int`

Remove all cached entries for a given schema hash.

**Parameters:**
- `schema_hash` (str): Hash of the schema to invalidate

**Returns:**
- int: Number of entries removed

#### Class: `ContextCompressor`

Reduces context size under token budget pressure.

##### `compress(text: str, budget_pressure: float) -> str`

Compress a text block by removing low-priority content based on budget pressure.

**Parameters:**
- `text` (str): Input text to compress
- `budget_pressure` (float): Value between 0.0 (no compression) and 1.0 (aggressive compression)

**Returns:**
- str: Compressed text

##### `compress_schema_context(schema: dict, budget_pressure: float) -> dict`

Compress a schema context dictionary, stripping optional fields under pressure.

**Parameters:**
- `schema` (dict): Schema context to compress
- `budget_pressure` (float): Value between 0.0 (no compression) and 1.0 (aggressive compression)

**Returns:**
- dict: Compressed schema context

---

### Error Knowledge Base

Persistent store of known error patterns and their fixes, enabling instant repair without LLM calls.

#### Class: `ErrorKnowledgeBase`

Stores, retrieves, and learns error-fix mappings.

##### `add_pattern(pattern: ErrorPattern) -> None`

Add a known error pattern and its fix to the knowledge base.

**Parameters:**
- `pattern` (ErrorPattern): Error pattern object containing language, error class, message pattern, and fix description

##### `lookup(language: str, error_class: str, error_msg: str) -> KBLookupResult`

Search the knowledge base for a matching fix.

**Parameters:**
- `language` (str): Programming language (e.g., "python", "rust")
- `error_class` (str): Classified error category
- `error_msg` (str): Raw error message text

**Returns:**
- KBLookupResult: Match result containing the fix description and confidence, or no match

##### `learn(language, error_class, error_message, fix_description) -> ErrorPattern`

Record a newly discovered error-fix pair learned from an LLM correction.

**Parameters:**
- `language`: Programming language
- `error_class`: Classified error category
- `error_message`: Error message that was resolved
- `fix_description`: Description of the fix applied

**Returns:**
- ErrorPattern: Newly created pattern added to the knowledge base

##### `load_seed() -> int`

Load seed error patterns from the built-in pattern library.

**Returns:**
- int: Number of seed patterns loaded

##### `record_success(pattern_id: str) -> None`

Increment the success counter for a pattern, boosting its ranking in future lookups.

**Parameters:**
- `pattern_id` (str): ID of the pattern that was successfully applied

---

### Parallel Execution

Decomposes pipeline work into independent tasks and executes them concurrently.

#### Class: `TaskDecomposer`

Breaks pipeline stages into parallelizable task sets.

##### `decompose_generation(languages: list[str] = None) -> DecompositionPlan`

Decompose the generation stage into per-language tasks that can run in parallel.

**Parameters:**
- `languages` (list[str], optional): Target languages; defaults to all configured languages

**Returns:**
- DecompositionPlan: Plan describing independent and dependent task groups

##### `decompose_verification() -> DecompositionPlan`

Decompose the verification stage into parallelizable verification tasks.

**Returns:**
- DecompositionPlan: Plan describing independent and dependent verification tasks

##### `decompose_full_pipeline() -> DecompositionPlan`

Decompose the entire pipeline into a maximal set of parallelizable tasks.

**Returns:**
- DecompositionPlan: Full pipeline decomposition plan

#### Class: `ParallelExecutor`

Executes a set of tasks concurrently up to a worker limit.

##### `__init__(max_workers: int = 4)`

Create an executor with a bounded worker pool.

**Parameters:**
- `max_workers` (int): Maximum number of concurrent workers (default 4)

##### `execute(tasks: list[ParallelTask], executor_fn: Callable) -> ExecutionResults`

Run all tasks using the provided executor function, respecting the worker limit.

**Parameters:**
- `tasks` (list[ParallelTask]): Tasks to execute
- `executor_fn` (Callable): Function invoked for each task

**Returns:**
- ExecutionResults: Aggregated results including per-task outcomes, timing, and any failures

#### Class: `Worker`

Individual worker that runs a single task with optional timeout.

##### `__init__(worker_id: str)`

Create a worker with a unique identifier.

**Parameters:**
- `worker_id` (str): Unique worker identifier

##### `run(fn: Callable, timeout: float = None) -> WorkerResult`

Execute a function with an optional timeout.

**Parameters:**
- `fn` (Callable): Function to execute
- `timeout` (float, optional): Maximum execution time in seconds

**Returns:**
- WorkerResult: Result containing the return value or error, plus elapsed time

---

### Verification

Deep verification and cross-language consistency checking for generated code.

#### Class: `DeepVerifier`

Runs comprehensive verification across all generated languages.

##### `verify_all(file_map: dict[str, list], verification_depth: str = "full") -> DeepVerifyResult`

Verify all generated files across all languages.

**Parameters:**
- `file_map` (dict[str, list]): Mapping of language name to list of generated file paths
- `verification_depth` (str): Depth of verification -- "fast", "standard", or "full" (default "full")

**Returns:**
- DeepVerifyResult: Aggregated verification result with per-language outcomes and summary

##### `verify_language(language: str, files: list) -> RunnerResult`

Verify the generated files for a single language.

**Parameters:**
- `language` (str): Language to verify (e.g., "python", "rust")
- `files` (list): List of file paths to verify

**Returns:**
- RunnerResult: Verification result for the specified language

#### Class: `ConsistencyChecker`

Checks semantic equivalence across generated SDK implementations.

##### `check(file_map: dict[str, list]) -> ConsistencyReport`

Check cross-language consistency from generated file paths.

**Parameters:**
- `file_map` (dict[str, list]): Mapping of language name to list of generated file paths

**Returns:**
- ConsistencyReport: Report detailing any inconsistencies found across languages

##### `check_from_interfaces(interfaces: dict[str, LanguageInterface]) -> ConsistencyReport`

Check cross-language consistency from pre-parsed interface definitions.

**Parameters:**
- `interfaces` (dict[str, LanguageInterface]): Mapping of language name to parsed interface

**Returns:**
- ConsistencyReport: Report detailing any inconsistencies found across languages

---

### Analysis

Metrics analysis, regression detection, and schema diffing for pipeline runs.

#### Class: `MetricsAnalyzer`

Statistical analysis utilities for pipeline metrics.

##### `moving_average(values: list[float], window: int) -> list[float]`

Compute a simple moving average over a series of values.

**Parameters:**
- `values` (list[float]): Input values
- `window` (int): Window size for the moving average

**Returns:**
- list[float]: Smoothed values

##### `analyze_trends(metric_history: dict) -> dict`

Analyze trends across multiple metrics over time.

**Parameters:**
- `metric_history` (dict): Mapping of metric names to lists of historical values

**Returns:**
- dict: Trend analysis per metric including direction, slope, and statistical significance

##### `detect_anomalies(values: list[float]) -> list[int]`

Identify anomalous data points in a series.

**Parameters:**
- `values` (list[float]): Input values

**Returns:**
- list[int]: Indices of values detected as anomalies

#### Class: `RegressionDetector`

Detects performance or quality regressions between pipeline runs.

##### `detect(history: list[dict], current: dict) -> RegressionReport`

Compare the current run against historical runs to detect regressions.

**Parameters:**
- `history` (list[dict]): List of historical run summaries
- `current` (dict): Current run summary

**Returns:**
- RegressionReport: Report listing detected regressions with severity and affected metrics

#### Class: `FieldDiff`

Compares schema versions to identify structural changes.

##### `compare(old_schema: dict, new_schema: dict) -> FieldDiffResult`

Compute a diff between two schema versions.

**Parameters:**
- `old_schema` (dict): Previous schema version
- `new_schema` (dict): New schema version

**Returns:**
- FieldDiffResult: Diff result listing added, removed, and modified fields

---

### Context Management

Pipeline manifest tracking and intelligent context window management.

#### Class: `PipelineManifest`

Tracks schemas, runs, and context segments across pipeline executions.

##### `register_schema(name: str, content_hash: str, ...) -> None`

Register a schema with its content hash and metadata.

**Parameters:**
- `name` (str): Schema name
- `content_hash` (str): Hash of the schema content

##### `record_run(run_data: dict) -> None`

Record a completed pipeline run for historical tracking.

**Parameters:**
- `run_data` (dict): Run summary data including timing, results, and configuration

##### `register_segment(segment_id: str, metadata: SegmentMetadata) -> None`

Register a context segment with its metadata for intelligent context assembly.

**Parameters:**
- `segment_id` (str): Unique segment identifier
- `metadata` (SegmentMetadata): Segment metadata including type, priority, and token estimate

#### Class: `IntelligentContextManager`

Assembles and manages the LLM context window to maximize utilization without overflow.

##### `__init__(max_tokens: int = 128000, utilization_limit: float = 0.9)`

Create a context manager with a token budget.

**Parameters:**
- `max_tokens` (int): Maximum context window size in tokens (default 128000)
- `utilization_limit` (float): Target utilization fraction (default 0.9)

##### `add_segment(segment_id: str, content: str, segment_type: str) -> None`

Add a content segment to the context pool.

**Parameters:**
- `segment_id` (str): Unique segment identifier
- `content` (str): Segment text content
- `segment_type` (str): Type of segment (e.g., "schema", "instruction", "example")

##### `build_context() -> str`

Assemble the final context string from registered segments, respecting the token budget.

**Returns:**
- str: Assembled context string fitting within the token budget

##### `get_context_for_cold_start() -> str`

Build a minimal context suitable for cold-start scenarios with no prior run history.

**Returns:**
- str: Minimal context string for first-run initialization

---

### Self-Optimization

Automatic performance tracking and configuration tuning across pipeline runs.

#### Class: `SelfOptimizer`

Tracks run metrics and generates optimization reports.

##### `__init__(report_every: int = 5)`

Create a self-optimizer that reports every N runs.

**Parameters:**
- `report_every` (int): Number of runs between optimization reports (default 5)

##### `record_run(run_data: dict) -> None`

Record metrics from a completed pipeline run.

**Parameters:**
- `run_data` (dict): Run summary including timing, token usage, and success rates

##### `should_report() -> bool`

Check whether enough runs have accumulated to generate a new optimization report.

**Returns:**
- bool: True if the reporting threshold has been reached

##### `generate_report() -> OptimizationReport`

Generate an optimization report with recommendations based on accumulated run data.

**Returns:**
- OptimizationReport: Report containing performance trends, bottleneck analysis, and tuning suggestions

#### Class: `ConfigTuner`

Tunes pipeline configuration parameters based on historical run data.

##### `tune_workers(run_history: list[dict], current_workers: int = 4) -> TuningResult`

Recommend an optimal worker count based on observed parallelism efficiency.

**Parameters:**
- `run_history` (list[dict]): Historical run summaries
- `current_workers` (int): Current worker pool size (default 4)

**Returns:**
- TuningResult: Tuning recommendation including suggested worker count and confidence

##### `tune_retry_depth(run_history: list[dict], current_depth: int = 3) -> TuningResult`

Recommend an optimal feedback loop retry depth based on historical fix rates.

**Parameters:**
- `run_history` (list[dict]): Historical run summaries
- `current_depth` (int): Current maximum retry depth (default 3)

**Returns:**
- TuningResult: Tuning recommendation including suggested retry depth and confidence

---

## Cross-Language Equivalence

All SDK implementations provide semantically equivalent operations:

| Operation | Python | Rust | C | JavaScript | Verilog |
|-----------|--------|------|---|------------|---------|
| Initialize | `__init__()` | `new()` | `init()` | `constructor()` | reset |
| Load | `load()` | `load()` | `load()` | `load()` | `load_en` |
| Accumulate | `accumulate()` | `accumulate()` | `accumulate()` | `accumulate()` | `accumulate_en` |
| Reconstruct | `reconstruct()` | `reconstruct()` | `reconstruct()` | `reconstruct()` | `read_en` |
| Status | `is_accumulator_zero()` | `is_accumulator_zero()` | `is_accumulator_zero()` | `isAccumulatorZero()` | `accumulator_zero` |
| Rollback | `rollback()` | `rollback()` | `rollback()` | `rollback()` | `rollback_en` |

### Mathematical Guarantees

All implementations guarantee:

1. **XOR Algebra**: `accumulator = delta_1 ⊕ delta_2 ⊕ ... ⊕ delta_n`
2. **Self-Inverse**: `accumulate(Δ); accumulate(Δ)` results in `accumulator = 0`
3. **Commutativity**: Order of deltas doesn't affect final accumulator
4. **Associativity**: Grouping of XOR operations doesn't matter
5. **Identity**: `0 ⊕ Δ = Δ`

---

## Error Handling

### Python

Raises `ValueError` for invalid operations:
- Loading invalid state values
- Rollback with count > history size (returns actual count, no exception)

### Rust

Uses Rust's type system for safety:
- All operations are infallible
- `rollback()` returns actual count rolled back

### C

No error codes returned:
- Caller responsible for valid inputs
- Rollback returns actual count (may be less than requested)

### JavaScript

Throws `TypeError` for invalid types:
- All operations otherwise infallible
- `rollback()` returns actual count rolled back

### Verilog

Hardware behavior:
- Invalid states handled by reset
- Operations complete in fixed time regardless of inputs
- No error signals (STATUS indicates accumulator state only)

---

**Document Version:** 2.0.0
**SDK Version:** 2.0.0
**Last Updated:** January 27, 2026
