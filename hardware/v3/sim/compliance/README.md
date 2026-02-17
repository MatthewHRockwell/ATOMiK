# RISC-V Compliance Test Harness

This directory will contain the Verilator-based test runner for RISC-V ISA compliance tests against the ATOMiK v3 custom RV64I CPU core.

## Status: Scaffold (Phase 0)

The actual test runner will be built in Phase 1 alongside the CPU core. This README documents the architecture and setup instructions.

## Prerequisites

### Clone riscv-tests

```bash
cd /home/mattrock/Projects
git clone https://github.com/riscv-software-src/riscv-tests
cd riscv-tests
git submodule update --init --recursive
```

### Build rv64ui test binaries

```bash
cd /path/to/riscv-tests
autoconf
./configure --prefix=$PWD/install
make rv64ui
```

This produces ELF binaries in `isa/rv64ui-p-*` (e.g., `rv64ui-p-add`, `rv64ui-p-sub`, etc.).

## Test Runner Architecture (Phase 1)

The compliance runner will:

1. **Load ELF** — Parse the riscv-tests ELF binary and load `.text` and `.data` sections into simulated memory
2. **Instantiate CPU** — Verilate the v3 RV64I core with memory model
3. **Run simulation** — Clock the CPU until it writes to the `tohost` CSR/MMIO address
4. **Check result** — `tohost == 1` means PASS, any other value encodes the failing test number

### Expected file structure (Phase 1)

```
sim/compliance/
├── README.md              (this file)
├── compliance_runner.cpp  (Verilator C++ harness)
├── elf_loader.h           (ELF parser for loading test binaries)
├── mem_model.h            (Simple memory model for simulation)
└── run_compliance.sh      (Script to run all rv64ui-p-* tests)
```

### Makefile integration

Once implemented, `make -C hardware/v3 compliance` will:

```bash
# Verilate the CPU core
verilator --cc --exe --build \
    hardware/v3/rtl/atomik_v3_cpu.v \
    hardware/v3/sim/compliance/compliance_runner.cpp

# Run each test
for test in /path/to/riscv-tests/isa/rv64ui-p-*; do
    ./obj_dir/Vatomik_v3_cpu $test
done
```
