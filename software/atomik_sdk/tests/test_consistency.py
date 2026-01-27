"""Tests for cross-language consistency checker."""

import pytest
from pipeline.verification.consistency import (
    ConsistencyChecker,
    ConsistencyReport,
    _normalize_to_snake,
)
from pipeline.verification.interfaces import (
    LanguageInterface,
    InterfaceField,
    InterfaceOperation,
)


class TestNormalization:
    def test_camel_to_snake(self):
        assert _normalize_to_snake("priceDelta") == "price_delta"

    def test_pascal_to_snake(self):
        assert _normalize_to_snake("PriceDelta") == "price_delta"

    def test_snake_unchanged(self):
        assert _normalize_to_snake("price_delta") == "price_delta"

    def test_kebab_to_snake(self):
        assert _normalize_to_snake("price-delta") == "price_delta"


class TestConsistencyChecker:
    def _make_interface(self, language, fields, operations=None):
        return LanguageInterface(
            language=language,
            fields=[InterfaceField(name=n) for n in fields],
            operations=[InterfaceOperation(name=n) for n in (operations or [])],
        )

    def test_consistent_interfaces(self):
        checker = ConsistencyChecker()
        interfaces = {
            "python": self._make_interface("python", ["price_delta", "volume"]),
            "rust": self._make_interface("rust", ["price_delta", "volume"]),
        }
        report = checker.check_from_interfaces(interfaces)
        assert report.consistent
        assert report.issue_count == 0

    def test_missing_field(self):
        checker = ConsistencyChecker()
        interfaces = {
            "python": self._make_interface("python", ["price_delta", "volume", "timestamp"]),
            "rust": self._make_interface("rust", ["price_delta", "volume"]),
        }
        report = checker.check_from_interfaces(interfaces)
        assert not report.consistent
        missing = [i for i in report.issues if i.issue_type == "missing_field"]
        assert len(missing) >= 1
        assert "rust" in missing[0].languages_missing

    def test_missing_operation(self):
        checker = ConsistencyChecker()
        interfaces = {
            "python": self._make_interface("python", [], ["rollback", "apply"]),
            "rust": self._make_interface("rust", [], ["apply"]),
        }
        report = checker.check_from_interfaces(interfaces)
        assert not report.consistent
        missing_ops = [i for i in report.issues if i.issue_type == "missing_operation"]
        assert len(missing_ops) >= 1

    def test_type_mismatch(self):
        checker = ConsistencyChecker()
        interfaces = {
            "python": LanguageInterface(
                language="python",
                fields=[InterfaceField(name="value", bit_width=64)],
            ),
            "rust": LanguageInterface(
                language="rust",
                fields=[InterfaceField(name="value", bit_width=32)],
            ),
        }
        report = checker.check_from_interfaces(interfaces)
        assert not report.consistent
        type_issues = [i for i in report.issues if i.issue_type == "type_mismatch"]
        assert len(type_issues) >= 1

    def test_naming_convention_normalization(self):
        checker = ConsistencyChecker()
        interfaces = {
            "python": self._make_interface("python", ["price_delta"]),
            "javascript": self._make_interface("javascript", ["priceDelta"]),
        }
        report = checker.check_from_interfaces(interfaces)
        # Both normalize to "price_delta" so should be consistent
        assert report.consistent

    def test_single_language_passes(self):
        checker = ConsistencyChecker()
        interfaces = {
            "python": self._make_interface("python", ["field_a"]),
        }
        report = checker.check_from_interfaces(interfaces)
        assert report.consistent
