# ATOMiK SDK Schema Guide

**Version**: 1.0.0
**Date**: January 26, 2026
**Audience**: SDK developers, integration engineers, technical users

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Schema Structure](#2-schema-structure)
3. [Field Type Reference](#3-field-type-reference)
4. [Operations Reference](#4-operations-reference)
5. [Constraints](#5-constraints)
6. [Validation](#6-validation)
7. [Examples](#7-examples)

---

## 1. Introduction

### 1.1 Purpose

The ATOMiK JSON schema specification drives **multi-language SDK generation** from a single declarative source. This enables:

- **Consistency**: Same computational behavior across Python, Rust, C, Verilog, and JavaScript
- **Multi-language support**: Generate SDKs for 5+ target languages from one schema
- **Hardware readiness**: Optional Verilog RTL generation for FPGA/ASIC deployment
- **Namespace automation**: Catalogue position determines API import paths

### 1.2 Benefits

| Benefit | Description |
|---------|-------------|
| **Write Once, Deploy Everywhere** | Define delta operations once, generate code for all platforms |
| **Type Safety** | JSON Schema validation catches errors before code generation |
| **Discoverability** | Hierarchical catalogue structure enables ecosystem browsing |
| **Hardware/Software Co-design** | Same schema generates both software SDK and hardware RTL |
| **Version Management** | Semantic versioning built into schema metadata |

### 1.3 Core Concepts

**Delta-State Computing**: Instead of storing full state, ATOMiK maintains:
- **Initial state** (S₀): The base state at time t=0
- **Delta accumulator** (Δ): XOR composition of all deltas
- **Current state**: S = S₀ ⊕ Δ (single XOR operation)

**Catalogue-Driven Namespaces**: The schema's catalogue position automatically determines:
- Import paths in all languages
- File/module structure
- Package naming conventions

**Example**:
```
Catalogue: {vertical: "Video", field: "Stream", object: "H264Delta"}
→ Python:     from atomik.Video.Stream import H264Delta
→ Rust:       use atomik::video::stream::H264Delta;
→ C:          #include <atomik/video/stream/h264_delta.h>
→ JavaScript: const {H264Delta} = require('@atomik/video/stream');
→ Verilog:    module atomik_video_stream_h264_delta
```

---

## 2. Schema Structure

An ATOMiK schema consists of three main sections:

### 2.1 Catalogue (Required)

**Purpose**: Positioning metadata that determines API namespace and module identity.

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

### 2.2 Schema (Required)

**Purpose**: Computational definition of delta fields, operations, and constraints.

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

### 2.3 Hardware (Optional)

**Purpose**: Hardware mapping for Verilog RTL generation.

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

---

## 3. Field Type Reference

### 3.1 Delta Field Types

ATOMiK supports three fundamental delta field types:

#### 3.1.1 `delta_stream`

**Purpose**: Continuous stream of deltas (e.g., video frames, sensor readings).

**Characteristics**:
- High-frequency updates
- Time-series data
- Typically compressed

**Use Cases**:
- Video frame deltas
- Audio sample deltas
- Network packet streams
- Sensor data streams

**Example**:
```json
{
  "network_delta": {
    "type": "delta_stream",
    "width": 128,
    "compression": "xor"
  }
}
```

#### 3.1.2 `bitmask_delta`

**Purpose**: Bit-level state changes (e.g., flags, status bits).

**Characteristics**:
- Sparse updates (few bits change)
- Boolean state tracking
- Efficient for large bitmaps

**Use Cases**:
- Device status flags
- Feature enable/disable
- Permission bits
- Configuration registers

**Example**:
```json
{
  "status_flags": {
    "type": "bitmask_delta",
    "width": 32,
    "encoding": "raw"
  }
}
```

#### 3.1.3 `parameter_delta`

**Purpose**: Configuration/parameter updates (e.g., settings, control values).

**Characteristics**:
- Infrequent updates
- Full-width values
- Human-readable intent

**Use Cases**:
- Configuration changes
- Control commands
- Parameter tuning
- System settings

**Example**:
```json
{
  "config_delta": {
    "type": "parameter_delta",
    "width": 64,
    "default_value": 0
  }
}
```

### 3.2 Delta Field Properties

#### 3.2.1 Width

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

#### 3.2.2 Encoding

**Required**: No
**Type**: String (enum)
**Values**: `spatiotemporal_4x4x4`, `raw`, `rle`
**Default**: `raw`

**Encoding Options**:

| Encoding | Description | Use Case |
|----------|-------------|----------|
| `raw` | No encoding, direct bit representation | Default, maximum speed |
| `spatiotemporal_4x4x4` | 4×4×4 block encoding | Video/image deltas |
| `rle` | Run-length encoding | Sparse data, long zero runs |

**Example**:
```json
{
  "video_delta": {
    "type": "delta_stream",
    "width": 256,
    "encoding": "spatiotemporal_4x4x4"
  }
}
```

#### 3.2.3 Compression

**Required**: No
**Type**: String (enum)
**Values**: `xor`, `rle`, `none`
**Default**: `none`

**Compression Options**:

| Compression | Description | Ratio | Speed |
|-------------|-------------|-------|-------|
| `none` | No compression | 1:1 | Fastest |
| `xor` | XOR-based delta compression | 10-100:1 | Fast |
| `rle` | Run-length encoding | 5-50:1 | Medium |

**Compression Trade-offs**:
- `none`: Maximum speed, no size reduction
- `xor`: Good balance, hardware-friendly (Phase 2: 95% memory reduction)
- `rle`: Best for sparse data with long runs

#### 3.2.4 Default Value

**Required**: No
**Type**: Integer (≥0)
**Default**: 0

Initial value for the delta field at system startup.

**Example**:
```json
{
  "counter_delta": {
    "type": "parameter_delta",
    "width": 32,
    "default_value": 100
  }
}
```

---

## 4. Operations Reference

### 4.1 Accumulate (Required)

**Purpose**: XOR-based delta accumulation (the core operation).

**Mathematical Form**:
```
δ_accumulated ← δ_accumulated ⊕ δ_new
```

**Properties** (proven in Phase 1 Lean4 proofs):
- **Commutative**: δ₁ ⊕ δ₂ = δ₂ ⊕ δ₁
- **Associative**: (δ₁ ⊕ δ₂) ⊕ δ₃ = δ₁ ⊕ (δ₂ ⊕ δ₃)
- **Self-Inverse**: δ ⊕ δ = 0
- **Identity**: δ ⊕ 0 = δ

**Hardware Performance** (Phase 3 validation):
- **Latency**: 1 clock cycle @ 94.5 MHz
- **Throughput**: 94.5 million deltas/second
- **Resource**: ~160 LUTs (7% of GW1NR-9)

**Schema Definition**:
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

### 4.2 Reconstruct

**Purpose**: State reconstruction from accumulated deltas.

**Mathematical Form**:
```
S_current = S_initial ⊕ δ_accumulated
```

**Complexity**:
- **Software**: O(N) if maintaining delta history, O(1) if maintaining accumulator
- **Hardware**: O(1) - single XOR operation (Phase 3 validated)

**Performance**:
- **Latency**: 1 clock cycle (combinational + output register)
- **No Penalty**: Phase 3 eliminated the 32% read penalty observed in Phase 2 software

**Schema Definition**:
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

**Use Cases**:
- Reading current state
- Checkpointing
- State export

### 4.3 Rollback

**Purpose**: Temporal state rollback via delta reversal.

**Mathematical Form**:
```
S_t-1 = S_t ⊕ δ_t  (reverse last delta)
```

**Key Property**: Self-inverse property (δ ⊕ δ = 0) enables perfect rollback.

**Requirements**:
- If `enabled` is `true`, `history_depth` must be specified
- History depth determines how many deltas to store

**Schema Definition**:
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

**Use Cases**:
- Undo/redo functionality
- Distributed consensus (conflict resolution)
- Time-travel debugging
- Checkpointing

**Memory Cost**:
```
memory_bytes = history_depth × (delta_width / 8)
Example: 256 entries × (128 bits / 8) = 4 KB
```

---

## 5. Constraints

Constraints define resource and performance limits for the generated SDK.

### 5.1 Memory Constraints

**Field**: `max_memory_mb`
**Type**: Integer (1 - 65,536 MB)
**Purpose**: Maximum memory usage for edge deployment.

**Example**:
```json
{
  "constraints": {
    "max_memory_mb": 8
  }
}
```

**Guidance**:
- **Edge devices**: 1-16 MB
- **Mobile**: 16-256 MB
- **Server**: 256+ MB

### 5.2 Power Constraints

**Field**: `max_power_mw`
**Type**: Integer (1 - 100,000 mW)
**Purpose**: Maximum power consumption for embedded systems.

**Example**:
```json
{
  "constraints": {
    "max_power_mw": 500
  }
}
```

**Guidance**:
- **Battery-powered**: 100-1,000 mW
- **USB-powered**: 1,000-5,000 mW
- **Mains-powered**: 5,000+ mW

### 5.3 Latency Constraints

**Field**: `update_latency_ms`
**Type**: Integer (0 - 10,000 ms)
**Purpose**: Maximum acceptable update latency for real-time applications.

**Example**:
```json
{
  "constraints": {
    "update_latency_ms": 10
  }
}
```

**Guidance**:
- **Real-time control**: 0-10 ms
- **Interactive**: 10-100 ms
- **Batch processing**: 100+ ms

### 5.4 Hardware Target

**Field**: `target_frequency_mhz`
**Type**: Number (1.0 - 1,000.0 MHz)
**Default**: 94.5 MHz
**Purpose**: Target clock frequency for hardware implementation.

**Example**:
```json
{
  "constraints": {
    "target_frequency_mhz": 94.5
  }
}
```

**Phase 3 Validation**:
- **Achieved**: 94.9 MHz Fmax on Gowin GW1NR-9
- **Timing margin**: +0.5%

---

## 6. Validation

### 6.1 JSON Schema Validation

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

### 6.2 Common Validation Errors

See [`specs/schema_validation_rules.md`](../specs/schema_validation_rules.md) for detailed error descriptions and fixes.

### 6.3 Validation Checklist

- [ ] Schema validates against `atomik_schema_v1.json`
- [ ] All required fields present (vertical, field, object, version, delta_fields, accumulate)
- [ ] Delta field widths are powers of 2 (8, 16, 32, 64, 128, 256)
- [ ] Object names are valid identifiers in all target languages
- [ ] If rollback enabled, history_depth is specified
- [ ] If hardware.rtl_params.DATA_WIDTH specified, matches delta field widths
- [ ] Semantic version follows semver format

---

## 7. Examples

### 7.1 Example 1: Terminal I/O (Control Primitive)

**File**: [`sdk/schemas/examples/terminal-io.json`](../sdk/schemas/examples/terminal-io.json)

**Vertical**: System
**Field**: Terminal
**Object**: TerminalIO

**Purpose**: Demonstrates basic I/O control with delta-based state.

**Key Features**:
- Two 64-bit delta fields (command, response)
- Minimal memory footprint (< 1 MB)
- Low latency (< 1 ms)
- Hardware-ready (GW1NR-9 target)

**Generated Namespace**:
- Python: `from atomik.System.Terminal import TerminalIO`
- Rust: `use atomik::system::terminal::TerminalIO;`

**Use Cases**:
- Command-line interfaces
- Serial terminal emulation
- Remote control protocols

### 7.2 Example 2: P2P Delta Exchange (Network Primitive)

**File**: [`sdk/schemas/examples/p2p-delta.json`](../sdk/schemas/examples/p2p-delta.json)

**Vertical**: Network
**Field**: P2P
**Object**: DeltaExchange

**Purpose**: Peer-to-peer delta synchronization for distributed state management.

**Key Features**:
- 128-bit delta stream
- XOR compression
- Rollback capability (256 entry history)
- Conflict resolution support

**Generated Namespace**:
- Python: `from atomik.Network.P2P import DeltaExchange`
- Rust: `use atomik::network::p2p::DeltaExchange;`

**Use Cases**:
- Distributed databases
- Collaborative editing
- Blockchain state sync
- IoT mesh networks

### 7.3 Example 3: Matrix Operations (Compute Primitive)

**File**: [`sdk/schemas/examples/matrix-ops.json`](../sdk/schemas/examples/matrix-ops.json)

**Vertical**: Compute
**Field**: Linear
**Object**: MatrixOps

**Purpose**: Matrix operations with delta-based computation for efficient incremental updates.

**Key Features**:
- 256-bit wide delta field
- Spatiotemporal encoding (4×4×4 blocks)
- Parallel hardware acceleration
- Optimized for sparse updates

**Generated Namespace**:
- Python: `from atomik.Compute.Linear import MatrixOps`
- Rust: `use atomik::compute::linear::MatrixOps;`

**Use Cases**:
- Machine learning (gradient updates)
- Scientific computing (iterative solvers)
- Computer graphics (transformation matrices)
- Quantum simulation (state evolution)

### 7.4 Domain SDK: Video H.264 Delta (Phase 4B)

**File**: [`sdk/schemas/domains/video-h264-delta.json`](../sdk/schemas/domains/video-h264-delta.json)

**Vertical**: Video
**Field**: Streaming
**Object**: H264Delta

**Purpose**: Delta-based video frame processing for H.264 streams with motion vector tracking.

**Key Features**:
- 256-bit `frame_delta` with spatiotemporal 4x4x4 encoding and XOR compression
- 256-bit `motion_vector` parameter delta
- Rollback support (512 frame history)
- Hardware-optimized for speed at 150 MHz target

**Generated Namespace**:
- Python: `from atomik.Video.Streaming import H264Delta`
- Rust: `use atomik::video::streaming::H264Delta;`

### 7.5 Domain SDK: Edge Sensor IMU Fusion (Phase 4B)

**File**: [`sdk/schemas/domains/edge-sensor-imu.json`](../sdk/schemas/domains/edge-sensor-imu.json)

**Vertical**: Edge
**Field**: Sensor
**Object**: IMUFusion

**Purpose**: Edge sensor fusion for IMU devices with anomaly detection via bitmask alerts.

**Key Features**:
- 64-bit `motion_delta` stream for accelerometer/gyroscope data
- 64-bit `alert_flags` bitmask delta for anomaly detection
- Rollback support (1024 sample history)
- Power-optimized hardware at 100 MHz target, 500 mW budget

**Generated Namespace**:
- Python: `from atomik.Edge.Sensor import IMUFusion`
- Rust: `use atomik::edge::sensor::IMUFusion;`

### 7.6 Domain SDK: Financial Price Tick (Phase 4B)

**File**: [`sdk/schemas/domains/finance-price-tick.json`](../sdk/schemas/domains/finance-price-tick.json)

**Vertical**: Finance
**Field**: Trading
**Object**: PriceTick

**Purpose**: High-frequency price tick delta processing with deep rollback for transaction audit.

**Key Features**:
- 64-bit `price_delta` (parameter delta for bid/ask changes)
- 64-bit `volume_delta` (delta stream with XOR compression)
- 64-bit `trade_flags` (bitmask delta for trade status)
- Rollback support (4096 transaction history)
- Speed-optimized hardware at 400 MHz target, 1 ms latency

**Generated Namespace**:
- Python: `from atomik.Finance.Trading import PriceTick`
- Rust: `use atomik::finance::trading::PriceTick;`

---

## Appendix A: Vertical Catalog

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

## Appendix B: Reference to Phase 1-3 Results

### Phase 1: Mathematical Proofs

92 theorems proven in Lean4, including:
- `delta_comm`: Commutativity (δ₁ ⊕ δ₂ = δ₂ ⊕ δ₁)
- `delta_assoc`: Associativity ((δ₁ ⊕ δ₂) ⊕ δ₃ = δ₁ ⊕ (δ₂ ⊕ δ₃))
- `delta_self_inverse`: Self-inverse (δ ⊕ δ = 0)
- `transition_compose`: State reconstruction correctness

See: [`math/proofs/ATOMiK/`](../math/proofs/ATOMiK/)

### Phase 2: Software Benchmarks

- Memory traffic: **95-100% reduction**
- Write-heavy speed: **+22% to +55%**
- Parallel efficiency: **0.85** (85%)

See: [`math/benchmarks/results/PERFORMANCE_COMPARISON.md`](../math/benchmarks/results/PERFORMANCE_COMPARISON.md)

### Phase 3: Hardware Validation

- FPGA: Gowin GW1NR-9 (Tang Nano 9K)
- Frequency: **94.9 MHz** achieved
- Resource: **7% logic, 9% registers**
- Tests: **10/10 passing**

See: [`archive/PHASE_3_COMPLETION_REPORT.md`](../archive/PHASE_3_COMPLETION_REPORT.md)

---

*SDK Schema Guide v1.1.0 - January 26, 2026*
*ATOMiK Project - Phase 4B (3 domain SDK examples added)*
