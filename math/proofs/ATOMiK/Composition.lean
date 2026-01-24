-- ATOMiK/Composition.lean
-- Composition operator semantics
--
-- This module defines and verifies sequential and parallel
-- composition operators for delta chains.
--
-- Task: T1.4 - Formalize composition operators
-- Phase: 1 - Mathematical Formalization

import ATOMiK.Properties

namespace ATOMiK

/-! ## Composition Operators

In ATOMiK, deltas can be composed in two conceptually distinct ways:

1. **Sequential Composition**: Applying deltas one after another to a state.
   Since XOR is associative, sequential composition of deltas is equivalent
   to composing the deltas first, then applying once.

2. **Parallel Composition**: Combining independent deltas that affect
   disjoint parts of the state. This is also XOR, but with semantic
   meaning that the deltas are independent.

Both operations are mathematically identical (XOR), but we provide
distinct APIs for semantic clarity and to enable future optimizations.
-/

/-! ### Sequential Composition -/

/-- Sequential composition: apply delta `a` then delta `b`.
    
    Mathematically: seq a b = a ⊕ b
    
    Semantically: first apply `a` to state, then apply `b` to result.
    Due to XOR properties, this equals applying (a ⊕ b) once.
-/
def Delta.seq (a b : Delta) : Delta := Delta.compose a b

/-- Sequential composition notation -/
infixl:65 " >> " => Delta.seq

/-- Sequential composition is associative -/
theorem Delta.seq_assoc (a b c : Delta) : (a >> b) >> c = a >> (b >> c) := by
  simp [Delta.seq, delta_assoc]

/-- Sequential composition with identity -/
theorem Delta.seq_zero_right (d : Delta) : d >> Delta.zero = d := by
  simp [Delta.seq, Delta.compose_zero_right]

theorem Delta.seq_zero_left (d : Delta) : Delta.zero >> d = d := by
  simp [Delta.seq, Delta.compose_zero_left]

/-- Sequential self-composition cancels -/
theorem Delta.seq_self (d : Delta) : d >> d = Delta.zero := by
  simp [Delta.seq, Delta.compose_self]

/-! ### Parallel Composition -/

/-- Parallel composition: combine independent deltas `a` and `b`.
    
    Mathematically: par a b = a ⊕ b
    
    Semantically: `a` and `b` affect independent state regions,
    so they can be applied in any order or simultaneously.
-/
def Delta.par (a b : Delta) : Delta := Delta.compose a b

/-- Parallel composition notation -/
infixl:70 " ||| " => Delta.par

