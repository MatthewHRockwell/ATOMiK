"""Regression detection system with baseline management."""

from .detector import PipelineRegressionDetector, RegressionGate, GateResult
from .baseline import BaselineManager, BaselineSnapshot

__all__ = [
    "PipelineRegressionDetector",
    "RegressionGate",
    "GateResult",
    "BaselineManager",
    "BaselineSnapshot",
]
