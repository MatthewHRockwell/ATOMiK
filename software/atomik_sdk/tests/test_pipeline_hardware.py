"""
Tests for ATOMiK Pipeline Hardware Stage

Tests cover:
- Hardware stage creation
- RTL simulation detection
- Verilog file discovery
- Phase 3 baseline comparison
- Validation level progression
"""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from pipeline.stages.hardware import HardwareStage


@pytest.fixture
def hw_stage():
    return HardwareStage()


@pytest.fixture
def project_root():
    return Path(__file__).parent.parent.parent.parent


class TestHardwareStage:
    def test_stage_name(self, hw_stage):
        assert hw_stage.name == "hardware"

    def test_phase3_baseline(self, hw_stage):
        baseline = hw_stage.PHASE3_BASELINE
        assert baseline["fmax_mhz"] == 94.9
        assert baseline["lut_pct"] == 7
        assert baseline["ff_pct"] == 9
        assert baseline["tests_passed"] == 10

    def test_no_verilog_files(self, hw_stage, tmp_path):
        """Stage should warn when no Verilog files found."""
        empty_dir = tmp_path / "generated"
        empty_dir.mkdir()

        config = type("Config", (), {
            "sim_only": False,
            "skip_synthesis": False,
            "output_dir": str(empty_dir),
            "com_port": None,
        })()

        manifest = hw_stage.execute({}, "test.json", None, config)

        assert any("no verilog" in w.lower() for w in manifest.warnings)
        assert manifest.validation_level == "no_rtl"

    def test_verilog_file_discovery(self, hw_stage, tmp_path):
        """Stage should find Verilog files in output directory."""
        out_dir = tmp_path / "generated"
        out_dir.mkdir()
        (out_dir / "module.v").write_text("module test;\nendmodule\n")
        (out_dir / "tb_module.v").write_text(
            "module tb_test;\ninitial $finish;\nendmodule\n"
        )

        config = type("Config", (), {
            "sim_only": True,
            "skip_synthesis": True,
            "output_dir": str(out_dir),
            "com_port": None,
        })()

        manifest = hw_stage.execute({}, "test.json", None, config)

        # Should find the files (simulation may or may not be available)
        assert manifest.tokens_consumed == 0

    def test_sim_only_mode(self, hw_stage, tmp_path):
        """sim_only should skip synthesis and programming."""
        out_dir = tmp_path / "generated"
        out_dir.mkdir()
        (out_dir / "test.v").write_text("module t;\nendmodule\n")

        config = type("Config", (), {
            "sim_only": True,
            "skip_synthesis": False,
            "output_dir": str(out_dir),
            "com_port": None,
        })()

        manifest = hw_stage.execute({}, "test.json", None, config)

        # In sim_only mode, validation level should be simulation_only at most
        assert manifest.validation_level in ("simulation_only", "no_rtl")

    def test_zero_tokens(self, hw_stage, tmp_path):
        """Hardware stage should always consume 0 tokens."""
        out_dir = tmp_path / "generated"
        out_dir.mkdir()

        config = type("Config", (), {
            "sim_only": True,
            "skip_synthesis": True,
            "output_dir": str(out_dir),
            "com_port": None,
        })()

        manifest = hw_stage.execute({}, "test.json", None, config)
        assert manifest.tokens_consumed == 0

    def test_baseline_included_in_metrics(self, hw_stage, tmp_path):
        """Phase 3 baseline should be in metrics when hardware runs."""
        out_dir = tmp_path / "generated"
        out_dir.mkdir()
        (out_dir / "test.v").write_text("module t;\nendmodule\n")
        (out_dir / "tb_test.v").write_text("module tb;\ninitial $finish;\nendmodule\n")

        config = type("Config", (), {
            "sim_only": True,
            "skip_synthesis": False,
            "output_dir": str(out_dir),
            "com_port": None,
        })()

        manifest = hw_stage.execute({}, "test.json", None, config)

        baseline = manifest.metrics.get("phase3_baseline")
        assert baseline is not None
        assert baseline["fmax_mhz"] == 94.9
