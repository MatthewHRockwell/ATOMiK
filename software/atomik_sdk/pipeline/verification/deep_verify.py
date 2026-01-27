"""
Deep Verification Engine

Extends Phase 4C lint-and-syntax verification with native toolchain
verification per language. Adapts verification depth to change scope.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from .runners.c_runner import CRunner
from .runners.js_runner import JavaScriptRunner
from .runners.python_runner import PythonRunner, RunnerResult
from .runners.rust_runner import RustRunner
from .runners.verilog_runner import VerilogRunner


@dataclass
class DeepVerifyResult:
    """Aggregated result of deep verification across all languages."""
    passed: bool = True
    language_results: dict[str, RunnerResult] = field(default_factory=dict)
    total_tests: int = 0
    total_passed: int = 0
    total_failed: int = 0
    all_errors: list[str] = field(default_factory=list)
    verification_depth: str = "full"  # "full", "partial", "metadata_only"

    def to_dict(self) -> dict[str, Any]:
        return {
            "passed": self.passed,
            "total_tests": self.total_tests,
            "total_passed": self.total_passed,
            "total_failed": self.total_failed,
            "verification_depth": self.verification_depth,
            "errors": self.all_errors,
            "languages": {
                lang: r.to_dict() for lang, r in self.language_results.items()
            },
        }


class DeepVerifier:
    """
    Deep verification engine with per-language native toolchain runners.

    Runs each language's native verification tools (pytest, cargo test,
    gcc, node --check, iverilog+vvp). Verification depth adapts to
    the change scope.

    Example:
        >>> verifier = DeepVerifier()
        >>> result = verifier.verify_all(file_map)
        >>> if not result.passed:
        ...     print(f"Failed: {result.all_errors}")
    """

    def __init__(self) -> None:
        self._runners = {
            "python": PythonRunner(),
            "rust": RustRunner(),
            "c": CRunner(),
            "javascript": JavaScriptRunner(),
            "verilog": VerilogRunner(),
        }

    def verify_all(
        self,
        files_by_language: dict[str, list[str]],
        verification_depth: str = "full",
    ) -> DeepVerifyResult:
        """
        Verify all language outputs.

        Args:
            files_by_language: Map of language -> list of file paths.
            verification_depth: "full", "partial", or "metadata_only".

        Returns:
            DeepVerifyResult with per-language results.
        """
        result = DeepVerifyResult(verification_depth=verification_depth)

        if verification_depth == "metadata_only":
            # Only verify Python and JavaScript (docstrings/JSDoc)
            languages = {"python", "javascript"} & set(files_by_language.keys())
        elif verification_depth == "partial":
            languages = set(files_by_language.keys())
        else:
            languages = set(files_by_language.keys())

        for lang in sorted(languages):
            files = files_by_language.get(lang, [])
            if not files:
                continue

            runner = self._runners.get(lang)
            if runner is None:
                continue

            lang_result = runner.verify(files)
            result.language_results[lang] = lang_result

            result.total_tests += lang_result.tests_run
            result.total_passed += lang_result.tests_passed
            result.total_failed += lang_result.tests_failed

            if not lang_result.passed:
                result.passed = False
                result.all_errors.extend(lang_result.errors)

        return result

    def verify_language(
        self,
        language: str,
        files: list[str],
    ) -> RunnerResult:
        """Verify a single language's output."""
        runner = self._runners.get(language)
        if runner is None:
            return RunnerResult(
                language=language,
                passed=False,
                errors=[f"No runner for language: {language}"],
                tool_available=False,
            )
        return runner.verify(files)
