"""
Error Pattern Knowledge Base

Persistent, versioned knowledge base of error patterns and their
proven fixes. Supports CRUD operations, fuzzy matching, and
auto-learning from successful LLM-diagnosed fixes.
"""

from __future__ import annotations

import json
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from .fuzzy_match import fuzzy_score


KB_SCHEMA_VERSION = "1.0"


@dataclass
class ErrorPattern:
    """A known error pattern with its fix."""
    pattern_id: str
    language: str             # "" for any language
    error_class: str
    signature: str            # Regex/substring signature for matching
    fix_template: str         # Description of the fix to apply
    fix_type: str             # "append_semicolon", "add_import", etc.
    success_count: int = 0
    failure_count: int = 0
    source: str = "seed"      # "seed" or "learned"
    created_at: str = ""
    last_matched: str = ""

    @property
    def confidence(self) -> float:
        total = self.success_count + self.failure_count
        if total == 0:
            return 0.5
        return self.success_count / total

    def to_dict(self) -> dict[str, Any]:
        return {
            "pattern_id": self.pattern_id,
            "language": self.language,
            "error_class": self.error_class,
            "signature": self.signature,
            "fix_template": self.fix_template,
            "fix_type": self.fix_type,
            "success_count": self.success_count,
            "failure_count": self.failure_count,
            "confidence": round(self.confidence, 3),
            "source": self.source,
            "created_at": self.created_at,
            "last_matched": self.last_matched,
        }

    @staticmethod
    def from_dict(data: dict[str, Any]) -> ErrorPattern:
        return ErrorPattern(
            pattern_id=data["pattern_id"],
            language=data.get("language", ""),
            error_class=data["error_class"],
            signature=data["signature"],
            fix_template=data["fix_template"],
            fix_type=data["fix_type"],
            success_count=data.get("success_count", 0),
            failure_count=data.get("failure_count", 0),
            source=data.get("source", "seed"),
            created_at=data.get("created_at", ""),
            last_matched=data.get("last_matched", ""),
        )


@dataclass
class KBLookupResult:
    """Result of a knowledge base lookup."""
    found: bool
    pattern: ErrorPattern | None = None
    match_score: float = 0.0
    match_type: str = ""  # "exact", "fuzzy", "none"

    def to_dict(self) -> dict[str, Any]:
        return {
            "found": self.found,
            "pattern_id": self.pattern.pattern_id if self.pattern else None,
            "match_score": round(self.match_score, 3),
            "match_type": self.match_type,
        }


