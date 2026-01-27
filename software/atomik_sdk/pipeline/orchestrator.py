"""
Event-Driven Pipeline Orchestrator

Replaces sequential stage dispatch with DAG-based scheduling.
Tasks declare dependencies explicitly; the orchestrator dispatches
work as dependencies are satisfied, enabling parallel execution
of independent tasks.
"""

from __future__ import annotations

from concurrent.futures import Future, ThreadPoolExecutor
from typing import Any

from .dag import CycleError, DAGTask, TaskDAG, TaskState
from .event_bus import Event, EventBus, EventType
from .stages import BaseStage, StageManifest, StageStatus


class Orchestrator:
    """
    Event-driven DAG orchestrator for the pipeline.

    Builds a task DAG from registered stages and their declared
    dependencies, dispatches ready tasks (potentially in parallel),
    and reacts to completion/failure events.

    When the DAG degenerates to a linear chain (all stages depend
    on the previous), behavior is identical to Phase 4C sequential
    execution.

    Example:
        >>> orch = Orchestrator()
        >>> orch.register_stage(validate_stage)
        >>> orch.register_stage(generate_stage, dependencies=["validate"])
        >>> result = orch.execute(schema, schema_path, config)
    """

    def __init__(self, max_workers: int = 1) -> None:
        self.event_bus = EventBus()
        self.max_workers = max_workers
        self._stages: dict[str, BaseStage] = {}
        self._stage_deps: dict[str, list[str]] = {}
        self._manifests: dict[str, StageManifest] = {}

    def register_stage(
        self,
        stage: BaseStage,
        dependencies: list[str] | None = None,
    ) -> None:
        """
        Register a pipeline stage with its dependencies.

        Args:
            stage: The stage to register.
            dependencies: Stage names that must complete before this stage.
        """
        self._stages[stage.name] = stage
        self._stage_deps[stage.name] = dependencies or []

    def build_dag(self) -> TaskDAG:
        """
        Build a task DAG from registered stages and their dependencies.

        Returns:
            TaskDAG ready for execution.

        Raises:
            CycleError: If stage dependencies form a cycle.
            ValueError: If a dependency references an unregistered stage.
        """
        dag = TaskDAG()

        # Sort stages topologically by inserting in dependency order
        added: set[str] = set()
        to_add = list(self._stages.keys())

        while to_add:
            progress = False
            remaining = []
            for name in to_add:
                deps = self._stage_deps.get(name, [])
                if all(d in added for d in deps):
                    dag.add_task(
                        task_id=name,
                        task_type="stage",
                        dependencies=deps,
                    )
                    added.add(name)
                    progress = True
                else:
                    remaining.append(name)
            to_add = remaining
            if not progress and to_add:
                raise CycleError(
                    f"Circular dependencies among stages: {to_add}"
                )

        return dag

    def execute(
        self,
        schema: dict[str, Any],
        schema_path: str,
        config: Any,
    ) -> dict[str, StageManifest]:
        """
        Execute the pipeline DAG.

        Dispatches ready tasks, waits for completion, and continues
        until all tasks reach a terminal state or a failure aborts
        the pipeline.

        Args:
            schema: Parsed schema dict.
            schema_path: Path to the schema file.
            config: Pipeline configuration.

        Returns:
            Dict mapping stage names to their StageManifest results.
        """
        dag = self.build_dag()
        self._manifests.clear()

        self.event_bus.emit(Event(
            EventType.PIPELINE_DONE,
            {"status": "started", "tasks": len(dag.get_all_tasks())},
            source="orchestrator",
        ))

        if self.max_workers > 1:
            self._execute_parallel(dag, schema, schema_path, config)
        else:
            self._execute_sequential(dag, schema, schema_path, config)

        status = "success" if not dag.has_failures() else "failed"
        self.event_bus.emit(Event(
            EventType.PIPELINE_DONE,
            {"status": status, "stages_completed": len(self._manifests)},
            source="orchestrator",
        ))

        return dict(self._manifests)

    def _execute_sequential(
        self,
        dag: TaskDAG,
        schema: dict[str, Any],
        schema_path: str,
        config: Any,
    ) -> None:
        """Execute DAG tasks sequentially in topological order."""
        order = dag.topological_order()

        for task_id in order:
            task = dag.get_task(task_id)
            if task is None:
                continue

            # Check if any dependency failed
            if self._any_dep_failed(task, dag):
                dag.mark_skipped(task_id)
                continue

            self._run_stage_task(task, dag, schema, schema_path, config)

            # Abort on failure (preserves Phase 4C behavior)
            if task.state == TaskState.FAILED:
                # Skip remaining tasks
                for remaining_id in order[order.index(task_id) + 1:]:
                    dag.mark_skipped(remaining_id)
                break

            # Handle short-circuit (diff stage returns SKIPPED)
            manifest = self._manifests.get(task_id)
            if manifest and manifest.status == StageStatus.SKIPPED:
                for remaining_id in order[order.index(task_id) + 1:]:
                    dag.mark_skipped(remaining_id)
                break

    def _execute_parallel(
        self,
        dag: TaskDAG,
        schema: dict[str, Any],
        schema_path: str,
        config: Any,
    ) -> None:
        """Execute DAG tasks with parallel dispatch of independent tasks."""
        with ThreadPoolExecutor(max_workers=self.max_workers) as pool:
            futures: dict[str, Future] = {}

            while not dag.is_complete():
                ready = dag.get_ready_tasks()

                if not ready and not futures:
                    # No ready tasks and no running tasks -- deadlock or done
                    break

                # Dispatch ready tasks
                for task in ready:
                    dag.mark_running(task.task_id)
                    self.event_bus.emit(Event(
                        EventType.TASK_STARTED,
                        {"task_id": task.task_id},
                        source="orchestrator",
                    ))
                    future = pool.submit(
                        self._run_stage_task,
                        task, dag, schema, schema_path, config,
                    )
                    futures[task.task_id] = future

                # Wait for at least one future to complete
                if futures:
                    done_ids = []
                    for tid, fut in futures.items():
                        if fut.done():
                            done_ids.append(tid)

                    if not done_ids:
                        # Wait for any one to finish
                        next_id = next(iter(futures))
                        futures[next_id].result()
                        done_ids.append(next_id)

                    for tid in done_ids:
                        del futures[tid]

                    # Check for abort conditions
                    if dag.has_failures():
                        # Cancel pending tasks
                        for task in dag.get_all_tasks():
                            if task.state == TaskState.PENDING:
                                dag.mark_skipped(task.task_id)
                        break

    def _run_stage_task(
        self,
        task: DAGTask,
        dag: TaskDAG,
        schema: dict[str, Any],
        schema_path: str,
        config: Any,
    ) -> None:
        """Execute a single stage task."""
        stage = self._stages.get(task.task_id)
        if stage is None:
            dag.mark_failed(task.task_id, {"error": f"No stage: {task.task_id}"})
            return

        dag.mark_running(task.task_id)
        self.event_bus.emit(Event(
            EventType.TASK_STARTED,
            {"task_id": task.task_id, "task_type": task.task_type},
            source="orchestrator",
        ))

        # Get the previous manifest (from last completed dependency)
        previous_manifest = self._get_previous_manifest(task)

        manifest = stage.execute(schema, schema_path, previous_manifest, config)
        self._manifests[task.task_id] = manifest

        if manifest.status == StageStatus.FAILED:
            dag.mark_failed(task.task_id, manifest.to_dict())
            self.event_bus.emit(Event(
                EventType.TASK_FAILED,
                {
                    "task_id": task.task_id,
                    "errors": manifest.errors,
                },
                source="orchestrator",
            ))
        else:
            dag.mark_completed(task.task_id, manifest.to_dict())
            self.event_bus.emit(Event(
                EventType.TASK_COMPLETED,
                {
                    "task_id": task.task_id,
                    "duration_ms": manifest.duration_ms,
                    "tokens_consumed": manifest.tokens_consumed,
                },
                source="orchestrator",
            ))

    def _get_previous_manifest(self, task: DAGTask) -> StageManifest | None:
        """Get the manifest from the last dependency (for stage input)."""
        for dep in reversed(task.dependencies):
            if dep in self._manifests:
                return self._manifests[dep]
        return None

    def _any_dep_failed(self, task: DAGTask, dag: TaskDAG) -> bool:
        """Check if any dependency of a task has failed."""
        for dep_id in task.dependencies:
            dep = dag.get_task(dep_id)
            if dep and dep.state == TaskState.FAILED:
                return True
        return False

    def get_manifests(self) -> dict[str, StageManifest]:
        """Get all stage manifests produced during execution."""
        return dict(self._manifests)

    def get_event_history(self) -> list[dict[str, Any]]:
        """Get the event history for reporting."""
        return [e.to_dict() for e in self.event_bus.get_history()]

    def set_coordinator(self, coordinator: Any) -> None:
        """
        Set a coordinator for specialist-based dispatch.

        When a coordinator is set, the orchestrator delegates
        generation and verification stages to the coordinator
        for parallel specialist dispatch.

        Args:
            coordinator: A Coordinator instance.
        """
        self._coordinator = coordinator

    @property
    def has_coordinator(self) -> bool:
        """Check if a coordinator is configured."""
        return hasattr(self, "_coordinator") and self._coordinator is not None

    def apply_tuning(self, tuning_results: list[dict[str, Any]]) -> list[str]:
        """
        Apply tuning recommendations from the self-optimizer.

        Args:
            tuning_results: List of TuningResult dicts from optimizer.

        Returns:
            List of applied parameter names.
        """
        applied = []
        for result in tuning_results:
            param = result.get("parameter", "")
            new_value = result.get("new_value")
            if param == "max_workers" and isinstance(new_value, int):
                self.max_workers = max(1, min(8, new_value))
                applied.append(param)
        return applied
