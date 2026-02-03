"""Baseline workload implementations using traditional stateful architecture."""

import random
from typing import List, Tuple

try:
    from .state_manager import StateManager
except ImportError:
    from state_manager import StateManager


class MatrixWorkload:
    """W1.1: Matrix operations using traditional state storage.

    Simulates dense matrix multiplication by storing full matrices
    and performing read-modify-write updates.
    """

    def __init__(self, size: int = 64):
        """Initialize matrix workload.

        Args:
            size: Matrix dimension (size x size)
        """
        self.size = size
        self.managers = [StateManager() for _ in range(size * size)]

    def run(self, iterations: int = 100) -> dict:
        """Execute matrix operations.

        Args:
            iterations: Number of matrix operations to perform

        Returns:
            Metrics dictionary with memory and operation counts
        """
        total_reads = 0
        total_writes = 0

        for _ in range(iterations):
            # Simulate matrix update (read-modify-write pattern)
            for manager in self.managers:
                manager.modify(lambda x: (x ^ random.randint(0, 2**64 - 1)) & ((1 << 64) - 1))

        # Collect metrics
        for manager in self.managers:
            total_reads += manager.read_count
            total_writes += manager.write_count

        return {
            'total_reads': total_reads,
            'total_writes': total_writes,
            'memory_bytes': sum(m.memory_footprint() for m in self.managers),
            'workload': 'matrix',
            'size': self.size,
        }


class StateMachineWorkload:
    """W1.2: Finite state automaton with traditional state storage."""

    def __init__(self, num_states: int = 256):
        """Initialize state machine workload.

        Args:
            num_states: Number of states in automaton
        """
        self.num_states = num_states
        self.manager = StateManager(initial_state=0, track_history=True)
        self.transitions = self._generate_transitions()

    def _generate_transitions(self) -> dict:
        """Generate random state transition table."""
        return {i: (i + 1) % self.num_states for i in range(self.num_states)}

    def run(self, steps: int = 1000) -> dict:
        """Execute state machine transitions.

        Args:
            steps: Number of state transitions

        Returns:
            Metrics dictionary
        """
        for _ in range(steps):
            current = self.manager.read()
            next_state = self.transitions[current % self.num_states]
            self.manager.write(next_state)

        return {
            'total_reads': self.manager.read_count,
            'total_writes': self.manager.write_count,
            'memory_bytes': self.manager.memory_footprint(),
            'history_length': len(self.manager.history) if self.manager.history else 0,
            'workload': 'state_machine',
            'num_states': self.num_states,
        }


class StreamingWorkload:
    """W1.3: Streaming data processing with intermediate state storage."""

    def __init__(self, num_stages: int = 10):
        """Initialize streaming workload.

        Args:
            num_stages: Number of processing stages
        """
        self.num_stages = num_stages
        self.stages = [StateManager() for _ in range(num_stages)]

    def run(self, data_points: int = 1000) -> dict:
        """Process streaming data through pipeline.

        Args:
            data_points: Number of data points to process

        Returns:
            Metrics dictionary
        """
        for _ in range(data_points):
            value = random.randint(0, 2**64 - 1)
            # Pass through each stage (serial pipeline)
            for stage in self.stages:
                stage.modify(lambda x: (x ^ value) & ((1 << 64) - 1))

        total_reads = sum(s.read_count for s in self.stages)
        total_writes = sum(s.write_count for s in self.stages)
        memory_bytes = sum(s.memory_footprint() for s in self.stages)

        return {
            'total_reads': total_reads,
            'total_writes': total_writes,
            'memory_bytes': memory_bytes,
            'workload': 'streaming',
            'num_stages': self.num_stages,
        }


class CompositionWorkload:
    """W2.1: Delta composition using sequential state updates."""

    def __init__(self, chain_length: int = 100):
        """Initialize composition workload.

        Args:
            chain_length: Number of operations to chain
        """
        self.chain_length = chain_length
        self.manager = StateManager()

    def run(self, repetitions: int = 100) -> dict:
        """Execute chained state updates.

        Args:
            repetitions: Number of times to repeat the chain

        Returns:
            Metrics dictionary
        """
        for _ in range(repetitions):
            # Chain of operations (each depends on previous)
            for _ in range(self.chain_length):
                delta = random.randint(0, 2**64 - 1)
                self.manager.modify(lambda x, d=delta: (x ^ d) & ((1 << 64) - 1))

        return {
            'total_reads': self.manager.read_count,
            'total_writes': self.manager.write_count,
            'memory_bytes': self.manager.memory_footprint(),
            'workload': 'composition',
            'chain_length': self.chain_length,
        }


