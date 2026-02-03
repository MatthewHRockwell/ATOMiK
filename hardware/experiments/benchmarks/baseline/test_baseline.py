"""Unit tests for baseline SCORE implementation."""

import sys
import os
sys.path.insert(0, os.path.dirname(__file__))

import unittest
from state_manager import StateManager
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


class TestStateManager(unittest.TestCase):
    """Test StateManager basic operations."""

    def test_initialization(self):
        """Test state manager initializes correctly."""
        sm = StateManager(initial_state=42)
        self.assertEqual(sm.state, 42)
        self.assertEqual(sm.read_count, 0)
        self.assertEqual(sm.write_count, 0)

    def test_read_write(self):
        """Test read and write operations."""
        sm = StateManager()
        sm.write(100)
        self.assertEqual(sm.read(), 100)
        self.assertEqual(sm.read_count, 1)
        self.assertEqual(sm.write_count, 1)

    def test_modify(self):
        """Test read-modify-write operation."""
        sm = StateManager(initial_state=10)
        sm.modify(lambda x: x * 2)
        self.assertEqual(sm.state, 20)
        self.assertEqual(sm.read_count, 1)
        self.assertEqual(sm.write_count, 1)

    def test_history_tracking(self):
        """Test state history tracking."""
        sm = StateManager(initial_state=0, track_history=True)
        sm.write(10)
        sm.write(20)
        sm.write(30)
        self.assertEqual(sm.get_state_at(0), 0)
        self.assertEqual(sm.get_state_at(1), 10)
        self.assertEqual(sm.get_state_at(2), 20)

    def test_memory_footprint(self):
        """Test memory footprint calculation."""
        sm = StateManager(track_history=True)
        base_footprint = sm.memory_footprint()
        sm.write(100)
        sm.write(200)
        # History should increase footprint
        self.assertGreater(sm.memory_footprint(), base_footprint)


class TestWorkloads(unittest.TestCase):
    """Test all workload implementations."""

    def test_matrix_workload(self):
        """Test matrix workload executes without errors."""
        wl = MatrixWorkload(size=8)
        result = wl.run(iterations=10)
        self.assertIn('memory_bytes', result)
        self.assertIn('total_reads', result)
        self.assertIn('total_writes', result)
        self.assertGreater(result['total_reads'], 0)

    def test_state_machine_workload(self):
        """Test state machine workload."""
        wl = StateMachineWorkload(num_states=10)
        result = wl.run(steps=100)
        self.assertEqual(result['total_reads'], 100)
        self.assertEqual(result['total_writes'], 100)

    def test_streaming_workload(self):
        """Test streaming workload."""
        wl = StreamingWorkload(num_stages=5)
        result = wl.run(data_points=100)
        self.assertGreater(result['memory_bytes'], 0)

    def test_composition_workload(self):
        """Test composition workload."""
        wl = CompositionWorkload(chain_length=10)
        result = wl.run(repetitions=10)
        self.assertEqual(result['total_reads'], 100)
        self.assertEqual(result['total_writes'], 100)

    def test_mixed_workload(self):
        """Test mixed read/write workload."""
        wl = MixedWorkload(read_ratio=0.5)
        result = wl.run(operations=100)
        # Should have both reads and writes
        self.assertGreater(result['total_reads'], 0)

    def test_scaling_workload(self):
        """Test scaling workload."""
        wl = ScalingWorkload(problem_size=10)
        result = wl.run(operations_per_element=5)
        self.assertEqual(result['problem_size'], 10)

    def test_parallel_workload(self):
        """Test parallel workload (serial baseline)."""
        wl = ParallelWorkload(num_operations=10)
        result = wl.run(repetitions=5)
        self.assertEqual(result['parallel_efficiency'], 0.0)

    def test_cache_workload(self):
        """Test cache workload."""
        wl = CacheWorkload(working_set_kb=1)
        result = wl.run(iterations=10)
        self.assertGreater(result['memory_bytes'], 0)


if __name__ == '__main__':
    unittest.main()
