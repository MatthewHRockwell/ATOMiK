"""Tests for token prediction, caching, and compression."""

import pytest
from pipeline.agents.token_predictor import TokenPredictor
from pipeline.agents.prompt_cache import PromptCache
from pipeline.agents.context_compressor import ContextCompressor


class TestTokenPredictor:
    def test_predict_with_history(self):
        predictor = TokenPredictor()
        predictor.record_actual("gen", 1000)
        predictor.record_actual("gen", 1200)
        predictor.record_actual("gen", 1100)
        predicted = predictor.predict("gen")
        assert 900 < predicted < 1300

    def test_predict_no_history(self):
        predictor = TokenPredictor(default_estimates={"gen": 2000})
        predicted = predictor.predict("gen")
        assert predicted == 2000

    def test_predict_no_history_no_default(self):
        predictor = TokenPredictor()
        predicted = predictor.predict("gen")
        assert predicted == 0

    def test_predict_and_track(self):
        predictor = TokenPredictor()
        predictor.record_actual("gen", 1000)
        predicted = predictor.predict_and_track("gen")
        assert isinstance(predicted, int)
        predictor.finalize_prediction("gen", 1050)
        records = predictor.get_predictions()
        assert len(records) == 1
        assert records[0]["actual"] == 1050

    def test_predict_remaining(self):
        predictor = TokenPredictor()
        predictor.record_actual("validate", 500)
        predictor.record_actual("generate", 3000)
        remaining = predictor.predict_remaining(["validate", "generate"])
        assert remaining > 0


class TestPromptCache:
    def test_cache_put_get(self):
        cache = PromptCache()
        cache.put("task1", "schema_abc", "cached response")
        result = cache.get("task1", "schema_abc")
        assert result == "cached response"

    def test_cache_miss(self):
        cache = PromptCache()
        result = cache.get("missing", "schema_abc")
        assert result is None

    def test_cache_hit_rate(self):
        cache = PromptCache()
        cache.put("t1", "s1", "val")
        cache.get("t1", "s1")  # hit
        cache.get("t2", "s2")  # miss
        rate = cache.hit_rate()
        assert 40 < rate < 60  # ~50% (returns percentage)

    def test_cache_invalidation(self):
        cache = PromptCache()
        cache.put("t1", "s1", "val")
        removed = cache.invalidate_schema("s1")
        assert removed >= 1
        result = cache.get("t1", "s1")
        assert result is None

    def test_cache_clear(self):
        cache = PromptCache()
        cache.put("t1", "s1", "val")
        cache.clear()
        result = cache.get("t1", "s1")
        assert result is None


class TestContextCompressor:
    def test_strip_comments(self):
        compressor = ContextCompressor()
        text = "code line\n# comment\nmore code\n// js comment"
        result = compressor.compress(text, budget_pressure=0.55)
        assert "code line" in result

    def test_progressive_compression(self):
        compressor = ContextCompressor()
        text = "a " * 500
        result_light = compressor.compress(text, budget_pressure=0.55)
        result_heavy = compressor.compress(text, budget_pressure=0.95)
        assert len(result_heavy) <= len(result_light)

    def test_schema_compression(self):
        compressor = ContextCompressor()
        schema = {
            "name": "test",
            "description": "A long description that should be stripped",
            "fields": [{"name": "a", "type": "int"}],
            "examples": ["ex1", "ex2"],
            "metadata": {"author": "test"},
        }
        result = compressor.compress_schema_context(schema, budget_pressure=0.75)
        assert isinstance(result, dict)
        assert "name" in result

    def test_no_compression_low_pressure(self):
        compressor = ContextCompressor()
        text = "unchanged text"
        result = compressor.compress(text, budget_pressure=0.1)
        assert result == text