class MixedWorkload:
    """W2.3: Mixed read/write operations."""

    def __init__(self, read_ratio: float = 0.7):
        """Initialize mixed workload.

        Args:
            read_ratio: Fraction of operations that are reads (0.0-1.0)
        """
        self.read_ratio = read_ratio
        self.manager = StateManager()

    def run(self, operations: int = 1000) -> dict:
        """Execute mixed read/write operations.

        Args:
            operations: Total number of operations

        Returns:
            Metrics dictionary
        """
        for _ in range(operations):
            if random.random() < self.read_ratio:
                # Read operation
                _ = self.manager.read()
            else:
                # Write operation
                delta = random.randint(0, 2**64 - 1)
                self.manager.modify(lambda x, d=delta: (x ^ d) & ((1 << 64) - 1))

        return {
            'total_reads': self.manager.read_count,
            'total_writes': self.manager.write_count,
            'memory_bytes': self.manager.memory_footprint(),
            'workload': 'mixed',
            'read_ratio': self.read_ratio,
        }


class ScalingWorkload:
    """W3.1: Problem size scaling test."""

    def __init__(self, problem_size: int = 256):
        """Initialize scaling workload.

        Args:
            problem_size: Number of state elements
        """
        self.problem_size = problem_size
        self.managers = [StateManager() for _ in range(problem_size)]

    def run(self, operations_per_element: int = 10) -> dict:
        """Execute operations across all elements.

        Args:
            operations_per_element: Operations per state element

        Returns:
            Metrics dictionary
        """
        for _ in range(operations_per_element):
            for manager in self.managers:
                delta = random.randint(0, 2**64 - 1)
                manager.modify(lambda x, d=delta: (x ^ d) & ((1 << 64) - 1))

        total_reads = sum(m.read_count for m in self.managers)
        total_writes = sum(m.write_count for m in self.managers)
        memory_bytes = sum(m.memory_footprint() for m in self.managers)

        return {
            'total_reads': total_reads,
            'total_writes': total_writes,
            'memory_bytes': memory_bytes,
            'workload': 'scaling',
            'problem_size': self.problem_size,
        }


class ParallelWorkload:
    """W3.2: Parallel composition (baseline has serial dependencies).

    Note: Traditional stateful approach CANNOT parallelize due to
    data dependencies. This serves as the baseline for comparison.
    """

    def __init__(self, num_operations: int = 100):
        """Initialize parallel workload.

        Args:
            num_operations: Number of operations to compose
        """
        self.num_operations = num_operations
        self.manager = StateManager()

    def run(self, repetitions: int = 10) -> dict:
        """Execute operations serially (cannot parallelize).

        Args:
            repetitions: Number of repetition runs

        Returns:
            Metrics dictionary
        """
        for _ in range(repetitions):
            # Must execute serially due to data dependencies
            for _ in range(self.num_operations):
                delta = random.randint(0, 2**64 - 1)
                self.manager.modify(lambda x, d=delta: (x ^ d) & ((1 << 64) - 1))

        return {
            'total_reads': self.manager.read_count,
            'total_writes': self.manager.write_count,
            'memory_bytes': self.manager.memory_footprint(),
            'parallel_efficiency': 0.0,  # Serial execution
            'workload': 'parallel',
            'num_operations': self.num_operations,
        }


class CacheWorkload:
    """W3.3: Cache locality test with varying working set sizes."""

    def __init__(self, working_set_kb: int = 64):
        """Initialize cache workload.

        Args:
            working_set_kb: Working set size in KB
        """
        self.working_set_kb = working_set_kb
        # Each StateManager ~= 24 bytes, so calculate count
        self.num_managers = (working_set_kb * 1024) // 24
        self.managers = [StateManager() for _ in range(self.num_managers)]

    def run(self, iterations: int = 100) -> dict:
        """Execute cache-thrashing pattern.

        Args:
            iterations: Number of full sweeps through working set

        Returns:
            Metrics dictionary
        """
        for _ in range(iterations):
            # Random access pattern (poor locality)
            indices = list(range(self.num_managers))
            random.shuffle(indices)
            for idx in indices:
                delta = random.randint(0, 2**64 - 1)
                self.managers[idx].modify(lambda x, d=delta: (x ^ d) & ((1 << 64) - 1))

        total_reads = sum(m.read_count for m in self.managers)
        total_writes = sum(m.write_count for m in self.managers)
        memory_bytes = sum(m.memory_footprint() for m in self.managers)

        return {
            'total_reads': total_reads,
            'total_writes': total_writes,
            'memory_bytes': memory_bytes,
            'workload': 'cache',
            'working_set_kb': self.working_set_kb,
        }
