# ATOMiK Schema Validation Rules

**Version**: 1.0.0
**Date**: January 26, 2026
**Schema**: atomik_schema_v1.json

---

## Overview

This document specifies the validation rules for ATOMiK JSON schemas. All schemas must comply with JSON Schema Draft 7 and the additional semantic rules defined here.

---

## Required Fields

### Catalogue Section

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `vertical` | string (enum) | **Yes** | Must be one of: Video, Edge, Network, Finance, Science, Compute, System, Storage |
| `field` | string | **Yes** | PascalCase identifier (1-64 chars), starts with uppercase letter |
| `object` | string | **Yes** | PascalCase identifier (1-64 chars), must be unique within vertical+field |
| `version` | string | **Yes** | Valid semantic version (e.g., "1.0.0", "2.1.3-beta") |
| `author` | string | No | Author or organization name (1-256 chars) |
| `license` | string | No | SPDX license identifier (e.g., "MIT", "Apache-2.0") |
| `description` | string | No | Human-readable description (1-1024 chars) |

### Schema Section

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `delta_fields` | object | **Yes** | At least one delta field must be defined |
| `operations.accumulate` | object | **Yes** | Accumulate operation must be defined and enabled |
| `operations.reconstruct` | object | No | State reconstruction operation |
| `operations.rollback` | object | No | Temporal rollback operation |
| `constraints` | object | No | Resource and performance constraints |

### Hardware Section (Optional)

All fields in the `hardware` section are optional. If specified, they must conform to the schema.

---

## Type Constraints

### Delta Fields

Each delta field must specify:

- **type**: One of `delta_stream`, `bitmask_delta`, or `parameter_delta`
- **width**: Power of 2, between 8 and 256 (values: 8, 16, 32, 64, 128, 256)
- **encoding** (optional): One of `spatiotemporal_4x4x4`, `raw`, or `rle` (default: `raw`)
- **compression** (optional): One of `xor`, `rle`, or `none` (default: `none`)
- **default_value** (optional): Non-negative integer

**Field Naming Convention**:
- Lowercase snake_case (e.g., `command_delta`, `network_state`)
- Pattern: `^[a-z][a-z0-9_]*$`

### Operations

#### Accumulate (Required)
```json
"accumulate": {
  "enabled": true,
  "latency_cycles": 1
}
```
- `enabled` must be `true` (accumulate is the core operation)
- `latency_cycles` must be a positive integer (default: 1)

#### Reconstruct (Optional)
```json
"reconstruct": {
  "enabled": true,
  "latency_cycles": 1
}
```
- `enabled` defaults to `true` if not specified
- `latency_cycles` must be a positive integer (default: 1)

#### Rollback (Optional)
```json
"rollback": {
  "enabled": true,
  "history_depth": 100
}
```
- If `enabled` is `true`, `history_depth` **must** be specified
- `history_depth` must be between 1 and 65,536

### Constraints

All constraint values must be positive integers within specified ranges:

| Constraint | Type | Range |
|------------|------|-------|
| `max_memory_mb` | integer | 1 - 65,536 MB |
| `max_power_mw` | integer | 1 - 100,000 mW |
| `update_latency_ms` | integer | 0 - 10,000 ms |
| `target_frequency_mhz` | number | 1.0 - 1,000.0 MHz (default: 94.5) |

---

## Cross-Field Dependencies

### Rule 1: Hardware DATA_WIDTH Consistency

If `hardware.rtl_params.DATA_WIDTH` is specified, it **must** match the width of all delta fields.

**Valid Example**:
```json
{
  "schema": {
    "delta_fields": {
      "command_delta": {"type": "delta_stream", "width": 64},
      "response_delta": {"type": "delta_stream", "width": 64}
    }
  },
  "hardware": {
    "rtl_params": {"DATA_WIDTH": 64}
  }
}
```

**Invalid Example**:
```json
{
  "schema": {
    "delta_fields": {
      "command_delta": {"type": "delta_stream", "width": 64}
    }
  },
  "hardware": {
    "rtl_params": {"DATA_WIDTH": 32}  // ERROR: Mismatch
  }
}
```

### Rule 2: Rollback History Depth

If `operations.rollback.enabled` is `true`, then `operations.rollback.history_depth` **must** be specified.

**Valid Example**:
```json
{
  "operations": {
    "rollback": {
      "enabled": true,
      "history_depth": 100
    }
  }
}
```

**Invalid Example**:
```json
{
  "operations": {
    "rollback": {
      "enabled": true
      // ERROR: Missing history_depth
    }
  }
}
```

### Rule 3: Multiple Delta Fields with Different Widths

If delta fields have different widths and `hardware.rtl_params.DATA_WIDTH` is specified, validation should fail (hardware generator expects uniform width).

**Workaround**: Create separate schemas for fields with different widths.

---

## Namespace Generation Rules

The catalogue position determines the generated API namespace across all target languages:

### Namespace Formula

```
import_path = catalogue.vertical + "." + catalogue.field + "." + catalogue.object
```

### Language-Specific Mapping

| Language | Namespace Format | Example |
|----------|------------------|---------|
| **Python** | `from atomik.{vertical}.{field} import {object}` | `from atomik.Video.Stream import H264Delta` |
| **Rust** | `use atomik::{vertical}::{field}::{object};` | `use atomik::video::stream::H264Delta;` |
| **C** | `#include <atomik/{vertical}/{field}/{object}.h>` | `#include <atomik/video/stream/h264_delta.h>` |
| **JavaScript** | `const {object} = require('@atomik/{vertical}/{field}');` | `const {H264Delta} = require('@atomik/video/stream');` |
| **Verilog** | `module atomik_{vertical}_{field}_{object}` | `module atomik_video_stream_h264_delta` |

