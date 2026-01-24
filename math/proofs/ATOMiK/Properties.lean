-- ATOMiK/Properties.lean
-- Associativity, commutativity, and other algebraic property proofs
--
-- This module proves the core algebraic properties that establish
-- Delta as an Abelian group under composition.
--
-- Task: T1.3 - Prove associativity/commutativity
-- Phase: 1 - Mathematical Formalization

import ATOMiK.Closure

namespace ATOMiK

/-! ## Core Algebraic Properties

These proofs establish that (Delta, ⊕, 𝟎) forms an Abelian group:
1. Associativity: (a ⊕ b) ⊕ c = a ⊕ (b ⊕ c)
2. Commutativity: a ⊕ b = b ⊕ a
3. Identity: a ⊕ 𝟎 = a = 𝟎 ⊕ a
4. Inverse: a ⊕ a = 𝟎

Properties 3 and 4 were proven in Delta.lean; we provide additional
forms here.
-/

/-! ### Associativity -/

/-- Delta composition is associative.
    
    (δ₁ ⊕ δ₂) ⊕ δ₃ = δ₁ ⊕ (δ₂ ⊕ δ₃)
    
    This follows directly from the associativity of XOR on bit vectors.
-/
theorem delta_assoc (a b c : Delta) :
    Delta.compose (Delta.compose a b) c = Delta.compose a (Delta.compose b c) := by
  simp [Delta.compose, BitVec.xor_assoc]

/-- Associativity using + notation -/
theorem delta_add_assoc (a b c : Delta) : (a + b) + c = a + (b + c) := by
  simp [HAdd.hAdd, delta_assoc]

/-! ### Commutativity -/

/-- Delta composition is commutative.
    
    δ₁ ⊕ δ₂ = δ₂ ⊕ δ₁
    
    This follows directly from the commutativity of XOR on bit vectors.
-/
theorem delta_comm (a b : Delta) :
    Delta.compose a b = Delta.compose b a := by
  simp [Delta.compose, BitVec.xor_comm]

/-- Commutativity using + notation -/
theorem delta_add_comm (a b : Delta) : a + b = b + a := by
  simp [HAdd.hAdd, delta_comm]

/-! ### Identity Properties (Additional Forms) -/

/-- Right identity: δ ⊕ 𝟎 = δ -/
theorem delta_zero_right (d : Delta) : Delta.compose d Delta.zero = d :=
  Delta.compose_zero_right d

/-- Left identity: 𝟎 ⊕ δ = δ -/
theorem delta_zero_left (d : Delta) : Delta.compose Delta.zero d = d :=
  Delta.compose_zero_left d

/-- Right identity using + notation -/
theorem delta_add_zero (d : Delta) : d + Delta.zero = d := by
  simp [HAdd.hAdd, Delta.compose_zero_right]

/-- Left identity using + notation -/
theorem delta_zero_add (d : Delta) : Delta.zero + d = d := by
  simp [HAdd.hAdd, Delta.compose_zero_left]

/-! ### Inverse Properties (Additional Forms) -/

/-- Self-inverse: δ ⊕ δ = 𝟎 -/
theorem delta_self_inverse (d : Delta) : Delta.compose d d = Delta.zero :=
  Delta.compose_self d

/-- Every delta is its own inverse -/
theorem delta_inverse_self (d : Delta) : Delta.inverse d = d := rfl

/-- Composing with inverse yields identity -/
theorem delta_compose_inverse (d : Delta) : 
    Delta.compose d (Delta.inverse d) = Delta.zero := by
  simp [Delta.inverse, Delta.compose_self]

/-- Double inverse is identity -/
theorem delta_inverse_inverse (d : Delta) : Delta.inverse (Delta.inverse d) = d := rfl

/-! ### Cancellation Properties -/

/-- Left cancellation: a ⊕ b = a ⊕ c → b = c -/
theorem delta_cancel_left (a b c : Delta) (h : Delta.compose a b = Delta.compose a c) : b = c := by
  have h1 : Delta.compose a (Delta.compose a b) = Delta.compose a (Delta.compose a c) := by
    rw [h]
  simp [← delta_assoc, Delta.compose_self, Delta.compose_zero_left] at h1
  exact h1

/-- Right cancellation: a ⊕ c = b ⊕ c → a = b -/
theorem delta_cancel_right (a b c : Delta) (h : Delta.compose a c = Delta.compose b c) : a = b := by
  have h1 : Delta.compose (Delta.compose a c) c = Delta.compose (Delta.compose b c) c := by
    rw [h]
  simp [delta_assoc, Delta.compose_self, Delta.compose_zero_right] at h1
  exact h1

/-! ### Uniqueness Properties -/

/-- The identity element is unique -/
theorem delta_identity_unique (e : Delta) (h : ∀ d : Delta, Delta.compose d e = d) : 
    e = Delta.zero := by
  have : Delta.compose Delta.zero e = Delta.zero := h Delta.zero
  simp [Delta.compose_zero_left] at this
  exact this

/-- The inverse is unique -/
theorem delta_inverse_unique (d i : Delta) (h : Delta.compose d i = Delta.zero) : 
    i = d := by
  have h1 : Delta.compose d i = Delta.compose d d := by
    rw [h, Delta.compose_self]
  exact delta_cancel_left d i d h1

/-! ### Derived Properties -/

/-- Helper: Two deltas are equal iff their bits are equal -/
theorem delta_eq_iff (a b : Delta) : a = b ↔ a.bits = b.bits := by
  constructor
  · intro h; rw [h]
  · intro h; cases a; cases b; simp at h; simp [h]

/-- Composition distributes over itself (rearrangement) -/
theorem delta_compose_compose (a b c d : Delta) :
    Delta.compose (Delta.compose a b) (Delta.compose c d) = 
    Delta.compose (Delta.compose a c) (Delta.compose b d) := by
  rw [delta_eq_iff]
  simp only [Delta.compose]
  rw [BitVec.xor_assoc, BitVec.xor_assoc]
  congr 1
  rw [← BitVec.xor_assoc b.bits c.bits d.bits]
  rw [BitVec.xor_comm b.bits c.bits]
  rw [BitVec.xor_assoc c.bits b.bits d.bits]

/-- Triple composition identity -/
theorem delta_triple_compose (a b c : Delta) :
    Delta.compose a (Delta.compose b c) = Delta.compose b (Delta.compose a c) := by
  rw [delta_eq_iff]
  simp only [Delta.compose]
  rw [← BitVec.xor_assoc, ← BitVec.xor_assoc]
  rw [BitVec.xor_comm a.bits b.bits]

/-- Negation distributes over addition -/
theorem delta_neg_compose (a b : Delta) : 
    -(Delta.compose a b) = Delta.compose (-a) (-b) := by
  simp [Neg.neg, Delta.inverse]

/-! ### Abelian Group Laws Summary -/

/-- Summary: Delta satisfies all Abelian group axioms -/
theorem delta_is_abelian_group :
    (∀ a b c : Delta, Delta.compose (Delta.compose a b) c = Delta.compose a (Delta.compose b c)) ∧
    (∀ a : Delta, Delta.compose a Delta.zero = a) ∧
    (∀ a : Delta, Delta.compose Delta.zero a = a) ∧
    (∀ a : Delta, Delta.compose a a = Delta.zero) ∧
    (∀ a b : Delta, Delta.compose a b = Delta.compose b a) :=
  ⟨delta_assoc, Delta.compose_zero_right, Delta.compose_zero_left, Delta.compose_self, delta_comm⟩

end ATOMiK
