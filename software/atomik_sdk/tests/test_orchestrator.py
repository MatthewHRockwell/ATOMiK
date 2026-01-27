"""Tests for event-driven orchestrator and DAG scheduler."""

import pytest
from pipeline.dag import TaskDAG, DAGTask, TaskState, CycleError
from pipeline.event_bus import EventBus, Event, EventType
from pipeline.orchestrator import Orchestrator


class TestTaskDAG:
    def test_add_task(self):
        dag = TaskDAG()
        dag.add_task("a", "stage")
        assert dag.get_task("a") is not None

    def test_topological_order_linear(self):
        dag = TaskDAG()
        dag.add_task("a", "stage")
        dag.add_task("b", "stage", dependencies=["a"])
        dag.add_task("c", "stage", dependencies=["b"])
        order = dag.topological_order()
        assert order.index("a") < order.index("b") < order.index("c")

    def test_ready_tasks(self):
        dag = TaskDAG()
        dag.add_task("a", "stage")
        dag.add_task("b", "stage", dependencies=["a"])
        ready = dag.get_ready_tasks()
        assert len(ready) == 1
        assert ready[0].task_id == "a"

    def test_parallel_ready(self):
        dag = TaskDAG()
        dag.add_task("root", "stage")
        dag.add_task("a", "stage", dependencies=["root"])
        dag.add_task("b", "stage", dependencies=["root"])
        dag.mark_running("root")
        dag.mark_completed("root")
        ready = dag.get_ready_tasks()
        assert len(ready) == 2

    def test_unknown_dependency_rejected(self):
        dag = TaskDAG()
        with pytest.raises(ValueError, match="Unknown dependency"):
            dag.add_task("a", "stage", dependencies=["nonexistent"])

    def test_cycle_detection(self):
        dag = TaskDAG()
        dag.add_task("a", "stage")
        dag.add_task("b", "stage", dependencies=["a"])
        # Manually inject a back-edge to create a cycle
        dag._tasks["a"].dependencies = ["b"]
        with pytest.raises(CycleError):
            dag.topological_order()

    def test_mark_states(self):
        dag = TaskDAG()
        dag.add_task("a", "stage")
        dag.mark_running("a")
        assert dag.get_task("a").state == TaskState.RUNNING
        dag.mark_completed("a", {"result": "ok"})
        assert dag.get_task("a").state == TaskState.COMPLETED

    def test_skip_on_failed_dep(self):
        dag = TaskDAG()
        dag.add_task("a", "stage")
        dag.add_task("b", "stage", dependencies=["a"])
        dag.mark_running("a")
        dag.mark_failed("a")
        dag.mark_skipped("b")
        assert dag.get_task("b").state == TaskState.SKIPPED


class TestEventBus:
    def test_subscribe_and_emit(self):
        bus = EventBus()
        received = []
        bus.subscribe(EventType.TASK_COMPLETED, lambda e: received.append(e))
        bus.emit(Event(EventType.TASK_COMPLETED, {"id": "test"}))
        assert len(received) == 1
        assert received[0].payload["id"] == "test"

    def test_history(self):
        bus = EventBus()
        bus.emit(Event(EventType.TASK_STARTED, {"id": "a"}))
        bus.emit(Event(EventType.TASK_COMPLETED, {"id": "a"}))
        assert len(bus.get_history()) == 2
        assert len(bus.get_history(EventType.TASK_STARTED)) == 1

    def test_unsubscribe(self):
        bus = EventBus()
        received = []
        handler = lambda e: received.append(e)
        bus.subscribe(EventType.TASK_COMPLETED, handler)
        bus.unsubscribe(EventType.TASK_COMPLETED, handler)
        bus.emit(Event(EventType.TASK_COMPLETED))
        assert len(received) == 0
