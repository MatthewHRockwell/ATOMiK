"""
Model Router

Routes pipeline subtasks to the cheapest capable model tier.
Implements the DETERMINISTIC -> MECHANICAL -> GENERATIVE -> NOVEL
escalation chain.
"""

from __future__ import annotations

from enum import Enum
from typing import Any


class ModelTier(Enum):
    """Model tiers ordered by cost (lowest to highest)."""
    LOCAL = "local"         # No LLM, deterministic execution
    HAIKU = "haiku"         # Haiku 4.5 -- mechanical tasks
    SONNET = "sonnet"       # Sonnet 4.5 -- generative tasks
    OPUS = "opus"           # Opus 4.5 -- novel reasoning


class TaskClass(Enum):
    """Classification of pipeline subtasks."""
    DETERMINISTIC = "deterministic"   # lint, test, diff, report, metrics
    MECHANICAL = "mechanical"         # known template, known fix
    GENERATIVE = "generative"         # code generation, new patterns
    NOVEL = "novel"                   # unknown error, architectural decision


# Default routing: task class -> model tier
DEFAULT_ROUTING = {
    TaskClass.DETERMINISTIC: ModelTier.LOCAL,
    TaskClass.MECHANICAL: ModelTier.HAIKU,
    TaskClass.GENERATIVE: ModelTier.SONNET,
    TaskClass.NOVEL: ModelTier.OPUS,
}

# Estimated token costs per model tier
TOKEN_ESTIMATES = {
    ModelTier.LOCAL: 0,
    ModelTier.HAIKU: 2000,
    ModelTier.SONNET: 8000,
    ModelTier.OPUS: 20000,
}

# Pipeline stage -> default task classification
STAGE_CLASSIFICATION = {
    "validate": TaskClass.DETERMINISTIC,
    "diff": TaskClass.DETERMINISTIC,
    "generate": TaskClass.DETERMINISTIC,    # GeneratorEngine is local
    "verify_lint": TaskClass.DETERMINISTIC,
    "verify_test": TaskClass.DETERMINISTIC,
    "verify_diagnosis": TaskClass.MECHANICAL,
    "hardware_sim": TaskClass.DETERMINISTIC,
    "hardware_synth": TaskClass.DETERMINISTIC,
    "hardware_program": TaskClass.DETERMINISTIC,
    "hardware_validate": TaskClass.DETERMINISTIC,
    "metrics_collect": TaskClass.DETERMINISTIC,
    "metrics_report": TaskClass.DETERMINISTIC,
    "self_correct_known": TaskClass.MECHANICAL,
    "self_correct_unknown": TaskClass.GENERATIVE,
}


class ModelRouter:
    """
    Routes pipeline subtasks to appropriate model tiers.

    Follows the principle: run locally whenever possible,
    escalate to LLM only when deterministic execution fails.

    Example:
        >>> router = ModelRouter()
        >>> tier = router.route("validate")
        >>> assert tier == ModelTier.LOCAL
        >>> tier = router.escalate("verify_diagnosis")
        >>> assert tier == ModelTier.SONNET
    """

    def __init__(self, custom_routing: dict[str, ModelTier] | None = None):
        self._routing = dict(STAGE_CLASSIFICATION)
        if custom_routing:
            for stage, tier in custom_routing.items():
                self._routing[stage] = tier

    def route(self, stage_name: str) -> ModelTier:
        """
        Determine the model tier for a pipeline subtask.

        Args:
            stage_name: Name of the pipeline stage or subtask.

        Returns:
            ModelTier to use for this task.
        """
        task_class = self._routing.get(stage_name, TaskClass.GENERATIVE)
        if isinstance(task_class, TaskClass):
            return DEFAULT_ROUTING[task_class]
        return task_class

    def escalate(self, stage_name: str) -> ModelTier:
        """
        Get the next-tier model for escalation after failure.

        Args:
            stage_name: Name of the stage that failed.

        Returns:
            Next higher ModelTier for escalation.
        """
        current = self.route(stage_name)
        tiers = list(ModelTier)
        idx = tiers.index(current)
        if idx < len(tiers) - 1:
            return tiers[idx + 1]
        return current  # Already at highest tier

    def estimate_tokens(self, stage_name: str) -> int:
        """Estimate token consumption for a stage."""
        tier = self.route(stage_name)
        return TOKEN_ESTIMATES.get(tier, 0)

    def get_routing_summary(self) -> dict[str, dict[str, Any]]:
        """Get a summary of all routing decisions."""
        summary = {}
        for stage, task_class in self._routing.items():
            if isinstance(task_class, TaskClass):
                tier = DEFAULT_ROUTING[task_class]
            else:
                tier = task_class
            summary[stage] = {
                "task_class": task_class.value if isinstance(task_class, TaskClass) else "custom",
                "model_tier": tier.value,
                "estimated_tokens": TOKEN_ESTIMATES.get(tier, 0),
            }
        return summary
