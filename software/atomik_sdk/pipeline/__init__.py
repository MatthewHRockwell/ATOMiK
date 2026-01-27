"""
ATOMiK Autonomous Pipeline System

Orchestrates schema validation, code generation, verification,
hardware-in-the-loop testing, and performance metrics collection
through a stage-based autonomous pipeline.

Phase 5 adds event-driven DAG orchestration, feedback loops,
adaptive model routing, multi-agent parallelism, deep verification,
and cross-run learning.
"""

from .consensus import ConsensusResolver, ConsensusResult
from .controller import Pipeline, PipelineConfig, PipelineResult
from .coordinator import Coordinator, CoordinatorResult
from .dag import CycleError, DAGTask, TaskDAG, TaskState
from .event_bus import Event, EventBus, EventType
from .orchestrator import Orchestrator

__all__ = [
    "Pipeline", "PipelineConfig", "PipelineResult",
    "TaskDAG", "DAGTask", "TaskState", "CycleError",
    "EventBus", "Event", "EventType",
    "Orchestrator",
    "Coordinator", "CoordinatorResult",
    "ConsensusResolver", "ConsensusResult",
]
