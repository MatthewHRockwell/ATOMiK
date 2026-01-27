# ATOMiK White Papers

This directory contains academic papers documenting the ATOMiK architecture.

## Publication Strategy

| Paper | Title | Status | Target | Timing |
|-------|-------|--------|--------|--------|
| **Paper 1** | Delta-State Algebra: A Formally Verified Foundation | ✅ Complete | arXiv → PLDI/CAL | Ready for submission |
| **Paper 2** | ATOMiK: Benchmarking Delta-State Execution | ✅ Complete | arXiv → MICRO/ASPLOS | Ready for submission |
| **Paper 3** | Hardware Implementation and SDK Architecture | 📋 Planned | FPGA/DATE, IEEE TCAD | Post-Phase 4B |

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
├── paper3-hardware/               # 📋 Planned
│   └── README.md                  # Placeholder
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

**Target venues**: IEEE MICRO, ACM ASPLOS, IEEE HPCA

## Paper 3: Hardware and SDK (Planned)

**Status**: 📋 Planned - Awaiting Phase 4B completion

**Planned content**:
- RTL architecture derived from formal proofs
- Delta accumulator and state reconstructor design
- FPGA synthesis results (Tang Nano 9K, 7% LUT @ 94.5 MHz)
- Multi-language SDK architecture (5 language targets)
- Code generation framework design
- Hardware-software co-design methodology

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

- All papers reference the same codebase: `math/proofs/` for Lean4, `rtl/` for Verilog
- Maintain consistency in notation across papers
- Paper 2 and 3 should cite Paper 1 for theoretical foundation
- Repository URL should be included once public
