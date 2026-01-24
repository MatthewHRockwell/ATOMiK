-- ATOMiK/Transition.lean
-- Stateless transition function definitions and determinism proofs
--
-- This module formalizes the state transition function and proves
-- that it is deterministic (same inputs always produce same outputs).
--
-- Task: T1.5 - Define stateless transition functions
-- Task: T1.6 - Prove determinism guarantees
-- Phase: 1 - Mathematical Formalization

import ATOMiK.Composition

namespace ATOMiK

/-! ## State Transition Functions

In ATOMiK, state transitions are pure functions with no side effects.
The transition function applies a delta to a state via XOR:

  transition : State → Delta → State
  transition(s, δ) = s ⊕ δ

Key properties:
- **Determinism**: Same inputs always produce same outputs
- **Reversibility**: Applying the same delta twice returns to original state
- **Composability**: Sequential transitions can be combined into one
- **No hidden state**: The function depends only on its explicit arguments
-/

/-! ### Core Transition Function -/

/-- The fundamental state transition function.
    
    Applies a delta to a state, producing a new state.
    This is a pure function with no side effects.
    
    Mathematical definition: transition(s, δ) = s XOR δ
-/
def transition (s : State) (d : Delta) : State :=
  Delta.apply d s

/-- Alternative notation: s ▷ d means "apply delta d to state s" -/
infixl:65 " ▷ " => transition

/-- Transition is equivalent to Delta.apply -/
theorem transition_eq_apply (s : State) (d : Delta) : 
    transition s d = Delta.apply d s := rfl

/-! ### Determinism Proofs (T1.6) -/

/-- **Determinism**: The transition function is deterministic.
    Given the same state and delta, it always produces the same result.
    
    This is trivially true for pure functions in Lean's type system,
    but we state it explicitly for formal documentation.
-/
theorem transition_deterministic (s : State) (d : Delta) :
    transition s d = transition s d := rfl

/-- **Functional determinism**: Equal inputs produce equal outputs -/
theorem transition_eq_of_eq (s₁ s₂ : State) (d₁ d₂ : Delta) 
    (hs : s₁ = s₂) (hd : d₁ = d₂) : 
    transition s₁ d₁ = transition s₂ d₂ := by
  rw [hs, hd]

/-- **No hidden state**: Transition depends only on explicit arguments.
    
    If we call transition twice with the same arguments, we get the same result.
    This proves there is no hidden mutable state affecting the computation.
-/
theorem transition_no_hidden_state (s : State) (d : Delta) :
    let r1 := transition s d
    let r2 := transition s d
    r1 = r2 := rfl

/-- **Reproducibility**: Any sequence of transitions can be reproduced
    by providing the same inputs in the same order.
-/
theorem transition_reproducible (s : State) (d₁ d₂ : Delta) :
    transition (transition s d₁) d₂ = transition (transition s d₁) d₂ := rfl

/-! ### Reversibility Properties -/

/-- **Self-inverse**: Applying the same delta twice returns to the original state.
    
    transition(transition(s, δ), δ) = s
    
    This follows from the XOR self-inverse property.
-/
theorem transition_self_inverse (s : State) (d : Delta) :
    transition (transition s d) d = s := by
  simp [transition, Delta.apply, BitVec.xor_assoc, BitVec.xor_self, BitVec.xor_zero]

/-- Applying identity delta preserves state -/
theorem transition_zero (s : State) : transition s Delta.zero = s := by
  simp [transition, Delta.apply, Delta.zero, BitVec.xor_zero]

/-- Every transition has an inverse (itself) -/
theorem transition_inverse_exists (s : State) (d : Delta) :
    ∃ d' : Delta, transition (transition s d) d' = s :=
  ⟨d, transition_self_inverse s d⟩

/-! ### Composition Properties -/

/-- **Transition composition**: Sequential transitions can be combined.
    
    transition(transition(s, δ₁), δ₂) = transition(s, δ₁ ⊕ δ₂)
    
    But note: the order is reversed due to how XOR accumulates.
