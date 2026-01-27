"""
Parallel Task Executor

Executes decomposed tasks using a thread pool with configurable
worker count. Handles partial failures (one language failing does
not block others) and result aggregation.
"""

from __future__ import annotations

import time
from concurrent.futures import Future, ThreadPoolExecutor, as_completed
from dataclasses import dataclass, field
from typing import Any, Callable

from .decomposer import ParallelTask


@dataclass
class TaskResult:
    """Result of executing a single parallel task."""
    task_id: str
    success: bool
    result: Any = None
    error: str = ""
    duration_ms: float = 0.0

    def to_dict(self) -> dict[str, Any]:
        return {
            "task_id": self.task_id,
            "success": self.success,
            "error": self.error,
            "duration_ms": round(self.duration_ms, 1),
        }


@dataclass
class ExecutionResult:
    """Aggregated result of parallel execution."""
    results: list[TaskResult] = field(default_factory=list)
    total_time_ms: float = 0.0
    parallel_speedup: float = 1.0

    @property
    def all_success(self) -> bool:
        return all(r.success for r in self.results)

    @property
    def failures(self) -> list[TaskResult]:
        return [r for r in self.results if not r.success]

    def to_dict(self) -> dict[str, Any]:
        return {
            "all_success": self.all_success,
            "total_time_ms": round(self.total_time_ms, 1),
            "parallel_speedup": round(self.parallel_speedup, 2),
            "task_count": len(self.results),
            "failures": len(self.failures),
            "results": [r.to_dict() for r in self.results],
        }


# Type alias: task executor callback
TaskExecutor = Callable[[ParallelTask], Any]


class ParallelExecutor:
    """
    Parallel task executor with configurable worker pool.

    Executes a list of parallel tasks using a thread pool,
    collects results, and handles partial failures gracefully.

    Example:
        >>> executor = ParallelExecutor(max_workers=4)
        >>> def run_task(task):
        ...     return f"Generated {task.language}"
        >>> results = executor.execute(tasks, run_task)
        >>> assert results.all_success
    """

    def __init__(self, max_workers: int = 4) -> None:
        self.max_workers = min(max_workers, 8)  # Cap at 8

    def execute(
        self,
        tasks: list[ParallelTask],
        executor_fn: TaskExecutor,
        timeout: float = 60.0,
    ) -> ExecutionResult:
        """
        Execute tasks in parallel.

        Args:
            tasks: List of tasks to execute concurrently.
            executor_fn: Function that executes a single task.
            timeout: Per-task timeout in seconds.

        Returns:
            ExecutionResult with all task results.
        """
        result = ExecutionResult()
        start = time.perf_counter()

        if not tasks:
            return result

        sequential_time = 0.0

        with ThreadPoolExecutor(max_workers=self.max_workers) as pool:
            future_to_task: dict[Future, ParallelTask] = {}

            for task in tasks:
                future = pool.submit(self._run_with_timing, task, executor_fn)
                future_to_task[future] = task

            for future in as_completed(future_to_task, timeout=timeout * len(tasks)):
                task = future_to_task[future]
                try:
                    task_result = future.result(timeout=timeout)
                    result.results.append(task_result)
                    sequential_time += task_result.duration_ms
                except Exception as e:
                    result.results.append(TaskResult(
                        task_id=task.task_id,
                        success=False,
                        error=str(e),
                    ))

        result.total_time_ms = (time.perf_counter() - start) * 1000
        if result.total_time_ms > 0 and sequential_time > 0:
            result.parallel_speedup = sequential_time / result.total_time_ms

        return result

    def execute_sequential(
        self,
        tasks: list[ParallelTask],
        executor_fn: TaskExecutor,
    ) -> ExecutionResult:
        """Execute tasks sequentially (for comparison/fallback)."""
        result = ExecutionResult()
        start = time.perf_counter()

        for task in tasks:
            task_result = self._run_with_timing(task, executor_fn)
            result.results.append(task_result)

        result.total_time_ms = (time.perf_counter() - start) * 1000
        result.parallel_speedup = 1.0
        return result

    def _run_with_timing(
        self,
        task: ParallelTask,
        executor_fn: TaskExecutor,
    ) -> TaskResult:
        """Execute a task with timing and error handling."""
        start = time.perf_counter()
        try:
            output = executor_fn(task)
            duration = (time.perf_counter() - start) * 1000
            return TaskResult(
                task_id=task.task_id,
                success=True,
                result=output,
                duration_ms=duration,
            )
        except Exception as e:
            duration = (time.perf_counter() - start) * 1000
            return TaskResult(
                task_id=task.task_id,
                success=False,
                error=str(e),
                duration_ms=duration,
            )
