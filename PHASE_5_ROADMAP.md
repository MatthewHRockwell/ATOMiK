# Phase 5 Roadmap: Autonomous Token-Efficient Agentic Orchestration

**Version:** 1.0
**Date:** January 27, 2026
**Status:** PLANNED
**Budget:** $250 (estimated ~312K tokens + 15% contingency)
**Primary Agents:** Orchestrator Agent (Opus 4.5), Specialist Agent (Sonnet 4.5), Validator Agent (Haiku 4.5)
**Dependencies:** Phase 4C (Autonomous Pipeline), Phase 4A (Generator Framework), Phase 4B (Domain SDKs)

---

## Executive Summary

Phase 5 transforms ATOMiK's Phase 4C linear six-stage pipeline into a **self-improving agentic orchestration system** with feedback loops, adaptive model routing, multi-agent parallelism, deep verification, and cross-run learning. The current pipeline executes stages sequentially with fixed model assignments and no memory between runs. Phase 5 replaces this static design with an event-driven orchestrator that decomposes tasks into a dependency DAG, routes each subtask to the optimal model based on complexity and error history, executes independent subtasks in parallel, verifies outputs through deep language-native checks, and feeds results back into a persistent knowledge base for continuous self-improvement.

Three new architectural pillars emerge:

1. **Feedback loops** -- Every verification failure feeds into a diagnosis-fix-retry cycle (depth 3) with escalating model tiers, replacing the Phase 4C execute-once-then-escalate pattern.
2. **Adaptive intelligence** -- An error pattern knowledge base, field-level structural diffing, and cross-run metrics analysis replace hardcoded fix patterns and section-level diffing.
3. **Multi-agent parallelism** -- A coordinator agent decomposes work across specialist agents (per-language generators, verifiers, synthesizers) executing in parallel, replacing the single-threaded stage model.

**Core thesis:** The Phase 4C pipeline proves that autonomous generation works. Phase 5 makes it *intelligent* -- learning from failures, predicting token costs, parallelizing independent work, verifying deeply, and improving across runs without human tuning. The target is a 40% reduction in per-run token cost, 3x faster pipeline throughput via parallelism, and zero regression between runs.

---

## Table of Contents

