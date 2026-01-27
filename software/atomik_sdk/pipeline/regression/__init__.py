"""Regression detection system with baseline management."""

from .baseline import BaselineManager, BaselineSnapshot
from .detector import GateResult, PipelineRegressionDetector, RegressionGate

__all__ = [
    "PipelineRegressionDetector",
    "RegressionGate",
    "GateResult",
    "BaselineManager",
    "BaselineSnapshot",
]