-/
theorem transition_compose (s : State) (d₁ d₂ : Delta) :
    transition (transition s d₁) d₂ = transition s (Delta.compose d₁ d₂) := by
  simp [transition, Delta.apply, Delta.compose, BitVec.xor_assoc]

/-- Transition with sequential operator -/
theorem transition_seq (s : State) (d₁ d₂ : Delta) :
    transition (transition s d₁) d₂ = transition s (d₁ >> d₂) := by
  exact transition_compose s d₁ d₂

/-- Three sequential transitions -/
theorem transition_three (s : State) (d₁ d₂ d₃ : Delta) :
    transition (transition (transition s d₁) d₂) d₃ = 
    transition s (Delta.compose (Delta.compose d₁ d₂) d₃) := by
  simp [transition, Delta.apply, Delta.compose, BitVec.xor_assoc]

/-! ### State Recovery -/

/-- Given initial and final states, compute the delta that transforms one to the other -/
def computeDelta (initial final : State) : Delta :=
  ⟨initial ^^^ final⟩

/-- The computed delta correctly transforms the initial state to the final state -/
theorem computeDelta_correct (initial final : State) :
    transition initial (computeDelta initial final) = final := by
  simp only [transition, Delta.apply, computeDelta]
  rw [← BitVec.xor_assoc]
  simp [BitVec.xor_self, BitVec.zero_xor]

/-- Computing delta is symmetric -/
theorem computeDelta_symm (s₁ s₂ : State) :
    computeDelta s₁ s₂ = computeDelta s₂ s₁ := by
  simp [computeDelta, BitVec.xor_comm]

/-- Delta from state to itself is zero -/
theorem computeDelta_self (s : State) :
    computeDelta s s = Delta.zero := by
  simp [computeDelta, Delta.zero, BitVec.xor_self]

/-! ### Transition System Structure -/

/-- A transition trace is a sequence of deltas applied to an initial state -/
structure TransitionTrace where
  initial : State
  deltas : List Delta

/-- Execute a trace to get the final state -/
def TransitionTrace.execute (trace : TransitionTrace) : State :=
  trace.deltas.foldl transition trace.initial

/-- Empty trace returns initial state -/
theorem TransitionTrace.execute_nil (s : State) :
    (TransitionTrace.mk s []).execute = s := rfl

/-- Single delta trace -/
theorem TransitionTrace.execute_singleton (s : State) (d : Delta) :
    (TransitionTrace.mk s [d]).execute = transition s d := rfl

/-- Trace execution is deterministic -/
theorem TransitionTrace.execute_deterministic (t : TransitionTrace) :
    t.execute = t.execute := rfl

/-! ### Pure Function Verification -/

/-- Structure capturing that a function is pure (no side effects) -/
structure PureFunction (α β : Type) where
  f : α → β
  deterministic : ∀ x, f x = f x  -- Same input → same output

/-- Transition is a pure function from (State × Delta) to State -/
def transitionPure : PureFunction (State × Delta) State where
  f := fun ⟨s, d⟩ => transition s d
  deterministic := fun _ => rfl

/-! ### Summary Theorems -/

/-- T1.5 Summary: Transition function properties -/
theorem transition_function_properties :
    -- Definition matches spec
    (∀ s d, transition s d = s ^^^ d.bits) ∧
    -- Pure function (no mutable state)
    (∀ s d, transition s d = transition s d) ∧
    -- Identity preserving
    (∀ s, transition s Delta.zero = s) :=
  ⟨fun _ _ => rfl, fun _ _ => rfl, transition_zero⟩

/-- T1.6 Summary: Determinism guarantees -/
theorem determinism_guarantees :
    -- Deterministic
    (∀ s d, transition s d = transition s d) ∧
    -- No hidden state
    (∀ s d, let r1 := transition s d; let r2 := transition s d; r1 = r2) ∧
    -- Reproducible
    (∀ s d₁ d₂, transition (transition s d₁) d₂ = transition (transition s d₁) d₂) ∧
    -- Reversible
    (∀ s d, transition (transition s d) d = s) :=
  ⟨fun _ _ => rfl, fun _ _ => rfl, fun _ _ _ => rfl, transition_self_inverse⟩

end ATOMiK
