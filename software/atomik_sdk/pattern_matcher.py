"""
Pattern Matcher Module - Mid-level API for motif vocabulary matching.

This module provides the PatternMatcher class for matching delta words
against a configurable vocabulary of patterns, enabling custom motion
detection and classification schemes.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple, Union
import json
from pathlib import Path


@dataclass
class MatchResult:
    """Result of a pattern matching operation."""
    pattern_id: int
    pattern_name: str
    confidence: float
    matched_bits: int
    total_bits: int


@dataclass
class Pattern:
    """A pattern definition for matching."""
    id: int
    name: str
    signature: int
    mask: int = 0xFFFFFFFFFFFFFFFF
    min_confidence: float = 0.7
    
    def matches(self, delta: int) -> Optional[MatchResult]:
        """Check if delta matches this pattern."""
        masked_delta = delta & self.mask
        masked_sig = self.signature & self.mask
        
        matching_bits = bin(~(masked_delta ^ masked_sig) & self.mask).count('1')
        total_bits = bin(self.mask).count('1')
        
        confidence = matching_bits / total_bits if total_bits > 0 else 0.0
        
        if confidence >= self.min_confidence:
            return MatchResult(
                pattern_id=self.id,
                pattern_name=self.name,
                confidence=confidence,
                matched_bits=matching_bits,
                total_bits=total_bits
            )
        return None


class PatternMatcher:
    """
    Matcher for classifying delta words against a pattern vocabulary.
    
    The PatternMatcher allows custom definition of patterns for domain-specific
    motion detection. Patterns are matched using bitwise comparison with
    configurable masks and confidence thresholds.
    
    Attributes:
        patterns: Dictionary of pattern ID to Pattern objects.
        default_pattern: Pattern returned when no match is found.
    
    Example:
        >>> matcher = PatternMatcher(vocab=["0xFFFF000000000000"])
        >>> for delta in deltas:
        ...     result = matcher.match(delta)
        ...     if result:
        ...         print(f"Matched: {result.pattern_name}")
    """
    
    def __init__(
        self,
        vocab: Optional[List[Union[str, int, Pattern]]] = None,
        default_pattern_name: str = "unknown"
    ):
        """
        Initialize the PatternMatcher.
        
        Args:
            vocab: List of patterns as hex strings, integers, or Pattern objects.
            default_pattern_name: Name for unmatched patterns.
        """
        self.patterns: Dict[int, Pattern] = {}
        self.default_pattern = Pattern(
            id=-1,
            name=default_pattern_name,
            signature=0,
            min_confidence=0.0
        )
        
        if vocab:
            for i, item in enumerate(vocab):
                self.add_pattern(item, pattern_id=i)
    
    def add_pattern(
        self,
        pattern: Union[str, int, Pattern],
        pattern_id: Optional[int] = None,
        name: Optional[str] = None,
        mask: int = 0xFFFFFFFFFFFFFFFF,
        min_confidence: float = 0.7
    ) -> int:
        """
        Add a pattern to the vocabulary.
        
        Args:
            pattern: Hex string, integer, or Pattern object.
            pattern_id: Optional ID (auto-assigned if not provided).
            name: Optional pattern name.
            mask: Bit mask for matching.
            min_confidence: Minimum confidence for a match.
        
        Returns:
            The assigned pattern ID.
        """
        if pattern_id is None:
            pattern_id = len(self.patterns)
        
        if isinstance(pattern, Pattern):
            self.patterns[pattern_id] = pattern
        elif isinstance(pattern, str):
            signature = int(pattern, 16) if pattern.startswith('0x') else int(pattern, 16)
            self.patterns[pattern_id] = Pattern(
                id=pattern_id,
                name=name or f"pattern_{pattern_id}",
                signature=signature,
                mask=mask,
                min_confidence=min_confidence
            )
        else:
            self.patterns[pattern_id] = Pattern(
                id=pattern_id,
                name=name or f"pattern_{pattern_id}",
                signature=int(pattern),
                mask=mask,
                min_confidence=min_confidence
            )
        
        return pattern_id
    
    def match(self, delta: int) -> Optional[MatchResult]:
        """
        Match a delta word against the vocabulary.
        
        Args:
            delta: The 64-bit delta word to match.
        
        Returns:
            MatchResult if a pattern matches, None otherwise.
        """
        best_match: Optional[MatchResult] = None
        best_confidence = 0.0
        
        for pattern in self.patterns.values():
            result = pattern.matches(delta)
            if result and result.confidence > best_confidence:
                best_match = result
                best_confidence = result.confidence
        
        return best_match
    
    def match_all(self, delta: int) -> List[MatchResult]:
        """
        Find all patterns matching the delta word.
        
        Args:
            delta: The 64-bit delta word to match.
        
        Returns:
            List of all matching MatchResults, sorted by confidence.
        """
        results = []
        
        for pattern in self.patterns.values():
            result = pattern.matches(delta)
            if result:
                results.append(result)
        
        return sorted(results, key=lambda r: r.confidence, reverse=True)
    
    def load_vocab(self, path: Union[str, Path]) -> None:
        """
        Load pattern vocabulary from a JSON file.
        
        Args:
            path: Path to the vocabulary JSON file.
        
        Expected format:
            {
                "patterns": [
                    {"id": 0, "name": "horizontal", "signature": "0xFF00...", "mask": "0xFF...", "min_confidence": 0.8},
                    ...
                ]
            }
        """
        with open(path) as f:
            data = json.load(f)
        
        for p in data.get("patterns", []):
            self.add_pattern(
                pattern=p["signature"],
                pattern_id=p.get("id"),
                name=p.get("name"),
                mask=int(p.get("mask", "0xFFFFFFFFFFFFFFFF"), 16) if isinstance(p.get("mask"), str) else p.get("mask", 0xFFFFFFFFFFFFFFFF),
                min_confidence=p.get("min_confidence", 0.7)
            )
    
    def save_vocab(self, path: Union[str, Path]) -> None:
        """
        Save pattern vocabulary to a JSON file.
        
        Args:
            path: Path for the output JSON file.
        """
        data = {
            "patterns": [
                {
                    "id": p.id,
                    "name": p.name,
                    "signature": f"0x{p.signature:016X}",
                    "mask": f"0x{p.mask:016X}",
                    "min_confidence": p.min_confidence
                }
                for p in self.patterns.values()
            ]
        }
        
        with open(path, 'w') as f:
            json.dump(data, f, indent=2)
    
    @classmethod
    def default_motion_vocab(cls) -> PatternMatcher:
        """
        Create a matcher with the default motion detection vocabulary.
        
        Returns:
            PatternMatcher configured for general motion detection.
        """
        matcher = cls()
        
        matcher.add_pattern(0xFF00000000000000, name="right_motion", min_confidence=0.6)
        matcher.add_pattern(0x00000000000000FF, name="left_motion", min_confidence=0.6)
        matcher.add_pattern(0x0101010101010101, name="down_motion", min_confidence=0.6)
        matcher.add_pattern(0x8080808080808080, name="up_motion", min_confidence=0.6)
        matcher.add_pattern(0x183C7EFFFF7E3C18, name="expansion", min_confidence=0.5)
        matcher.add_pattern(0xE7C38100008143E7, name="contraction", min_confidence=0.5)
        
        return matcher
