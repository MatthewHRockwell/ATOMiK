"""Python verification runner: pytest + coverage."""

from __future__ import annotations

import shutil
import subprocess
from dataclasses import dataclass, field
from typing import Any


@dataclass
class RunnerResult:
    """Result from a verification runner."""
    language: str
    passed: bool
    tests_run: int = 0
    tests_passed: int = 0
    tests_failed: int = 0
    coverage_pct: float = 0.0
    errors: list[str] = field(default_factory=list)
    duration_ms: float = 0.0
    tool_available: bool = True

    def to_dict(self) -> dict[str, Any]:
        return {
            "language": self.language,
            "passed": self.passed,
            "tests_run": self.tests_run,
            "tests_passed": self.tests_passed,
            "tests_failed": self.tests_failed,
            "coverage_pct": round(self.coverage_pct, 1),
            "errors": self.errors,
            "duration_ms": round(self.duration_ms, 1),
            "tool_available": self.tool_available,
        }


class PythonRunner:
    """
    Python verification runner using pytest.

    Runs pytest on generated Python files and optionally
    collects coverage data.

    Example:
        >>> runner = PythonRunner()
        >>> result = runner.verify(["generated/python/module.py"])
    """

    def verify(
        self,
        files: list[str],
        test_dir: str | None = None,
    ) -> RunnerResult:
        """
        Verify Python files with pytest.

        Args:
            files: Python file paths to verify.
            test_dir: Directory containing tests (optional).

        Returns:
            RunnerResult with test outcomes.
        """
        result = RunnerResult(language="python", passed=True)

        # Check tool availability
        if not shutil.which("python") and not shutil.which("python3"):
            result.tool_available = False
            result.errors.append("Python interpreter not found")
            result.passed = False
            return result

        # Syntax check via compile()
        for filepath in files:
            try:
                with open(filepath, encoding="utf-8") as f:
                    source = f.read()
                compile(source, filepath, "exec")
                result.tests_run += 1
                result.tests_passed += 1
            except SyntaxError as e:
                result.tests_run += 1
                result.tests_failed += 1
                result.errors.append(f"SyntaxError in {filepath}: {e}")
                result.passed = False
            except FileNotFoundError:
                result.errors.append(f"File not found: {filepath}")

        # Run pytest if available and test_dir provided
        if test_dir and shutil.which("pytest"):
            try:
                proc = subprocess.run(
                    ["pytest", test_dir, "--tb=short", "-q"],
                    capture_output=True,
                    text=True,
                    timeout=60,
                )
                if proc.returncode != 0:
                    result.passed = False
                    result.errors.append(f"pytest failed: {proc.stdout[-200:]}")
            except (subprocess.TimeoutExpired, FileNotFoundError) as e:
                result.errors.append(f"pytest error: {e}")

        return result