### Identifier Validity Rules

Object names must be valid identifiers in **all** target languages:

1. **Start with uppercase letter** (PascalCase)
2. **No reserved keywords** in any language (e.g., avoid "Class", "Interface", "Module")
3. **No special characters** (alphanumeric only)
4. **Length**: 1-64 characters

**Valid Object Names**:
- `TerminalIO`
- `DeltaExchange`
- `MatrixOps`
- `H264Delta`

**Invalid Object Names**:
- `terminal-io` (contains hyphen)
- `123Delta` (starts with number)
- `class` (reserved keyword in multiple languages)
- `IO` (too short, ambiguous)

---

## Validation Procedure

### Step 1: JSON Schema Validation

Validate the schema file against `specs/atomik_schema_v1.json` using a JSON Schema Draft 7 validator.

**Example (Python)**:
```python
import jsonschema
import json

with open('specs/atomik_schema_v1.json') as f:
    schema = json.load(f)

with open('sdk/schemas/examples/terminal-io.json') as f:
    instance = json.load(f)

jsonschema.validate(instance=instance, schema=schema)
```

### Step 2: Cross-Field Validation

Check cross-field dependencies:

1. If `operations.rollback.enabled == true`, verify `history_depth` is specified
2. If `hardware.rtl_params.DATA_WIDTH` is specified, verify it matches all delta field widths
3. Verify `catalogue.object` is a valid identifier in all target languages

### Step 3: Uniqueness Check

Verify that `(vertical, field, object)` tuple is unique across all schemas in the repository.

**Query**:
```bash
find sdk/schemas -name "*.json" -exec jq -r '"\(.catalogue.vertical)/\(.catalogue.field)/\(.catalogue.object)"' {} \;
```

No duplicates should appear in the output.

---

## Common Validation Errors

### Error 1: Missing Required Field

```json
{
  "catalogue": {
    "vertical": "System"
    // ERROR: Missing "field", "object", "version"
  }
}
```

**Fix**: Add all required catalogue fields.

### Error 2: Invalid Delta Field Width

```json
{
  "schema": {
    "delta_fields": {
      "data": {"type": "delta_stream", "width": 48}
      // ERROR: 48 is not a power of 2 in [8, 16, 32, 64, 128, 256]
    }
  }
}
```

**Fix**: Use a valid width (8, 16, 32, 64, 128, or 256).

### Error 3: Accumulate Not Enabled

```json
{
  "schema": {
    "operations": {
      "accumulate": {"enabled": false}
      // ERROR: Accumulate must be enabled
    }
  }
}
```

**Fix**: Set `"enabled": true` for the accumulate operation.

### Error 4: Rollback Enabled Without History Depth

```json
{
  "schema": {
    "operations": {
      "rollback": {"enabled": true}
      // ERROR: Missing history_depth
    }
  }
}
```

**Fix**: Add `"history_depth": N` where N is between 1 and 65,536.

### Error 5: Invalid Identifier

```json
{
  "catalogue": {
    "vertical": "Network",
    "field": "p2p-sync",  // ERROR: Contains hyphen
    "object": "deltaExchange",  // ERROR: Starts with lowercase
    "version": "1.0.0"
  }
}
```

**Fix**: Use PascalCase identifiers (`P2PSync`, `DeltaExchange`).

---

## Best Practices

### 1. Use Descriptive Names

Choose clear, self-documenting names:
- **Good**: `H264Delta`, `NetworkPacketDelta`, `SensorFusion`
- **Bad**: `Delta1`, `MyDelta`, `Test`

### 2. Follow Vertical Conventions

Align with existing vertical/field structure:
- Video: Stream, Codec, Frame
- Network: P2P, Packet, Protocol
- Edge: Sensor, Actuator, Gateway
- Compute: Linear, Matrix, Transform

### 3. Start Simple

Begin with minimal schemas:
- One delta field
- Accumulate and reconstruct operations only
- No hardware section until needed

### 4. Version Carefully

Follow semantic versioning:
- **Major**: Breaking changes to API
- **Minor**: New features, backwards-compatible
- **Patch**: Bug fixes only

### 5. Document Thoroughly

Use the `description` field to explain:
- What problem this module solves
- Typical use cases
- Any domain-specific assumptions

---

## Automated Validation

### Command-Line Validator

Create a validation script:

```python
#!/usr/bin/env python3
import sys
import json
import jsonschema

def validate_schema(schema_file, instance_file):
    with open(schema_file) as f:
        schema = json.load(f)

    with open(instance_file) as f:
        instance = json.load(f)

    try:
        jsonschema.validate(instance=instance, schema=schema)
        print(f"✓ {instance_file} is valid")
        return True
    except jsonschema.ValidationError as e:
        print(f"✗ {instance_file} is invalid:")
        print(f"  {e.message}")
        return False

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: validate.py <schema> <instance>")
        sys.exit(1)

    valid = validate_schema(sys.argv[1], sys.argv[2])
    sys.exit(0 if valid else 1)
```

**Usage**:
```bash
python validate.py specs/atomik_schema_v1.json sdk/schemas/examples/terminal-io.json
```

---

## Future Extensions

Potential additions to the schema specification:

1. **Security constraints**: Encryption, authentication requirements
2. **Performance profiles**: Low-power, high-throughput, ultra-low-latency
3. **Multi-field composition**: Relationships between delta fields
4. **Conditional operations**: Operation availability based on state
5. **Metadata extensions**: Tags, categories, discovery mechanisms

---

*Validation Rules v1.0.0 - January 26, 2026*
*ATOMiK Project - Phase 4A.1*
