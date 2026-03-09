# ATOMiK White Papers

This directory contains academic papers documenting the ATOMiK architecture.

## Publication Strategy

| Paper | Title | Status | Target | Timing |
|-------|-------|--------|--------|--------|
| **Paper 1** | Delta-State Algebra: A Formally Verified Foundation | ✅ Complete | arXiv → PLDI/CAL | Ready for submission |
| **Paper 2** | ATOMiK: Empirical Validation of Delta-State Computation | ✅ Complete | Scientific Reports (Springer Nature) | Under peer review |
| **Paper 3** | From Proofs to Silicon: Hardware Implementation | ✅ Draft complete | FPGA/DATE, IEEE TCAD, IEEE Micro | Ready for review |

## Directory Structure

```
papers/
├── paper1-formal-verification/    # ✅ Complete
│   ├── Delta_State_Algebra.pdf    # Final manuscript (445 KB)
│   ├── Delta_State_Algebra.tex    # LaTeX source
│   ├── references.bib             # Bibliography
│   ├── arxiv-metadata.txt         # arXiv submission metadata
│   ├── figures/                   # Figure sources
│   └── README.md                  # Paper-specific instructions
├── paper2-benchmarks/             # ✅ Complete
│   ├── Paper_2_ATOMiK_Benchmarks.pdf  # Final manuscript (545 KB)
│   ├── Paper_2_ATOMiK_Benchmarks.tex  # LaTeX source
│   ├── references.bib             # Bibliography
│   ├── figures/                   # Benchmark plots
│   ├── compile.sh                 # Build script
│   └── README.md                  # Paper-specific instructions
├── paper3-hardware/               # ✅ Draft complete
│   ├── Paper_3_Hardware_Implementation.tex  # LaTeX source
│   ├── references.bib             # Bibliography
│   ├── figures/                   # TikZ figures (inline)
│   ├── compile.sh                 # Build script
│   └── README.md                  # Paper-specific instructions
└── README.md                      # This file
```

## Paper 1: Formal Verification

**Status**: ✅ Complete - Ready for arXiv submission

**Content**:
- Abstract with key contributions
- Introduction with motivation and related work
- All 17 major theorems with complete proofs
- Lean4 formal verification code listings
- Mathematical ↔ computational notation mapping
- Hardware implications and implementation discussion
- Complete bibliography

**Build**:
```bash
cd paper1-formal-verification
pdflatex Delta_State_Algebra.tex
bibtex Delta_State_Algebra
pdflatex Delta_State_Algebra.tex
pdflatex Delta_State_Algebra.tex
```

## Paper 2: Benchmarks

**Status**: ✅ Complete - Ready for arXiv submission

**Content**:
- SCORE baseline implementation and methodology
- ATOMiK variant benchmark design
- Memory efficiency analysis (95-100% reduction validated)
- Computational overhead measurements across 4 workloads
- Scalability results with statistical validation
- 360 measurements with p < 0.05 significance

**Build**:
```bash
cd paper2-benchmarks
./compile.sh
# or manually:
pdflatex Paper_2_ATOMiK_Benchmarks.tex
bibtex Paper_2_ATOMiK_Benchmarks
pdflatex Paper_2_ATOMiK_Benchmarks.tex
pdflatex Paper_2_ATOMiK_Benchmarks.tex
```

**Target venue**: Scientific Reports (Springer Nature) — under peer review

## Paper 3: Hardware Implementation

**Status**: ✅ Draft complete

**Content**:
- Three-generation hardware evolution (standalone → PicoRV32 SoC → custom RV64I ISA)
- Synthesis optimization methodology (+42% Fmax via carry-chain prevention)
- 25-configuration parallel bank sweep with 80 hardware-validated tests
- Perfect linear scaling to 16 banks (1,056 Mops/s on $13.50 FPGA)
- Cross-platform portability (Gowin GW1NR-9 → Xilinx Zynq-7020)
- Custom RISC-V ISA instructions with zero-overhead CPI
- SDK generation framework (5 languages, 353 tests)
- Complete validation chain from Lean4 proofs to production deployment

**Build**:
```bash
cd paper3-hardware
./compile.sh
# or manually:
pdflatex Paper_3_Hardware_Implementation.tex
bibtex Paper_3_Hardware_Implementation
pdflatex Paper_3_Hardware_Implementation.tex
pdflatex Paper_3_Hardware_Implementation.tex
```

**Target venues**: FPGA Conference, DATE, IEEE TCAD, IEEE Micro

## arXiv Categories

- **Primary**: cs.AR (Computer Architecture)
- **Cross-list**: cs.PL (Programming Languages), cs.LO (Logic in Computer Science)

## Citation

Once published, papers should be cited as:

```bibtex
@article{atomik2026formal,
  title={Delta-State Algebra: A Formally Verified Foundation for Transient State Computation},
  author={[Authors]},
  journal={arXiv preprint arXiv:XXXX.XXXXX},
  year={2026}
}
```

## Notes

- All papers reference the same codebase: `math/proofs/` for Lean4, `hardware/rtl/` for Verilog
- Maintain consistency in notation across papers
- Paper 2 and 3 should cite Paper 1 for theoretical foundation
- Repository URL should be included once public
