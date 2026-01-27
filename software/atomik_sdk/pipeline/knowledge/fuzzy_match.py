"""
Fuzzy Error Matching

Matches error messages against known patterns using edit distance
and token overlap. Returns the best-matching pattern above a
configurable similarity threshold.
"""

from __future__ import annotations


def edit_distance(a: str, b: str) -> int:
    """Compute Levenshtein edit distance between two strings."""
    if len(a) < len(b):
        return edit_distance(b, a)

    if len(b) == 0:
        return len(a)

    previous_row = list(range(len(b) + 1))
    for i, ca in enumerate(a):
        current_row = [i + 1]
        for j, cb in enumerate(b):
            cost = 0 if ca == cb else 1
            current_row.append(min(
                current_row[j] + 1,        # insert
                previous_row[j + 1] + 1,   # delete
                previous_row[j] + cost,     # replace
            ))
        previous_row = current_row

    return previous_row[-1]


def token_overlap(a: str, b: str) -> float:
    """
    Compute token overlap ratio between two strings.

    Tokenizes by whitespace and special characters, then computes
    Jaccard similarity.

    Returns:
        Overlap ratio between 0.0 and 1.0.
    """
    tokens_a = _tokenize(a)
    tokens_b = _tokenize(b)

    if not tokens_a or not tokens_b:
        return 0.0

    intersection = tokens_a & tokens_b
    union = tokens_a | tokens_b

    return len(intersection) / len(union) if union else 0.0


def fuzzy_score(query: str, candidate: str, max_edit_distance: int = 3) -> float:
    """
    Compute fuzzy match score between a query and candidate string.

    Combines edit distance (for short patterns) and token overlap
    (for longer error messages) into a single score.

    Args:
        query: The error message to match.
        candidate: The known pattern to compare against.
        max_edit_distance: Maximum edit distance for a match.

    Returns:
        Score between 0.0 (no match) and 1.0 (exact match).
    """
    query_lower = query.lower()
    candidate_lower = candidate.lower()

    # Exact substring match
    if candidate_lower in query_lower:
        return 1.0

    # For short patterns (< 20 chars), use edit distance
    if len(candidate_lower) < 20:
        dist = edit_distance(query_lower, candidate_lower)
        if dist <= max_edit_distance:
            max_len = max(len(query_lower), len(candidate_lower))
            return 1.0 - (dist / max_len) if max_len > 0 else 0.0

    # For longer patterns, use token overlap
    overlap = token_overlap(query_lower, candidate_lower)
    return overlap


def _tokenize(text: str) -> set[str]:
    """Tokenize text into a set of lowercase tokens."""
    import re
    tokens = re.findall(r'\b\w+\b', text.lower())
    return set(tokens)