class ErrorKnowledgeBase:
    """
    Persistent error pattern knowledge base.

    Stores known error patterns with fix templates, supports fuzzy
    matching for near-miss patterns, and auto-learns from successful
    LLM-diagnosed fixes.

    Example:
        >>> kb = ErrorKnowledgeBase()
        >>> kb.load_seed()
        >>> result = kb.lookup("python", "import_error", "ModuleNotFoundError: atomik")
        >>> if result.found:
        ...     print(f"Fix: {result.pattern.fix_template}")
    """

    def __init__(
        self,
        min_confidence: float = 0.3,
        fuzzy_threshold: float = 0.6,
    ) -> None:
        self._patterns: dict[str, ErrorPattern] = {}
        self._min_confidence = min_confidence
        self._fuzzy_threshold = fuzzy_threshold

    def add_pattern(self, pattern: ErrorPattern) -> None:
        """Add a pattern to the knowledge base."""
        if not pattern.created_at:
            pattern.created_at = time.strftime(
                "%Y-%m-%dT%H:%M:%SZ", time.gmtime()
            )
        self._patterns[pattern.pattern_id] = pattern

    def remove_pattern(self, pattern_id: str) -> bool:
        """Remove a pattern by ID. Returns True if found and removed."""
        return self._patterns.pop(pattern_id, None) is not None

    def get_pattern(self, pattern_id: str) -> ErrorPattern | None:
        """Get a pattern by ID."""
        return self._patterns.get(pattern_id)

    def get_all_patterns(self) -> list[ErrorPattern]:
        """Get all patterns."""
        return list(self._patterns.values())

    def lookup(
        self,
        language: str,
        error_class: str,
        error_message: str,
    ) -> KBLookupResult:
        """
        Look up a known fix for an error.

        First attempts exact substring match, then fuzzy match.
        Filters by language and error class, then scores by signature.

        Args:
            language: Language of the failing code.
            error_class: Classified error type.
            error_message: The actual error message.

        Returns:
            KBLookupResult with the best matching pattern.
        """
        candidates = [
            p for p in self._patterns.values()
            if (not p.language or p.language == language)
            and p.confidence >= self._min_confidence
        ]

        if not candidates:
            return KBLookupResult(found=False, match_type="none")

        # Score candidates
        best: ErrorPattern | None = None
        best_score = 0.0
        best_type = "none"

        for pattern in candidates:
            # Exact class + signature match
            if pattern.error_class == error_class:
                sig_lower = pattern.signature.lower()
                msg_lower = error_message.lower()
                if sig_lower in msg_lower:
                    score = 1.0
                    match_type = "exact"
                else:
                    score = fuzzy_score(error_message, pattern.signature)
                    match_type = "fuzzy"

                if score > best_score:
                    best_score = score
                    best = pattern
                    best_type = match_type

            # Cross-class fuzzy match
            else:
                score = fuzzy_score(error_message, pattern.signature) * 0.7
                if score > best_score:
                    best_score = score
                    best = pattern
                    best_type = "fuzzy"

        if best and best_score >= self._fuzzy_threshold:
            best.last_matched = time.strftime(
                "%Y-%m-%dT%H:%M:%SZ", time.gmtime()
            )
            return KBLookupResult(
                found=True,
                pattern=best,
                match_score=best_score,
                match_type=best_type,
            )

        return KBLookupResult(found=False, match_type="none", match_score=best_score)

    def record_success(self, pattern_id: str) -> None:
        """Record a successful fix application."""
        pattern = self._patterns.get(pattern_id)
        if pattern:
            pattern.success_count += 1

    def record_failure(self, pattern_id: str) -> None:
        """Record a failed fix application."""
        pattern = self._patterns.get(pattern_id)
        if pattern:
            pattern.failure_count += 1

    def learn(
        self,
        language: str,
        error_class: str,
        error_message: str,
        fix_description: str,
        fix_type: str = "learned",
    ) -> ErrorPattern:
        """
        Learn a new pattern from a successful LLM-diagnosed fix.

        Creates a new pattern entry with the error signature and fix.
        """
        pattern_id = f"learned_{len(self._patterns)}_{int(time.time())}"
        pattern = ErrorPattern(
            pattern_id=pattern_id,
            language=language,
            error_class=error_class,
            signature=error_message[:200],  # Truncate long messages
            fix_template=fix_description,
            fix_type=fix_type,
            success_count=1,  # Already succeeded
            source="learned",
        )
        self.add_pattern(pattern)
        return pattern

    def load_seed(self, seed_path: str | Path | None = None) -> int:
        """
        Load seed patterns from JSON file.

        Returns number of patterns loaded.
        """
        if seed_path is None:
            seed_path = Path(__file__).parent / "error_patterns.json"

        path = Path(seed_path)
        if not path.exists():
            return 0

        with open(path, encoding="utf-8") as f:
            data = json.load(f)

        patterns = data.get("patterns", [])
        for p_data in patterns:
            pattern = ErrorPattern.from_dict(p_data)
            self.add_pattern(pattern)

        return len(patterns)

    def save(self, path: str | Path) -> None:
        """Save knowledge base to JSON file."""
        path = Path(path)
        path.parent.mkdir(parents=True, exist_ok=True)

        data = {
            "version": KB_SCHEMA_VERSION,
            "saved_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "pattern_count": len(self._patterns),
            "patterns": [p.to_dict() for p in self._patterns.values()],
        }

        with open(path, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2)

    def load(self, path: str | Path) -> int:
        """Load knowledge base from JSON file. Returns pattern count."""
        path = Path(path)
        if not path.exists():
            return 0

        with open(path, encoding="utf-8") as f:
            data = json.load(f)

        self._patterns.clear()
        for p_data in data.get("patterns", []):
            pattern = ErrorPattern.from_dict(p_data)
            self._patterns[pattern.pattern_id] = pattern

        return len(self._patterns)

    def summary(self) -> dict[str, Any]:
        """Get knowledge base summary."""
        patterns = list(self._patterns.values())
        seed = [p for p in patterns if p.source == "seed"]
        learned = [p for p in patterns if p.source == "learned"]

        return {
            "version": KB_SCHEMA_VERSION,
            "total_patterns": len(patterns),
            "seed_patterns": len(seed),
            "learned_patterns": len(learned),
            "avg_confidence": (
                sum(p.confidence for p in patterns) / len(patterns)
                if patterns else 0.0
            ),
            "total_matches": sum(
                p.success_count + p.failure_count for p in patterns
            ),
        }
