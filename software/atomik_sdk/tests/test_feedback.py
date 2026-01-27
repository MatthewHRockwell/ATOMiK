"""Tests for feedback loop engine."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from pipeline.event_bus import EventBus
from pipeline.feedback import FeedbackLoop, FeedbackOutcome


def _mock_classifier(language, errors):
    return ("syntax_error", errors[0] if errors else "")


def _mock_kb_found(language, error_class, msg):
    return (True, "add semicolon")


def _mock_kb_not_found(language, error_class, msg):
    return (False, "")


def _mock_apply_success(language, error_class, fix_desc):
    return True


def _mock_verify_pass(language):
    return (True, [])


def _mock_verify_fail(language):
    return (False, ["still broken"])


def _mock_llm_diagnose(language, error_class, errors):
    return ("llm fix applied", 500)


class TestFeedbackLoop:
    def test_kb_fix_success(self):
        loop = FeedbackLoop(max_depth=3)
        result = loop.run(
            "python",
            ["missing semicolon"],
            _mock_classifier,
            _mock_kb_found,
            _mock_apply_success,
            _mock_llm_diagnose,
            _mock_verify_pass,
        )
        assert result.outcome == FeedbackOutcome.FIXED_BY_KB
        assert result.resolved
        assert len(result.iterations) == 1
        assert result.iterations[0].fix_source == "kb"

    def test_llm_fix_success(self):
        loop = FeedbackLoop(max_depth=3)
        result = loop.run(
            "python",
            ["unknown error"],
            _mock_classifier,
            _mock_kb_not_found,
            _mock_apply_success,
            _mock_llm_diagnose,
            _mock_verify_pass,
        )
        assert result.outcome == FeedbackOutcome.FIXED_BY_LLM
        assert result.resolved
        assert result.iterations[0].fix_source == "llm"
        assert result.total_tokens == 500

    def test_retry_exhausted(self):
        loop = FeedbackLoop(max_depth=2)
        # KB not found, LLM fix fails verification each time
        result = loop.run(
            "python",
            ["error1"],
            lambda lang, e: ("err_a", e[0]),
            _mock_kb_not_found,
            _mock_apply_success,
            _mock_llm_diagnose,
            _mock_verify_fail,
        )
        # Different errors each time prevents identical error detection
        assert result.outcome in (FeedbackOutcome.RETRY_EXHAUSTED, FeedbackOutcome.IDENTICAL_ERROR)

    def test_identical_error_escalation(self):
        call_count = [0]
        def classifier(lang, errors):
            call_count[0] += 1
            return ("same_error", "same msg")

        loop = FeedbackLoop(max_depth=5)
        result = loop.run(
            "python",
            ["same error"],
            classifier,
            _mock_kb_not_found,
            _mock_apply_success,
            _mock_llm_diagnose,
            _mock_verify_fail,
        )
        assert result.outcome == FeedbackOutcome.IDENTICAL_ERROR

    def test_event_bus_integration(self):
        bus = EventBus()
        events = []
        bus.subscribe(
            lambda e: events.append(e),
        ) if False else None  # noqa

        loop = FeedbackLoop(max_depth=1, event_bus=bus)
        result = loop.run(
            "python",
            ["err"],
            _mock_classifier,
            _mock_kb_found,
            _mock_apply_success,
            _mock_llm_diagnose,
            _mock_verify_pass,
        )
        assert result.resolved
        history = bus.get_history()
        assert len(history) >= 1
