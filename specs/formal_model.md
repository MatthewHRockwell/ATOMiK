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

### 1.2 Core Operations

| Operation | Symbol | Definition |
|-----------|--------|------------|
| Composition | ⊕ | δ₁ ⊕ δ₂ = δ₁ XOR δ₂ |
| Identity | 𝟎 | 64-bit zero vector |
| Inverse | δ⁻¹ | δ⁻¹ = δ (self-inverse under XOR) |

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

| Property | Lean4 File | Status |
|----------|------------|--------|
| Closure | `Closure.lean` | Pending |
| Associativity | `Properties.lean` | Pending |
| Commutativity | `Properties.lean` | Pending |
| Identity | `Properties.lean` | Pending |
| Inverse | `Properties.lean` | Pending |
| Determinism | `Transition.lean` | Pending |
| Turing Completeness | `TuringComplete.lean` | Pending |

---

*Document version: 1.0*
*Last updated: January 24, 2026*
*Related proofs: `math/proofs/ATOMiK/`*
