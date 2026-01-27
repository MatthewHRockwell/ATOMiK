"""Verilog verification runner: iverilog + vvp simulation."""

from __future__ import annotations

import os
import shutil
import subprocess
import tempfile

from .python_runner import RunnerResult


class VerilogRunner:
    """Verilog verification using iverilog compilation and vvp simulation."""

    def verify(
        self,
        files: list[str],
        testbench: str | None = None,
    ) -> RunnerResult:
        """
        Verify Verilog files with iverilog and vvp.

        Args:
            files: Verilog source files.
            testbench: Path to testbench file (optional).

        Returns:
            RunnerResult with simulation outcomes.
        """
        result = RunnerResult(language="verilog", passed=True)

        if not shutil.which("iverilog"):
            result.tool_available = False
            result.errors.append("iverilog not found")
            result.passed = False
            return result

        # Lint check with iverilog -t null
        for filepath in files:
            result.tests_run += 1
            try:
                proc = subprocess.run(
                    ["iverilog", "-t", "null", filepath],
                    capture_output=True,
                    text=True,
                    timeout=30,
                )
                if proc.returncode == 0:
                    result.tests_passed += 1
                else:
                    result.tests_failed += 1
                    result.errors.append(f"iverilog: {proc.stderr.strip()[:200]}")
                    result.passed = False
            except (subprocess.TimeoutExpired, FileNotFoundError) as e:
                result.tests_failed += 1
                result.errors.append(f"iverilog error: {e}")
                result.passed = False

        # Run simulation if testbench provided
        if testbench and shutil.which("vvp") and result.passed:
            all_files = list(files) + [testbench]
            try:
                with tempfile.NamedTemporaryFile(suffix=".vvp", delete=False) as tmp:
                    tmp_path = tmp.name

                # Compile
                proc = subprocess.run(
                    ["iverilog", "-o", tmp_path] + all_files,
                    capture_output=True,
                    text=True,
                    timeout=30,
                )
                if proc.returncode == 0:
                    # Simulate
                    proc = subprocess.run(
                        ["vvp", tmp_path],
                        capture_output=True,
                        text=True,
                        timeout=60,
                    )
                    result.tests_run += 1
                    if proc.returncode == 0:
                        result.tests_passed += 1
                    else:
                        result.tests_failed += 1
                        result.errors.append(f"vvp: {proc.stdout[-200:]}")
                        result.passed = False
                else:
                    result.errors.append(f"Compile failed: {proc.stderr[:200]}")
                    result.passed = False

                # Cleanup
                if os.path.exists(tmp_path):
                    os.unlink(tmp_path)
            except (subprocess.TimeoutExpired, FileNotFoundError) as e:
                result.errors.append(f"Simulation error: {e}")
                result.passed = False

        return result
