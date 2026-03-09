# Paper 3: From Proofs to Silicon

**Title**: "From Proofs to Silicon: Hardware Implementation of Formally Verified Delta-State Computation"

**Status**: Draft complete

## Target Venues

- IEEE TCAD (Transactions on Computer-Aided Design of Integrated Circuits and Systems)
- FPGA Conference (ACM/SIGDA International Symposium on FPGAs)
- DATE (Design, Automation, and Test in Europe)
- IEEE Micro

## Build

```bash
cd papers/paper3-hardware
./compile.sh
# or manually:
pdflatex Paper_3_Hardware_Implementation.tex
bibtex Paper_3_Hardware_Implementation
pdflatex Paper_3_Hardware_Implementation.tex
pdflatex Paper_3_Hardware_Implementation.tex
```

## Content

- Three-generation hardware evolution (standalone core → PicoRV32 SoC → custom RV64I ISA)
- Synthesis optimization: preventing carry-chain inference on XOR datapaths (+42% Fmax)
- 25-configuration parallel bank sweep with 80 hardware-validated tests
- Perfect linear scaling to 16 banks (1,056 Mops/s on $13.50 FPGA)
- Cross-platform portability (Gowin GW1NR-9 → Xilinx Zynq-7020)
- Custom RISC-V ISA instructions (zero-overhead ATOMiK ops)
- SDK generation framework (5 languages, 353 tests)
- Complete validation chain: Lean4 proofs → Python → RTL sim → FPGA → production

## Data Sources

- `hardware/rtl/` — v2 core Verilog
- `hardware/v3/rtl/` — v3 SoC Verilog
- `hardware/zynq/rtl/` — Zynq AXI wrapper
- `hardware/sweep/` — 25-config synthesis results
- `hardware/experiments/data/` — Benchmark measurements
- `software/atomik_sdk/` — SDK framework (353 tests)
- `software/libatomik/` — C library + Python bindings
- `math/proofs/` — Lean4 formal proofs

## Figures

All figures use inline TikZ (no external PDF dependencies). Figures include:
- Hardware evolution timeline (Fig. 1)
- Core architecture block diagram (Fig. 2)
- Parallel bank merge tree (Fig. 3)
- Validation chain (Fig. 4)
