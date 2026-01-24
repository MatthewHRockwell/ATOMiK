-- ATOMiK/Basic.lean
-- Core type definitions and utilities for delta-state algebra
--
-- This module provides foundational definitions used throughout
-- the ATOMiK formal verification library.
--
-- Task: T1.1 - Define delta-state algebra axioms
-- Phase: 1 - Mathematical Formalization

namespace ATOMiK

/-- The bit width used for all delta computations.
    ATOMiK uses 64-bit vectors for state representation. -/
def DELTA_WIDTH : Nat := 64

/-- State is represented as a 64-bit vector.
    In ATOMiK, state is the accumulation of all applied deltas. -/
abbrev State := BitVec DELTA_WIDTH

/-- The zero state - all bits cleared.
    This serves as the initial state before any deltas are applied. -/
def State.zero : State := 0

/-- Apply XOR operation between two states.
    This is the fundamental operation for delta application. -/
def State.xor (s₁ s₂ : State) : State := s₁ ^^^ s₂

/-- Notation for state XOR operation -/
instance : HXor State State State where
  hXor := State.xor

end ATOMiK
