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

After installation, the `atomik-gen` command is available for schema validation and multi-language SDK generation:

```bash
atomik-gen generate <schema> [--output-dir DIR] [--languages LANG ...]
atomik-gen validate <schema>
atomik-gen info <schema>
atomik-gen batch <directory> [--output-dir DIR] [--report FILE]
atomik-gen list
atomik-gen --version
```

Example:
```bash
atomik-gen generate sdk/schemas/examples/terminal-io.json --languages python rust
atomik-gen batch sdk/schemas/domains/ --report report.json
```

A companion [VS Code extension](../vscode-extension/atomik-vscode/README.md) provides schema intellisense, snippets, and command palette integration with `atomik-gen`.

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

## Testing

```bash
pytest tests/ -v --cov=atomik_sdk
```
