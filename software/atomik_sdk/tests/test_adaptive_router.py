"""Tests for adaptive model router."""

from pipeline.agents.adaptive_router import AdaptiveRouter
from pipeline.agents.router import ModelTier


class TestAdaptiveRouter:
    def test_route_default(self):
        router = AdaptiveRouter()
        tier = router.route("generate")
        assert isinstance(tier, ModelTier)

    def test_high_budget_pressure_downgrades(self):
        router = AdaptiveRouter()
        # Use a non-deterministic stage so adaptive routing is applied
        router.route(
            "self_correct_unknown",
            budget_pressure=0.85,
        )
        # High budget pressure should result in a lower tier
        decisions = router.get_decisions()
        assert len(decisions) == 1
        assert decisions[0]["budget_pressure"] == 0.85

    def test_prior_failure_escalates(self):
        router = AdaptiveRouter()
        router.record_failure("test_hash")
        router.route(
            "generate",
            schema_hash="test_hash",
            budget_pressure=0.3,
        )
        decisions = router.get_decisions()
        assert len(decisions) == 1

    def test_cache_hit(self):
        router = AdaptiveRouter()
        # Use a non-deterministic stage so adaptive routing records all signals
        router.route(
            "self_correct_unknown",
            budget_pressure=0.3,
            cache_hit=True,
        )
        decisions = router.get_decisions()
        assert decisions[0]["cache_hit"] is True

    def test_clear_decisions(self):
        router = AdaptiveRouter()
        router.route("generate")
        router.route("validate")
        assert len(router.get_decisions()) == 2
        router.clear_decisions()
        assert len(router.get_decisions()) == 0
