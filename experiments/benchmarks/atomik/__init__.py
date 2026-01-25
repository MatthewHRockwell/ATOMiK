"""ATOMiK delta-state implementation.

This module implements the delta-state algebra proven in Phase 1
for benchmarking against traditional SCORE.
"""

from .delta_engine import DeltaEngine
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
    'DeltaEngine',
    'MatrixWorkload',
    'StateMachineWorkload',
    'StreamingWorkload',
    'CompositionWorkload',
    'MixedWorkload',
    'ScalingWorkload',
    'ParallelWorkload',
    'CacheWorkload',
]
