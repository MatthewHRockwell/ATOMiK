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
