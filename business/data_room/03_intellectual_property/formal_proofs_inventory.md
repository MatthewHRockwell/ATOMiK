# Formal Proofs Inventory

*Generated: 2026-02-03*

## Overview

ATOMiK's mathematical foundations are verified by **92 theorems** in
Lean4, covering the complete delta-state algebra.

## Proof Categories

| Category | Count | Directory |
|----------|-------|-----------|
| Core algebra (group axioms) | 12 | `math/proofs/ATOMiK/` |
| Commutativity & associativity | 8 | `math/proofs/ATOMiK/` |
| Self-inverse properties | 6 | `math/proofs/ATOMiK/` |
| Parallel merge correctness | 10 | `math/proofs/ATOMiK/` |
| State reconstruction | 8 | `math/proofs/ATOMiK/` |
| Turing completeness | 15 | `math/proofs/ATOMiK/` |
| Hardware correspondence | 12 | `math/proofs/ATOMiK/` |
| Edge cases & boundaries | 21 | `math/proofs/ATOMiK/` |

## Verification

```bash
cd math/proofs && lake build
# All 92 theorems verified, 0 sorry statements
```

## Key Theorems

1. **delta_self_inverse**: For all d, d XOR d = 0
2. **accumulate_commutative**: For all a b, a XOR b = b XOR a
3. **merge_tree_correct**: N-bank merge produces same result as sequential
4. **turing_complete**: Counter machine simulation via delta-state ops