/-- Parallel composition is commutative (order doesn't matter) -/
theorem Delta.par_comm (a b : Delta) : a ||| b = b ||| a := by
  simp [Delta.par, delta_comm]

/-- Parallel composition is associative -/
theorem Delta.par_assoc (a b c : Delta) : (a ||| b) ||| c = a ||| (b ||| c) := by
  simp [Delta.par, delta_assoc]

/-- Parallel composition with identity -/
theorem Delta.par_zero (d : Delta) : d ||| Delta.zero = d := by
  simp [Delta.par, Delta.compose_zero_right]

/-- Parallel self-composition cancels -/
theorem Delta.par_self (d : Delta) : d ||| d = Delta.zero := by
  simp [Delta.par, Delta.compose_self]

/-! ### Equivalence of Sequential and Parallel -/

/-- Sequential and parallel composition are identical operations -/
theorem Delta.seq_eq_par (a b : Delta) : a >> b = a ||| b := rfl

/-- Sequential composition is also commutative (inherited from XOR) -/
theorem Delta.seq_comm (a b : Delta) : a >> b = b >> a := by
  simp [Delta.seq, delta_comm]

/-! ### Delta List Operations -/

/-- Compose a list of deltas into a single delta (left fold) -/
def Delta.composeAll (ds : List Delta) : Delta :=
  ds.foldl Delta.compose Delta.zero

/-- Empty list composes to identity -/
theorem Delta.composeAll_nil : Delta.composeAll [] = Delta.zero := rfl

/-- Singleton list composes to itself -/
theorem Delta.composeAll_singleton (d : Delta) : 
    Delta.composeAll [d] = d := by
  simp [Delta.composeAll, Delta.compose_zero_left]

/-- Two element list -/
theorem Delta.composeAll_pair (a b : Delta) :
    Delta.composeAll [a, b] = Delta.compose a b := by
  simp [Delta.composeAll, Delta.compose_zero_left]

/-- Three element list -/
theorem Delta.composeAll_triple (a b c : Delta) :
    Delta.composeAll [a, b, c] = Delta.compose (Delta.compose a b) c := by
  simp [Delta.composeAll, Delta.compose_zero_left]

/-! ### State Application -/

/-- Apply a list of deltas to a state sequentially -/
def Delta.applyAll (ds : List Delta) (s : State) : State :=
  ds.foldl (fun st d => Delta.apply d st) s

/-- Applying empty list returns original state -/
theorem Delta.applyAll_nil (s : State) : Delta.applyAll [] s = s := rfl

/-- Applying singleton -/
theorem Delta.applyAll_singleton (d : Delta) (s : State) :
    Delta.applyAll [d] s = Delta.apply d s := rfl

/-! ### Composition Preserves Application -/

/-- Key theorem: applying composed delta equals sequential application -/
theorem Delta.apply_compose (a b : Delta) (s : State) :
    Delta.apply (Delta.compose a b) s = Delta.apply b (Delta.apply a s) := by
  simp [Delta.apply, Delta.compose, BitVec.xor_assoc]

/-- Applying two deltas sequentially -/
theorem Delta.apply_seq (a b : Delta) (s : State) :
    Delta.apply (a >> b) s = Delta.apply b (Delta.apply a s) := by
  exact Delta.apply_compose a b s

/-- Applying three deltas -/
theorem Delta.apply_three (a b c : Delta) (s : State) :
    Delta.apply c (Delta.apply b (Delta.apply a s)) = 
    Delta.apply (Delta.compose (Delta.compose a b) c) s := by
  simp [Delta.apply, Delta.compose, BitVec.xor_assoc]

/-! ### Parallel Reduction -/

/-- Reduce a list of independent deltas in parallel -/
def Delta.parAll (ds : List Delta) : Delta :=
  ds.foldl Delta.par Delta.zero

/-- parAll and composeAll are definitionally equal -/
theorem Delta.parAll_def (ds : List Delta) :
    Delta.parAll ds = ds.foldl Delta.compose Delta.zero := rfl

/-! ### Three-Way Composition -/

/-- Three deltas can be composed in any grouping -/
theorem Delta.compose_three (a b c : Delta) :
    Delta.compose (Delta.compose a b) c = 
    Delta.compose a (Delta.compose b c) :=
  delta_assoc a b c

/-- Three deltas can be composed in any order -/
theorem Delta.compose_three_perm (a b c : Delta) :
    Delta.compose (Delta.compose a b) c = Delta.compose (Delta.compose a c) b := by
  rw [delta_assoc, delta_assoc]
  congr 1
  exact delta_comm b c

/-- Four-way composition rearrangement -/
theorem Delta.compose_four (a b c d : Delta) :
    Delta.compose (Delta.compose (Delta.compose a b) c) d =
    Delta.compose (Delta.compose a b) (Delta.compose c d) := by
  rw [delta_assoc]

/-! ### Operator Laws Summary -/

/-- Summary of composition operator laws -/
theorem composition_laws :
    -- Sequential laws
    (∀ a b c : Delta, (a >> b) >> c = a >> (b >> c)) ∧
    (∀ d : Delta, d >> Delta.zero = d) ∧
    (∀ d : Delta, Delta.zero >> d = d) ∧
    -- Parallel laws  
    (∀ a b : Delta, a ||| b = b ||| a) ∧
    (∀ a b c : Delta, (a ||| b) ||| c = a ||| (b ||| c)) ∧
    (∀ d : Delta, d ||| Delta.zero = d) ∧
    -- Equivalence
    (∀ a b : Delta, a >> b = a ||| b) :=
  ⟨Delta.seq_assoc, Delta.seq_zero_right, Delta.seq_zero_left,
   Delta.par_comm, Delta.par_assoc, Delta.par_zero,
   fun _ _ => rfl⟩

end ATOMiK
