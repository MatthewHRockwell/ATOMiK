"""Tests for parallel task execution."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from pipeline.parallel.decomposer import ParallelTask, TaskDecomposer
from pipeline.parallel.executor import ParallelExecutor
from pipeline.parallel.worker import Worker, WorkerState


class TestTaskDecomposer:
    def test_decompose_generation(self):
        decomposer = TaskDecomposer()
        plan = decomposer.decompose_generation()
        assert plan.task_count == 5
        assert plan.max_parallelism == 5

    def test_decompose_generation_subset(self):
        decomposer = TaskDecomposer()
        plan = decomposer.decompose_generation(["python", "rust"])
        assert plan.task_count == 2

    def test_decompose_verification(self):
        decomposer = TaskDecomposer()
        plan = decomposer.decompose_verification()
        assert plan.task_count == 5
        for task in plan.tasks:
            assert len(task.dependencies) >= 1

    def test_decompose_full_pipeline(self):
        decomposer = TaskDecomposer()
        plan = decomposer.decompose_full_pipeline()
        assert plan.task_count >= 12
        assert len(plan.parallel_groups) == 5

    def test_parallel_groups(self):
        decomposer = TaskDecomposer()
        plan = decomposer.decompose_generation(["python", "rust", "c"])
        assert plan.max_parallelism == 3


class TestParallelExecutor:
    def test_execute_tasks(self):
        executor = ParallelExecutor(max_workers=2)

        def worker_fn(task):
            return f"done_{task.task_id}"

        tasks = [
            ParallelTask(task_id="a", task_type="generate", language="python"),
            ParallelTask(task_id="b", task_type="generate", language="rust"),
        ]
        results = executor.execute(tasks, worker_fn)
        assert len(results.results) == 2
        assert results.all_success

    def test_single_task(self):
        executor = ParallelExecutor(max_workers=4)

        def worker_fn(task):
            return "ok"

        tasks = [ParallelTask(task_id="solo", task_type="generate")]
        results = executor.execute(tasks, worker_fn)
        assert len(results.results) == 1
        assert results.all_success

    def test_empty_tasks(self):
        executor = ParallelExecutor(max_workers=2)
        results = executor.execute([], lambda t: None)
        assert len(results.results) == 0


class TestWorker:
    def test_worker_creation(self):
        worker = Worker("w1")
        assert worker.worker_id == "w1"
        assert worker.state == WorkerState.IDLE

    def test_worker_run(self):
        worker = Worker("w1")
        result = worker.run(lambda: {"done": True})
        assert result.output == {"done": True}
        assert result.state == WorkerState.COMPLETED
        assert worker.state == WorkerState.COMPLETED

    def test_worker_error(self):
        worker = Worker("w1")
        result = worker.run(lambda: 1 / 0)
        assert result.state == WorkerState.FAILED
        assert "division" in result.error.lower()
