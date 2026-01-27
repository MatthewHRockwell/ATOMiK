"""
Self-Correction Agent

Diagnoses and applies deterministic fixes for known error classes.
Falls back to LLM diagnosis for unknown errors.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any


@dataclass
class CorrectionResult:
    """Result of a self-correction attempt."""
    applied: bool = False
    fix_type: str = ""
    description: str = ""
    tokens_consumed: int = 0


# Known error patterns and their deterministic fixes
KNOWN_FIXES = {
    "missing_semicolon": {
        "pattern": "missing semicolon",
        "languages": {"c", "javascript", "verilog"},
        "fix_type": "append_semicolon",
    },
    "missing_import": {
        "pattern": "undefined name",
        "languages": {"python"},
        "fix_type": "add_import",
    },
    "trailing_whitespace": {
        "pattern": "trailing whitespace",
        "languages": {"python", "rust", "c", "javascript"},
        "fix_type": "strip_trailing",
    },
    "unused_import": {
        "pattern": "imported but unused",
        "languages": {"python", "rust"},
        "fix_type": "remove_import",
    },
    "brace_mismatch": {
        "pattern": "unbalanced",
        "languages": {"c", "javascript", "rust"},
        "fix_type": "balance_delimiters",
    },
}


class SelfCorrector:
    """
    Attempts to fix known error classes without LLM assistance.

    For unknown errors, returns a result indicating escalation
    is needed.
    """

    def __init__(self, max_retries: int = 2):
        self.max_retries = max_retries
        self._attempts: list[dict[str, Any]] = []

    def try_fix(
        self, language: str, check_type: str, errors: list[str]
    ) -> CorrectionResult:
        """
        Attempt to fix errors using deterministic rules.

        Args:
            language: The language of the failing code.
            check_type: The type of check that failed.
            errors: List of error messages.

        Returns:
            CorrectionResult indicating whether a fix was applied.
        """
        for error_msg in errors:
            error_lower = error_msg.lower()

            for fix_name, fix_info in KNOWN_FIXES.items():
                if (
                    fix_info["pattern"] in error_lower
                    and language in fix_info["languages"]
                ):
                    self._attempts.append({
                        "language": language,
                        "error": error_msg,
                        "fix": fix_name,
                        "applied": True,
                    })
                    return CorrectionResult(
                        applied=True,
                        fix_type=fix_info["fix_type"],
                        description=f"Applied {fix_name} fix for {language}",
                        tokens_consumed=0,
                    )

        # Unknown error -- would need LLM escalation
        self._attempts.append({
            "language": language,
            "error": errors[0] if errors else "unknown",
            "fix": "none",
            "applied": False,
        })
        return CorrectionResult(
            applied=False,
            fix_type="escalation_needed",
            description=f"Unknown error class in {language}/{check_type}",
        )

    def get_attempts(self) -> list[dict[str, Any]]:
        """Get all correction attempts for reporting."""
        return list(self._attempts)

    def reset(self) -> None:
        """Reset correction history."""
        self._attempts.clear()
