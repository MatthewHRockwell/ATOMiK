# ATOMiK v3 --- Phase Context (Living Document)

> **Purpose:** This file is the single source of truth carried between
> phases. It is **rewritten/curated at each phase boundary** to retain
> only current, relevant facts. Remove obsolete assumptions and
> superseded decisions.

------------------------------------------------------------------------

## 1. Phase Summary

-   **Current Phase:** Phase 1 --- Custom RV64I CPU Core
-   **Date Updated:** YYYY-MM-DD
-   **Status:** In Progress / Complete
-   **Exit Decision:** Go / No-Go / Conditional Go (with risks)

------------------------------------------------------------------------

## 2. Scope & Goals (Current Phase)

-   Brief, concrete statement of what this phase is responsible for
    delivering.
-   Explicit exclusions (what is *not* in scope for this phase).

------------------------------------------------------------------------

## 3. Acceptance Criteria (Contract)

-   List the measurable exit gates for this phase (tests, synthesis,
    metrics, etc.).
-   These should match the Phase Initiation Prompt exactly or note any
    approved changes.

------------------------------------------------------------------------

## 4. Architecture Snapshot (As-Built)

-   High-level block diagram description (text is fine; link to diagrams
    if available).
-   Key microarchitectural decisions that are **now implemented and
    verified**.
-   Important invariants (e.g., pipeline structure, FSM stages, memory
    latencies).

------------------------------------------------------------------------

## 5. Verified Assumptions

-   Assumptions that have been **tested and confirmed**.
-   Include how they were validated (test name, synthesis result,
    measurement, etc.).

------------------------------------------------------------------------

## 6. Open Risks & Watch Items

-   Items that are not yet fully proven or that could impact later
    phases.
-   For each:
    -   Description
    -   Why it matters
    -   Mitigation plan or trigger to revisit

------------------------------------------------------------------------

## 7. Key Tradeoffs & Rationale

-   Decisions where alternatives were considered and rejected.
-   Record:
    -   The chosen approach
    -   The main alternative(s)
    -   Why the chosen approach won

------------------------------------------------------------------------

## 8. Tooling & Infrastructure State

-   List tools/scripts/harnesses that are now part of the workflow:
    -   Simulation
    -   Coverage
    -   Compliance
    -   Synthesis
    -   Log parsing / debug helpers
-   Note any known limitations or planned improvements.

------------------------------------------------------------------------

## 9. Resource & Performance Snapshot

-   Synthesis results (LUT, FF, BSRAM, Fmax) --- latest known good.
-   Simulation performance (if relevant).
-   Any trends or regressions vs. expectations.

------------------------------------------------------------------------

## 10. Memory Configuration Record

-   For each BSRAM instance in use:
    -   Purpose (e.g., register file, state table)
    -   Port configuration (true dual-port, etc.)
    -   Read-during-write behavior
    -   Output registering
    -   Write enable behavior
    -   Width/depth configuration
-   Rationale for each configuration.

------------------------------------------------------------------------

## 11. Known Issues Cross-Reference

-   Link to relevant entries in `ATOMiK/docs/KNOWN_ISSUES.md` that:
    -   Were encountered in this phase
    -   Are still open
    -   Were resolved and may inform future phases

------------------------------------------------------------------------

## 12. Phase Exit Summary

-   What is *done* and considered stable.
-   What is *intentionally deferred* to the next phase.
-   Any constraints the next phase must respect.

------------------------------------------------------------------------

## 13. Next Phase Handoff

-   **Next Phase:** Phase X — *Name*
-   Key constraints to carry forward.
-   Key risks to watch immediately.
-   Any required setup or prerequisites before starting.

------------------------------------------------------------------------

> **Maintenance Rule:** At the end of each phase, **rewrite this
> document** to reflect only the current truth. Do not let it accumulate
> historical clutter---history belongs in git and in `KNOWN_ISSUES.md`.
