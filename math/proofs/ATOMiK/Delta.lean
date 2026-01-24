-- ATOMiK/Delta.lean
-- Delta type and XOR composition operations
--
-- A Delta represents an atomic state difference that can be composed
-- with other deltas via XOR operations. The delta-state algebra forms
-- an Abelian group under composition.
--
-- Task: T1.1 - Define delta-state algebra axioms
-- Phase: 1 - Mathematical Formalization

import ATOMiK.Basic

namespace ATOMiK

/-- A Delta represents a state difference encoded as a 64-bit vector.

    Deltas are the fundamental unit of computation in ATOMiK.
    They encode transformations that can be:
    - Composed with other deltas (via XOR)
    - Applied to states (via XOR)
    - Inverted (self-inverse property of XOR)

    Mathematical properties:
    - Closure: δ₁ ⊕ δ₂ ∈ Delta
    - Associativity: (δ₁ ⊕ δ₂) ⊕ δ₃ = δ₁ ⊕ (δ₂ ⊕ δ₃)
    - Identity: δ ⊕ 𝟎 = δ
    - Inverse: δ ⊕ δ = 𝟎
    - Commutativity: δ₁ ⊕ δ₂ = δ₂ ⊕ δ₁
-/
structure Delta where
  /-- The underlying bit vector representation -/
  bits : BitVec DELTA_WIDTH
  deriving DecidableEq, Repr, Inhabited

/-- Identity element: the zero delta (all bits cleared).
    Composing any delta with zero returns the original delta. -/
def Delta.zero : Delta := ⟨0⟩

/-- Alternative name for identity element -/
def Delta.identity : Delta := Delta.zero

/-- Compose two deltas via XOR.
    This is the group operation for the delta-state algebra.

    Properties proven in Properties.lean:
    - Associative: compose (compose a b) c = compose a (compose b c)
    - Commutative: compose a b = compose b a
    - Identity: compose a zero = a
    - Self-inverse: compose a a = zero
-/
def Delta.compose (a b : Delta) : Delta :=
  ⟨a.bits ^^^ b.bits⟩

/-- Infix notation for delta composition: δ₁ ⊕ δ₂ -/
instance : Add Delta where
  add := Delta.compose

/-- HAdd instance for delta composition -/
instance : HAdd Delta Delta Delta where
  hAdd := Delta.compose

/-- The inverse of a delta is itself (XOR self-inverse property).
    For any delta δ: δ ⊕ δ = 𝟎 -/
def Delta.inverse (d : Delta) : Delta := d

/-- Negation is identity for deltas (self-inverse) -/
instance : Neg Delta where
  neg := Delta.inverse

/-- Apply a delta to a state, producing a new state.
    This is the fundamental state transition operation.

    transition(s, δ) = s XOR δ
-/
def Delta.apply (d : Delta) (s : State) : State :=
  s ^^^ d.bits

/-- Create a delta from a natural number -/
def Delta.ofNat (n : Nat) : Delta :=
  ⟨BitVec.ofNat DELTA_WIDTH n⟩

/-- Create a delta from a bit vector -/
def Delta.ofBitVec (bv : BitVec DELTA_WIDTH) : Delta := ⟨bv⟩

/-- Extract the underlying bit vector -/
def Delta.toBitVec (d : Delta) : BitVec DELTA_WIDTH := d.bits

/-- Check if a delta is the identity (zero) -/
def Delta.isZero (d : Delta) : Bool := d.bits == 0

/-- Helper function to count set bits in a natural number -/
private def countOnes (n : Nat) : Nat :=
  if h : n = 0 then 0
  else (n % 2) + countOnes (n / 2)
termination_by n
decreasing_by
  simp_wf
  exact Nat.div_lt_self (Nat.pos_of_ne_zero h) (by omega)

/-- Count the number of set bits in a delta (population count / Hamming weight) -/
def Delta.popCount (d : Delta) : Nat := countOnes d.bits.toNat

/-- Compute the Hamming distance between two deltas -/
def Delta.hammingDistance (a b : Delta) : Nat :=
  (Delta.compose a b).popCount

/-! ## Coercions and Conversions -/

/-- Coerce a BitVec to a Delta -/
instance : Coe (BitVec DELTA_WIDTH) Delta where
  coe := Delta.ofBitVec

/-- Coerce a Delta to a BitVec -/
instance : Coe Delta (BitVec DELTA_WIDTH) where
  coe := Delta.toBitVec

/-! ## Basic Lemmas (Definitional) -/

/-- Composing with zero on the right is identity (by definition) -/
theorem Delta.compose_zero_right (d : Delta) : Delta.compose d Delta.zero = d := by
  simp [Delta.compose, Delta.zero, BitVec.xor_zero]

/-- Composing with zero on the left is identity (by definition) -/
theorem Delta.compose_zero_left (d : Delta) : Delta.compose Delta.zero d = d := by
  simp [Delta.compose, Delta.zero, BitVec.zero_xor]

/-- Self-composition yields zero (by definition) -/
theorem Delta.compose_self (d : Delta) : Delta.compose d d = Delta.zero := by
  simp [Delta.compose, Delta.zero, BitVec.xor_self]

/-- The inverse property: d + (-d) = 0 -/
theorem Delta.add_neg_self (d : Delta) : d + (-d) = Delta.zero := by
  simp [HAdd.hAdd, Neg.neg, Delta.inverse, Delta.compose, Delta.zero, BitVec.xor_self]

end ATOMiK
