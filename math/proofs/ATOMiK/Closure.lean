-- ATOMiK/Closure.lean
-- Closure property proofs for delta-state algebra
--
-- This module proves that the set of deltas is closed under composition:
-- For any two deltas δ₁ and δ₂, their composition δ₁ ⊕ δ₂ is also a delta.
--
-- Task: T1.2 - Prove closure properties
-- Phase: 1 - Mathematical Formalization

import ATOMiK.Delta

namespace ATOMiK

/-! ## Closure Properties

The closure property is automatically satisfied by our type system:
`Delta.compose` has type `Delta → Delta → Delta`, which means the
result is always a valid Delta by construction.

This is a key advantage of using dependent types - closure is enforced
at compile time rather than requiring runtime checks or separate proofs.

We state explicit closure theorems for documentation and to establish
the formal foundation for the Abelian group structure.
-/

/-! ### Primary Closure Theorem -/

/-- Closure under composition: composing two deltas produces a valid delta.
    
    This is the first axiom of a group: the set is closed under the operation.
    In our case, this is guaranteed by the type signature of `Delta.compose`,
    but we state it explicitly for formal completeness.
    
    ∀ a b : Delta, ∃ c : Delta, c = a ⊕ b
-/
theorem delta_closure (a b : Delta) : ∃ c : Delta, c = Delta.compose a b :=
  ⟨Delta.compose a b, rfl⟩

/-- Alternative statement: the composition of two deltas is a delta.
    This is trivially true by the return type of compose. -/
theorem delta_compose_is_delta (a b : Delta) : (Delta.compose a b).bits.toNat < 2^DELTA_WIDTH := by
  exact (Delta.compose a b).bits.isLt

/-! ### Closure of Specific Operations -/

/-- XOR of two valid bit vectors produces a valid bit vector.
    This underlies the closure of delta composition. -/
theorem bitvec_xor_closure (a b : BitVec DELTA_WIDTH) : 
    (a ^^^ b).toNat < 2^DELTA_WIDTH :=
  (a ^^^ b).isLt

/-- Closure is preserved through the Delta constructor. -/
theorem delta_mk_closure (bv : BitVec DELTA_WIDTH) : 
    (⟨bv⟩ : Delta).bits = bv :=
  rfl

/-- Composing any delta with identity preserves membership in Delta. -/
theorem delta_compose_zero_closure (d : Delta) : 
    ∃ c : Delta, c = Delta.compose d Delta.zero :=
  ⟨d, (Delta.compose_zero_right d).symm⟩

/-- Self-composition produces the identity element (which is in Delta). -/
theorem delta_compose_self_closure (d : Delta) : 
    ∃ c : Delta, c = Delta.compose d d ∧ c = Delta.zero :=
  ⟨Delta.zero, ⟨(Delta.compose_self d).symm, rfl⟩⟩

/-! ### Closure Under Derived Operations -/

/-- The inverse of a delta is a delta (trivially, since inverse = identity function). -/
theorem delta_inverse_closure (d : Delta) : ∃ c : Delta, c = Delta.inverse d :=
  ⟨d, rfl⟩

/-- Applying a delta to a state produces a valid state. -/
theorem delta_apply_closure (d : Delta) (s : State) : 
    (Delta.apply d s).toNat < 2^DELTA_WIDTH :=
  (Delta.apply d s).isLt

/-! ### N-ary Closure -/

/-- Closure extends to lists: folding compose over a list of deltas produces a delta. -/
def Delta.composeList (ds : List Delta) : Delta :=
  ds.foldl Delta.compose Delta.zero

/-- The result of composeList is always a valid delta. -/
theorem delta_compose_list_closure (ds : List Delta) : 
    ∃ c : Delta, c = Delta.composeList ds :=
  ⟨Delta.composeList ds, rfl⟩

/-- Composing a list with a single element equals that element. -/
theorem delta_compose_list_singleton (d : Delta) : 
    Delta.composeList [d] = d := by
  simp [Delta.composeList, Delta.compose_zero_left]

/-- Composing an empty list produces the identity. -/
theorem delta_compose_list_nil : Delta.composeList [] = Delta.zero := rfl

/-! ### Type-Level Closure Guarantees -/

/-- The Add instance preserves closure (by definition). -/
theorem delta_add_closure (a b : Delta) : ∃ c : Delta, c = a + b :=
  ⟨a + b, rfl⟩

/-- The negation instance preserves closure (by definition). -/
theorem delta_neg_closure (d : Delta) : ∃ c : Delta, c = -d :=
  ⟨-d, rfl⟩

end ATOMiK
