//! ATOMiK Delta-State Module
//! Generated from schema: Finance/Trading/PriceTick
//!
//! This module provides delta-state operations based on XOR algebra.

use std::collections::VecDeque;

/// PriceTick delta-state manager
#[derive(Debug, Clone)]
pub struct PriceTick {
    /// Initial state
    initial_state: u64,
    /// Delta accumulator (XOR of all deltas)
    accumulator: u64,
    /// Delta history for rollback
    history: VecDeque<u64>,
    /// Maximum history depth
    max_history: usize,
}

impl PriceTick {
    /// Create a new delta-state manager
    pub fn new() -> Self {
        Self {
            initial_state: 0,
            accumulator: 0,
            history: VecDeque::new(),
            max_history: 4096,
        }
    }

    /// Load initial state (LOAD operation)
    pub fn load(&mut self, initial_state: u64) {
        self.initial_state = initial_state;
        self.accumulator = 0;
        self.history.clear();
    }

    /// Accumulate delta (ACCUMULATE operation)
    ///
    /// XORs the delta into the accumulator.
    pub fn accumulate(&mut self, delta: u64) {
        // Save to history
        self.history.push_back(delta);
        if self.history.len() > self.max_history {
            self.history.pop_front();
        }
        // XOR delta into accumulator
        self.accumulator ^= delta;
    }

    /// Reconstruct current state (READ operation)
    ///
    /// Returns current_state = initial_state XOR accumulator
    pub fn reconstruct(&self) -> u64 {
        self.initial_state ^ self.accumulator
    }

    /// Check if accumulator is zero (STATUS operation)
    pub fn is_accumulator_zero(&self) -> bool {
        self.accumulator == 0
    }

    /// Rollback the last N delta operations
    ///
    /// Returns the number of deltas actually rolled back.
    pub fn rollback(&mut self, count: usize) -> usize {
        let actual_count = count.min(self.history.len());
        for _ in 0..actual_count {
            if let Some(delta) = self.history.pop_back() {
                // XOR removes the delta (self-inverse property)
                self.accumulator ^= delta;
            }
        }
        actual_count
    }

    /// Get the current accumulator value
    pub fn get_accumulator(&self) -> u64 {
        self.accumulator
    }

    /// Get the initial state
    pub fn get_initial_state(&self) -> u64 {
        self.initial_state
    }

    /// Get the number of deltas in history
    pub fn history_size(&self) -> usize {
        self.history.len()
    }

}

impl Default for PriceTick {
    fn default() -> Self {
        Self::new()
    }
}
