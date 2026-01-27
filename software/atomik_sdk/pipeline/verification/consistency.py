"""
Cross-Language Consistency Checker

Verifies that all language outputs implement the same interface by
extracting structural information and comparing across languages.
Detects field mismatches, missing operations, and type mapping
inconsistencies.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from .extractors.c_extractor import CExtractor
from .extractors.js_extractor import JavaScriptExtractor
from .extractors.python_extractor import PythonExtractor
from .extractors.rust_extractor import RustExtractor
from .extractors.verilog_extractor import VerilogExtractor
from .interfaces import LanguageInterface


@dataclass
class ConsistencyIssue:
    """A detected cross-language consistency issue."""
    issue_type: str     # "missing_field", "missing_operation", "type_mismatch", "naming_error"
    field_name: str
    languages_present: list[str] = field(default_factory=list)
    languages_missing: list[str] = field(default_factory=list)
    details: str = ""

    def to_dict(self) -> dict[str, Any]:
        return {
            "issue_type": self.issue_type,
            "field_name": self.field_name,
            "languages_present": self.languages_present,
            "languages_missing": self.languages_missing,
            "details": self.details,
        }


@dataclass
class ConsistencyReport:
    """Report of cross-language consistency check."""
    consistent: bool = True
    issues: list[ConsistencyIssue] = field(default_factory=list)
    interfaces: dict[str, LanguageInterface] = field(default_factory=dict)

    @property
    def issue_count(self) -> int:
        return len(self.issues)

    def to_dict(self) -> dict[str, Any]:
        return {
            "consistent": self.consistent,
            "issue_count": self.issue_count,
            "issues": [i.to_dict() for i in self.issues],
            "interfaces": {
                lang: iface.to_dict()
                for lang, iface in self.interfaces.items()
            },
        }


# Naming convention mappings (snake_case is canonical)
NAMING_CONVENTIONS = {
    "python": "snake_case",
    "rust": "snake_case",
    "c": "snake_case",
    "javascript": "camelCase",
    "verilog": "snake_case",
}


class ConsistencyChecker:
    """
    Cross-language consistency checker.

    Extracts interface definitions from each generated file and
    compares them to ensure all languages implement the same fields,
    operations, and type mappings.

    Example:
        >>> checker = ConsistencyChecker()
        >>> report = checker.check(files_by_language)
        >>> if not report.consistent:
        ...     for issue in report.issues:
        ...         print(f"{issue.issue_type}: {issue.field_name}")
    """

    def __init__(self) -> None:
        self._extractors = {
            "python": PythonExtractor(),
            "rust": RustExtractor(),
            "c": CExtractor(),
            "javascript": JavaScriptExtractor(),
            "verilog": VerilogExtractor(),
        }

    def check(
        self,
        files_by_language: dict[str, str],
    ) -> ConsistencyReport:
        """
        Check consistency across language outputs.

        Args:
            files_by_language: Map of language -> file path.

        Returns:
            ConsistencyReport with issues and extracted interfaces.
        """
        report = ConsistencyReport()

        # Extract interfaces
        for lang, filepath in files_by_language.items():
            extractor = self._extractors.get(lang)
            if extractor is None:
                continue
            try:
                iface = extractor.extract(filepath)
                iface.language = lang
                iface.file_path = filepath
                report.interfaces[lang] = iface
            except Exception:
                continue

        if len(report.interfaces) < 2:
            return report

        # Check field consistency
        self._check_fields(report)

        # Check operation consistency
        self._check_operations(report)

        # Check type mapping consistency
        self._check_type_mappings(report)

        return report

    def check_from_interfaces(
        self,
        interfaces: dict[str, LanguageInterface],
    ) -> ConsistencyReport:
        """Check consistency from pre-extracted interfaces."""
        report = ConsistencyReport()
        report.interfaces = dict(interfaces)

        if len(interfaces) < 2:
            return report

        self._check_fields(report)
        self._check_operations(report)
        self._check_type_mappings(report)

        return report

    def _check_fields(self, report: ConsistencyReport) -> None:
        """Check that all languages have the same fields."""
        # Collect normalized field names per language
        lang_fields: dict[str, set[str]] = {}
        for lang, iface in report.interfaces.items():
            lang_fields[lang] = {
                _normalize_to_snake(f.name) for f in iface.fields
            }

        # Union of all fields
        all_fields = set()
        for fields in lang_fields.values():
            all_fields |= fields

        for field_name in sorted(all_fields):
            present_in = [
                lang for lang, fields in lang_fields.items()
                if field_name in fields
            ]
            missing_in = [
                lang for lang in lang_fields
                if lang not in present_in
            ]

            if missing_in:
                report.consistent = False
                report.issues.append(ConsistencyIssue(
                    issue_type="missing_field",
                    field_name=field_name,
                    languages_present=present_in,
                    languages_missing=missing_in,
                    details=f"Field '{field_name}' missing in: {', '.join(missing_in)}",
                ))

    def _check_operations(self, report: ConsistencyReport) -> None:
        """Check that all languages have the same operations."""
        lang_ops: dict[str, set[str]] = {}
        for lang, iface in report.interfaces.items():
            lang_ops[lang] = {
                _normalize_to_snake(op.name) for op in iface.operations
            }

        all_ops = set()
        for ops in lang_ops.values():
            all_ops |= ops

        for op_name in sorted(all_ops):
            present_in = [
                lang for lang, ops in lang_ops.items()
                if op_name in ops
            ]
            missing_in = [
                lang for lang in lang_ops
                if lang not in present_in
            ]

            if missing_in:
                report.consistent = False
                report.issues.append(ConsistencyIssue(
                    issue_type="missing_operation",
                    field_name=op_name,
                    languages_present=present_in,
                    languages_missing=missing_in,
                    details=f"Operation '{op_name}' missing in: {', '.join(missing_in)}",
                ))

    def _check_type_mappings(self, report: ConsistencyReport) -> None:
        """Check for type mapping inconsistencies across languages."""
        # Collect field bit widths per normalized name
        field_widths: dict[str, dict[str, int]] = {}

        for lang, iface in report.interfaces.items():
            for f in iface.fields:
                norm_name = _normalize_to_snake(f.name)
                if f.bit_width > 0:
                    if norm_name not in field_widths:
                        field_widths[norm_name] = {}
                    field_widths[norm_name][lang] = f.bit_width

        for field_name, widths in field_widths.items():
            unique_widths = set(widths.values())
            if len(unique_widths) > 1:
                report.consistent = False
                width_details = ", ".join(
                    f"{lang}={w}bit" for lang, w in sorted(widths.items())
                )
                report.issues.append(ConsistencyIssue(
                    issue_type="type_mismatch",
                    field_name=field_name,
                    languages_present=list(widths.keys()),
                    details=f"Bit width mismatch for '{field_name}': {width_details}",
                ))


def _normalize_to_snake(name: str) -> str:
    """Normalize a name to snake_case for comparison."""
    result = []
    for i, ch in enumerate(name):
        if ch.isupper() and i > 0 and not name[i - 1].isupper():
            result.append("_")
        result.append(ch.lower())
    return "".join(result).replace("-", "_")
