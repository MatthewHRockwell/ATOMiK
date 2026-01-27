"""Tests for deep verification engine."""

import pytest
from pipeline.verification.deep_verify import DeepVerifier, DeepVerifyResult
from pipeline.verification.runners.python_runner import PythonRunner, RunnerResult


class TestDeepVerifyResult:
    def test_default_result(self):
        result = DeepVerifyResult()
        assert result.passed is True
        assert result.total_tests == 0
        assert result.verification_depth == "full"

    def test_to_dict(self):
        result = DeepVerifyResult(
            passed=True,
            total_tests=10,
            total_passed=9,
            total_failed=1,
        )
        d = result.to_dict()
        assert d["total_tests"] == 10
        assert d["total_passed"] == 9


class TestRunnerResult:
    def test_creation(self):
        result = RunnerResult(language="python", passed=True)
        assert result.language == "python"
        assert result.passed is True
        assert result.tool_available is True

    def test_to_dict(self):
        result = RunnerResult(
            language="python",
            passed=True,
            tests_run=5,
            tests_passed=5,
        )
        d = result.to_dict()
        assert d["language"] == "python"
        assert d["passed"] is True


class TestDeepVerifier:
    def test_verify_all_empty(self):
        verifier = DeepVerifier()
        result = verifier.verify_all({})
        assert result.passed is True
        assert result.total_tests == 0

    def test_verify_unknown_language(self):
        verifier = DeepVerifier()
        result = verifier.verify_language("unknown", [])
        assert not result.passed
        assert not result.tool_available

    def test_metadata_only_depth(self):
        verifier = DeepVerifier()
        result = verifier.verify_all(
            {"python": [], "rust": [], "c": []},
            verification_depth="metadata_only",
        )
        assert result.verification_depth == "metadata_only"

    def test_verify_all_empty_files(self):
        verifier = DeepVerifier()
        result = verifier.verify_all({"python": []})
        assert result.total_tests == 0
