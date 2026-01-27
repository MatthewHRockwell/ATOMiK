"""
Feedback Loop Engine

Implements the generate-verify-diagnose-fix-retry cycle with
configurable depth. Each iteration classifies the error, consults
the knowledge base for known fixes, applies the fix (or invokes
LLM diagnosis), and re-verifies.
"""

from __future__ import annotations

import time
from dataclasses import dataclass, field
from enum import Enum
from typing import Any, Callable

from .event_bus import Event, EventBus, EventType


class FeedbackOutcome(Enum):
    """Outcome of a feedback loop iteration."""
    FIXED_BY_KB = "fixed_by_kb"        # Known fix applied from knowledge base
    FIXED_BY_LLM = "fixed_by_llm"      # LLM-diagnosed fix applied
    RETRY_EXHAUSTED = "retry_exhausted"  # Max retries reached
    ESCALATED = "escalated"             # Escalated to higher tier / human
    IDENTICAL_ERROR = "identical_error"  # Same error repeated -- immediate escalation


@dataclass
class FeedbackIteration:
    """Record of a single feedback loop iteration."""
    iteration: int
    error_class: str
    error_message: str
    fix_source: str = ""        # "kb", "llm", or ""
    fix_applied: bool = False
    fix_description: str = ""
    tokens_consumed: int = 0
    re_verify_passed: bool = False
    duration_ms: float = 0.0

    def to_dict(self) -> dict[str, Any]:
        return {
            "iteration": self.iteration,
            "error_class": self.error_class,
            "error_message": self.error_message,
            "fix_source": self.fix_source,
            "fix_applied": self.fix_applied,
            "fix_description": self.fix_description,
            "tokens_consumed": self.tokens_consumed,
            "re_verify_passed": self.re_verify_passed,
            "duration_ms": round(self.duration_ms, 1),
        }


@dataclass
class FeedbackResult:
    """Result of a complete feedback loop execution."""
    outcome: FeedbackOutcome
    iterations: list[FeedbackIteration] = field(default_factory=list)
    total_tokens: int = 0
    final_errors: list[str] = field(default_factory=list)

    @property
    def resolved(self) -> bool:
        return self.outcome in (FeedbackOutcome.FIXED_BY_KB, FeedbackOutcome.FIXED_BY_LLM)

    def to_dict(self) -> dict[str, Any]:
        return {
            "outcome": self.outcome.value,
            "resolved": self.resolved,
            "iterations": [it.to_dict() for it in self.iterations],
            "total_tokens": self.total_tokens,
            "final_errors": self.final_errors,
        }


# Type aliases for pluggable callbacks
ErrorClassifier = Callable[[str, list[str]], tuple[str, str]]
"""(language, errors) -> (error_class, primary_error_message)"""

KBLookup = Callable[[str, str, str], tuple[bool, str]]
"""(language, error_class, error_msg) -> (found, fix_description)"""

FixApplier = Callable[[str, str, str], bool]
"""(language, error_class, fix_description) -> success"""

LLMDiagnoser = Callable[[str, str, list[str]], tuple[str, int]]
"""(language, error_class, errors) -> (fix_description, tokens_consumed)"""

Verifier = Callable[[str], tuple[bool, list[str]]]
"""(language) -> (passed, errors)"""

KBRecorder = Callable[[str, str, str, str], None]
"""(language, error_class, error_msg, fix_description) -> None"""


