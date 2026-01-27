"""
Task DAG Builder

Builds a directed acyclic graph of pipeline tasks with dependency
tracking, cycle detection, topological ordering, and critical path
analysis. Tasks declare their dependencies explicitly; the DAG
enforces execution ordering.
"""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass, field
from enum import Enum
from typing import Any


class TaskState(Enum):
    """Execution state of a DAG task."""
    PENDING = "pending"
    READY = "ready"       # All dependencies satisfied
    RUNNING = "running"
    COMPLETED = "completed"
    FAILED = "failed"
    SKIPPED = "skipped"


@dataclass
class DAGTask:
    """A node in the task DAG."""
    task_id: str
    task_type: str
    dependencies: list[str] = field(default_factory=list)
    state: TaskState = TaskState.PENDING
    metadata: dict[str, Any] = field(default_factory=dict)
    estimated_tokens: int = 0
    result: dict[str, Any] | None = None

    @property
    def is_terminal(self) -> bool:
        return self.state in (TaskState.COMPLETED, TaskState.FAILED, TaskState.SKIPPED)

    def to_dict(self) -> dict[str, Any]:
        return {
            "task_id": self.task_id,
            "task_type": self.task_type,
            "dependencies": self.dependencies,
            "state": self.state.value,
            "estimated_tokens": self.estimated_tokens,
            "metadata": self.metadata,
        }


class CycleError(Exception):
    """Raised when a cycle is detected in the task DAG."""


