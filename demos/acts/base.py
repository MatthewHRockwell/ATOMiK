"""Base class for demo acts."""

from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from demos.node import NodeController


@dataclass
class ActResult:
    """Outcome of running an act."""
    act_number: int
    title: str
    passed: bool
    summary: str
    details: list[str]


class ActBase(ABC):
    """Abstract base for a demo act."""

    number: int = 0
    title: str = ""
    narration: str = ""

    @abstractmethod
    def run(self, nodes: list[NodeController]) -> ActResult:
        """Execute the act and return results."""
        ...

    def _result(self, passed: bool, summary: str, details: list[str] | None = None) -> ActResult:
        return ActResult(
            act_number=self.number,
            title=self.title,
            passed=passed,
            summary=summary,
            details=details or [],
        )
