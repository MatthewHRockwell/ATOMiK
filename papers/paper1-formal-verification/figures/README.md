# Figures for Paper 1

This directory contains figure source files for the delta-state algebra paper.

## Files

| File | Description | Format |
|------|-------------|--------|
| `fig1-accumulator.tex` | Delta Accumulator Architecture | TikZ (standalone) |
| `fig2-dependencies.tex` | Lean4 Module Dependency Graph | TikZ (standalone) |

## Building Figures

Each `.tex` file can be compiled standalone:

```bash
pdflatex fig1-accumulator.tex
pdflatex fig2-dependencies.tex
```

Or convert to PNG for inclusion:
```bash
pdftoppm -png -r 300 fig1-accumulator.pdf fig1-accumulator
pdftoppm -png -r 300 fig2-dependencies.pdf fig2-dependencies
```

## Including in Main Document

Option 1: Compile standalone and include PDF
```latex
\includegraphics[width=0.8\textwidth]{figures/fig1-accumulator.pdf}
```

Option 2: Include TikZ directly (requires same preamble)
```latex
\input{figures/fig1-accumulator.tex}
```

## Figure Specifications

### Figure 1: Delta Accumulator Architecture

**Purpose**: Illustrate how deltas are accumulated via XOR tree before single state application.

**Key elements**:
- Input deltas (δ₁, δ₂, ..., δₙ) in blue
- XOR tree showing parallel reduction in orange
- Accumulated delta (δₐcc) 
- Final state computation (S₀ ⊕ δₐcc = S')
- Timing annotations: O(log n) for tree, O(1) for final XOR

**Insight conveyed**: Order-independent accumulation enables parallel hardware reduction.

### Figure 2: Module Dependency Graph

**Purpose**: Show the structure of the Lean4 formalization.

**Key elements**:
- 8 modules as nodes with theorem counts
- Directed edges showing import dependencies
- Color coding: Core (blue), Properties (green), Advanced (orange)
- Layer layout showing build order

**Insight conveyed**: Incremental verification builds on verified foundations.

## Style Guidelines

- Use consistent colors across figures
- Blue (#4A90D9) for inputs/definitions
- Orange (#F5A623) for operations/transformations  
- Green (#7ED321) for outputs/states
- Gray arrows for dependencies
- Clean, minimal style appropriate for academic publication
