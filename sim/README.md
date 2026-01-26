# ATOMiK Simulation Directory

This directory contains testbenches and verification infrastructure for Phase 3.

## Structure

```
sim/
├── tb_delta_acc.v      # Delta accumulator testbench (T3.2)
├── tb_state_rec.v      # State reconstructor testbench (T3.3)
├── tb_atomik_core.v    # Integrated core testbench (T3.5)
├── reference_model.py  # Python reference for test vector generation
├── vectors/            # Test vector files (.hex)
├── run_delta_acc.sh    # Run accumulator tests
├── run_state_rec.sh    # Run reconstructor tests
├── run_all_tests.sh    # Run complete test suite
└── SIMULATION_RESULTS.md  # Test results documentation
```

## Running Tests

```bash
# Individual module tests
./run_delta_acc.sh
./run_state_rec.sh

# Full test suite
./run_all_tests.sh

# View waveforms (after running tests)
gtkwave dump.vcd
```

## Test Vector Format

Test vectors use Verilog `$readmemh` format:
- One value per line
- Hexadecimal, no prefix
- Comments with //

Example (test_basic.hex):
```
0000000000000000  // Initial state
123456789ABCDEF0  // Delta 1
FEDCBA9876543210  // Delta 2
```

## Verification Against Python Reference

The reference_model.py generates test vectors that match the behavior of:
- experiments/benchmarks/atomik/delta_engine.py

This ensures hardware matches the validated software implementation.