class TaskDAG:
    """
    Directed acyclic graph of pipeline tasks.

    Provides:
    - Task registration with explicit dependencies
    - Cycle detection at construction time
    - Topological ordering for sequential fallback
    - Ready-task discovery for parallel dispatch
    - Critical path analysis for budget estimation

    Example:
        >>> dag = TaskDAG()
        >>> dag.add_task("validate", "stage")
        >>> dag.add_task("generate", "stage", dependencies=["validate"])
        >>> dag.add_task("verify", "stage", dependencies=["generate"])
        >>> ready = dag.get_ready_tasks()
        >>> assert [t.task_id for t in ready] == ["validate"]
    """

    def __init__(self) -> None:
        self._tasks: dict[str, DAGTask] = {}

    def add_task(
        self,
        task_id: str,
        task_type: str,
        dependencies: list[str] | None = None,
        estimated_tokens: int = 0,
        metadata: dict[str, Any] | None = None,
    ) -> DAGTask:
        """
        Add a task to the DAG.

        Args:
            task_id: Unique identifier for this task.
            task_type: Type of task (e.g., "stage", "generation", "verification").
            dependencies: List of task IDs that must complete before this task.
            estimated_tokens: Estimated token cost for this task.
            metadata: Arbitrary metadata attached to the task.

        Returns:
            The created DAGTask.

        Raises:
            CycleError: If adding this task would create a cycle.
            ValueError: If a dependency references an unknown task.
        """
        deps = dependencies or []
        for dep in deps:
            if dep not in self._tasks:
                raise ValueError(f"Unknown dependency: {dep}")

        task = DAGTask(
            task_id=task_id,
            task_type=task_type,
            dependencies=list(deps),
            estimated_tokens=estimated_tokens,
            metadata=metadata or {},
        )
        self._tasks[task_id] = task

        # Check for cycles after adding
        if self._has_cycle():
            del self._tasks[task_id]
            raise CycleError(f"Adding task '{task_id}' would create a cycle")

        return task

    def get_task(self, task_id: str) -> DAGTask | None:
        """Get a task by ID."""
        return self._tasks.get(task_id)

    def get_all_tasks(self) -> list[DAGTask]:
        """Get all tasks in insertion order."""
        return list(self._tasks.values())

    def get_ready_tasks(self) -> list[DAGTask]:
        """
        Get all tasks whose dependencies are satisfied and that
        are not yet running or complete.

        Returns:
            List of tasks ready for execution.
        """
        ready = []
        for task in self._tasks.values():
            if task.state != TaskState.PENDING:
                continue
            deps_met = all(
                self._tasks[dep].state in (TaskState.COMPLETED, TaskState.SKIPPED)
                for dep in task.dependencies
                if dep in self._tasks
            )
            if deps_met:
                ready.append(task)
        return ready

    def mark_ready(self, task_id: str) -> None:
        """Mark a task as ready for execution."""
        task = self._tasks.get(task_id)
        if task and task.state == TaskState.PENDING:
            task.state = TaskState.READY

    def mark_running(self, task_id: str) -> None:
        """Mark a task as currently executing."""
        task = self._tasks.get(task_id)
        if task:
            task.state = TaskState.RUNNING

    def mark_completed(self, task_id: str, result: dict[str, Any] | None = None) -> None:
        """Mark a task as successfully completed."""
        task = self._tasks.get(task_id)
        if task:
            task.state = TaskState.COMPLETED
            task.result = result

    def mark_failed(self, task_id: str, result: dict[str, Any] | None = None) -> None:
        """Mark a task as failed."""
        task = self._tasks.get(task_id)
        if task:
            task.state = TaskState.FAILED
            task.result = result

    def mark_skipped(self, task_id: str) -> None:
        """Mark a task as skipped."""
        task = self._tasks.get(task_id)
        if task:
            task.state = TaskState.SKIPPED

    def is_complete(self) -> bool:
        """Check if all tasks have reached a terminal state."""
        return all(t.is_terminal for t in self._tasks.values())

    def has_failures(self) -> bool:
        """Check if any tasks have failed."""
        return any(t.state == TaskState.FAILED for t in self._tasks.values())

    def topological_order(self) -> list[str]:
        """
        Return task IDs in topological order (dependencies first).

        Raises:
            CycleError: If the graph contains a cycle.
        """
        in_degree: dict[str, int] = {tid: 0 for tid in self._tasks}
        for task in self._tasks.values():
            for dep in task.dependencies:
                if dep in in_degree:
                    in_degree[task.task_id] = in_degree.get(task.task_id, 0)

        # Compute actual in-degrees from reverse edges
        in_degree = {tid: 0 for tid in self._tasks}
        for task in self._tasks.values():
            for dep in task.dependencies:
                if dep in self._tasks:
                    # task depends on dep, so dep -> task edge
                    # in_degree of task increases
                    pass  # handled below

        # Recompute properly: in_degree[t] = number of tasks that t depends on
        # that haven't been processed
        adj: dict[str, list[str]] = {tid: [] for tid in self._tasks}
        in_degree = {tid: 0 for tid in self._tasks}

        for task in self._tasks.values():
            for dep in task.dependencies:
                if dep in self._tasks:
                    adj[dep].append(task.task_id)
                    in_degree[task.task_id] += 1

        queue = deque(tid for tid, deg in in_degree.items() if deg == 0)
        order: list[str] = []

        while queue:
            tid = queue.popleft()
            order.append(tid)
            for neighbor in adj[tid]:
                in_degree[neighbor] -= 1
                if in_degree[neighbor] == 0:
                    queue.append(neighbor)

        if len(order) != len(self._tasks):
            raise CycleError("DAG contains a cycle")

        return order

    def critical_path(self) -> list[str]:
        """
        Compute the critical path (longest dependency chain).

        Returns:
            List of task IDs on the critical path.
        """
        if not self._tasks:
            return []

        order = self.topological_order()

        # Compute longest path to each node
        dist: dict[str, int] = {tid: 0 for tid in self._tasks}
        prev: dict[str, str | None] = {tid: None for tid in self._tasks}

        for tid in order:
            task = self._tasks[tid]
            for dep in task.dependencies:
                if dep in dist:
                    new_dist = dist[dep] + 1
                    if new_dist > dist[tid]:
                        dist[tid] = new_dist
                        prev[tid] = dep

        # Find the node with maximum distance
        end = max(dist, key=lambda t: dist[t])
        path: list[str] = []
        current: str | None = end
        while current is not None:
            path.append(current)
            current = prev[current]

        path.reverse()
        return path

    def critical_path_tokens(self) -> int:
        """Estimated tokens on the critical path."""
        return sum(
            self._tasks[tid].estimated_tokens
            for tid in self.critical_path()
            if tid in self._tasks
        )

    def get_dependents(self, task_id: str) -> list[str]:
        """Get tasks that depend on the given task."""
        return [
            t.task_id for t in self._tasks.values()
            if task_id in t.dependencies
        ]

    def _has_cycle(self) -> bool:
        """Detect cycles using Kahn's algorithm."""
        try:
            self.topological_order()
            return False
        except CycleError:
            return True

    def summary(self) -> dict[str, Any]:
        """Get a summary of the DAG state."""
        states = {}
        for task in self._tasks.values():
            s = task.state.value
            states[s] = states.get(s, 0) + 1

        return {
            "total_tasks": len(self._tasks),
            "states": states,
            "is_complete": self.is_complete(),
            "has_failures": self.has_failures(),
            "critical_path": self.critical_path(),
            "critical_path_tokens": self.critical_path_tokens(),
        }
