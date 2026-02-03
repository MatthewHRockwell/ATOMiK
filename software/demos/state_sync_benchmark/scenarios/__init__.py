"""Benchmark scenario implementations."""

from .distributed_sync import DistributedSyncScenario
from .memory_traffic import MemoryTrafficScenario
from .rollback import RollbackScenario
from .sensor_fusion import SensorFusionScenario

ALL_SCENARIOS = [
    SensorFusionScenario,
    RollbackScenario,
    DistributedSyncScenario,
    MemoryTrafficScenario,
]

__all__ = [
    "ALL_SCENARIOS",
    "SensorFusionScenario",
    "RollbackScenario",
    "DistributedSyncScenario",
    "MemoryTrafficScenario",
]
