"""
Coordinator Agent

Top-level coordinator that receives pipeline work requests, decomposes
them via the task decomposer, dispatches subtasks to specialists via
the registry, collects and aggregates results, and resolves conflicts
when specialists produce inconsistent outputs.
"""

from __future__ import annotations

import time
from concurrent.futures import ThreadPoolExecutor
from concurrent.futures import TimeoutError as FuturesTimeout
from dataclasses import dataclass, field
from typing import Any

from .agents.registry import AgentRegistry
from .consensus import ConsensusResolver, ConsensusResult
from .event_bus import Event, EventBus, EventType
from .parallel.decomposer import DecompositionPlan, ParallelTask, TaskDecomposer


@dataclass
class SubtaskResult:
    """Result from a single specialist subtask."""
    task_id: str
    language: str
    agent_name: str
    success: bool
    output: dict[str, Any] = field(default_factory=dict)
    error: str = ""
    duration_ms: float = 0.0
    tokens_consumed: int = 0

    def to_dict(self) -> dict[str, Any]:
        return {
            "task_id": self.task_id,
            "language": self.language,
            "agent_name": self.agent_name,
            "success": self.success,
            "output": self.output,
            "error": self.error,
            "duration_ms": round(self.duration_ms, 1),
            "tokens_consumed": self.tokens_consumed,
        }


@dataclass
class CoordinatorResult:
    """Aggregated result from the coordinator."""
    success: bool = True
    subtask_results: list[SubtaskResult] = field(default_factory=list)
    consensus: ConsensusResult | None = None
    total_tokens: int = 0
    total_duration_ms: float = 0.0
    parallel_speedup: float = 1.0

    @property
    def failed_count(self) -> int:
        return sum(1 for r in self.subtask_results if not r.success)

    def to_dict(self) -> dict[str, Any]:
        return {
            "success": self.success,
            "subtask_count": len(self.subtask_results),
            "failed_count": self.failed_count,
            "total_tokens": self.total_tokens,
            "total_duration_ms": round(self.total_duration_ms, 1),
            "parallel_speedup": round(self.parallel_speedup, 2),
            "subtasks": [r.to_dict() for r in self.subtask_results],
            "consensus": self.consensus.to_dict() if self.consensus else None,
        }


