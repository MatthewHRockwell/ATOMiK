"""Unit tests for ATOMiK delta-state implementation."""

import sys
import os
sys.path.insert(0, os.path.dirname(__file__))

import unittest
from delta_engine import DeltaEngine
from workloads import (
    MatrixWorkload,
    StateMachineWorkload,
    StreamingWorkload,
    CompositionWorkload,
    MixedWorkload,
    ScalingWorkload,
    ParallelWorkload,
    CacheWorkload,
)


class TestDeltaEngine(unittest.TestCase):
    """Test DeltaEngine basic operations."""

    def test_initialization(self):
        """Test delta engine initializes correctly."""
        de = DeltaEngine(initial_state=42)
        self.assertEqual(de.initial_state, 42)
        self.assertEqual(de.delta_accumulator, 0)
        self.assertEqual(de.accumulate_count, 0)

    def test_accumulate(self):
        """Test delta accumulation."""
        de = DeltaEngine()
        de.accumulate(100)
        self.assertEqual(de.delta_accumulator, 100)
        self.assertEqual(de.accumulate_count, 1)

    def test_reconstruct(self):
        """Test state reconstruction."""
        de = DeltaEngine(initial_state=10)
        de.accumulate(5)  # 10 XOR 5 = 15
        state = de.reconstruct()
        self.assertEqual(state, 10 ^ 5)
        self.assertEqual(de.reconstruct_count, 1)

    def test_identity_property(self):
        """Test identity: δ ⊕ 0 = δ"""
        de = DeltaEngine()
        delta = 123456
        de.accumulate(delta)
        de.accumulate(0)
        self.assertEqual(de.delta_accumulator, delta)

    def test_inverse_property(self):
        """Test inverse: δ ⊕ δ = 0"""
        de = DeltaEngine()
        delta = 987654
        de.accumulate(delta)
        de.accumulate(delta)
        self.assertEqual(de.delta_accumulator, 0)

    def test_commutativity(self):
        """Test commutativity: δ₁ ⊕ δ₂ = δ₂ ⊕ δ₁"""
        de1 = DeltaEngine()
        de2 = DeltaEngine()

        delta1 = 111
        delta2 = 222

        # Order 1
        de1.accumulate(delta1)
        de1.accumulate(delta2)

        # Order 2
        de2.accumulate(delta2)
        de2.accumulate(delta1)

        self.assertEqual(de1.delta_accumulator, de2.delta_accumulator)

    def test_associativity(self):
        """Test associativity: (δ₁ ⊕ δ₂) ⊕ δ₃ = δ₁ ⊕ (δ₂ ⊕ δ₃)"""
        delta1, delta2, delta3 = 111, 222, 333

        # Left associative
        left = (delta1 ^ delta2) ^ delta3

        # Right associative
        right = delta1 ^ (delta2 ^ delta3)

        self.assertEqual(left, right)

    def test_delta_history(self):
        """Test delta history tracking."""
        de = DeltaEngine(track_deltas=True)
        de.accumulate(10)
        de.accumulate(20)
        de.accumulate(30)
        self.assertEqual(de.get_delta_at(0), 10)
        self.assertEqual(de.get_delta_at(1), 20)
        self.assertEqual(de.get_delta_at(2), 30)

    def test_memory_footprint(self):
        """Test memory footprint calculation."""
        de = DeltaEngine(track_deltas=True)
        base_footprint = de.memory_footprint()
        de.accumulate(100)
        de.accumulate(200)
        # Delta history should increase footprint
        self.assertGreater(de.memory_footprint(), base_footprint)

    def test_verify_properties(self):
        """Test that algebraic properties hold."""
        de = DeltaEngine()
        self.assertTrue(de.verify_properties())


class TestWorkloads(unittest.TestCase):
    """Test all workload implementations."""

    def test_matrix_workload(self):
        """Test matrix workload executes without errors."""
        wl = MatrixWorkload(size=8)
        result = wl.run(iterations=10)
        self.assertIn('memory_bytes', result)
        self.assertIn('total_accumulates', result)
        self.assertGreater(result['total_accumulates'], 0)

    def test_state_machine_workload(self):
        """Test state machine workload."""
        wl = StateMachineWorkload(num_states=10)
        result = wl.run(steps=100)
        self.assertEqual(result['total_accumulates'], 100)
        # Should have reconstructions for state queries
        self.assertGreater(result['total_reconstructs'], 0)

    def test_streaming_workload(self):
        """Test streaming workload."""
        wl = StreamingWorkload(num_stages=5)
        result = wl.run(data_points=100)
        self.assertGreater(result['memory_bytes'], 0)

    def test_composition_workload(self):
        """Test composition workload."""
        wl = CompositionWorkload(chain_length=10)
        result = wl.run(repetitions=10)
        self.assertEqual(result['total_accumulates'], 100)

    def test_mixed_workload(self):
        """Test mixed workload."""
        wl = MixedWorkload(read_ratio=0.5)
        result = wl.run(operations=100)
        # Should have both accumulates and reconstructs
        self.assertGreater(result['total_reconstructs'], 0)

    def test_scaling_workload(self):
        """Test scaling workload."""
        wl = ScalingWorkload(problem_size=10)
        result = wl.run(operations_per_element=5)
        self.assertEqual(result['problem_size'], 10)

    def test_parallel_workload(self):
        """Test parallel workload."""
        wl = ParallelWorkload(num_operations=10)
        result = wl.run(repetitions=5)
        # ATOMiK should have parallel efficiency > 0
        self.assertGreater(result['parallel_efficiency'], 0.0)

    def test_cache_workload(self):
        """Test cache workload."""
        wl = CacheWorkload(working_set_kb=1)
        result = wl.run(iterations=10)
        self.assertGreater(result['memory_bytes'], 0)


class TestEquivalence(unittest.TestCase):
    """Test that ATOMiK and baseline produce equivalent results."""

    def test_xor_equivalence(self):
        """Verify XOR composition produces same result as sequential application."""
        initial = 0
        deltas = [10, 20, 30, 40]

        # Sequential XOR
        result_sequential = initial
        for d in deltas:
            result_sequential ^= d

        # Batch XOR (what ATOMiK does)
        de = DeltaEngine(initial_state=initial)
        de.batch_accumulate(deltas)
        result_batch = de.reconstruct()

        self.assertEqual(result_sequential, result_batch)


if __name__ == '__main__':
    unittest.main()
