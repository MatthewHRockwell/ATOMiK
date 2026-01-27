"""
Field-Level Structural Diff

Traverses old and new schema as trees, producing a diff that
identifies exactly which fields, operations, constraints, or
metadata changed, with full JSON path tracking.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any


@dataclass
class FieldChange:
    """A single field-level change with path tracking."""
    path: str              # JSON path, e.g., "delta_fields.price_delta.data_type"
    change_type: str       # "added", "removed", "modified"
    old_value: Any = None
    new_value: Any = None
    section: str = ""      # Top-level section: "delta_fields", "operations", etc.

    def to_dict(self) -> dict[str, Any]:
        result: dict[str, Any] = {
            "path": self.path,
            "change_type": self.change_type,
            "section": self.section,
        }
        if self.change_type in ("removed", "modified"):
            result["old_value"] = self.old_value
        if self.change_type in ("added", "modified"):
            result["new_value"] = self.new_value
        return result


@dataclass
class FieldDiffResult:
    """Complete result of a field-level diff."""
    changes: list[FieldChange] = field(default_factory=list)
    sections_changed: set[str] = field(default_factory=set)
    is_identical: bool = True

    @property
    def change_count(self) -> int:
        return len(self.changes)

    def changes_in_section(self, section: str) -> list[FieldChange]:
        """Get changes within a specific top-level section."""
        return [c for c in self.changes if c.section == section]

    def to_dict(self) -> dict[str, Any]:
        return {
            "is_identical": self.is_identical,
            "change_count": self.change_count,
            "sections_changed": sorted(self.sections_changed),
            "changes": [c.to_dict() for c in self.changes],
        }


class FieldDiff:
    """
    Structural field-level diff engine for ATOMiK schemas.

    Traverses both schemas as trees and produces a diff with
    exact JSON paths for every changed field.

    Example:
        >>> diff = FieldDiff()
        >>> result = diff.compare(old_schema, new_schema)
        >>> for change in result.changes:
        ...     print(f"{change.path}: {change.change_type}")
    """

    # Top-level schema sections
    SECTIONS = [
        "catalogue",
        "delta_fields",
        "operations",
        "hardware",
        "constraints",
        "metadata",
    ]

    def compare(
        self,
        old_schema: dict[str, Any],
        new_schema: dict[str, Any],
    ) -> FieldDiffResult:
        """
        Compare two schemas and produce a field-level diff.

        Args:
            old_schema: Previous schema dict.
            new_schema: Current schema dict.

        Returns:
            FieldDiffResult with all changes and affected sections.
        """
        result = FieldDiffResult()

        self._diff_recursive(
            old_schema, new_schema, path="", section="",
            result=result,
        )

        result.is_identical = len(result.changes) == 0
        return result

    def _diff_recursive(
        self,
        old: Any,
        new: Any,
        path: str,
        section: str,
        result: FieldDiffResult,
    ) -> None:
        """Recursively diff two values with path tracking."""
        # Determine section from top-level path
        if not section and path:
            top = path.split(".")[0]
            if top in self.SECTIONS:
                section = top

        if isinstance(old, dict) and isinstance(new, dict):
            all_keys = set(old.keys()) | set(new.keys())
            for key in sorted(all_keys):
                child_path = f"{path}.{key}" if path else key
                child_section = section or (key if key in self.SECTIONS else "")

                if key not in old:
                    result.changes.append(FieldChange(
                        path=child_path,
                        change_type="added",
                        new_value=new[key],
                        section=child_section,
                    ))
                    result.sections_changed.add(child_section)
                elif key not in new:
                    result.changes.append(FieldChange(
                        path=child_path,
                        change_type="removed",
                        old_value=old[key],
                        section=child_section,
                    ))
                    result.sections_changed.add(child_section)
                else:
                    self._diff_recursive(
                        old[key], new[key], child_path, child_section, result
                    )

        elif isinstance(old, list) and isinstance(new, list):
            max_len = max(len(old), len(new))
            for i in range(max_len):
                child_path = f"{path}[{i}]"
                if i >= len(old):
                    result.changes.append(FieldChange(
                        path=child_path,
                        change_type="added",
                        new_value=new[i],
                        section=section,
                    ))
                    result.sections_changed.add(section)
                elif i >= len(new):
                    result.changes.append(FieldChange(
                        path=child_path,
                        change_type="removed",
                        old_value=old[i],
                        section=section,
                    ))
                    result.sections_changed.add(section)
                else:
                    self._diff_recursive(
                        old[i], new[i], child_path, section, result
                    )
        else:
            if old != new:
                result.changes.append(FieldChange(
                    path=path,
                    change_type="modified",
                    old_value=old,
                    new_value=new,
                    section=section,
                ))
                if section:
                    result.sections_changed.add(section)
