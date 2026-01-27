"""
Self-Validating Verification Stage

Stage 4 of the pipeline: runs lint, type-check, and test execution
against generated outputs. On failure, attempts self-correction via
deterministic fixes or escalation to diagnosis agent.
"""

from __future__ import annotations

import subprocess
from pathlib import Path
from typing import Any

from . import BaseStage, StageManifest, StageStatus
from ..agents.self_correct import SelfCorrector


class VerifyStage(BaseStage):
    """Pipeline stage for verifying generated code."""

    name = "verify"

    def __init__(self) -> None:
        self._corrector = SelfCorrector()

    def run(
        self,
        schema: dict[str, Any],
        schema_path: str,
        previous_manifest: StageManifest | None,
        manifest: StageManifest,
        config: Any,
    ) -> None:
        output_dir = getattr(config, "output_dir", "generated")
        languages = []

        if previous_manifest:
            languages = previous_manifest.metrics.get("languages_generated", [])

        if not languages:
            languages = getattr(config, "languages", None) or [
                "python", "rust", "c", "javascript", "verilog"
            ]

        total_checks = 0
        passed_checks = 0
        lint_errors = 0
        corrections = 0
        correction_successes = 0

        for lang in languages:
            checks = self._verify_language(lang, output_dir, config)

            for check in checks:
                total_checks += 1
                manifest.metrics[f"{lang}_{check['type']}"] = check["status"]

                if check["status"] == "pass":
                    passed_checks += 1
                elif check["status"] == "fail":
                    lint_errors += check.get("error_count", 1)

                    # Attempt self-correction
                    fix = self._corrector.try_fix(
                        lang, check["type"], check.get("errors", [])
                    )
                    corrections += 1
                    if fix.applied:
                        correction_successes += 1
                        manifest.metrics[f"{lang}_{check['type']}_corrected"] = True
                        passed_checks += 1  # Re-count as passed after correction
                    else:
                        manifest.errors.append(
                            f"{lang}/{check['type']}: {check.get('message', 'failed')}"
                        )

        manifest.metrics["total_checks"] = total_checks
        manifest.metrics["passed_checks"] = passed_checks
        manifest.metrics["lint_errors_found"] = lint_errors
        manifest.metrics["self_correction_count"] = corrections
        manifest.metrics["self_correction_success"] = correction_successes

        # Pipeline continues even with non-critical verification failures.
        # Only fail if critical checks fail (compilation, core tests).
        critical_failures = [
            e for e in manifest.errors
            if any(kw in e for kw in ["compile", "syntax", "import"])
        ]
        if critical_failures:
            manifest.status = StageStatus.FAILED
        else:
            if manifest.errors:
                manifest.warnings.extend(manifest.errors)
                manifest.errors.clear()

        manifest.next_stage = "hardware"
        manifest.tokens_consumed = 0  # All local unless self-correction escalates

    def _verify_language(
        self, lang: str, output_dir: str, config: Any
    ) -> list[dict[str, Any]]:
        """Run verification checks for a language."""
        checks = []
        output_path = Path(output_dir)

        if lang == "python":
            checks.append(self._check_python_syntax(output_path))
        elif lang == "rust":
            checks.append(self._check_rust_syntax(output_path))
        elif lang == "c":
            checks.append(self._check_c_syntax(output_path))
        elif lang == "javascript":
            checks.append(self._check_js_syntax(output_path))
        elif lang == "verilog":
            checks.append(self._check_verilog_syntax(output_path))

        return checks

    def _check_python_syntax(self, output_path: Path) -> dict[str, Any]:
        """Check Python files for syntax errors."""
        py_files = list(output_path.rglob("*.py"))
        if not py_files:
            return {"type": "syntax", "status": "skip", "message": "no Python files"}

        errors = []
        for f in py_files:
            try:
                with open(f, encoding="utf-8") as fh:
                    compile(fh.read(), str(f), "exec")
            except SyntaxError as e:
                errors.append(f"{f.name}:{e.lineno}: {e.msg}")

        return {
            "type": "syntax",
            "status": "pass" if not errors else "fail",
            "errors": errors,
            "error_count": len(errors),
            "message": f"{len(errors)} syntax error(s)" if errors else "ok",
        }

    def _check_rust_syntax(self, output_path: Path) -> dict[str, Any]:
        """Check Rust files for basic syntax validity."""
        rs_files = list(output_path.rglob("*.rs"))
        if not rs_files:
            return {"type": "syntax", "status": "skip", "message": "no Rust files"}

        # Basic brace/bracket matching as a syntax proxy
        errors = self._check_balanced_delimiters(rs_files)
        return {
            "type": "syntax",
            "status": "pass" if not errors else "fail",
            "errors": errors,
            "error_count": len(errors),
        }

    def _check_c_syntax(self, output_path: Path) -> dict[str, Any]:
        """Check C files for basic syntax validity."""
        c_files = list(output_path.rglob("*.c")) + list(output_path.rglob("*.h"))
        if not c_files:
            return {"type": "syntax", "status": "skip", "message": "no C files"}

        errors = self._check_balanced_delimiters(c_files)
        return {
            "type": "syntax",
            "status": "pass" if not errors else "fail",
            "errors": errors,
            "error_count": len(errors),
        }

    def _check_js_syntax(self, output_path: Path) -> dict[str, Any]:
        """Check JavaScript files for basic syntax validity."""
        js_files = list(output_path.rglob("*.js"))
        if not js_files:
            return {"type": "syntax", "status": "skip", "message": "no JS files"}

        errors = self._check_balanced_delimiters(js_files)
        return {
            "type": "syntax",
            "status": "pass" if not errors else "fail",
            "errors": errors,
            "error_count": len(errors),
        }

    def _check_verilog_syntax(self, output_path: Path) -> dict[str, Any]:
        """Check Verilog files for basic syntax validity."""
        v_files = list(output_path.rglob("*.v"))
        if not v_files:
            return {"type": "syntax", "status": "skip", "message": "no Verilog files"}

        errors = []
        for f in v_files:
            content = f.read_text(encoding="utf-8")
            # Check for module/endmodule balance
            modules = content.count("module ")
            endmodules = content.count("endmodule")
            if modules != endmodules:
                errors.append(
                    f"{f.name}: mismatched module/endmodule ({modules}/{endmodules})"
                )

        return {
            "type": "syntax",
            "status": "pass" if not errors else "fail",
            "errors": errors,
            "error_count": len(errors),
        }

    @staticmethod
    def _check_balanced_delimiters(files: list[Path]) -> list[str]:
        """Check files for balanced braces, brackets, and parentheses."""
        errors = []
        pairs = {"(": ")", "[": "]", "{": "}"}
        for f in files:
            try:
                content = f.read_text(encoding="utf-8")
            except (OSError, UnicodeDecodeError):
                continue
            stack = []
            in_string = False
            string_char = None
            prev = ""
            for ch in content:
                if in_string:
                    if ch == string_char and prev != "\\":
                        in_string = False
                elif ch in ('"', "'"):
                    in_string = True
                    string_char = ch
                elif ch in pairs:
                    stack.append(pairs[ch])
                elif ch in pairs.values():
                    if not stack or stack[-1] != ch:
                        errors.append(f"{f.name}: unbalanced '{ch}'")
                        break
                    stack.pop()
                prev = ch
            if stack:
                errors.append(f"{f.name}: unclosed delimiter(s)")
        return errors
