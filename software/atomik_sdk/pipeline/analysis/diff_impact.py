"""
Diff Impact Mapper

Maps changed JSON paths to affected generators and verification
scopes. Enables minimal regeneration by identifying exactly which
outputs need updating.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from .field_diff import FieldDiffResult

# Section-to-generator impact mapping (from Phase 4C, refined for field-level)
SECTION_IMPACT: dict[str, set[str]] = {
    "delta_fields": {"python", "rust", "c", "javascript", "verilog"},
    "operations": {"python", "rust", "c", "javascript", "verilog"},
    "catalogue": {"python", "rust", "c", "javascript", "verilog"},
    "hardware": {"verilog"},
    "constraints": {"verilog"},
    "metadata": {"python", "javascript"},
}

# Sections that require re-synthesis when changed
RESYNTHESIS_SECTIONS = {"delta_fields", "operations", "hardware", "constraints", "catalogue"}


@dataclass
class ImpactAnalysis:
    """Analysis of which generators and scopes are affected by a diff."""
    affected_languages: set[str] = field(default_factory=set)
    requires_resynthesis: bool = False
    verification_scope: str = "full"  # "full", "partial", "metadata_only"
    change_summary: dict[str, int] = field(default_factory=dict)  # section -> change count

    def to_dict(self) -> dict[str, Any]:
        return {
            "affected_languages": sorted(self.affected_languages),
            "requires_resynthesis": self.requires_resynthesis,
            "verification_scope": self.verification_scope,
            "change_summary": self.change_summary,
        }


class DiffImpactMapper:
    """
    Maps field-level diff results to generator and verification impact.

    Determines which languages need regeneration and what level of
    verification is needed based on the specific fields that changed.

    Example:
        >>> mapper = DiffImpactMapper()
        >>> impact = mapper.analyze(diff_result)
        >>> print(f"Affected: {impact.affected_languages}")
        >>> print(f"Re-synthesize: {impact.requires_resynthesis}")
    """

    def __init__(
        self,
        section_impact: dict[str, set[str]] | None = None,
    ) -> None:
        self._impact = dict(SECTION_IMPACT)
        if section_impact:
            self._impact.update(section_impact)

    def analyze(self, diff_result: FieldDiffResult) -> ImpactAnalysis:
        """
        Analyze a field diff result and determine impact.

        Args:
            diff_result: Output from FieldDiff.compare().

        Returns:
            ImpactAnalysis with affected languages and scopes.
        """
        if diff_result.is_identical:
            return ImpactAnalysis(verification_scope="none")

        analysis = ImpactAnalysis()

        # Count changes per section
        for change in diff_result.changes:
            section = change.section
            if section:
                analysis.change_summary[section] = (
                    analysis.change_summary.get(section, 0) + 1
                )
                # Map section to affected languages
                affected = self._impact.get(section, set())
                analysis.affected_languages.update(affected)

        # Check if re-synthesis is needed
        analysis.requires_resynthesis = bool(
            diff_result.sections_changed & RESYNTHESIS_SECTIONS
        )

        # Determine verification scope
        if diff_result.sections_changed <= {"metadata"}:
            analysis.verification_scope = "metadata_only"
        elif diff_result.sections_changed <= {"metadata", "constraints"}:
            analysis.verification_scope = "partial"
        else:
            analysis.verification_scope = "full"

        return analysis