class Coordinator:
    """
    Top-level coordinator agent for pipeline work dispatch.

    Decomposes pipeline requests into subtasks, dispatches each
    to the best-fit specialist via the registry, collects results,
    and runs consensus resolution on overlapping outputs.

    Example:
        >>> coord = Coordinator(registry, decomposer)
        >>> result = coord.dispatch_generation(schema, context)
        >>> if result.consensus and not result.consensus.agreed:
        ...     print("Interface conflicts detected")
    """

    def __init__(
        self,
        registry: AgentRegistry,
        decomposer: TaskDecomposer | None = None,
        consensus_resolver: ConsensusResolver | None = None,
        event_bus: EventBus | None = None,
        max_workers: int = 4,
        specialist_timeout: float = 60.0,
    ) -> None:
        self.registry = registry
        self.decomposer = decomposer or TaskDecomposer()
        self.consensus = consensus_resolver or ConsensusResolver()
        self.event_bus = event_bus
        self.max_workers = min(max_workers, 8)
        self.specialist_timeout = specialist_timeout

    def dispatch_generation(
        self,
        schema: dict[str, Any],
        context: dict[str, Any],
        languages: list[str] | None = None,
    ) -> CoordinatorResult:
        """
        Decompose and dispatch code generation to specialists.

        Args:
            schema: Parsed schema dict.
            context: Execution context (config, manifests, etc.).
            languages: Languages to generate (None = all).

        Returns:
            CoordinatorResult with per-language results and consensus.
        """
        plan = self.decomposer.decompose_generation(languages)
        return self._execute_plan(plan, schema, context)

    def dispatch_verification(
        self,
        schema: dict[str, Any],
        context: dict[str, Any],
        languages: list[str] | None = None,
    ) -> CoordinatorResult:
        """Decompose and dispatch verification to specialists."""
        plan = self.decomposer.decompose_verification(languages)
        return self._execute_plan(plan, schema, context)

    def dispatch_full_pipeline(
        self,
        schema: dict[str, Any],
        context: dict[str, Any],
        languages: list[str] | None = None,
        include_hardware: bool = True,
    ) -> CoordinatorResult:
        """Decompose and dispatch a full pipeline run."""
        plan = self.decomposer.decompose_full_pipeline(
            languages, include_hardware
        )
        return self._execute_plan(plan, schema, context)

    def _execute_plan(
        self,
        plan: DecompositionPlan,
        schema: dict[str, Any],
        context: dict[str, Any],
    ) -> CoordinatorResult:
        """Execute a decomposition plan by dispatching to specialists."""
        coord_start = time.perf_counter()
        result = CoordinatorResult()
        sequential_time = 0.0

        for group in plan.parallel_groups:
            group_tasks = [
                t for t in plan.tasks if t.task_id in group
            ]
            if not group_tasks:
                continue

            group_results = self._dispatch_group(group_tasks, schema, context)
            result.subtask_results.extend(group_results)

            for sr in group_results:
                sequential_time += sr.duration_ms
                result.total_tokens += sr.tokens_consumed
                if not sr.success:
                    result.success = False

        result.total_duration_ms = (time.perf_counter() - coord_start) * 1000

        # Compute parallel speedup
        if result.total_duration_ms > 0:
            result.parallel_speedup = sequential_time / result.total_duration_ms
        else:
            result.parallel_speedup = 1.0

        # Run consensus if we have generation results from multiple languages
        gen_results = {
            r.language: r.output
            for r in result.subtask_results
            if r.success and r.output and r.language
        }
        if len(gen_results) > 1:
            result.consensus = self.consensus.resolve(gen_results)

        return result

    def _dispatch_group(
        self,
        tasks: list[ParallelTask],
        schema: dict[str, Any],
        context: dict[str, Any],
    ) -> list[SubtaskResult]:
        """Dispatch a group of tasks in parallel."""
        if len(tasks) == 1:
            return [self._dispatch_single(tasks[0], schema, context)]

        results: list[SubtaskResult] = []
        with ThreadPoolExecutor(max_workers=self.max_workers) as pool:
            futures = {
                pool.submit(
                    self._dispatch_single, task, schema, context
                ): task
                for task in tasks
            }

            for future in futures:
                try:
                    sr = future.result(timeout=self.specialist_timeout)
                    results.append(sr)
                except FuturesTimeout:
                    task = futures[future]
                    results.append(SubtaskResult(
                        task_id=task.task_id,
                        language=task.language,
                        agent_name="timeout",
                        success=False,
                        error=f"Specialist timeout after {self.specialist_timeout}s",
                    ))
                except Exception as e:
                    task = futures[future]
                    results.append(SubtaskResult(
                        task_id=task.task_id,
                        language=task.language,
                        agent_name="error",
                        success=False,
                        error=str(e),
                    ))

        return results

    def _dispatch_single(
        self,
        task: ParallelTask,
        schema: dict[str, Any],
        context: dict[str, Any],
    ) -> SubtaskResult:
        """Dispatch a single task to the best-fit specialist."""
        start = time.perf_counter()

        agent = self.registry.find(task.task_type, language=task.language)
        if agent is None:
            return SubtaskResult(
                task_id=task.task_id,
                language=task.language,
                agent_name="none",
                success=False,
                error=f"No specialist for {task.task_type}/{task.language}",
                duration_ms=(time.perf_counter() - start) * 1000,
            )

        if self.event_bus:
            self.event_bus.emit(Event(
                EventType.TASK_STARTED,
                {
                    "task_id": task.task_id,
                    "agent": agent.name,
                    "language": task.language,
                },
                source="coordinator",
            ))

        try:
            task_desc = {
                "task_id": task.task_id,
                "task_type": task.task_type,
                "language": task.language,
                "schema": schema,
                **task.metadata,
            }
            output = agent.execute(task_desc, context)
            duration_ms = (time.perf_counter() - start) * 1000

            sr = SubtaskResult(
                task_id=task.task_id,
                language=task.language,
                agent_name=agent.name,
                success=True,
                output=output,
                duration_ms=duration_ms,
                tokens_consumed=output.get("tokens_consumed", 0),
            )

            if self.event_bus:
                self.event_bus.emit(Event(
                    EventType.TASK_COMPLETED,
                    {"task_id": task.task_id, "agent": agent.name},
                    source="coordinator",
                ))

            return sr

        except Exception as e:
            duration_ms = (time.perf_counter() - start) * 1000

            if self.event_bus:
                self.event_bus.emit(Event(
                    EventType.TASK_FAILED,
                    {
                        "task_id": task.task_id,
                        "agent": agent.name,
                        "error": str(e),
                    },
                    source="coordinator",
                ))

            return SubtaskResult(
                task_id=task.task_id,
                language=task.language,
                agent_name=agent.name,
                success=False,
                error=str(e),
                duration_ms=duration_ms,
            )

    def get_specialist_status(self) -> list[dict[str, Any]]:
        """Get the status of all registered specialists."""
        return self.registry.list_agents()
