"""JavaScript verification runner: node --check."""

from __future__ import annotations

import shutil
import subprocess

from .python_runner import RunnerResult


class JavaScriptRunner:
    """JavaScript verification using node --check for syntax."""

    def verify(self, files: list[str], test_dir: str | None = None) -> RunnerResult:
        """Verify JavaScript files with node --check."""
        result = RunnerResult(language="javascript", passed=True)

        if not shutil.which("node"):
            result.tool_available = False
            result.errors.append("node not found")
            result.passed = False
            return result

        for filepath in files:
            result.tests_run += 1
            try:
                proc = subprocess.run(
                    ["node", "--check", filepath],
                    capture_output=True,
                    text=True,
                    timeout=30,
                )
                if proc.returncode == 0:
                    result.tests_passed += 1
                else:
                    result.tests_failed += 1
                    result.errors.append(f"node: {proc.stderr.strip()[:200]}")
                    result.passed = False
            except (subprocess.TimeoutExpired, FileNotFoundError) as e:
                result.tests_failed += 1
                result.errors.append(f"node error: {e}")
                result.passed = False

        return result
