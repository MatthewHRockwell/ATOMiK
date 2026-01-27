"""
Error Diagnosis

Classifies errors by type and produces structured diagnostic
information for the feedback loop and knowledge base.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

# Error class patterns for classification
ERROR_PATTERNS: list[dict[str, Any]] = [
    {
        "class": "syntax_error",
        "patterns": ["syntaxerror", "syntax error", "unexpected token", "parse error"],
        "severity": "critical",
    },
    {
        "class": "import_error",
        "patterns": ["importerror", "modulenotfounderror", "undefined name", "not found"],
        "severity": "critical",
    },
    {
        "class": "type_error",
        "patterns": ["typeerror", "type mismatch", "incompatible type", "expected type"],
        "severity": "critical",
    },
    {
        "class": "missing_semicolon",
        "patterns": ["missing semicolon", "expected ';'", "expected semicolon"],
        "severity": "minor",
    },
    {
        "class": "brace_mismatch",
        "patterns": ["unbalanced", "unexpected }", "expected }", "unmatched"],
        "severity": "critical",
    },
    {
        "class": "naming_error",
        "patterns": ["naming convention", "invalid identifier", "expected identifier"],
        "severity": "minor",
    },
    {
        "class": "lint_warning",
        "patterns": ["trailing whitespace", "unused import", "imported but unused", "unused variable"],
        "severity": "warning",
    },
    {
        "class": "test_failure",
        "patterns": ["assertionerror", "test failed", "assert", "expected"],
        "severity": "critical",
    },
    {
        "class": "compilation_error",
        "patterns": ["error:", "fatal error", "cannot compile", "compilation failed"],
        "severity": "critical",
    },
]


@dataclass
class Diagnosis:
    """Structured diagnosis of an error."""
    error_class: str
    severity: str          # "critical", "minor", "warning"
    primary_message: str
    language: str = ""
    all_messages: list[str] = field(default_factory=list)

    def to_dict(self) -> dict[str, Any]:
        return {
            "error_class": self.error_class,
            "severity": self.severity,
            "primary_message": self.primary_message,
            "language": self.language,
            "all_messages": self.all_messages,
        }


class ErrorClassifier:
    """
    Classifies error messages into known error classes.

    Uses pattern matching against a database of known error signatures.
    Returns structured Diagnosis objects.

    Example:
        >>> classifier = ErrorClassifier()
        >>> diag = classifier.classify("python", ["SyntaxError: unexpected EOF"])
        >>> assert diag.error_class == "syntax_error"
    """

    def __init__(
        self,
        custom_patterns: list[dict[str, Any]] | None = None,
    ) -> None:
        self._patterns = list(ERROR_PATTERNS)
        if custom_patterns:
            self._patterns.extend(custom_patterns)

    def classify(self, language: str, errors: list[str]) -> Diagnosis:
        """
        Classify errors into a known error class.

        Args:
            language: Language that produced the errors.
            errors: List of error messages.

        Returns:
            Diagnosis with error class and severity.
        """
        primary = errors[0] if errors else "unknown error"
        primary_lower = primary.lower()

        for pattern_info in self._patterns:
            for pattern in pattern_info["patterns"]:
                if pattern in primary_lower:
                    return Diagnosis(
                        error_class=pattern_info["class"],
                        severity=pattern_info["severity"],
                        primary_message=primary,
                        language=language,
                        all_messages=list(errors),
                    )

        # Check remaining errors if primary didn't match
        for error_msg in errors[1:]:
            error_lower = error_msg.lower()
            for pattern_info in self._patterns:
                for pattern in pattern_info["patterns"]:
                    if pattern in error_lower:
                        return Diagnosis(
                            error_class=pattern_info["class"],
                            severity=pattern_info["severity"],
                            primary_message=error_msg,
                            language=language,
                            all_messages=list(errors),
                        )

        return Diagnosis(
            error_class="unknown",
            severity="critical",
            primary_message=primary,
            language=language,
            all_messages=list(errors),
        )

    def classify_tuple(
        self, language: str, errors: list[str]
    ) -> tuple[str, str]:
        """
        Classify and return (error_class, primary_message) tuple.

        Compatible with FeedbackLoop.ErrorClassifier callback type.
        """
        diag = self.classify(language, errors)
        return (diag.error_class, diag.primary_message)
