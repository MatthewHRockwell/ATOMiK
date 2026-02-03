# ATOMiK SDK

Python SDK for the ATOMiK stateless delta-driven computational architecture.

## Installation

```bash
pip install -e .
```

For video support:
```bash
pip install -e ".[video]"
```

For development:
```bash
pip install -e ".[dev]"
```

## CLI Tool: `atomik-gen`

After installation, the `atomik-gen` command is available:

### Schema & Code Generation
```bash
atomik-gen generate <schema> [--output-dir DIR] [--languages LANG ...]
atomik-gen validate <schema>
atomik-gen info <schema>
atomik-gen batch <directory> [--output-dir DIR] [--report FILE]
atomik-gen list
```

### From-Source Pipeline
```bash
atomik-gen from-source <source-file> [--output-dir DIR] [--languages LANG ...]
```
Infer an ATOMiK schema directly from existing source code, then generate SDKs.

### Hardware Demo
```bash
atomik-gen demo <domain> [--sim-only] [--com-port PORT] [--report FILE] [-v]
```
Run domain demos (e.g. `video`) through simulation and optional FPGA hardware validation. Produces a JSON report with validation level, timing, and tool information.

### Pipeline (Advanced)
```bash
atomik-gen pipeline <source-file> [--output-dir DIR] [--stages STAGE ...]
atomik-gen pipeline-status <run-id>
```
Run the full bidirectional pipeline: extract → infer → validate → diff → generate → verify → hardware → metrics.

### Examples
```bash
atomik-gen generate sdk/schemas/examples/terminal-io.json --languages python rust
atomik-gen from-source my_app.py --languages python rust c
atomik-gen demo video --sim-only --report demo_report.json
atomik-gen batch sdk/schemas/domains/ --report report.json
```

A companion [VS Code extension](../sdk/vscode-extension/atomik-vscode/README.md) provides schema intellisense, snippets, and command palette integration with `atomik-gen`.

## Quick Start

```python
from atomik_sdk import DeltaStream, Motif

# Process video as delta stream
stream = DeltaStream.from_video("input.mp4", voxel=(4,4,4))

for delta in stream:
    if delta.motif == Motif.HORIZONTAL_MOTION:
        print(f"Motion detected at frame {delta.frame_index}")
```

## API Levels

The SDK provides three API levels:

**Level 1 (High-Level)**: Application developers
- `DeltaStream` - Video-to-delta processing
- `Motif` - Motion pattern classification

**Level 2 (Mid-Level)**: Algorithm designers  
- `VoxelEncoder` - Spatiotemporal encoding
- `PatternMatcher` - Custom pattern matching

**Level 3 (Low-Level)**: Hardware engineers
- `GenomeCompiler` - Configuration compilation
- `BitstreamGenerator` - Verilog synthesis

## Hardware Discovery

The `atomik_sdk.hardware_discovery` module centralises all FPGA tool and board detection:

- `find_gowin_root()` — Locate Gowin EDA installation (env, Windows, Linux probe paths)
- `find_tool(name)` — Find tools like `gw_sh`, `programmer_cli`, `openFPGALoader`
- `detect_board()` — Detect connected Tang Nano 9K via programmer_cli or openFPGALoader
- `detect_com_port()` — Auto-detect UART port via pyserial VID:PID matching

Cross-platform (Windows + Linux). Used by the CLI, pipeline hardware stage, and build scripts.

## Testing

```bash
pytest tests/ -v --cov=atomik_sdk
```

353 tests covering generators, pipeline stages, CLI commands, and hardware discovery.
