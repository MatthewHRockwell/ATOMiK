# ATOMiK SDK API Reference

**Version:** 1.0.0
**Last Updated:** January 26, 2026

## Table of Contents

1. [Overview](#overview)
2. [CLI Tool Reference](#cli-tool-reference)
3. [Python SDK API](#python-sdk-api)
4. [Rust SDK API](#rust-sdk-api)
5. [C SDK API](#c-sdk-api)
6. [JavaScript SDK API](#javascript-sdk-api)
7. [Verilog RTL Interface](#verilog-rtl-interface)

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
| `atomik-gen --version` | Show version |

### Options (generate/batch)

| Option | Description | Default |
|--------|-------------|---------|
| `--output-dir DIR` | Output directory | `generated` |
| `--languages LANG [LANG ...]` | Target languages | all 5 |
| `--report FILE` | Write JSON report | none |
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

**Document Version:** 1.0.0
**SDK Version:** 1.0.0
**Last Updated:** January 26, 2026
