# ATOMiK Formal Model Specification

## Overview

This document defines the mathematical foundations of the ATOMiK delta-state algebra, providing the formal specification that underlies all Lean4 proofs in `math/proofs/`.

---

## 1. Delta-State Algebra

### 1.1 Delta Type

A **Delta** (δ) represents an atomic state difference that can be composed with other deltas via XOR operations.

```
Delta := BitVec(64)
```

**Lean4 Implementation** (`ATOMiK/Delta.lean`):
```lean
structure Delta where
  bits : BitVec DELTA_WIDTH
  deriving DecidableEq, Repr, Inhabited
```

### 1.2 Core Operations

| Operation | Symbol | Definition | Lean4 Function |
|-----------|--------|------------|----------------|
| Composition | ⊕ | δ₁ ⊕ δ₂ = δ₁ XOR δ₂ | `Delta.compose` |
| Identity | 𝟎 | 64-bit zero vector | `Delta.zero` |
| Inverse | δ⁻¹ | δ⁻¹ = δ (self-inverse under XOR) | `Delta.inverse` |
| Application | · | s · δ = s XOR δ | `Delta.apply` |

### 1.3 Algebraic Properties

The delta-state algebra forms an **Abelian group** under composition:

1. **Closure**: ∀ δ₁, δ₂ ∈ Delta: δ₁ ⊕ δ₂ ∈ Delta
2. **Associativity**: ∀ δ₁, δ₂, δ₃ ∈ Delta: (δ₁ ⊕ δ₂) ⊕ δ₃ = δ₁ ⊕ (δ₂ ⊕ δ₃)
3. **Identity**: ∀ δ ∈ Delta: δ ⊕ 𝟎 = δ
4. **Inverse**: ∀ δ ∈ Delta: δ ⊕ δ = 𝟎
5. **Commutativity**: ∀ δ₁, δ₂ ∈ Delta: δ₁ ⊕ δ₂ = δ₂ ⊕ δ₁

---

## 2. State Transitions

### 2.1 Transition Function

A stateless transition function transforms input state to output state through delta application:

```
transition : State → Delta → State
transition(s, δ) = s XOR δ
```

**Lean4 Implementation** (`ATOMiK/Delta.lean`):
```lean
def Delta.apply (d : Delta) (s : State) : State :=
  s ^^^ d.bits
```

### 2.2 Determinism Guarantee

For any state `s` and delta `δ`, the transition function always produces the same result:

```
∀ s, δ: transition(s, δ) = transition(s, δ)
```

This is trivially true for pure functions with no side effects.

---

## 3. Computational Model

### 3.1 Equivalence to Traditional Stateful Model

The ATOMiK model is computationally equivalent to traditional stateful computation:

**Traditional**: State × Input → State × Output
**ATOMiK**: State × Delta → State (where Delta encodes both input and output transformation)

### 3.2 Turing Completeness

ATOMiK achieves Turing completeness through:
- Conditional branching via delta selection
- Iteration via recursive delta composition
- Memory via state accumulation

---

## 4. Proof Obligations

| Property | Lean4 File | Status | Task |
|----------|------------|--------|------|
| Type definitions | `Basic.lean`, `Delta.lean` | ✅ Complete | T1.1 |
| Closure | `Closure.lean` | ⏳ Pending | T1.2 |
| Associativity | `Properties.lean` | ⏳ Pending | T1.3 |
| Commutativity | `Properties.lean` | ⏳ Pending | T1.3 |
| Identity | `Delta.lean` | ✅ Proven | T1.1 |
| Inverse | `Delta.lean` | ✅ Proven | T1.1 |
| Determinism | `Transition.lean` | ⏳ Pending | T1.6 |
| Turing Completeness | `TuringComplete.lean` | ⏳ Pending | T1.8 |

---

## 5. T1.1 Deliverables

### 5.1 Basic.lean
- `DELTA_WIDTH` constant (64 bits)
- `State` type alias
- `State.zero` and `State.xor` operations

### 5.2 Delta.lean
- `Delta` structure with `bits : BitVec 64`
- `Delta.zero` / `Delta.identity` - identity element
- `Delta.compose` - group operation (XOR)
- `Delta.inverse` - self-inverse
- `Delta.apply` - state transition
- Utility functions: `ofNat`, `ofBitVec`, `toBitVec`, `isZero`, `popCount`, `hammingDistance`
- Basic lemmas:
  - `Delta.compose_zero_right`
  - `Delta.compose_zero_left`
  - `Delta.compose_self`
  - `Delta.add_neg_self`

---

*Document version: 1.1*
*Last updated: January 24, 2026*
*T1.1 completed: January 24, 2026*
*Related proofs: `math/proofs/ATOMiK/`*
