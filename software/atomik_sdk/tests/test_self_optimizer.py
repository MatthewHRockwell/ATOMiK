"""Tests for pipeline self-optimization engine."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from pipeline.consensus import ConsensusResolver
from pipeline.context.intelligent_manager import IntelligentContextManager
from pipeline.context.segment_tracker import SegmentTracker
from pipeline.optimization.self_optimizer import OptimizationReport, SelfOptimizer
from pipeline.optimization.tuner import ConfigTuner
from pipeline.regression.baseline import BaselineManager
from pipeline.regression.detector import RegressionGate


class TestConfigTuner:
    def test_tune_workers_diminishing(self):
        tuner = ConfigTuner()
        history = [
            {"parallel_speedup": 1.5},
            {"parallel_speedup": 1.6},
            {"parallel_speedup": 1.4},
        ]
        result = tuner.tune_workers(history, current_workers=4)
        # Efficiency = 1.5/4 = 37.5%, should suggest decrease
        assert result is not None
        assert result.new_value < 4

    def test_tune_workers_efficient(self):
        tuner = ConfigTuner()
        history = [
            {"parallel_speedup": 3.5},
            {"parallel_speedup": 3.8},
        ]
        result = tuner.tune_workers(history, current_workers=4)
        # Efficiency = 3.65/4 = 91%, should suggest increase
        assert result is not None
        assert result.new_value > 4

    def test_tune_retry_depth(self):
        tuner = ConfigTuner()
        history = [
            {"feedback_iterations": [
                {"iteration": 1, "re_verify_passed": True},
            ]},
            {"feedback_iterations": [
                {"iteration": 1, "re_verify_passed": True},
            ]},
        ]
        result = tuner.tune_retry_depth(history, current_depth=5)
        # All fixes at depth 1, should suggest reducing
        assert result is not None
        assert result.new_value <= 3


class TestSelfOptimizer:
    def test_should_report(self):
        optimizer = SelfOptimizer(report_every=3)
        assert not optimizer.should_report()
        optimizer.record_run({"metric": 1})
        optimizer.record_run({"metric": 2})
        optimizer.record_run({"metric": 3})
        assert optimizer.should_report()

    def test_generate_report(self):
        optimizer = SelfOptimizer(report_every=2)
        optimizer.record_run({
            "stage_durations": {"validate": 100, "generate": 500, "verify": 200},
        })
        optimizer.record_run({
            "stage_durations": {"validate": 120, "generate": 480, "verify": 210},
        })
        report = optimizer.generate_report()
        assert isinstance(report, OptimizationReport)
        assert report.run_count_analyzed == 2
        assert report.bottleneck_stage == "generate"

    def test_bottleneck_recommendation(self):
        optimizer = SelfOptimizer(report_every=1)
        optimizer.record_run({
            "stage_durations": {"validate": 50, "generate": 900, "verify": 50},
        })
        report = optimizer.generate_report()
        assert report.bottleneck_pct > 50
        bottleneck_recs = [r for r in report.recommendations if r.category == "bottleneck"]
        assert len(bottleneck_recs) >= 1


class TestBaselineManager:
    def test_create_and_get(self, tmp_path):
        mgr = BaselineManager(tmp_path / "baselines")
        snapshot = mgr.create_baseline("test_schema", {"fmax_mhz": 100.0})
        assert snapshot.schema_name == "test_schema"
        loaded = mgr.get_baseline("test_schema")
        assert loaded is not None
        assert loaded.metrics["fmax_mhz"] == 100.0

    def test_create_if_missing(self, tmp_path):
        mgr = BaselineManager(tmp_path / "baselines")
        snap, created = mgr.create_if_missing("new", {"m": 1.0})
        assert created
        snap2, created2 = mgr.create_if_missing("new", {"m": 2.0})
        assert not created2
        assert snap2.metrics["m"] == 1.0  # Unchanged

    def test_update_baseline_ema(self, tmp_path):
        mgr = BaselineManager(tmp_path / "baselines")
        mgr.create_baseline("test", {"value": 100.0})
        updated = mgr.update_baseline("test", {"value": 200.0})
        # EMA: 100*0.7 + 200*0.3 = 130
        assert 125 < updated.metrics["value"] < 135

    def test_list_baselines(self, tmp_path):
        mgr = BaselineManager(tmp_path / "baselines")
        mgr.create_baseline("schema_a", {"m": 1.0})
        mgr.create_baseline("schema_b", {"m": 2.0})
        names = mgr.list_baselines()
        assert len(names) == 2


class TestRegressionGate:
    def test_first_run_creates_baseline(self, tmp_path):
        gate = RegressionGate(
            baseline_manager=BaselineManager(tmp_path / "baselines"),
        )
        result = gate.check("test", {"fmax_mhz": 100.0})
        assert result.baseline_created
        assert result.passed

    def test_no_regression(self, tmp_path):
        mgr = BaselineManager(tmp_path / "baselines")
        mgr.create_baseline("test", {"fmax_mhz": 100.0})
        gate = RegressionGate(baseline_manager=mgr)
        result = gate.check("test", {"fmax_mhz": 100.0})
        assert result.passed
        assert not result.blocked


class TestConsensusResolver:
    def test_all_agree(self):
        resolver = ConsensusResolver()
        result = resolver.resolve({
            "agent_a": {"field": "value"},
            "agent_b": {"field": "value"},
        })
        assert result.agreed
        assert result.merged_output == {"field": "value"}

    def test_conflict_detected(self):
        resolver = ConsensusResolver()
        result = resolver.resolve({
            "agent_a": {"field": "value_a"},
            "agent_b": {"field": "value_b"},
        })
        assert not result.agreed
        assert result.conflict_count >= 1

    def test_majority_vote(self):
        resolver = ConsensusResolver()
        result = resolver.resolve({
            "agent_a": {"field": "correct"},
            "agent_b": {"field": "correct"},
            "agent_c": {"field": "wrong"},
        })
        assert result.merged_output["field"] == "correct"

    def test_interface_field_check(self):
        resolver = ConsensusResolver()
        result = resolver.resolve_interface_fields({
            "python": ["price_delta", "volume"],
            "rust": ["price_delta"],
        })
        assert not result.agreed
        assert len(result.escalated) >= 1


class TestSegmentTracker:
    def test_add_and_get(self):
        tracker = SegmentTracker()
        seg = tracker.add("s1", "content", "schema")
        assert seg.segment_id == "s1"
        retrieved = tracker.get("s1")
        assert retrieved is not None

    def test_rank_by_relevance(self):
        tracker = SegmentTracker()
        tracker.add("s1", "content", "schema", ["generate"])
        tracker.add("s2", "content", "kb_entry", ["verify"])
        ranked = tracker.rank_by_relevance("generate")
        assert ranked[0].segment_id == "s1"  # Higher affinity

    def test_stale_eviction(self):
        tracker = SegmentTracker(stale_threshold_tasks=2)
        tracker.add("s1", "content", "schema")
        tracker.advance_task()
        tracker.advance_task()
        stale = tracker.get_stale_segments()
        assert len(stale) >= 1
        evicted = tracker.evict_stale()
        assert "s1" in evicted


class TestIntelligentContextManager:
    def test_add_and_build(self):
        mgr = IntelligentContextManager(max_tokens=10000)
        mgr.add_segment("s1", "hello world", "schema")
        context = mgr.build_context()
        assert "hello world" in context

    def test_cold_start(self):
        mgr = IntelligentContextManager(max_tokens=10000)
        mgr.add_segment("s1", "schema data", "schema")
        mgr.add_segment("s2", "kb data", "kb_entry")
        cold = mgr.get_context_for_cold_start()
        assert len(cold) > 0

    def test_budget_utilization(self):
        mgr = IntelligentContextManager(max_tokens=100, utilization_limit=0.8)
        mgr.add_segment("s1", "x" * 400, "schema")  # ~100 tokens
        util = mgr.get_utilization()
        assert util["budget"]["utilization"] > 0
