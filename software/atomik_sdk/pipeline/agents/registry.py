"""
Specialist Agent Registry

Dynamic registry where specialist agents declare capabilities.
The coordinator queries the registry to find the best-fit specialist
for each task. New specialists can be registered without modifying
the coordinator.
"""

from __future__ import annotations

from typing import Any

from .specialist import SpecialistAgent


class AgentRegistry:
    """
    Registry of specialist agents with capability matching.

    Agents register with their capabilities (languages, task types,
    model tier). The registry finds the best-matching agent for
    a given task request and supports health checking.

    Example:
        >>> registry = AgentRegistry()
        >>> registry.register(python_gen_agent)
        >>> agent = registry.find("generate", language="python")
        >>> assert agent.name == "python_generator"
    """

    def __init__(self) -> None:
        self._agents: dict[str, SpecialistAgent] = {}

    def register(self, agent: SpecialistAgent) -> None:
        """Register a specialist agent."""
        self._agents[agent.name] = agent

    def unregister(self, name: str) -> bool:
        """Unregister an agent. Returns True if found."""
        return self._agents.pop(name, None) is not None

    def get(self, name: str) -> SpecialistAgent | None:
        """Get an agent by name."""
        return self._agents.get(name)

    def find(
        self,
        task_type: str,
        language: str = "",
    ) -> SpecialistAgent | None:
        """
        Find the best-fit specialist for a task.

        Matches by task type first, then language if specified.
        Prefers healthy agents over unhealthy ones.

        Args:
            task_type: Type of task (e.g., "generate", "verify").
            language: Target language (empty = any).

        Returns:
            Best matching SpecialistAgent, or None.
        """
        candidates = []

        for agent in self._agents.values():
            cap = agent.capability

            # Must support the task type
            if task_type not in cap.task_types:
                continue

            # If language specified, must support it
            if language and cap.languages and language not in cap.languages:
                continue

            candidates.append(agent)

        if not candidates:
            return None

        # Prefer healthy agents
        healthy = [a for a in candidates if a.is_healthy]
        if healthy:
            candidates = healthy

        # Prefer agents with specific language match over generic
        if language:
            specific = [
                a for a in candidates
                if language in a.capability.languages
            ]
            if specific:
                return specific[0]

        return candidates[0]

    def find_all(
        self,
        task_type: str,
        language: str = "",
    ) -> list[SpecialistAgent]:
        """Find all matching specialists for a task."""
        results = []
        for agent in self._agents.values():
            cap = agent.capability
            if task_type not in cap.task_types:
                continue
            if language and cap.languages and language not in cap.languages:
                continue
            results.append(agent)
        return results

    def list_agents(self) -> list[dict[str, Any]]:
        """List all registered agents with their capabilities."""
        return [agent.to_dict() for agent in self._agents.values()]

    def health_check(self) -> dict[str, bool]:
        """Check health of all registered agents."""
        return {name: agent.is_healthy for name, agent in self._agents.items()}

    @property
    def count(self) -> int:
        return len(self._agents)
