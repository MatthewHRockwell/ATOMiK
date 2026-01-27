"""
Consensus Resolution

Resolves conflicts when multiple specialist agents produce inconsistent
outputs for overlapping artifacts. Implements majority voting, field-level
merge, and escalation for irreconcilable conflicts.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any


@dataclass
class ConflictItem:
    """A single detected conflict between specialist outputs."""
    field_path: str
    values: dict[str, Any]   # agent_name -> value
    resolution: str = ""      # "majority", "merged", "escalated"
    resolved_value: Any = None

    def to_dict(self) -> dict[str, Any]:
        return {
            "field_path": self.field_path,
            "values": self.values,
            "resolution": self.resolution,
            "resolved_value": self.resolved_value,
        }


@dataclass
class ConsensusResult:
    """Result of consensus resolution across specialist outputs."""
    agreed: bool = True
    conflicts: list[ConflictItem] = field(default_factory=list)
    merged_output: dict[str, Any] = field(default_factory=dict)
    escalated: list[str] = field(default_factory=list)

    @property
    def conflict_count(self) -> int:
        return len(self.conflicts)

    def to_dict(self) -> dict[str, Any]:
        return {
            "agreed": self.agreed,
            "conflict_count": self.conflict_count,
            "conflicts": [c.to_dict() for c in self.conflicts],
            "escalated_fields": self.escalated,
        }


class ConsensusResolver:
    """
    Resolves conflicts between specialist agent outputs.

    When multiple specialists contribute to overlapping artifacts
    (e.g., interface definitions across languages), the resolver
    detects inconsistencies and applies resolution strategies:

    1. Majority vote: If 3+ agents agree and 1 disagrees, majority wins.
    2. Field merge: Non-conflicting fields merged from all agents.
    3. Escalation: Irreconcilable conflicts flagged for human review.

    Example:
        >>> resolver = ConsensusResolver()
        >>> result = resolver.resolve({"agent_a": output_a, "agent_b": output_b})
        >>> if not result.agreed:
        ...     print(f"{result.conflict_count} conflicts found")
    """

    def resolve(
        self,
        agent_outputs: dict[str, dict[str, Any]],
    ) -> ConsensusResult:
        """
        Resolve conflicts across agent outputs.

        Args:
            agent_outputs: Map of agent_name -> output dict.

        Returns:
            ConsensusResult with merged output and conflict details.
        """
        result = ConsensusResult()

        if len(agent_outputs) <= 1:
            if agent_outputs:
                result.merged_output = next(iter(agent_outputs.values()))
            return result

        # Collect all unique field paths across agents
        all_fields = self._collect_all_fields(agent_outputs)

        for field_path in sorted(all_fields):
            values: dict[str, Any] = {}
            for agent_name, output in agent_outputs.items():
                val = self._get_nested(output, field_path)
                if val is not None:
                    values[agent_name] = val

            if not values:
                continue

            unique_values = set(self._hashable(v) for v in values.values())

            if len(unique_values) == 1:
                # All agents agree
                self._set_nested(
                    result.merged_output,
                    field_path,
                    next(iter(values.values())),
                )
            else:
                # Conflict detected
                result.agreed = False
                conflict = ConflictItem(
                    field_path=field_path,
                    values=values,
                )

                resolved = self._try_majority_vote(values)
                if resolved is not None:
                    conflict.resolution = "majority"
                    conflict.resolved_value = resolved
                    self._set_nested(result.merged_output, field_path, resolved)
                else:
                    conflict.resolution = "escalated"
                    result.escalated.append(field_path)

                result.conflicts.append(conflict)

        return result

    def resolve_interface_fields(
        self,
        language_fields: dict[str, list[str]],
    ) -> ConsensusResult:
        """
        Check interface consistency across languages.

        Verifies that all languages implement the same set of fields
        (accounting for naming convention differences).

        Args:
            language_fields: Map of language -> list of field names.

        Returns:
            ConsensusResult with any missing field conflicts.
        """
        result = ConsensusResult()

        if len(language_fields) <= 1:
            return result

        # Normalize field names to snake_case for comparison
        normalized: dict[str, set[str]] = {}
        for lang, fields in language_fields.items():
            normalized[lang] = {self._normalize_name(f) for f in fields}

        # Find the union of all fields
        all_fields = set()
        for fields in normalized.values():
            all_fields |= fields

        # Check each language for missing fields
        for field_name in sorted(all_fields):
            missing_in: list[str] = []
            present_in: list[str] = []

            for lang, fields in normalized.items():
                if field_name in fields:
                    present_in.append(lang)
                else:
                    missing_in.append(lang)

            if missing_in:
                result.agreed = False
                conflict = ConflictItem(
                    field_path=field_name,
                    values={
                        lang: "present" if lang in present_in else "missing"
                        for lang in language_fields
                    },
                    resolution="escalated",
                )
                result.conflicts.append(conflict)
                result.escalated.append(
                    f"{field_name} missing in: {', '.join(missing_in)}"
                )

        return result

    def _collect_all_fields(
        self, agent_outputs: dict[str, dict[str, Any]]
    ) -> set[str]:
        """Collect all dotted field paths across agent outputs."""
        paths: set[str] = set()
        for output in agent_outputs.values():
            self._flatten_paths(output, "", paths)
        return paths

    def _flatten_paths(
        self,
        obj: Any,
        prefix: str,
        paths: set[str],
    ) -> None:
        """Recursively flatten a dict into dotted paths."""
        if isinstance(obj, dict):
            for key, val in obj.items():
                path = f"{prefix}.{key}" if prefix else key
                if isinstance(val, dict):
                    self._flatten_paths(val, path, paths)
                else:
                    paths.add(path)
        elif prefix:
            paths.add(prefix)

    def _get_nested(self, obj: dict[str, Any], path: str) -> Any:
        """Get a value from a nested dict using dotted path."""
        parts = path.split(".")
        current: Any = obj
        for part in parts:
            if isinstance(current, dict) and part in current:
                current = current[part]
            else:
                return None
        return current

    def _set_nested(self, obj: dict[str, Any], path: str, value: Any) -> None:
        """Set a value in a nested dict using dotted path."""
        parts = path.split(".")
        current = obj
        for part in parts[:-1]:
            if part not in current:
                current[part] = {}
            current = current[part]
        current[parts[-1]] = value

    def _try_majority_vote(self, values: dict[str, Any]) -> Any | None:
        """Try to resolve a conflict by majority vote."""
        if len(values) < 3:
            return None

        # Count occurrences of each value
        counts: dict[Any, int] = {}
        val_map: dict[Any, Any] = {}
        for val in values.values():
            key = self._hashable(val)
            counts[key] = counts.get(key, 0) + 1
            val_map[key] = val

        # Find the most common value
        max_count = max(counts.values())
        if max_count > len(values) / 2:
            for key, count in counts.items():
                if count == max_count:
                    return val_map[key]
        return None

    @staticmethod
    def _hashable(val: Any) -> Any:
        """Convert a value to a hashable representation."""
        if isinstance(val, dict):
            return tuple(sorted(val.items()))
        if isinstance(val, list):
            return tuple(val)
        return val

    @staticmethod
    def _normalize_name(name: str) -> str:
        """Normalize a field name to snake_case for comparison."""
        result = []
        for i, ch in enumerate(name):
            if ch.isupper() and i > 0:
                result.append("_")
            result.append(ch.lower())
        return "".join(result).replace("-", "_")
