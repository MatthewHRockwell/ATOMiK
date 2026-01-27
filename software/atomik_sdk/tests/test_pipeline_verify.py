"""
Tests for ATOMiK Pipeline Verification Stage

Tests cover:
- Python syntax checking
- Verilog module/endmodule balance
- Balanced delimiter checking
- Self-correction for known error classes
- Verification manifest generation
"""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from pipeline.agents.self_correct import SelfCorrector
from pipeline.stages import StageManifest
from pipeline.stages.verify import VerifyStage


@pytest.fixture
def verify_stage():
    return VerifyStage()


class TestVerifyStage:
    def test_stage_name(self, verify_stage):
        assert verify_stage.name == "verify"

    def test_verify_valid_python(self, verify_stage, tmp_path):
        """Valid Python files should pass verification."""
        py_dir = tmp_path / "generated"
        py_dir.mkdir()
        (py_dir / "module.py").write_text(
            "class Delta:\n    def __init__(self):\n        self.value = 0\n"
        )

        prev = StageManifest(stage="generate")
        prev.metrics["languages_generated"] = ["python"]

        config = type("Config", (), {"output_dir": str(py_dir)})()
        manifest = verify_stage.execute({}, "test.json", prev, config)

        assert manifest.metrics.get("python_syntax") == "pass"

    def test_verify_invalid_python_syntax(self, verify_stage, tmp_path):
        """Invalid Python should be detected."""
        py_dir = tmp_path / "generated"
        py_dir.mkdir()
        (py_dir / "bad.py").write_text("def foo(\n")  # Missing closing paren

        prev = StageManifest(stage="generate")
        prev.metrics["languages_generated"] = ["python"]

        config = type("Config", (), {"output_dir": str(py_dir)})()
        manifest = verify_stage.execute({}, "test.json", prev, config)

        # Should detect the syntax error
        assert manifest.metrics.get("lint_errors_found", 0) > 0

    def test_verify_verilog_balanced_modules(self, verify_stage, tmp_path):
        """Balanced module/endmodule should pass."""
        v_dir = tmp_path / "generated"
        v_dir.mkdir()
        (v_dir / "test.v").write_text(
            "module test;\n  wire a;\nendmodule\n"
        )

        prev = StageManifest(stage="generate")
        prev.metrics["languages_generated"] = ["verilog"]

        config = type("Config", (), {"output_dir": str(v_dir)})()
        manifest = verify_stage.execute({}, "test.json", prev, config)

        assert manifest.metrics.get("verilog_syntax") == "pass"

    def test_verify_verilog_unbalanced_modules(self, verify_stage, tmp_path):
        """Unbalanced module/endmodule should fail."""
        v_dir = tmp_path / "generated"
        v_dir.mkdir()
        (v_dir / "bad.v").write_text(
            "module test;\n  wire a;\n"  # Missing endmodule
        )

        prev = StageManifest(stage="generate")
        prev.metrics["languages_generated"] = ["verilog"]

        config = type("Config", (), {"output_dir": str(v_dir)})()
        manifest = verify_stage.execute({}, "test.json", prev, config)

        assert manifest.metrics.get("lint_errors_found", 0) > 0

    def test_no_files_skips(self, verify_stage, tmp_path):
        """Empty output directory should skip checks."""
        empty_dir = tmp_path / "empty"
        empty_dir.mkdir()

        prev = StageManifest(stage="generate")
        prev.metrics["languages_generated"] = ["python"]

        config = type("Config", (), {"output_dir": str(empty_dir)})()
        manifest = verify_stage.execute({}, "test.json", prev, config)

        # Should skip since no files found
        assert manifest.metrics.get("python_syntax") == "skip"


class TestSelfCorrector:
    def test_known_fix_trailing_whitespace(self):
        corrector = SelfCorrector()
        result = corrector.try_fix(
            "python", "lint", ["trailing whitespace on line 5"]
        )
        assert result.applied is True
        assert result.tokens_consumed == 0

    def test_known_fix_missing_semicolon(self):
        corrector = SelfCorrector()
        result = corrector.try_fix(
            "c", "lint", ["error: missing semicolon"]
        )
        assert result.applied is True

    def test_unknown_error_needs_escalation(self):
        corrector = SelfCorrector()
        result = corrector.try_fix(
            "python", "test", ["AssertionError: unexpected value"]
        )
        assert result.applied is False
        assert result.fix_type == "escalation_needed"

    def test_language_specific_fixes(self):
        corrector = SelfCorrector()
        # Python doesn't have missing semicolon fix
        result = corrector.try_fix(
            "python", "lint", ["missing semicolon"]
        )
        assert result.applied is False

    def test_attempts_tracking(self):
        corrector = SelfCorrector()
        corrector.try_fix("python", "lint", ["trailing whitespace"])
        corrector.try_fix("c", "lint", ["unknown error"])
        attempts = corrector.get_attempts()
        assert len(attempts) == 2
        assert attempts[0]["applied"] is True
        assert attempts[1]["applied"] is False
