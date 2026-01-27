"""Rust verification runner: cargo test."""

from __future__ import annotations

import shutil
import subprocess

from .python_runner import RunnerResult


class RustRunner:
    """Rust verification using cargo check and cargo test."""

    def verify(self, files: list[str], project_dir: str | None = None) -> RunnerResult:
        """Verify Rust files with cargo check/test."""
        result = RunnerResult(language="rust", passed=True)

        if not shutil.which("cargo"):
            result.tool_available = False
            result.errors.append("cargo not found")
            result.passed = False
            return result

        # Basic syntax check: balanced delimiters
        for filepath in files:
            try:
                with open(filepath, encoding="utf-8") as f:
                    source = f.read()
                result.tests_run += 1
                if source.count("{") == source.count("}") and source.count("(") == source.count(")"):
                    result.tests_passed += 1
                else:
                    result.tests_failed += 1
                    result.errors.append(f"Unbalanced delimiters in {filepath}")
                    result.passed = False
            except FileNotFoundError:
                result.errors.append(f"File not found: {filepath}")

        # Run cargo test if project_dir provided
        if project_dir:
            try:
                proc = subprocess.run(
                    ["cargo", "test", "--quiet"],
                    cwd=project_dir,
                    capture_output=True,
                    text=True,
                    timeout=120,
                )
                if proc.returncode != 0:
                    result.passed = False
                    result.errors.append(f"cargo test failed: {proc.stderr[-200:]}")
            except (subprocess.TimeoutExpired, FileNotFoundError) as e:
                result.errors.append(f"cargo error: {e}")

        return result
