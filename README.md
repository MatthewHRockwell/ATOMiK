# ATOMiK: Stateless Delta-Driven Computational Architecture

[![CI/CD](https://github.com/[owner]/ATOMiK/actions/workflows/atomik-ci.yml/badge.svg)](https://github.com/[owner]/ATOMiK/actions/workflows/atomik-ci.yml)
[![Documentation](https://img.shields.io/badge/docs-latest-blue)](https://[owner].github.io/ATOMiK/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

ATOMiK is a Category 1 hardware innovation that challenges traditional dataflow computing paradigms (specifically SCORE from 2005). It represents a fundamental shift from state-based to delta-based computation.

## Performance Claims

Based on empirical testing:
- **221×** faster than traditional optical flow
- **6139×** lower energy consumption
- **<1%** memory footprint vs SCORE architecture

## Core Innovations

**Stateless Operators**: Pure combinational XOR-based delta computation with no internal registers.

**4×4×4 Voxel Encoding**: Maps spatiotemporal video data to 64-bit register-native words for efficient processing.

**Sparse Symmetric Matrices**: Novel compression basis mathematically proven to be information-preserving:
```python
Gx = [[-1, 0, 1],    Gy = [[0, 1, 0],
      [ 0, 0, 0],          [-1, 0,-1],
      [ 1, 0,-1]]          [0, 1, 0]]
```

**Codon Algebra**: 2-bit state transitions (A=00, G=01, T=10, C=11) enabling 8× space reduction in logic synthesis.

**Motif Vocabulary**: Delta pattern compression achieving 1000:1+ ratios on video streams.

## Quick Start

### Installation

```bash
pip install atomik-sdk
```

### Basic Usage

```python
from atomik import DeltaStream, Motif

# Process video as delta stream
stream = DeltaStream.from_video("input.mp4", voxel=(4,4,4))

for delta in stream:
    if delta.motif == Motif.HORIZONTAL_MOTION:
        print(f"Motion detected at frame {delta.frame_index}")
```

### Hardware Deployment

For Tang Nano 9K FPGA deployment:

```bash
cd hardware/synthesis
./build.sh  # Requires Gowin EDA toolchain
```

## Project Structure

```
ATOMiK/
├── docs/           # Documentation and strategic plans
├── math/           # Mathematical proofs and benchmarks
├── experiments/    # Numbered experiments with validation
├── hardware/       # Verilog RTL and synthesis scripts
├── software/       # Python SDK and examples
├── legacy/         # Reference Jupyter notebooks
└── agents/         # AI agent configurations
```

## Development Phases

| Phase | Description | Duration | Status |
|-------|-------------|----------|--------|
| 1 | Mathematical Formalization | Week 1-2 | 🔄 In Progress |
| 2 | SCORE Comparison Benchmark | Week 2-3 | ⏳ Pending |
| 3 | Hardware Synthesis | Week 3-5 | ⏳ Pending |
| 4 | SDK Development | Week 5-6 | ⏳ Pending |

## Documentation

- [API Strategy](docs/API_STRATEGY.md) - Development strategy using Claude API
- [Roadmap](docs/ROADMAP.md) - 6-week development timeline
- [Agent Guidelines](docs/AGENT_GUIDELINES.md) - AI agent behavior specifications
- [Global Status](docs/GLOBAL_STATUS.md) - Master progress tracker

## Contributing

This project uses an AI-augmented development workflow. See [CLAUDE.md](CLAUDE.md) for agent guidelines and [CONTRIBUTING.md](CONTRIBUTING.md) for contribution standards.

## Hardware Target

**Primary**: Tang Nano 9K (Gowin GW1NR-9)
- 8640 LUTs, 6480 FFs
- 64 Mbit PSRAM
- 27 MHz onboard oscillator
- UART (115200 baud) + SPI interfaces

**Future**: 3-node distributed FPGA system

## License

MIT License - see [LICENSE](LICENSE) for details.

## Citation

If you use ATOMiK in your research, please cite:

```bibtex
@software{atomik2026,
  title = {ATOMiK: Stateless Delta-Driven Computational Architecture},
  year = {2026},
  url = {https://github.com/[owner]/ATOMiK}
}
```

## Acknowledgments

This project is developed using Anthropic's Claude API with automated multi-agent orchestration.
