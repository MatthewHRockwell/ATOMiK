"""Baseline SCORE implementation - Traditional stateful architecture.

This module implements the traditional State-Centric Operation with
Register Execution (SCORE) model for benchmarking against ATOMiK.
"""

from .state_manager import StateManager
from .workloads import (
    MatrixWorkload,
    StateMachineWorkload,
    StreamingWorkload,
    CompositionWorkload,
    MixedWorkload,
    ScalingWorkload,
    ParallelWorkload,
    CacheWorkload,
)

__all__ = [
    'StateManager',
    'MatrixWorkload',
    'StateMachineWorkload',
    'StreamingWorkload',
    'CompositionWorkload',
    'MixedWorkload',
    'ScalingWorkload',
    'ParallelWorkload',
    'CacheWorkload',
]
