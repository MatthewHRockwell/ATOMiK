# RISC-V Compliance Test Harness

Verilator-based test runner for RISC-V ISA compliance tests against the ATOMiK v3 custom RV64I CPU core.

## Status: Operational (Phase 1 Complete)

**Results:** 53/54 `rv64ui-p-*` tests pass. Only `ma_data` (misaligned access trap) fails by design.

## Prerequisites

### Clone riscv-tests

```bash
cd ~/Projects
git clone https://github.com/riscv-software-src/riscv-tests
cd riscv-tests
git submodule update --init --recursive
```

### Build rv64ui test binaries

```bash
cd ~/Projects/riscv-tests
autoconf
./configure --prefix=$PWD/install
make isa
```

This produces ELF binaries in `isa/rv64ui-p-*` (e.g., `rv64ui-p-add`, `rv64ui-p-sub`, etc.).

## Running

```bash
# From repo root:
make -C hardware/v3 compliance
```

This builds the compliance runner via Verilator and runs all `rv64ui-p-*` tests. Each test is loaded as an ELF, simulated for up to 500,000 cycles, and checked for a `tohost` write (1 = PASS).

## File Structure

```
sim/compliance/
├── README.md               (this file)
├── compliance_runner.cpp   (Verilator C++ harness — ELF load, simulate, tohost check)
├── elf_loader.h            (Minimal ELF64 parser — PT_LOAD segments, tohost symbol)
├── mem_model.h             (Flat 16MB memory model at 0x80000000, 32-bit bus interface)
└── debug_runner.cpp        (Verbose bus trace tool for debugging failures)
```

## How It Works

1. **Load ELF** — Parse the riscv-tests ELF binary, load PT_LOAD segments into simulated memory, find `tohost` symbol address
2. **Instantiate CPU** — Verilate `atomik_v3_cpu` with `RESET_PC=0x80000000`
3. **Run simulation** — Clock the CPU, respond to bus transactions via `mem_model`, monitor writes to `tohost`
4. **Check result** — `tohost == 1` means PASS, any other value encodes the failing test number as `(test_num << 1) | 1`