class FeedbackLoop:
    """
    Feedback loop engine for self-correcting pipeline failures.

    Wraps a verify-diagnose-fix cycle with configurable max depth.
    On each iteration:
    1. Classify the error
    2. Check the knowledge base for a known fix
    3. If known: apply fix (0 tokens), re-verify
    4. If unknown: invoke LLM diagnosis, apply fix, re-verify
    5. If fix succeeds: record in KB for future use
    6. If identical error repeats: escalate immediately

    Example:
        >>> loop = FeedbackLoop(max_depth=3)
        >>> result = loop.run("python", errors, classifier, kb_lookup, ...)
        >>> if result.resolved:
        ...     print(f"Fixed in {len(result.iterations)} iterations")
    """

    def __init__(
        self,
        max_depth: int = 3,
        event_bus: EventBus | None = None,
    ) -> None:
        self.max_depth = max_depth
        self.event_bus = event_bus

    def run(
        self,
        language: str,
        initial_errors: list[str],
        classify: ErrorClassifier,
        kb_lookup: KBLookup,
        apply_fix: FixApplier,
        llm_diagnose: LLMDiagnoser,
        verify: Verifier,
        kb_record: KBRecorder | None = None,
    ) -> FeedbackResult:
        """
        Execute the feedback loop.

        Args:
            language: Language of the failing code.
            initial_errors: Initial error messages from verification.
            classify: Callback to classify errors.
            kb_lookup: Callback to look up known fixes.
            apply_fix: Callback to apply a fix.
            llm_diagnose: Callback to invoke LLM diagnosis.
            verify: Callback to re-verify after fix.
            kb_record: Optional callback to record new patterns in KB.

        Returns:
            FeedbackResult with outcome and iteration details.
        """
        result = FeedbackResult(outcome=FeedbackOutcome.RETRY_EXHAUSTED)
        errors = list(initial_errors)
        prev_error_class = ""

        for i in range(self.max_depth):
            start = time.perf_counter()
            error_class, primary_error = classify(language, errors)
            iteration = FeedbackIteration(
                iteration=i + 1,
                error_class=error_class,
                error_message=primary_error,
            )

            if self.event_bus:
                self.event_bus.emit(Event(
                    EventType.FEEDBACK_START,
                    {
                        "language": language,
                        "error_class": error_class,
                        "retry_number": i + 1,
                        "max_retries": self.max_depth,
                    },
                    source="feedback_loop",
                ))

            # Identical error detection -- prevent oscillation
            if error_class == prev_error_class and i > 0:
                iteration.duration_ms = (time.perf_counter() - start) * 1000
                result.iterations.append(iteration)
                result.outcome = FeedbackOutcome.IDENTICAL_ERROR
                result.final_errors = errors
                break

            prev_error_class = error_class

            # Step 1: Check knowledge base
            kb_found, fix_desc = kb_lookup(language, error_class, primary_error)

            if kb_found:
                iteration.fix_source = "kb"
                iteration.fix_description = fix_desc
                success = apply_fix(language, error_class, fix_desc)
                iteration.fix_applied = success

                if success:
                    passed, new_errors = verify(language)
                    iteration.re_verify_passed = passed
                    iteration.duration_ms = (time.perf_counter() - start) * 1000
                    result.iterations.append(iteration)

                    if self.event_bus:
                        self.event_bus.emit(Event(
                            EventType.FEEDBACK_RESULT,
                            {
                                "fix_source": "kb",
                                "success": passed,
                                "iteration": i + 1,
                            },
                            source="feedback_loop",
                        ))

                    if passed:
                        result.outcome = FeedbackOutcome.FIXED_BY_KB
                        result.total_tokens = sum(
                            it.tokens_consumed for it in result.iterations
                        )
                        return result
                    errors = new_errors
                    continue

            # Step 2: LLM diagnosis
            fix_desc, tokens = llm_diagnose(language, error_class, errors)
            iteration.fix_source = "llm"
            iteration.fix_description = fix_desc
            iteration.tokens_consumed = tokens

            success = apply_fix(language, error_class, fix_desc)
            iteration.fix_applied = success

            if success:
                passed, new_errors = verify(language)
                iteration.re_verify_passed = passed
                iteration.duration_ms = (time.perf_counter() - start) * 1000
                result.iterations.append(iteration)

                if self.event_bus:
                    self.event_bus.emit(Event(
                        EventType.FEEDBACK_RESULT,
                        {
                            "fix_source": "llm",
                            "success": passed,
                            "iteration": i + 1,
                        },
                        source="feedback_loop",
                    ))

                if passed:
                    result.outcome = FeedbackOutcome.FIXED_BY_LLM
                    # Record successful LLM fix in KB for future use
                    if kb_record:
                        kb_record(language, error_class, primary_error, fix_desc)
                    result.total_tokens = sum(
                        it.tokens_consumed for it in result.iterations
                    )
                    return result
                errors = new_errors
            else:
                iteration.duration_ms = (time.perf_counter() - start) * 1000
                result.iterations.append(iteration)

        result.final_errors = errors
        result.total_tokens = sum(it.tokens_consumed for it in result.iterations)
        return result
