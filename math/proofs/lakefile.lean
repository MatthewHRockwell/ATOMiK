import Lake
open Lake DSL

package ATOMiK where
  -- Package metadata
  version := v!"1.0.0"
  
  -- Lean compiler options
  leanOptions := #[
    ⟨`autoImplicit, false⟩,
    ⟨`relaxedAutoImplicit, false⟩
  ]

-- Main proof library
@[default_target]
lean_lib ATOMiK where
  globs := #[.submodules `ATOMiK]
  
/-!
# ATOMiK Lean4 Proof Library

## Phase 1: Mathematical Formalization

This package contains formal proofs for the ATOMiK delta-state algebra.

### Module Structure

- `ATOMiK.Basic` - Core type definitions (State, DELTA_WIDTH)
- `ATOMiK.Delta` - Delta type and operations
- `ATOMiK.Closure` - Closure under composition
- `ATOMiK.Properties` - Algebraic properties (assoc, comm, identity, inverse)
- `ATOMiK.Composition` - Sequential and parallel composition operators
- `ATOMiK.Transition` - State transition functions and determinism
- `ATOMiK.Equivalence` - Computational equivalence proofs
- `ATOMiK.TuringComplete` - Turing completeness via counter machine simulation

### Build Instructions

```bash
lake build
```

### Verification

All proofs are verified by the Lean4 type checker. No `sorry` statements.

### Requirements

- Lean 4.27.0 (see `lean-toolchain`)
- Lake build system

### Version History

- v1.0.0 (2026-01-24): Phase 1 complete - Mathematical formalization
-/
