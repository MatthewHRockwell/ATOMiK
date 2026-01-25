# ATOMiK White Papers

This directory contains academic papers documenting the ATOMiK architecture.

## Publication Strategy

| Paper | Title | Status | Target | Timing |
|-------|-------|--------|--------|--------|
| **Paper 1** | Delta-State Algebra: A Formally Verified Foundation | ✅ Draft Complete | arXiv → PLDI/CAL | **Now** |
| **Paper 2** | ATOMiK: Benchmarking Delta-State Execution | ⏳ Pending Phase 2 | MICRO/ASPLOS | Post-benchmarks |
| **Paper 3** | From Proofs to Silicon: Verified Hardware Synthesis | ⏳ Pending Phase 3 | FPGA/DATE | Post-synthesis |

## Directory Structure

```
papers/
├── paper1-formal-verification/    # ✅ Ready for completion
│   ├── main.tex                   # Main LaTeX document
│   ├── references.bib             # Bibliography
│   ├── figures/                   # TikZ figure sources
│   └── README.md                  # Paper-specific instructions
├── paper2-benchmarks/             # ⏳ After Phase 2
│   └── (placeholder)
├── paper3-hardware/               # ⏳ After Phase 3
│   └── (placeholder)
└── README.md                      # This file
```

## Paper 1: Formal Verification

**Status**: Draft complete, ready for final polish

**Content completed by Opus 4.5**:
- Abstract with key contributions
- Introduction with motivation
- All 17 major theorems with proofs
- Lean4 code for each theorem
- Mathematical ↔ computational notation mapping
- Hardware implications discussion
- Related work survey
- Bibliography

**Remaining work**:
- Author information
- Create figures from TikZ specifications
- Final formatting
- arXiv submission metadata

**Build**:
```bash
cd paper1-formal-verification
pdflatex main.tex && bibtex main && pdflatex main.tex && pdflatex main.tex
```

## Paper 2: Benchmarks (Planned)

**Timing**: After Phase 2 completion (~3-4 weeks)

**Planned content**:
- SCORE baseline implementation
- ATOMiK variant benchmarks
- Memory efficiency metrics
- Computational overhead analysis
- Scalability results
- Statistical validation

**Target venues**: IEEE MICRO, ACM ASPLOS, IEEE HPCA

## Paper 3: Hardware (Planned)

**Timing**: After Phase 3 completion (~6-8 weeks)

**Planned content**:
- RTL architecture derived from proofs
- Delta accumulator design
- State reconstructor module
- FPGA synthesis results
- Timing analysis
- Resource utilization

**Target venues**: FPGA Conference, DATE, IEEE TCAD

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
