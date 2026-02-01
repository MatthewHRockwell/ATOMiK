"""
Tests for the ``atomik-gen demo`` CLI command.

All subprocess calls are mocked — no real hardware, iverilog, or
synthesis tools are needed.
"""

import argparse
import json
import sys
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).parent.parent))

from cli import (  # noqa: E402
    EXIT_FILE_ERROR,
    EXIT_HARDWARE_FAILURE,
    EXIT_SUCCESS,
    cmd_demo,
)

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _demo_args(**overrides) -> argparse.Namespace:
    """Build a minimal ``argparse.Namespace`` for ``cmd_demo``."""
    defaults = {
        "domain": "video",
        "com_port": None,
        "sim_only": False,
        "report": None,
        "verbose": False,
    }
    defaults.update(overrides)
    return argparse.Namespace(**defaults)


def _make_sim_output(passed: int, failed: int) -> str:
    """Produce the summary block that our testbenches emit."""
    lines = []
    if failed == 0:
        lines.append("*** ALL TESTS PASSED ***")
    else:
        lines.append("*** SOME TESTS FAILED ***")
    lines.append(f"Passed: {passed}")
    lines.append(f"Failed: {failed}")
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

class TestDemoUnknownDomain:
    def test_returns_exit_file_error(self):
        args = _demo_args(domain="nonexistent")
        assert cmd_demo(args) == EXIT_FILE_ERROR


class TestDemoMissingDemoDir:
    def test_returns_exit_file_error(self):
        """A valid domain name that isn't in valid_domains gives EXIT_FILE_ERROR."""
        assert cmd_demo(_demo_args(domain="bogus")) == EXIT_FILE_ERROR


class TestDemoSimulationPass:
    """Mock iverilog + vvp succeeding."""

    @mock.patch("subprocess.run")
    @mock.patch("shutil.which")
    @mock.patch("atomik_sdk.hardware_discovery.detect_board", return_value=None)
    @mock.patch("atomik_sdk.hardware_discovery.find_tool", return_value=None)
    def test_simulation_pass(
        self, mock_find_tool, mock_detect, mock_which, mock_run, tmp_path
    ):
        def which_side_effect(name):
            if name in ("iverilog", "vvp"):
                return f"/usr/bin/{name}"
            return None

        mock_which.side_effect = which_side_effect

        compile_result = mock.MagicMock(returncode=0, stderr="")
        sim_result = mock.MagicMock(
            returncode=0,
            stdout=_make_sim_output(10, 0),
            stderr="",
        )
        mock_run.side_effect = [compile_result, sim_result]

        report_file = str(tmp_path / "report.json")
        args = _demo_args(report=report_file)
        rc = cmd_demo(args)

        assert rc == EXIT_SUCCESS

        report = json.loads(Path(report_file).read_text())
        assert report["success"] is True
        assert report["mode"] == "auto"
        assert "domain" in report
        assert "validation_level" in report
        assert "duration_ms" in report
        assert "tools" in report


class TestDemoSimulationFail:
    """Mock vvp output containing FAIL."""

    @mock.patch("subprocess.run")
    @mock.patch("shutil.which")
    @mock.patch("atomik_sdk.hardware_discovery.detect_board", return_value=None)
    @mock.patch("atomik_sdk.hardware_discovery.find_tool", return_value=None)
    def test_simulation_fail(
        self, mock_find_tool, mock_detect, mock_which, mock_run, tmp_path
    ):
        mock_which.side_effect = (
            lambda n: f"/usr/bin/{n}" if n in ("iverilog", "vvp") else None
        )

        compile_result = mock.MagicMock(returncode=0, stderr="")
        sim_result = mock.MagicMock(
            returncode=0,
            stdout=_make_sim_output(8, 2),
            stderr="",
        )
        mock_run.side_effect = [compile_result, sim_result]

        report_file = str(tmp_path / "report.json")
        args = _demo_args(report=report_file)
        rc = cmd_demo(args)

        assert rc == EXIT_HARDWARE_FAILURE

        report = json.loads(Path(report_file).read_text())
        assert report["success"] is False
        assert report["validation_level"] == "simulation_only"


class TestDemoSimOnlyMode:
    """--sim-only should set mode to 'simulation' and skip board detection."""

    @mock.patch("subprocess.run")
    @mock.patch("shutil.which")
    @mock.patch("atomik_sdk.hardware_discovery.detect_board")
    @mock.patch("atomik_sdk.hardware_discovery.find_tool", return_value=None)
    def test_sim_only_mode(
        self, mock_find_tool, mock_detect, mock_which, mock_run, tmp_path
    ):
        mock_which.side_effect = (
            lambda n: f"/usr/bin/{n}" if n in ("iverilog", "vvp") else None
        )

        compile_result = mock.MagicMock(returncode=0, stderr="")
        sim_result = mock.MagicMock(
            returncode=0,
            stdout=_make_sim_output(10, 0),
            stderr="",
        )
        mock_run.side_effect = [compile_result, sim_result]

        report_file = str(tmp_path / "report.json")
        args = _demo_args(sim_only=True, report=report_file)
        rc = cmd_demo(args)

        assert rc == EXIT_SUCCESS
        # detect_board should NOT have been called (sim-only skips it)
        mock_detect.assert_not_called()

        report = json.loads(Path(report_file).read_text())
        assert report["mode"] == "simulation"


class TestDemoBoardDetected:
    """Mock programmer_cli returning 'Cable found'."""

    @mock.patch("subprocess.run")
    @mock.patch("shutil.which")
    @mock.patch("atomik_sdk.hardware_discovery.detect_board", return_value="tangnano9k")
    @mock.patch("atomik_sdk.hardware_discovery.find_tool", return_value=None)
    def test_board_detected(
        self, mock_find_tool, mock_detect, mock_which, mock_run, tmp_path
    ):
        mock_which.side_effect = (
            lambda n: f"/usr/bin/{n}" if n in ("iverilog", "vvp") else None
        )

        compile_result = mock.MagicMock(returncode=0, stderr="")
        sim_result = mock.MagicMock(
            returncode=0,
            stdout=_make_sim_output(10, 0),
            stderr="",
        )
        mock_run.side_effect = [compile_result, sim_result]

        report_file = str(tmp_path / "report.json")
        args = _demo_args(report=report_file)
        rc = cmd_demo(args)

        assert rc == EXIT_SUCCESS

        report = json.loads(Path(report_file).read_text())
        assert report["board_detected"] is True


class TestDemoNoIverilog:
    """shutil.which returns None for iverilog — should report failure."""

    @mock.patch("shutil.which", return_value=None)
    @mock.patch("atomik_sdk.hardware_discovery.detect_board", return_value=None)
    @mock.patch("atomik_sdk.hardware_discovery.find_tool", return_value=None)
    def test_no_iverilog(self, mock_find_tool, mock_detect, mock_which):
        args = _demo_args()
        rc = cmd_demo(args)
        assert rc == EXIT_HARDWARE_FAILURE
