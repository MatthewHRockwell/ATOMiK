"""
Specialist Agent Base Class

Base class for specialist agents that declare their capabilities.
Specialists register with the coordinator via a capability protocol.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any


@dataclass
class AgentCapability:
    """Capability declaration for a specialist agent."""
    languages: list[str] = field(default_factory=list)
    task_types: list[str] = field(default_factory=list)
    model_tier: str = "local"
    estimated_tokens: int = 0

    def to_dict(self) -> dict[str, Any]:
        return {
            "languages": self.languages,
            "task_types": self.task_types,
            "model_tier": self.model_tier,
            "estimated_tokens": self.estimated_tokens,
        }


class SpecialistAgent:
    """
    Base class for specialist agents.

    Each specialist declares its capabilities and implements
    an execute method. The coordinator dispatches work to
    specialists based on capability matching.

    Subclasses must set:
    - name: Unique agent name
    - capability: What this agent can do

    Example:
        >>> class PythonGen(SpecialistAgent):
        ...     name = "python_generator"
        ...     capability = AgentCapability(
        ...         languages=["python"],
        ...         task_types=["generate"],
        ...     )
        ...     def execute(self, task, context):
        ...         return {"files_generated": 3}
    """

    name: str = "base_specialist"
    capability: AgentCapability = AgentCapability()

    def execute(self, task: dict[str, Any], context: dict[str, Any]) -> dict[str, Any]:
        """
        Execute the specialist's task.

        Args:
            task: Task description with type, language, schema, etc.
            context: Execution context (config, manifests, etc.).

        Returns:
            Result dict with task-specific output.
        """
        raise NotImplementedError

    @property
    def is_healthy(self) -> bool:
        """Health check for the specialist."""
        return True

    def to_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "capability": self.capability.to_dict(),
            "healthy": self.is_healthy,
        }