1. [Design Principles](#1-design-principles)
2. [Architecture Overview](#2-architecture-overview)
3. [Task Breakdown](#3-task-breakdown)
4. [Token Budget Summary](#4-token-budget-summary)
5. [Execution Schedule](#5-execution-schedule)
6. [Agent Coordination Model](#6-agent-coordination-model)
7. [Risk Analysis](#7-risk-analysis)
8. [Integration Points with Phase 4C](#8-integration-points-with-phase-4c)
9. [New Directory Structure](#9-new-directory-structure)
10. [Success Criteria](#10-success-criteria)
11. [Validation Gates](#11-validation-gates)

---

## 1. Design Principles

### 1.1 Event-Driven, Not Sequential

The pipeline is a directed acyclic graph (DAG) of tasks, not a linear chain. Each task emits events on completion or failure. The orchestrator reacts to events by dispatching dependent tasks, triggering retries, or escalating -- never by polling or sleeping in a fixed sequence.

- Tasks declare their dependencies explicitly
- The orchestrator dispatches tasks whose dependencies are satisfied
- Independent tasks execute in parallel automatically
- Failure of one branch does not block unrelated branches

### 1.2 Learn from Every Failure

Every error encountered during generation, verification, or synthesis is recorded in a persistent knowledge base. The system matches new errors against known patterns using fuzzy matching, applies proven fixes without LLM invocation, and records new patterns when LLM-assisted diagnosis succeeds. The knowledge base grows with every pipeline run.

- Known errors are resolved locally (0 tokens)
- Novel errors trigger LLM diagnosis; successful fixes are recorded
- Error patterns include language, error class, regex signature, and fix template
- Knowledge base is versioned and exportable

### 1.3 Adaptive Model Routing

Model selection is not static. The router considers task complexity (AST depth, field count, operation count), error history (has this schema failed before?), budget pressure (remaining tokens vs. remaining work), and prompt cache availability. Cheaper models handle more work as the knowledge base grows.

| Signal | Effect on Routing |
|--------|-------------------|
| Low complexity task | Route to Haiku or local execution |
| High complexity task | Route to Sonnet or Opus |
| Previous failure on this schema | Escalate one tier preemptively |
| Budget pressure (>80% consumed) | Prefer local execution, compress prompts |
| Prompt cache hit | Re-use cached context, reduce input tokens |

### 1.4 Verify Deeply, Not Superficially

Phase 4C verification runs lint and basic syntax checks. Phase 5 adds deep verification: `pytest` with coverage for Python, `cargo test` for Rust, `gcc -Wall -Werror` for C, `node --check` for JavaScript, and `iverilog` + `vvp` for Verilog. Cross-language consistency checks ensure that all five language outputs implement the same interface (field names, types, operation signatures).

- Every language output is verified by its native toolchain
- Cross-language consistency is checked structurally, not textually
- Verification depth adapts to change scope (metadata-only changes get lighter checks)

### 1.5 Parallel by Default

Independent work is parallelized automatically. Per-language generation, per-language verification, and per-domain hardware synthesis all run concurrently when their dependencies are satisfied. The coordinator agent manages work distribution and result aggregation.

- Per-language generation runs in parallel (5 languages concurrently)
- Per-language verification runs in parallel
- Cross-domain pipeline runs can overlap
- Result aggregation waits for all parallel branches before proceeding

### 1.6 Predictive Budget Management

Token budget is not a passive check-and-abort mechanism. The budget engine predicts token consumption before each task based on historical data, warns when projected spend exceeds the budget, compresses context proactively when budget pressure rises, and caches prompts aggressively to reduce input tokens.

- Pre-task estimation based on historical per-task token averages
- Budget warnings at 50%, 75%, and 90% thresholds
- Automatic context compression when budget pressure exceeds 80%
- Prompt caching with TTL-based invalidation

---

## 2. Architecture Overview

### 2.1 Event-Driven Orchestrator Topology

```
                    ┌──────────────────────────────────┐
                    │    Event-Driven Orchestrator      │
                    │                                    │
                    │  - DAG task scheduler               │
                    │  - Event bus (emit/subscribe)       │
                    │  - Adaptive model router             │
                    │  - Predictive budget engine           │
                    │  - Cross-run knowledge base           │
                    └───────────────┬──────────────────┘
                                    │
              ┌─────────────────────┼─────────────────────┐
              │                     │                     │
              ▼                     ▼                     ▼
    ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
    │  Task Decomposer │  │  Coordinator     │  │  Feedback Loop   │
    │                  │  │  Agent           │  │  Engine          │
    │  - Parse work    │  │  - Dispatch to   │  │  - Diagnose      │
    │  - Build DAG     │  │    specialists   │  │  - Fix           │
    │  - Identify      │  │  - Aggregate     │  │  - Retry (x3)    │
    │    parallelism   │  │    results       │  │  - Learn         │
    │  - Estimate cost │  │  - Consensus     │  │  - Escalate      │
    └──────────────────┘  └──────────────────┘  └──────────────────┘
                                    │
         ┌──────────────────────────┼──────────────────────────┐
         │                          │                          │
         ▼                          ▼                          ▼
┌──────────────────┐    ┌──────────────────┐    ┌──────────────────┐
│  Generator       │    │  Verifier        │    │  Synthesizer     │
│  Specialists     │    │  Specialists     │    │  Specialists     │
│                  │    │                  │    │                  │
│  Python Agent    │    │  pytest runner   │    │  iverilog sim    │
│  Rust Agent      │    │  cargo test      │    │  Gowin synth     │
│  C Agent         │    │  gcc verify      │    │  FPGA program    │
│  JS Agent        │    │  node --check    │    │  HW test suite   │
│  Verilog Agent   │    │  verilator lint  │    │  Perf profiling  │
└──────────────────┘    └──────────────────┘    └──────────────────┘
```

### 2.2 Feedback Loop Architecture

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  Generate    │────>│  Verify     │────>│  Pass?      │
│  (Stage N)   │     │  (Deep)     │     │             │
└─────────────┘     └─────────────┘     └──────┬──────┘
                                                │
                                     ┌──────────┴──────────┐
                                     │ YES                  │ NO
                                     ▼                      ▼
                              ┌─────────────┐      ┌─────────────┐
                              │  Continue    │      │  Diagnose   │
                              │  to next     │      │  Error      │
                              │  stage       │      └──────┬──────┘
                              └─────────────┘             │
                                                          ▼
                                                   ┌─────────────┐
                                                   │  Match KB?  │
                                                   └──────┬──────┘
                                                          │
                                               ┌──────────┴──────────┐
                                               │ YES                  │ NO
                                               ▼                      ▼
                                        ┌─────────────┐      ┌─────────────┐
                                        │  Apply       │      │  LLM        │
                                        │  Known Fix   │      │  Diagnosis  │
                                        │  (0 tokens)  │      │  (2-8K tok) │
                                        └──────┬──────┘      └──────┬──────┘
                                               │                    │
                                               ▼                    ▼
                                        ┌─────────────┐      ┌─────────────┐
                                        │  Re-verify   │      │  Apply Fix  │
                                        │  (depth-1)   │      │  + Record   │
                                        └─────────────┘      │  in KB      │
                                                              └──────┬──────┘
                                                                     │
                                                                     ▼
                                                              ┌─────────────┐
                                                              │  Re-verify   │
                                                              │  (depth-1)   │
                                                              └─────────────┘

                    Max retry depth: 3 (configurable)
                    Escalation: Local -> Haiku -> Sonnet -> Opus -> Human
```

### 2.3 Agent Topology

```
                         ┌────────────────────────────┐
                         │       Coordinator Agent     │
                         │       (Opus 4.5)            │
                         │                              │
                         │   - Work decomposition       │
                         │   - Specialist dispatch      │
                         │   - Result aggregation       │
                         │   - Consensus resolution     │
                         └──────────────┬───────────────┘
                                        │
         ┌──────────────────┬───────────┼───────────┬──────────────────┐
         │                  │           │           │                  │
         ▼                  ▼           ▼           ▼                  ▼
  ┌─────────────┐  ┌─────────────┐ ┌────────┐ ┌─────────────┐ ┌─────────────┐
  │  Generator  │  │  Verifier   │ │ Synth  │ │  Diagnostics│ │  Metrics    │
  │  Pool       │  │  Pool       │ │ Pool   │ │  Agent      │ │  Analyzer   │
  │  (Sonnet)   │  │  (Haiku)    │ │(Local) │ │  (Sonnet)   │ │  (Local)    │
  │             │  │             │ │        │ │             │ │             │
  │  5 language │  │  5 language │ │ iverlog│ │  Error KB   │ │  Trends     │
  │  specialists│  │  verifiers  │ │ Gowin  │ │  Fuzzy match│ │  Anomalies  │
  │  in parallel│  │  in parallel│ │ Flash  │ │  Fix record │ │  Regression │
  └─────────────┘  └─────────────┘ └────────┘ └─────────────┘ └─────────────┘

  Agent Registry: Dynamic registration of specialist agents
  Each specialist declares: capabilities, model tier, token cost estimate
```

---

## 3. Task Breakdown

### Phase 5A: Foundation (T5.1 -- T5.4)

---

### T5.1: Event-Driven Pipeline Orchestrator

**Agent:** Orchestrator Agent (Opus 4.5)
**Estimated tokens:** 25K
**Dependencies:** Phase 4C controller.py

**Description:**
Replaces the Phase 4C sequential stage dispatcher with an event-driven DAG scheduler. Tasks declare dependencies explicitly and the orchestrator dispatches work as dependencies are satisfied. An event bus enables publish-subscribe communication between stages, replacing the linear manifest chain.

**Deliverables:**
- `software/atomik_sdk/pipeline/orchestrator.py` -- Event-driven DAG scheduler with topological dispatch
- `software/atomik_sdk/pipeline/event_bus.py` -- Publish-subscribe event system for inter-stage communication
- `software/atomik_sdk/pipeline/dag.py` -- Task DAG builder with cycle detection and critical path analysis
- Updated `software/atomik_sdk/pipeline/controller.py` -- Delegate to orchestrator while preserving CLI interface

**Acceptance criteria:**
- DAG scheduler dispatches independent tasks in parallel
- Cycle detection rejects invalid dependency graphs at construction time
- Event bus delivers completion/failure events within 1ms of emission
- Phase 4C sequential behavior preserved when DAG degenerates to a chain
- Critical path analysis identifies the longest execution path for budget estimation

---

### T5.2: Feedback Loop Engine

**Agent:** Specialist Agent (Sonnet 4.5)
**Estimated tokens:** 20K
**Dependencies:** T5.1

**Description:**
Implements the generate-verify-diagnose-fix-retry feedback cycle with configurable depth (default 3). Each iteration classifies the error, consults the knowledge base for known fixes, applies the fix (or invokes LLM diagnosis for unknown errors), and re-verifies. Successful fixes are recorded in the knowledge base for future use.

**Deliverables:**
- `software/atomik_sdk/pipeline/feedback.py` -- Feedback loop controller with configurable retry depth
- `software/atomik_sdk/pipeline/diagnosis.py` -- Error classification and LLM-assisted diagnosis
- Updated `software/atomik_sdk/pipeline/agents/self_correct.py` -- Integrate with feedback loop and knowledge base

**Acceptance criteria:**
- Retry loop terminates after configurable max depth (default 3)
- Known errors resolved without LLM invocation (0 tokens)
- Novel errors trigger LLM diagnosis with escalating model tiers (Haiku -> Sonnet -> Opus)
- Successful LLM-diagnosed fixes are persisted to the knowledge base
- Infinite loop protection: identical error on consecutive retries triggers immediate escalation

---

### T5.3: Adaptive Model Router

**Agent:** Specialist Agent (Sonnet 4.5)
**Estimated tokens:** 18K
**Dependencies:** T5.1, T5.2

**Description:**
Replaces the Phase 4C static model routing table with an adaptive router that considers task complexity, error history, budget pressure, and prompt cache state. The router scores each available model tier against a weighted cost function and selects the cheapest model expected to succeed. Routing decisions are logged for post-run analysis.

**Deliverables:**
- `software/atomik_sdk/pipeline/agents/adaptive_router.py` -- Multi-signal model selection engine
- `software/atomik_sdk/pipeline/agents/complexity_scorer.py` -- Schema and task complexity scoring
- Updated `software/atomik_sdk/pipeline/agents/router.py` -- Delegate to adaptive router, preserve fallback static routing

**Acceptance criteria:**
- Router selects Haiku for low-complexity tasks (field count < 5, no hardware section)
- Router escalates preemptively when schema has prior failure history
- Router downgrades model tier when budget pressure exceeds 80%
- Routing decisions logged with reasoning (complexity score, budget state, cache hit)
- Static routing preserved as fallback when adaptive signals are unavailable

---

### T5.4: Token Efficiency Engine

**Agent:** Specialist Agent (Sonnet 4.5)
**Estimated tokens:** 18K
**Dependencies:** T5.1, T5.3

**Description:**
Extends the Phase 4C token budget module with predictive estimation, prompt caching, and context compression. Before each task, the engine estimates token cost from historical per-task averages. When budget pressure rises, the engine compresses context by stripping comments, truncating examples, and using structured deltas instead of full file contents. Prompt caching with TTL-based invalidation reduces repeat input tokens.

**Deliverables:**
- `software/atomik_sdk/pipeline/agents/token_predictor.py` -- Predictive token estimation from historical data
- `software/atomik_sdk/pipeline/agents/prompt_cache.py` -- Prompt caching with TTL and schema-aware invalidation
- `software/atomik_sdk/pipeline/agents/context_compressor.py` -- Progressive context compression under budget pressure
- Updated `software/atomik_sdk/pipeline/agents/token_budget.py` -- Integrate prediction, caching, and compression

**Acceptance criteria:**
- Pre-task token estimates within 25% of actual consumption (measured over 10 runs)
- Prompt cache achieves 30%+ hit rate on repeated schema processing
- Context compression reduces prompt size by 40%+ when budget pressure exceeds 80%
- Budget warnings emitted at 50%, 75%, and 90% thresholds
- Total per-run token cost reduced 40% vs. Phase 4C baseline (measured)

---

### Phase 5B: Intelligence (T5.5 -- T5.7)

---

### T5.5: Error Pattern Knowledge Base

**Agent:** Specialist Agent (Sonnet 4.5)
**Estimated tokens:** 20K
**Dependencies:** T5.2

**Description:**
Creates a persistent, versioned knowledge base of error patterns and their proven fixes. Each entry includes a language, error class, regex signature for matching, fix template, success count, and failure count. The knowledge base supports fuzzy matching (edit distance on error messages) for near-miss patterns. New patterns are learned automatically when LLM-diagnosed fixes succeed on re-verification.

**Deliverables:**
- `software/atomik_sdk/pipeline/knowledge/error_kb.py` -- Error pattern knowledge base with CRUD operations
- `software/atomik_sdk/pipeline/knowledge/fuzzy_match.py` -- Fuzzy error matching using edit distance and token overlap
- `software/atomik_sdk/pipeline/knowledge/error_patterns.json` -- Seed knowledge base with Phase 4C's 5 known fix patterns
- `software/atomik_sdk/pipeline/knowledge/__init__.py` -- Package init

**Acceptance criteria:**
- Seed KB contains all 5 Phase 4C known error classes (import path, naming convention, missing semicolon, type mismatch, missing import)
- Fuzzy matching resolves errors within edit distance 3 of a known pattern
- New patterns auto-recorded when LLM fix succeeds and re-verify passes
- KB is versioned (schema version field) and exportable to JSON
- Success/failure counts updated on every match attempt

---

### T5.6: Field-Level Differential Analysis

**Agent:** Specialist Agent (Sonnet 4.5)
**Estimated tokens:** 18K
**Dependencies:** T5.5

**Description:**
Replaces the Phase 4C section-level diff with a structural, field-level diff engine. The engine traverses both the old and new schema as trees, producing a diff that identifies exactly which fields, operations, constraints, or metadata changed, with full path tracking (e.g., `delta_fields.price_delta.data_type`). The diff output drives minimal regeneration: only generators affected by the specific changed paths are invoked.

**Deliverables:**
- `software/atomik_sdk/pipeline/analysis/field_diff.py` -- Field-level structural diff with JSON path tracking
- `software/atomik_sdk/pipeline/analysis/diff_impact.py` -- Maps changed paths to affected generators and verification scopes
- `software/atomik_sdk/pipeline/analysis/__init__.py` -- Package init
- Updated `software/atomik_sdk/pipeline/stages/diff.py` -- Delegate to field-level engine, preserve section-level as fallback

**Acceptance criteria:**
- Diff output includes exact JSON paths for every changed field (e.g., `hardware.data_width`)
- Adding a single field regenerates only affected languages (not all 5)
- Metadata-only changes (description text) regenerate only Python and JavaScript (docstrings/JSDoc)
- Hardware constraint changes trigger only Verilog regeneration and re-synthesis
- Section-level diff preserved as fallback for schemas that fail structural parsing

---

### T5.7: Cross-Run Metrics Analyzer

**Agent:** Validator Agent (Haiku 4.5)
**Estimated tokens:** 15K
**Dependencies:** T5.4

**Description:**
Analyzes metrics trends across pipeline runs to detect performance regressions, anomalies, and optimization opportunities. Reads the CSV metrics history produced by Phase 4C, computes moving averages for key metrics (token cost, generation time, test pass rate, hardware utilization), flags runs that deviate by more than 2 standard deviations, and produces a trend report. Regression detection triggers alerts in the pipeline report.

**Deliverables:**
- `software/atomik_sdk/pipeline/analysis/metrics_analyzer.py` -- Cross-run trend analysis with moving averages and anomaly detection
- `software/atomik_sdk/pipeline/analysis/regression_detector.py` -- Regression detection using statistical deviation thresholds
- Updated `software/atomik_sdk/pipeline/metrics/reporter.py` -- Include trend data and regression alerts in pipeline report

**Acceptance criteria:**
- Moving averages computed over configurable window (default: last 10 runs)
- Anomalies flagged when metric deviates by >2 standard deviations from moving average
- Regression alerts included in pipeline report with affected metric, expected value, and actual value
- Trend report shows improvement/degradation direction for each metric
- Analysis runs locally (0 LLM tokens) using statistical computation only

---

### Phase 5C: Orchestration (T5.8 -- T5.10)

---

### T5.8: Task Decomposer & Parallel Executor

**Agent:** Orchestrator Agent (Opus 4.5)
**Estimated tokens:** 22K
**Dependencies:** T5.1, T5.6

**Description:**
Decomposes pipeline work into fine-grained tasks that can execute in parallel. For code generation, each language becomes an independent task. For verification, each language's test suite is an independent task. For hardware, simulation and synthesis can overlap with software verification. The executor manages a worker pool, dispatches tasks, collects results, and handles partial failures (one language failing does not block others).

**Deliverables:**
- `software/atomik_sdk/pipeline/parallel/decomposer.py` -- Work decomposition engine that converts pipeline stages into parallel task sets
- `software/atomik_sdk/pipeline/parallel/executor.py` -- Parallel task executor with configurable worker pool and result collection
- `software/atomik_sdk/pipeline/parallel/worker.py` -- Worker process wrapper with timeout, cancellation, and result reporting
- `software/atomik_sdk/pipeline/parallel/__init__.py` -- Package init

**Acceptance criteria:**
- 5-language generation completes in ~1x single-language time (parallel speedup)
- 5-language verification runs concurrently
- Partial failure in one language does not block other languages
- Worker pool size configurable (default: CPU count, max: 8)
- Result aggregation waits for all workers before producing combined manifest

---

### T5.9: Specialist Agent Registry

**Agent:** Specialist Agent (Sonnet 4.5)
**Estimated tokens:** 18K
**Dependencies:** T5.3, T5.8

**Description:**
Creates a dynamic registry where specialist agents declare their capabilities (languages supported, task types handled, model tier, estimated token cost). The coordinator agent queries the registry to find the best-fit specialist for each decomposed task. New specialists can be registered without modifying the coordinator. The registry supports capability matching, load balancing, and health checking.

**Deliverables:**
- `software/atomik_sdk/pipeline/agents/registry.py` -- Specialist agent registry with capability matching and health status
- `software/atomik_sdk/pipeline/agents/specialist.py` -- Specialist agent base class with capability declaration protocol
- `software/atomik_sdk/pipeline/agents/specialists/` -- Directory with per-domain specialist implementations
- `software/atomik_sdk/pipeline/agents/specialists/__init__.py` -- Package init
- `software/atomik_sdk/pipeline/agents/specialists/python_gen.py` -- Python generation specialist
- `software/atomik_sdk/pipeline/agents/specialists/rust_gen.py` -- Rust generation specialist
- `software/atomik_sdk/pipeline/agents/specialists/verilog_gen.py` -- Verilog generation and synthesis specialist

**Acceptance criteria:**
- Registry discovers and lists all registered specialists at startup
- Capability matching selects correct specialist for each language/task pair
- New specialist registration requires no coordinator code changes (plugin pattern)
- Health check detects unresponsive specialists and routes around them
- Registry state queryable via CLI (`atomik-gen agents list`)

---

### T5.10: Coordinator Agent with Result Aggregation

**Agent:** Orchestrator Agent (Opus 4.5)
**Estimated tokens:** 20K
**Dependencies:** T5.8, T5.9

**Description:**
Top-level coordinator agent that receives pipeline work requests, decomposes them via the task decomposer, dispatches subtasks to specialists via the registry, collects and aggregates results, and resolves conflicts when specialists produce inconsistent outputs. Implements a consensus protocol for cases where multiple specialists contribute to overlapping artifacts (e.g., cross-language interface consistency).

**Deliverables:**
- `software/atomik_sdk/pipeline/coordinator.py` -- Coordinator agent with decomposition, dispatch, and aggregation
- `software/atomik_sdk/pipeline/consensus.py` -- Consensus resolution for multi-specialist result conflicts
- Updated `software/atomik_sdk/pipeline/orchestrator.py` -- Integrate coordinator as the top-level dispatch target

**Acceptance criteria:**
- Coordinator decomposes a full pipeline run into parallel task sets automatically
- Results from all specialists aggregated into a single pipeline manifest
- Consensus resolution detects interface inconsistencies across languages (field name mismatches)
- Coordinator handles specialist timeout (configurable, default 60s) with graceful degradation
- End-to-end pipeline throughput improved 3x vs. Phase 4C sequential execution (measured)

---

### Phase 5D: Verification (T5.11 -- T5.12)

---

### T5.11: Deep Verification Engine

**Agent:** Validator Agent (Haiku 4.5)
**Estimated tokens:** 18K
**Dependencies:** T5.2, T5.8

**Description:**
Extends Phase 4C's lint-and-syntax verification with deep, language-native verification. Each language output is verified by its native toolchain: `pytest` with coverage for Python, `cargo test` for Rust, `gcc -Wall -Werror` compilation for C, `node --check` for JavaScript, and `iverilog` + `vvp` simulation for Verilog. Verification depth adapts to change scope -- metadata-only changes receive lighter verification, while field changes trigger full test suites.

**Deliverables:**
- `software/atomik_sdk/pipeline/verification/deep_verify.py` -- Deep verification orchestrator with per-language native tool invocation
- `software/atomik_sdk/pipeline/verification/runners/python_runner.py` -- Python pytest + coverage runner
- `software/atomik_sdk/pipeline/verification/runners/rust_runner.py` -- Rust cargo test runner
- `software/atomik_sdk/pipeline/verification/runners/c_runner.py` -- C gcc compilation + test runner
- `software/atomik_sdk/pipeline/verification/runners/js_runner.py` -- JavaScript node --check + test runner
- `software/atomik_sdk/pipeline/verification/runners/verilog_runner.py` -- Verilog iverilog + vvp simulation runner
- `software/atomik_sdk/pipeline/verification/runners/__init__.py` -- Package init
- `software/atomik_sdk/pipeline/verification/__init__.py` -- Package init

**Acceptance criteria:**
- Python verification runs `pytest` and reports line coverage percentage
- Rust verification runs `cargo test` and captures test count + pass/fail
- C verification compiles with `-Wall -Werror` and runs `make test`
- JavaScript verification runs `node --check` for syntax and `npm test` if available
- Verilog verification runs `iverilog` compilation + `vvp` simulation with pass/fail parsing
- Verification depth adapts: metadata changes skip full test suites, field changes run all tests

---

### T5.12: Cross-Language Consistency Checker

**Agent:** Specialist Agent (Sonnet 4.5)
**Estimated tokens:** 18K
**Dependencies:** T5.11

**Description:**
Verifies that all five language outputs implement the same interface. Extracts structural information from each generated file (field names, types, operation signatures, constants) and compares them across languages. Detects inconsistencies such as mismatched field names, missing operations, or incompatible type mappings. Produces a consistency report with exact locations of divergence.

**Deliverables:**
- `software/atomik_sdk/pipeline/verification/consistency.py` -- Cross-language consistency checker with structural comparison
- `software/atomik_sdk/pipeline/verification/extractors/` -- Per-language interface extractors
- `software/atomik_sdk/pipeline/verification/extractors/__init__.py` -- Package init
- `software/atomik_sdk/pipeline/verification/extractors/python_extractor.py` -- Extract Python class fields and methods
- `software/atomik_sdk/pipeline/verification/extractors/rust_extractor.py` -- Extract Rust struct fields and impl methods
- `software/atomik_sdk/pipeline/verification/extractors/c_extractor.py` -- Extract C struct members and function signatures
- `software/atomik_sdk/pipeline/verification/extractors/js_extractor.py` -- Extract JavaScript class properties and methods
- `software/atomik_sdk/pipeline/verification/extractors/verilog_extractor.py` -- Extract Verilog module ports and parameters

**Acceptance criteria:**
- Detects field name mismatches across any two languages (e.g., `price_delta` in Python vs. `priceDelta` in JavaScript -- expected naming convention difference vs. actual error)
- Detects missing operations (e.g., `rollback` present in Python but absent in Rust)
- Detects type mapping inconsistencies (e.g., 64-bit field mapped to `i32` in Rust)
- Produces consistency report with exact file paths and line numbers of divergence
- Accounts for language-specific naming conventions (snake_case, camelCase, PascalCase) as expected differences

---

### Phase 5E: Optimization (T5.13 -- T5.16)

---

### T5.13: Regression Detection System

**Agent:** Validator Agent (Haiku 4.5)
**Estimated tokens:** 15K
**Dependencies:** T5.7, T5.11

**Description:**
Monitors pipeline outputs across runs to detect regressions: tests that previously passed now failing, hardware metrics degrading (Fmax dropping, utilization increasing), token cost increasing, or generation time growing. Regressions are classified by severity (critical: test failure; warning: metric degradation; info: cost increase) and surfaced prominently in the pipeline report. Optionally blocks pipeline completion on critical regressions.

**Deliverables:**
- `software/atomik_sdk/pipeline/regression/detector.py` -- Regression detection across runs with severity classification
- `software/atomik_sdk/pipeline/regression/baseline.py` -- Baseline snapshot management (create, compare, update)
- `software/atomik_sdk/pipeline/regression/__init__.py` -- Package init
- Updated `software/atomik_sdk/pipeline/reports/pipeline_report.py` -- Include regression section with severity badges

**Acceptance criteria:**
- Detects test count regression (fewer tests passing than baseline)
- Detects hardware metric regression (Fmax drop > 5%, LUT increase > 10%)
- Detects token cost regression (per-run cost increase > 25% over moving average)
- Critical regressions block pipeline completion (configurable: `--fail-on-regression`)
- Baseline snapshots created automatically after first successful run per schema

---

### T5.14: Intelligent Context Manager

**Agent:** Specialist Agent (Sonnet 4.5)
**Estimated tokens:** 20K
**Dependencies:** T5.4, T5.5

**Description:**
Manages the context window intelligently across multi-turn agent interactions. Tracks which context segments are most relevant to the current task, evicts stale segments, injects relevant knowledge base entries, and pre-loads schema sections that the current task is likely to need. Context relevance is scored by recency, task-type affinity, and error history.

**Deliverables:**
- `software/atomik_sdk/pipeline/context/intelligent_manager.py` -- Context window manager with relevance scoring and eviction
- `software/atomik_sdk/pipeline/context/segment_tracker.py` -- Tracks context segments with usage statistics and relevance scores
- Updated `software/atomik_sdk/pipeline/context/manifest.py` -- Add segment-level metadata for intelligent loading
- Updated `software/atomik_sdk/pipeline/context/cache.py` -- Integrate with relevance scoring for cache prioritization

**Acceptance criteria:**
- Context loading prioritizes segments relevant to current task type
- Stale segments (not referenced in last 3 tasks) evicted automatically
- Knowledge base entries for the current schema's error history pre-loaded
- Context window utilization stays below 80% of model's maximum (prevents truncation)
- Cold-start context loading reduced to <1.5K tokens (vs. Phase 4C's <2K)

---

### T5.15: Pipeline Self-Optimization Engine

**Agent:** Orchestrator Agent (Opus 4.5)
**Estimated tokens:** 25K
**Dependencies:** T5.7, T5.10, T5.13

**Description:**
Analyzes pipeline execution patterns across runs and automatically adjusts configuration for optimal performance. Identifies bottleneck stages, adjusts worker pool sizes, tunes retry depths, updates model routing weights, and recommends knowledge base expansions. Produces an optimization report after every N runs (configurable, default 5) with specific recommendations.

**Deliverables:**
- `software/atomik_sdk/pipeline/optimization/self_optimizer.py` -- Pipeline self-optimization engine with bottleneck analysis
- `software/atomik_sdk/pipeline/optimization/tuner.py` -- Configuration auto-tuning based on historical performance data
- `software/atomik_sdk/pipeline/optimization/__init__.py` -- Package init
- Updated `software/atomik_sdk/pipeline/orchestrator.py` -- Apply tuning recommendations between runs

**Acceptance criteria:**
- Bottleneck analysis correctly identifies the slowest pipeline stage per run
- Worker pool size tuned based on observed parallelism benefit (diminishing returns detection)
- Retry depth tuned based on historical success rate at each depth level
- Model routing weights updated based on per-tier success rate and cost efficiency
- Optimization report generated every 5 runs with specific, actionable recommendations

---

### T5.16: Test Suite & Documentation

**Agent:** Validator Agent (Haiku 4.5)
**Estimated tokens:** 22K
**Dependencies:** T5.1 through T5.15

**Description:**
Comprehensive test suite covering all Phase 5 modules, integration tests for the feedback loop and parallel execution, and updated documentation. Performance benchmarks compare Phase 5 against Phase 4C baselines. Documentation updates cover the new architecture, agent model, configuration options, and CLI extensions.

**Deliverables:**
- `software/atomik_sdk/tests/test_orchestrator.py` -- Event-driven orchestrator and DAG scheduler tests
- `software/atomik_sdk/tests/test_feedback.py` -- Feedback loop engine tests with mock errors
- `software/atomik_sdk/tests/test_adaptive_router.py` -- Adaptive model router tests
- `software/atomik_sdk/tests/test_token_efficiency.py` -- Token prediction, caching, and compression tests
- `software/atomik_sdk/tests/test_error_kb.py` -- Error pattern knowledge base tests
- `software/atomik_sdk/tests/test_field_diff.py` -- Field-level differential analysis tests
- `software/atomik_sdk/tests/test_metrics_analyzer.py` -- Cross-run metrics and regression detection tests
- `software/atomik_sdk/tests/test_parallel_executor.py` -- Parallel task execution tests
- `software/atomik_sdk/tests/test_agent_registry.py` -- Specialist agent registry tests
- `software/atomik_sdk/tests/test_deep_verify.py` -- Deep verification engine tests
- `software/atomik_sdk/tests/test_consistency.py` -- Cross-language consistency checker tests
- `software/atomik_sdk/tests/test_self_optimizer.py` -- Pipeline self-optimization tests
- Updated `docs/SDK_DEVELOPER_GUIDE.md` -- Phase 5 architecture, feedback loops, agent model
- Updated `docs/SDK_API_REFERENCE.md` -- New CLI commands, configuration options
- Updated `README.md` -- Phase 5 overview and capabilities
- Phase 5 completion report with performance benchmarks vs. Phase 4C

**Acceptance criteria:**
- 50+ new tests covering all Phase 5 modules
- All existing tests (124+) continue to pass
- Integration test: full pipeline run with feedback loop triggered by injected error
- Integration test: parallel execution produces identical output to sequential execution
- Performance benchmark: 40% token reduction vs. Phase 4C measured and documented
- Performance benchmark: 3x throughput improvement via parallelism measured and documented
- Documentation accurately reflects new architecture and all configuration options

---

## 4. Token Budget Summary

### 4.1 Per-Task Token Budget

| Task | Title | Estimated Tokens | Agent | Model Tier |
|------|-------|-----------------|-------|------------|
| T5.1 | Event-Driven Pipeline Orchestrator | 25K | Orchestrator | Opus 4.5 |
| T5.2 | Feedback Loop Engine | 20K | Specialist | Sonnet 4.5 |
| T5.3 | Adaptive Model Router | 18K | Specialist | Sonnet 4.5 |
| T5.4 | Token Efficiency Engine | 18K | Specialist | Sonnet 4.5 |
| T5.5 | Error Pattern Knowledge Base | 20K | Specialist | Sonnet 4.5 |
| T5.6 | Field-Level Differential Analysis | 18K | Specialist | Sonnet 4.5 |
| T5.7 | Cross-Run Metrics Analyzer | 15K | Validator | Haiku 4.5 |
| T5.8 | Task Decomposer & Parallel Executor | 22K | Orchestrator | Opus 4.5 |
| T5.9 | Specialist Agent Registry | 18K | Specialist | Sonnet 4.5 |
| T5.10 | Coordinator Agent with Result Aggregation | 20K | Orchestrator | Opus 4.5 |
| T5.11 | Deep Verification Engine | 18K | Validator | Haiku 4.5 |
| T5.12 | Cross-Language Consistency Checker | 18K | Specialist | Sonnet 4.5 |
| T5.13 | Regression Detection System | 15K | Validator | Haiku 4.5 |
| T5.14 | Intelligent Context Manager | 20K | Specialist | Sonnet 4.5 |
| T5.15 | Pipeline Self-Optimization Engine | 25K | Orchestrator | Opus 4.5 |
| T5.16 | Test Suite & Documentation | 22K | Validator | Haiku 4.5 |
| | **Subtotal** | **312K** | | |
| | **Contingency (15%)** | **46.8K** | | |
| | **Grand Total** | **358.8K** | | **~$250** |

### 4.2 Budget by Model Tier

| Model Tier | Tasks | Total Tokens | Estimated Cost |
|------------|-------|-------------|----------------|
| Opus 4.5 | T5.1, T5.8, T5.10, T5.15 | 92K | ~$65 |
| Sonnet 4.5 | T5.2, T5.3, T5.4, T5.5, T5.6, T5.9, T5.12, T5.14 | 150K | ~$107 |
| Haiku 4.5 | T5.7, T5.11, T5.13, T5.16 | 70K | ~$48 |
| | **Subtotal** | **312K** | **~$220** |
| | **Contingency (15%)** | **46.8K** | **~$30** |
| | **Grand Total** | **358.8K** | **~$250** |

### 4.3 Budget by Sub-Phase

| Sub-Phase | Tasks | Tokens | Cost |
|-----------|-------|--------|------|
| 5A: Foundation | T5.1 -- T5.4 | 81K | ~$57 |
| 5B: Intelligence | T5.5 -- T5.7 | 53K | ~$37 |
| 5C: Orchestration | T5.8 -- T5.10 | 60K | ~$43 |
| 5D: Verification | T5.11 -- T5.12 | 36K | ~$25 |
| 5E: Optimization | T5.13 -- T5.16 | 82K | ~$58 |
| | **Subtotal** | **312K** | **~$220** |

### 4.4 Steady-State Token Cost (Post-Phase 5)

| Scenario | Phase 4C Tokens | Phase 5 Tokens | Reduction |
|----------|----------------|----------------|-----------|
| New schema (full pipeline) | ~12K | ~7K | 42% |
| Schema update (differential) | ~4K | ~2K | 50% |
| No change (short-circuit) | 0 | 0 | -- |
| Failure + self-correction (known error) | ~6K | 0 | 100% |
| Failure + self-correction (novel error) | ~6K | ~4K | 33% |
| Hardware validation only | 0 | 0 | -- |
| Cross-run regression check | 0 | 0 | -- |

---

## 5. Execution Schedule

| Step | Tasks | Dependencies | Est. Tokens | Parallelism |
|------|-------|-------------|-------------|-------------|
| 1 | T5.1 (Orchestrator) | Phase 4C complete | 25K | Sequential (foundation) |
| 2 | T5.2 (Feedback) + T5.3 (Router) + T5.4 (Token Engine) | Step 1 | 56K | 3 tasks parallel |
| 3 | T5.5 (Error KB) + T5.6 (Field Diff) + T5.7 (Metrics Analyzer) | Step 2 | 53K | 3 tasks parallel |
| 4 | T5.8 (Decomposer) + T5.9 (Registry) + T5.11 (Deep Verify) | Step 3 | 58K | 3 tasks parallel |
| 5 | T5.10 (Coordinator) + T5.12 (Consistency) + T5.13 (Regression) + T5.14 (Context Mgr) | Step 4 | 73K | 4 tasks parallel |
| 6 | T5.15 (Self-Optimizer) + T5.16 (Tests & Docs) | Step 5 | 47K | 2 tasks parallel |
| **Total** | **16 tasks** | | **312K + 46.8K contingency** | |

Steps within each group contain independent tasks that can execute in parallel. The critical path is: T5.1 -> T5.2 -> T5.5 -> T5.8 -> T5.10 -> T5.15 (6 sequential steps, 132K tokens on critical path).

```
Step 1:  [T5.1 Orchestrator                    ]
Step 2:  [T5.2 Feedback    ] [T5.3 Router] [T5.4 Token Engine]
Step 3:  [T5.5 Error KB    ] [T5.6 Diff  ] [T5.7 Metrics     ]
Step 4:  [T5.8 Decomposer  ] [T5.9 Reg   ] [T5.11 Deep Verify]
Step 5:  [T5.10 Coordinator] [T5.12 Cons ] [T5.13 Regr] [T5.14 Ctx]
Step 6:  [T5.15 Self-Optimizer    ] [T5.16 Tests & Docs       ]
```

---

## 6. Agent Coordination Model

### 6.1 Agent Roles

| Agent | Role | Model Tier | Invocation Trigger |
|-------|------|------------|-------------------|
| **Orchestrator Agent** | DAG scheduling, event dispatch, self-optimization | Opus 4.5 | Pipeline start, optimization cycle |
| **Coordinator Agent** | Task decomposition, specialist dispatch, result aggregation | Opus 4.5 | Work request from orchestrator |
| **Generator Specialist** | Per-language code generation | Sonnet 4.5 | Generation task from coordinator |
| **Verifier Specialist** | Per-language deep verification | Haiku 4.5 | Verification task from coordinator |
| **Diagnostics Agent** | Error analysis, knowledge base consultation, fix suggestion | Sonnet 4.5 | Verification failure |
| **Synthesis Specialist** | RTL simulation, FPGA synthesis, hardware validation | Local (no LLM) | Hardware verification task |
| **Metrics Analyzer** | Cross-run trend analysis, regression detection, anomaly flagging | Local (no LLM) | Pipeline completion, optimization cycle |
| **Context Manager** | Context window optimization, segment tracking, cache management | Local (no LLM) | Every LLM invocation |

### 6.2 Agent Communication Protocol

Agents communicate exclusively through typed artifacts on disk and events on the event bus. No agent reads another agent's conversation context.

```
Event Bus Messages:

  TASK_READY      { task_id, task_type, dependencies_met }
  TASK_STARTED    { task_id, agent_id, model_tier, estimated_tokens }
  TASK_COMPLETED  { task_id, agent_id, manifest_path, tokens_consumed, duration_ms }
  TASK_FAILED     { task_id, agent_id, error_class, error_message, retry_depth }
  FEEDBACK_START  { task_id, error_class, retry_number, max_retries }
  FEEDBACK_RESULT { task_id, fix_applied, fix_source (kb|llm), success }
  BUDGET_WARNING  { threshold_pct, consumed, remaining, projected_total }
  REGRESSION_ALERT{ metric_name, expected_value, actual_value, severity }
  PIPELINE_DONE   { status, total_tokens, total_time_ms, report_path }
```

### 6.3 Escalation Chain

```
Local execution (0 tokens)
    | fails
    v
Knowledge Base lookup (0 tokens)
    | no match
    v
Haiku diagnosis (2K tokens)
    | fails or uncertain
    v
Sonnet diagnosis (8K tokens)
    | fails
    v
Opus reasoning (20K tokens, budget check required)
    | fails
    v
Human intervention (pipeline paused, report generated)
```

### 6.4 Consensus Protocol

When multiple specialists contribute to related artifacts, the coordinator runs a consistency check before accepting results:

1. Collect all specialist results
2. Extract structural interface (field names, types, operations) from each language output
3. Compare interfaces pairwise
4. If consistent: accept all results, merge manifests
5. If inconsistent: flag divergent fields, invoke diagnostics agent to determine correct interface
6. Re-generate only the divergent outputs
7. Re-verify consistency

---

## 7. Risk Analysis

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Feedback loop thrashing (oscillating between fixes) | Medium | High | Max 3 retries; identical-error detection triggers immediate escalation; loop state tracked to prevent oscillation |
| Parallel execution race conditions | Medium | High | All inter-task communication via immutable artifacts on disk; no shared mutable state; DAG enforces ordering |
| Knowledge base poisoning (incorrect fix recorded) | Low | High | Fixes only recorded after successful re-verification; success/failure counters enable automatic deprecation of low-success patterns |
| Adaptive router over-fitting to recent history | Medium | Medium | Routing weights use exponential decay (recent runs weighted more, but old data not discarded); static routing preserved as fallback |
| Cross-language consistency checker false positives | High | Low | Language-specific naming convention mappings (snake_case, camelCase) built into the checker; configurable tolerance for expected differences |

---

## 8. Integration Points with Phase 4C

| Phase 4C Component | Phase 5 Integration | Migration Strategy |
|---------------------|--------------------|--------------------|
| `pipeline/controller.py` | Delegates to `orchestrator.py` for DAG scheduling; controller becomes a thin CLI adapter | Controller preserved as entry point; orchestrator injected as strategy |
| `pipeline/stages/diff.py` | Wrapped by `analysis/field_diff.py` for structural diffing; section-level diff preserved as fallback | Field-level engine called first; falls back to section-level on parse failure |
| `pipeline/stages/verify.py` | Extended by `verification/deep_verify.py` with native toolchain runners | Phase 4C lint checks still run; deep verification adds tool-specific runners |
| `pipeline/agents/self_correct.py` | Integrated with `feedback.py` loop and `knowledge/error_kb.py` knowledge base | Self-correct module calls KB before LLM; feedback loop wraps self-correct |
| `pipeline/agents/router.py` | Extended by `agents/adaptive_router.py` with multi-signal routing | Static routing table preserved; adaptive router scores and overrides when signals available |
| `pipeline/agents/token_budget.py` | Extended with prediction (`token_predictor.py`), caching (`prompt_cache.py`), and compression (`context_compressor.py`) | Budget module gains new capabilities; existing accounting unchanged |
| `pipeline/context/manifest.py` | Extended with segment-level metadata for intelligent loading | Manifest format version bumped to 3.0; backward-compatible with 2.0 manifests |
| `pipeline/context/cache.py` | Integrated with relevance scoring for prioritized cache eviction | Cache interface unchanged; eviction policy upgraded |
| `pipeline/context/checkpoint.py` | Extended with regression baseline snapshots | Checkpoint format extended; old checkpoints importable |
| `pipeline/metrics/collector.py` | Extended with cross-run trend data and anomaly flags | Collection API unchanged; new fields added to metrics output |
| `pipeline/metrics/reporter.py` | Extended with trend analysis, regression alerts, and optimization recommendations | Report format extended; new sections appended |
| `pipeline/stages/hardware.py` | Unchanged; integrated as a specialist in the agent registry | Hardware stage registered as synthesis specialist |
| `pipeline/reports/pipeline_report.py` | Extended with regression section, consistency report, and optimization recommendations | Report JSON schema extended with new top-level keys |
| CLI (`cli.py`) | New subcommands: `atomik-gen agents list`, `atomik-gen optimize`, `atomik-gen regression` | New command groups added alongside existing pipeline/metrics/demo groups |

---

## 9. New Directory Structure

### 9.1 New Files (25 files)

```
software/atomik_sdk/
├── pipeline/
│   ├── orchestrator.py              # [NEW] Event-driven DAG scheduler
│   ├── event_bus.py                 # [NEW] Publish-subscribe event system
│   ├── dag.py                       # [NEW] Task DAG builder with cycle detection
│   ├── feedback.py                  # [NEW] Feedback loop controller (depth 3)
│   ├── diagnosis.py                 # [NEW] Error classification and LLM diagnosis
│   ├── coordinator.py               # [NEW] Coordinator agent (decompose, dispatch, aggregate)
│   ├── consensus.py                 # [NEW] Multi-specialist consensus resolution
│   ├── agents/
│   │   ├── adaptive_router.py       # [NEW] Multi-signal adaptive model router
│   │   ├── complexity_scorer.py     # [NEW] Schema/task complexity scoring
│   │   ├── token_predictor.py       # [NEW] Predictive token estimation
│   │   ├── prompt_cache.py          # [NEW] Prompt caching with TTL
│   │   ├── context_compressor.py    # [NEW] Progressive context compression
│   │   ├── registry.py              # [NEW] Specialist agent registry
│   │   ├── specialist.py            # [NEW] Specialist base class
│   │   └── specialists/
│   │       ├── __init__.py          # [NEW] Package init
│   │       ├── python_gen.py        # [NEW] Python generation specialist
│   │       ├── rust_gen.py          # [NEW] Rust generation specialist
│   │       └── verilog_gen.py       # [NEW] Verilog generation + synthesis specialist
│   ├── analysis/
│   │   ├── __init__.py              # [NEW] Package init
│   │   ├── field_diff.py            # [NEW] Field-level structural diff
│   │   ├── diff_impact.py           # [NEW] Diff-to-generator impact mapping
│   │   ├── metrics_analyzer.py      # [NEW] Cross-run trend analysis
│   │   └── regression_detector.py   # [NEW] Regression detection (statistical)
│   ├── knowledge/
│   │   ├── __init__.py              # [NEW] Package init
│   │   ├── error_kb.py              # [NEW] Error pattern knowledge base
│   │   ├── fuzzy_match.py           # [NEW] Fuzzy error matching
│   │   └── error_patterns.json      # [NEW] Seed KB (5 Phase 4C patterns)
│   ├── parallel/
│   │   ├── __init__.py              # [NEW] Package init
│   │   ├── decomposer.py            # [NEW] Work decomposition engine
│   │   ├── executor.py              # [NEW] Parallel task executor
│   │   └── worker.py                # [NEW] Worker process wrapper
│   ├── verification/
│   │   ├── __init__.py              # [NEW] Package init
│   │   ├── deep_verify.py           # [NEW] Deep verification orchestrator
│   │   ├── consistency.py           # [NEW] Cross-language consistency checker
│   │   ├── runners/
│   │   │   ├── __init__.py          # [NEW] Package init
│   │   │   ├── python_runner.py     # [NEW] pytest + coverage runner
│   │   │   ├── rust_runner.py       # [NEW] cargo test runner
│   │   │   ├── c_runner.py          # [NEW] gcc + make test runner
│   │   │   ├── js_runner.py         # [NEW] node --check + npm test runner
│   │   │   └── verilog_runner.py    # [NEW] iverilog + vvp runner
│   │   └── extractors/
│   │       ├── __init__.py          # [NEW] Package init
│   │       ├── python_extractor.py  # [NEW] Python interface extractor
│   │       ├── rust_extractor.py    # [NEW] Rust interface extractor
│   │       ├── c_extractor.py       # [NEW] C interface extractor
│   │       ├── js_extractor.py      # [NEW] JavaScript interface extractor
│   │       └── verilog_extractor.py # [NEW] Verilog interface extractor
│   ├── regression/
│   │   ├── __init__.py              # [NEW] Package init
│   │   ├── detector.py              # [NEW] Regression detection
│   │   └── baseline.py              # [NEW] Baseline snapshot management
│   ├── optimization/
│   │   ├── __init__.py              # [NEW] Package init
│   │   ├── self_optimizer.py        # [NEW] Pipeline self-optimization
│   │   └── tuner.py                 # [NEW] Configuration auto-tuning
│   └── context/
│       ├── intelligent_manager.py   # [NEW] Intelligent context window manager
│       └── segment_tracker.py       # [NEW] Context segment tracking
```

**Total new files: 52** (25 substantive modules + 12 `__init__.py` + 5 verification runners + 5 extractors + 3 specialists + 1 JSON seed + 1 package init)

### 9.2 Modified Files (14 files)

| File | Changes |
|------|---------|
| `software/atomik_sdk/pipeline/controller.py` | Delegate to orchestrator; preserve CLI adapter |
| `software/atomik_sdk/pipeline/stages/diff.py` | Delegate to field-level engine; preserve section-level fallback |
| `software/atomik_sdk/pipeline/stages/verify.py` | Integrate with deep verification engine |
| `software/atomik_sdk/pipeline/agents/self_correct.py` | Integrate with feedback loop and knowledge base |
| `software/atomik_sdk/pipeline/agents/router.py` | Delegate to adaptive router; preserve static fallback |
| `software/atomik_sdk/pipeline/agents/token_budget.py` | Add prediction, caching, and compression integration |
| `software/atomik_sdk/pipeline/context/manifest.py` | Add segment-level metadata; bump to v3.0 |
| `software/atomik_sdk/pipeline/context/cache.py` | Integrate relevance scoring for eviction |
| `software/atomik_sdk/pipeline/context/checkpoint.py` | Add regression baseline snapshots |
| `software/atomik_sdk/pipeline/metrics/collector.py` | Add cross-run trend data fields |
| `software/atomik_sdk/pipeline/metrics/reporter.py` | Add trends, regression alerts, optimization recs |
| `software/atomik_sdk/pipeline/reports/pipeline_report.py` | Add regression, consistency, and optimization sections |
| `software/atomik_sdk/pipeline/orchestrator.py` | Integrate coordinator as top-level dispatch (created in T5.1, modified in T5.10) |
| `software/atomik_sdk/cli.py` | Add `agents list`, `optimize`, `regression` subcommands |

### 9.3 New Test Files (12 files)

| File | Est. Tests | Coverage |
|------|-----------|----------|
| `tests/test_orchestrator.py` | 5 | DAG scheduling, event dispatch, cycle detection |
| `tests/test_feedback.py` | 5 | Feedback loop, retry depth, KB integration |
| `tests/test_adaptive_router.py` | 4 | Multi-signal routing, budget pressure, fallback |
| `tests/test_token_efficiency.py` | 4 | Prediction accuracy, cache hits, compression |
| `tests/test_error_kb.py` | 5 | CRUD, fuzzy match, auto-learn, versioning |
| `tests/test_field_diff.py` | 4 | Path tracking, impact mapping, fallback |
| `tests/test_metrics_analyzer.py` | 4 | Trends, anomalies, moving averages |
| `tests/test_parallel_executor.py` | 4 | Parallel dispatch, partial failure, timeout |
| `tests/test_agent_registry.py` | 4 | Registration, capability match, health check |
| `tests/test_deep_verify.py` | 5 | Per-language runners, adaptive depth |
| `tests/test_consistency.py` | 4 | Cross-language comparison, naming conventions |
| `tests/test_self_optimizer.py` | 4 | Bottleneck analysis, tuning, recommendations |
| **Total** | **52** | |

### 9.4 Estimated Lines of Code

| Category | Files | Est. Lines |
|----------|-------|-----------|
| Core orchestration (orchestrator, DAG, event bus, coordinator, consensus) | 5 | 900 |
| Feedback & diagnosis (feedback, diagnosis) | 2 | 350 |
| Agents (adaptive router, complexity scorer, token predictor, prompt cache, context compressor, registry, specialist, 3 specialists) | 10 | 800 |
| Analysis (field diff, diff impact, metrics analyzer, regression detector) | 4 | 500 |
| Knowledge base (error KB, fuzzy match, seed JSON) | 3 | 350 |
| Parallel execution (decomposer, executor, worker) | 3 | 400 |
| Verification (deep verify, consistency, 5 runners, 5 extractors) | 12 | 700 |
| Regression (detector, baseline) | 2 | 250 |
| Optimization (self optimizer, tuner) | 2 | 300 |
| Context (intelligent manager, segment tracker) | 2 | 250 |
| Package inits | 12 | 60 |
| Tests | 12 | 600 |
| **Total new lines** | **67** | **~5,460** |

---

## 10. Success Criteria

Phase 5 is complete when:

1. **Event-driven orchestrator** dispatches tasks via DAG with automatic parallelism (no sequential bottleneck for independent tasks)
2. **Feedback loop** resolves injected lint/test errors within 3 retries without human intervention
3. **Knowledge base** contains 15+ error patterns (5 seed + 10+ learned) with >80% fuzzy match accuracy
4. **Adaptive router** reduces per-run token cost by 40% vs. Phase 4C baseline (measured over 10 runs)
5. **Parallel execution** achieves 3x pipeline throughput improvement vs. Phase 4C sequential execution (measured)
6. **Deep verification** runs native toolchain checks for all 5 languages (pytest, cargo test, gcc, node --check, iverilog+vvp)
7. **Cross-language consistency** detects and reports interface mismatches across all 5 language outputs
8. **Regression detection** flags test count drops, hardware metric degradation, and token cost increases
9. **Self-optimization** produces actionable tuning recommendations after every 5 pipeline runs
10. **All existing tests (124+) continue to pass** alongside 50+ new Phase 5 tests

---

## 11. Validation Gates

| Gate | Metric | Threshold |
|------|--------|-----------|
| DAG scheduling | Independent tasks dispatched in parallel | Verified by timing |
| Cycle detection | Invalid DAGs rejected at construction | 100% |
| Feedback loop termination | Max retry depth respected | 3 retries max |
| Feedback loop learning | Known fixes resolved at 0 tokens | Verified on 5 seed patterns |
| Adaptive routing accuracy | Router selects appropriate tier | 90%+ correct selections |
| Token prediction accuracy | Pre-task estimates vs. actual | Within 25% |
| Prompt cache hit rate | Cache hits on repeated schemas | >30% |
| Context compression ratio | Prompt size reduction under pressure | >40% |
| Token cost reduction | Per-run cost vs. Phase 4C | >40% reduction |
| Pipeline throughput | End-to-end time vs. Phase 4C | >3x improvement |
| Deep verification coverage | All 5 languages verified by native tools | 100% |
| Cross-language consistency | Interface mismatches detected | 100% true positives |
| Consistency false positives | Naming convention differences not flagged | <5% false positive rate |
| Regression detection | Test count drops detected | 100% |
| Hardware regression | Fmax/LUT degradation flagged | >5% deviation detected |
| Knowledge base growth | Patterns learned from novel errors | 10+ new patterns |
| Self-optimization | Actionable recommendations produced | Every 5 runs |
| Existing tests | All pre-existing tests pass | 124+ pass |
| New tests | Phase 5 test suite passes | 50+ pass |
| Integration | Full pipeline with feedback + parallelism | End-to-end success |

---

## Phase 4C -> Phase 5 Transformation Summary

| Phase 4C (Current) | Phase 5 (Target) |
|---------------------|-------------------|
| Sequential 6-stage loop | Event-driven orchestrator with DAG |
| Execute-once, no retries | Generate -> Verify -> Diagnose -> Fix -> Retry (depth 3) |
| Static model routing | Adaptive routing (complexity + error history + budget pressure) |
| 5 hardcoded fix patterns | Extensible knowledge base with fuzzy matching + learning |
| Syntax-only verification | Deep verify (pytest, gcc, cargo check, node --check) |
| Single-threaded stages | Parallel per-language generation and verification |
| No cross-run learning | Metrics trends, anomaly detection, regression alerts |
| Passive budget check | Predictive budget, prompt caching, context compression |
| Section-level diffing | Field-level structural diff with path tracking |
| Single agent model | Coordinator + specialist agents + consensus |

---

**Document Version:** 1.0
**Date:** January 27, 2026
**Author:** Claude Opus 4.5
**Phase:** 5 - Autonomous Token-Efficient Agentic Orchestration
**Status:** PLANNED
