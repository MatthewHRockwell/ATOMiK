"""C verification runner: gcc -Wall -Werror compilation."""

from __future__ import annotations

import subprocess
import shutil
from typing import Any

from .python_runner import RunnerResult


class CRunner:
    """C verification using gcc compilation with strict warnings."""

    def verify(self, files: list[str], test_dir: str | None = None) -> RunnerResult:
        """Verify C files with gcc -Wall -Werror."""
        result = RunnerResult(language="c", passed=True)

        if not shutil.which("gcc"):
            result.tool_available = False
            result.errors.append("gcc not found")
            result.passed = False
            return result

        for filepath in files:
            result.tests_run += 1
            try:
                proc = subprocess.run(
                    ["gcc", "-Wall", "-Werror", "-fsyntax-only", filepath],
                    capture_output=True,
                    text=True,
                    timeout=30,
                )
                if proc.returncode == 0:
                    result.tests_passed += 1
                else:
                    result.tests_failed += 1
                    result.errors.append(f"gcc: {proc.stderr.strip()[:200]}")
                    result.passed = False
            except (subprocess.TimeoutExpired, FileNotFoundError) as e:
                result.tests_failed += 1
                result.errors.append(f"gcc error: {e}")
                result.passed = False

        return result
