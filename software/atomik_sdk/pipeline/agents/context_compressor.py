"""
Context Compressor

Progressive context compression under budget pressure.
Strips comments, truncates examples, and uses structured
deltas instead of full file contents.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Any


@dataclass
class CompressionResult:
    """Result of a context compression pass."""
    original_size: int
    compressed_size: int
    technique: str
    reduction_pct: float

    def to_dict(self) -> dict[str, Any]:
        return {
            "original_size": self.original_size,
            "compressed_size": self.compressed_size,
            "technique": self.technique,
            "reduction_pct": round(self.reduction_pct, 1),
        }


class ContextCompressor:
    """
    Progressive context compression for budget-constrained execution.

    Applies increasingly aggressive compression techniques as budget
    pressure rises:
    - Level 1 (>50%): Strip comments and blank lines
    - Level 2 (>70%): Truncate examples and long strings
    - Level 3 (>80%): Use structural summaries instead of full content
    - Level 4 (>90%): Minimal skeleton only

    Example:
        >>> compressor = ContextCompressor()
        >>> compressed = compressor.compress(content, budget_pressure=0.85)
        >>> assert len(compressed) < len(content)
    """

    def compress(
        self,
        content: str,
        budget_pressure: float = 0.0,
    ) -> str:
        """
        Compress content based on budget pressure.

        Args:
            content: Original content string.
            budget_pressure: Budget utilization (0.0 to 1.0).

        Returns:
            Compressed content string.
        """
        if budget_pressure < 0.5:
            return content

        result = content

        if budget_pressure >= 0.5:
            result = self._strip_comments(result)

        if budget_pressure >= 0.7:
            result = self._truncate_examples(result)

        if budget_pressure >= 0.8:
            result = self._compress_whitespace(result)

        if budget_pressure >= 0.9:
            result = self._minimal_skeleton(result)

        return result

    def compress_with_stats(
        self,
        content: str,
        budget_pressure: float = 0.0,
    ) -> tuple[str, CompressionResult]:
        """Compress and return statistics."""
        original_size = len(content)
        compressed = self.compress(content, budget_pressure)
        compressed_size = len(compressed)

        technique = self._technique_for_pressure(budget_pressure)
        reduction = (
            100.0 * (1.0 - compressed_size / original_size)
            if original_size > 0
            else 0.0
        )

        return compressed, CompressionResult(
            original_size=original_size,
            compressed_size=compressed_size,
            technique=technique,
            reduction_pct=reduction,
        )

    def compress_schema_context(
        self,
        schema: dict[str, Any],
        budget_pressure: float = 0.0,
    ) -> dict[str, Any]:
        """
        Compress a schema dict for context loading.

        Under high pressure, strips descriptions, examples, and
        metadata to leave only structural information.
        """
        if budget_pressure < 0.5:
            return schema

        import copy
        compressed = copy.deepcopy(schema)

        if budget_pressure >= 0.5:
            # Strip description fields
            self._strip_key_recursive(compressed, "description")

        if budget_pressure >= 0.7:
            # Strip examples
            self._strip_key_recursive(compressed, "examples")
            self._strip_key_recursive(compressed, "example")

        if budget_pressure >= 0.8:
            # Strip metadata
            if "metadata" in compressed:
                compressed["metadata"] = {"compressed": True}

        return compressed

    def _strip_comments(self, content: str) -> str:
        """Remove comment lines and docstrings."""
        lines = content.split("\n")
        result = []
        in_docstring = False

        for line in lines:
            stripped = line.strip()

            # Toggle docstring detection
            if '"""' in stripped or "'''" in stripped:
                count = stripped.count('"""') + stripped.count("'''")
                if count >= 2:
                    continue  # Single-line docstring
                in_docstring = not in_docstring
                continue

            if in_docstring:
                continue

            # Skip comment-only lines
            if stripped.startswith("#") or stripped.startswith("//"):
                continue

            # Keep non-empty lines
            if stripped:
                result.append(line)

        return "\n".join(result)

    def _truncate_examples(self, content: str) -> str:
        """Truncate long string literals and examples."""
        # Truncate long strings (>80 chars) to first 40 + "..."
        def truncate_match(m: re.Match) -> str:
            s = m.group(0)
            if len(s) > 80:
                return s[:40] + '..."'
            return s

        return re.sub(r'"[^"]{80,}"', truncate_match, content)

    def _compress_whitespace(self, content: str) -> str:
        """Collapse multiple blank lines to single."""
        return re.sub(r"\n{3,}", "\n\n", content)

    def _minimal_skeleton(self, content: str) -> str:
        """Extract only structural elements (class/function signatures)."""
        lines = content.split("\n")
        result = []
        for line in lines:
            stripped = line.strip()
            if any(stripped.startswith(kw) for kw in (
                "class ", "def ", "fn ", "struct ", "enum ",
                "module ", "interface ", "function ", "pub ",
                "import ", "from ", "use ", "#include",
            )):
                result.append(line)
        return "\n".join(result) if result else content[:200]

    def _technique_for_pressure(self, pressure: float) -> str:
        """Return the technique name for a given pressure level."""
        if pressure >= 0.9:
            return "minimal_skeleton"
        elif pressure >= 0.8:
            return "structural_summary"
        elif pressure >= 0.7:
            return "truncate_examples"
        elif pressure >= 0.5:
            return "strip_comments"
        return "none"

    def _strip_key_recursive(self, obj: Any, key: str) -> None:
        """Recursively remove a key from nested dicts."""
        if isinstance(obj, dict):
            obj.pop(key, None)
            for v in obj.values():
                self._strip_key_recursive(v, key)
        elif isinstance(obj, list):
            for v in obj:
                self._strip_key_recursive(v, key)
