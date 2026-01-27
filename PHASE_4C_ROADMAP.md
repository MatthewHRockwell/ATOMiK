# Phase 4C Roadmap: Autonomous Token-Efficient Pipelines for Agentic Execution

**Version:** 1.0
**Date:** January 26, 2026
**Status:** PLANNED
**Budget:** $80 (estimated ~120K tokens)
**Primary Agents:** SDK Agent (Sonnet 4.5), Validator Agent (Haiku 4.5)
**Dependencies:** Phase 4A (Generator Framework), Phase 4B (Domain SDKs)

---

## Executive Summary

Phase 4C transforms ATOMiK's existing generation and validation tooling into an autonomous, self-coordinating pipeline system. The goal is to enable end-to-end schema-to-validated-output execution with zero human intervention during routine operations, while minimizing token consumption through differential processing, structured caching, and compressed context protocols.

**Core thesis:** The ATOMiK pipeline currently requires human orchestration between steps (schema authoring, validation, generation, testing, hardware deployment). Phase 4C replaces this manual coordination with an autonomous agent pipeline that selects the right model tier for each subtask, caches intermediate artifacts, and self-corrects on failure — achieving sub-15K token cost per new domain schema end-to-end.

---

## Table of Contents

1. [Design Principles](#1-design-principles)
2. [Architecture Overview](#2-architecture-overview)
3. [Task Breakdown](#3-task-breakdown)
4. [Token Efficiency Strategy](#4-token-efficiency-strategy)
5. [Agent Coordination Model](#5-agent-coordination-model)
6. [Pipeline Stages](#6-pipeline-stages)
7. [Deliverables](#7-deliverables)
8. [Validation Gates](#8-validation-gates)
9. [Risk Analysis](#9-risk-analysis)
10. [Integration with Previous Phases](#10-integration-with-previous-phases)

---

## 1. Design Principles

### 1.1 Autonomous by Default

Every pipeline stage must be executable without human input. Human intervention is an escalation path, not a requirement. The pipeline should:

- Accept a schema file (or directory) as its sole input
- Produce validated, tested, lint-clean output in all target languages
- Generate a structured report of what it did, what passed, and what failed
- Self-correct on recoverable failures (lint errors, test failures with obvious fixes)

### 1.2 Token-Efficient at Every Layer

Token cost is a first-class constraint, not an afterthought. Every design decision must answer: "How does this minimize tokens while preserving correctness?"

| Layer | Strategy |
|-------|----------|
| **Model selection** | Route tasks to cheapest capable model (Haiku for lint/validation, Sonnet for generation, Opus only for novel reasoning) |
| **Context loading** | Structured manifests instead of prose summaries; delta-based context updates |
| **Generation** | Differential regeneration (only changed files); template caching |
| **Validation** | Deterministic checks run locally (no LLM); LLM only for diagnosis |
| **Reporting** | Structured JSON, not narrative prose |

### 1.3 Artifact-Driven Coordination

Agents communicate through artifacts on disk, not through conversation context. Every inter-agent handoff is a file (JSON manifest, generated code, test result) — never a prompt injection or context window relay.

### 1.4 Fail-Fast, Self-Correct, Then Escalate

```
Stage fails → Retry with diagnosis (max 2 retries) → Escalate to higher-tier model → Escalate to human
```

Routine failures (lint, test) are handled autonomously. Novel failures (new error classes, ambiguous requirements) escalate immediately.

---

## 2. Architecture Overview

### 2.1 Pipeline Topology

```
                         ┌─────────────────────────┐
                         │    Pipeline Controller   │
                         │    (Orchestrator Agent)   │
                         │                           │
                         │  - Schema intake           │
                         │  - Stage dispatch           │
                         │  - Token accounting          │
                         │  - Failure routing           │
                         └──────────┬────────────────┘
                                    │
               ┌────────────────────┼────────────────────┐
               │                    │                    │
               ▼                    ▼                    ▼
     ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
     │   Validate    │    │   Generate    │    │   Verify     │
     │   Stage       │    │   Stage       │    │   Stage      │
     │               │    │               │    │              │
     │  SchemaValidator   │  GeneratorEngine   │  Test runner  │
     │  Spec check   │    │  5-lang emit  │    │  Lint check  │
     │  Diff detect  │    │  Diff regen   │    │  Self-correct│
     └──────┬───────┘    └──────┬───────┘    └──────┬───────┘
            │                    │                    │
            ▼                    ▼                    ▼
     ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
     │  Validation   │    │  Generated    │    │  Test Results │
     │  Manifest     │    │  Artifacts    │    │  Manifest     │
     │  (.json)      │    │  (code files) │    │  (.json)      │
     └──────────────┘    └──────────────┘    └──────────────┘
                                    │
                                    ▼
                         ┌──────────────────┐
                         │  Hardware Stage   │
                         │  (Optional)       │
                         │                   │
                         │  RTL simulation   │
                         │  FPGA program     │
                         │  HW validation    │
                         └──────────────────┘
```

### 2.2 Agent-to-Model Routing

| Pipeline Stage | Default Model | Escalation Model | Token Budget |
|----------------|--------------|-------------------|--------------|
| Schema validation | Local (no LLM) | Haiku 4.5 | 0 / 2K |
| Diff detection | Local (no LLM) | — | 0 |
| Code generation | Sonnet 4.5 | Opus 4.5 | 8K / 20K |
| Lint & format | Local (no LLM) | Haiku 4.5 | 0 / 1K |
| Test execution | Local (no LLM) | Sonnet 4.5 | 0 / 5K |
| Failure diagnosis | Haiku 4.5 | Sonnet 4.5 | 2K / 8K |
| Report generation | Local (no LLM) | — | 0 |
| Hardware validation | Local (no LLM) | Sonnet 4.5 | 0 / 5K |

**Target: 80%+ of pipeline stages execute with zero LLM tokens** (deterministic local execution).

### 2.3 Directory Structure

```
software/atomik_sdk/
├── pipeline/                    # NEW: Autonomous pipeline system
│   ├── __init__.py
│   ├── controller.py            # Pipeline orchestrator
│   ├── stages/
│   │   ├── __init__.py
│   │   ├── validate.py          # Schema validation stage
│   │   ├── diff.py              # Differential change detection
│   │   ├── generate.py          # Code generation stage
│   │   ├── verify.py            # Test + lint verification stage
│   │   └── hardware.py          # Hardware-in-the-loop stage
│   ├── context/
│   │   ├── __init__.py
│   │   ├── manifest.py          # Structured context manifests
│   │   ├── cache.py             # Artifact caching layer
│   │   └── checkpoint.py        # Cross-session state checkpoints
│   ├── agents/
│   │   ├── __init__.py
│   │   ├── router.py            # Model selection / routing logic
│   │   ├── token_budget.py      # Token accounting and budgeting
│   │   └── self_correct.py      # Failure diagnosis and retry
│   └── reports/
│       ├── __init__.py
│       └── pipeline_report.py   # Structured JSON report generation
├── cli.py                       # EXTEND: Add pipeline subcommands
└── ...
```

---

## 3. Task Breakdown

### T4C.1: Pipeline Controller & Stage Dispatcher

**Agent:** SDK Agent (Sonnet 4.5)
**Estimated tokens:** 20K
**Dependencies:** Phase 4A GeneratorEngine, Phase 4B CLI tool

**Description:**
Core orchestrator that accepts a schema (or directory of schemas), dispatches work through a sequence of stages, tracks progress, and produces a final report. Implements the fail-fast/self-correct/escalate pattern.

**Deliverables:**
- `software/atomik_sdk/pipeline/controller.py` — Pipeline orchestration logic
- `software/atomik_sdk/pipeline/stages/__init__.py` — Stage protocol (base class/interface)
- CLI extension: `atomik-gen pipeline <schema|directory>` subcommand

**Acceptance criteria:**
- Pipeline runs end-to-end on an existing domain schema with no manual steps
- Each stage produces a structured manifest (JSON) consumed by the next stage
- Pipeline exits with code 0 on success, structured error on failure
- Total pipeline execution for a single schema completes without hanging

### T4C.2: Differential Change Detection

**Agent:** SDK Agent (Sonnet 4.5)
**Estimated tokens:** 12K
**Dependencies:** T4C.1

**Description:**
Detects what changed between a schema revision and the previously generated output. Produces a diff manifest that tells the generation stage which languages/files need regeneration. Compares schema content (not timestamps) using structural diff.

**Deliverables:**
- `software/atomik_sdk/pipeline/stages/diff.py` — Schema diff engine
- Diff manifest format specification (JSON)

**Key behaviors:**
- **No change detected:** Pipeline short-circuits, reports "up to date," 0 tokens consumed
- **Field change:** Regenerate all languages (field names affect all emitters)
- **Metadata-only change:** Regenerate only affected files (e.g., description changes affect Python docstrings but not Verilog)
- **Hardware section change:** Regenerate only Verilog output + constraint files
- **New operation added:** Regenerate all languages (operations are cross-cutting)

**Acceptance criteria:**
- Correctly identifies "no change" for identical schema re-runs
- Correctly identifies field-level, metadata, and hardware changes
- Diff manifest consumed by generation stage to skip unchanged outputs

### T4C.3: Selective Generation Stage

**Agent:** SDK Agent (Sonnet 4.5)
**Estimated tokens:** 15K
**Dependencies:** T4C.1, T4C.2

**Description:**
Wraps GeneratorEngine with selective execution. Reads the diff manifest to determine which generators to invoke. Tracks per-file generation status and produces an artifact manifest.

**Deliverables:**
- `software/atomik_sdk/pipeline/stages/generate.py` — Selective generation wrapper
- Artifact manifest format (JSON listing all generated files with checksums)

**Key behaviors:**
- Full generation: Invoked when no previous output exists or diff indicates cross-cutting change
- Selective generation: Only invoke generators for affected languages
- Artifact checksums: SHA256 of each generated file for downstream verification
- Idempotent: Running twice with same input produces identical output

**Acceptance criteria:**
- Selective generation produces identical output to full generation for unchanged languages
- Token savings measurable: metadata-only change uses <50% of full generation tokens
- Artifact manifest includes file path, language, checksum, and generation timestamp

### T4C.4: Self-Validating Verification Stage

**Agent:** Validator Agent (Haiku 4.5) for diagnosis only
**Estimated tokens:** 5K (diagnosis budget; most verification is local)
**Dependencies:** T4C.1, T4C.3

**Description:**
Runs lint, type-check, and test execution against generated outputs. On failure, diagnoses the issue and attempts self-correction (up to 2 retries). Produces a verification manifest.

**Deliverables:**
- `software/atomik_sdk/pipeline/stages/verify.py` — Verification orchestrator
- `software/atomik_sdk/pipeline/agents/self_correct.py` — Failure diagnosis and retry logic
- Verification manifest format (JSON)

**Verification checks (all local, no LLM):**

| Language | Lint | Type Check | Test |
|----------|------|------------|------|
| Python | `ruff check` | `mypy` (optional) | `pytest` |
| Rust | `cargo check` | Built-in | `cargo test` |
| C | Compiler warnings (`-Wall -Werror`) | Built-in | `make test` |
| JavaScript | `eslint` (optional) | — | `npm test` |
| Verilog | `verilator --lint-only` | — | `iverilog` + `vvp` |

**Self-correction protocol:**
1. Run verification checks (local, 0 tokens)
2. On failure: extract error message, classify error type
3. If known error class (import path, naming convention, missing semicolon): apply deterministic fix locally
4. If unknown error class: invoke Haiku for diagnosis, apply suggested fix
5. Re-run verification
6. After 2 failed retries: escalate to Sonnet with full error context

**Acceptance criteria:**
- All existing domain schemas pass verification with 0 LLM tokens
- Known error classes handled without LLM (documented list of auto-fixes)
- Self-correction succeeds on injected lint errors (test with intentional defect)

### T4C.5: Context Compression Protocol

**Agent:** SDK Agent (Sonnet 4.5)
**Estimated tokens:** 10K
**Dependencies:** T4C.1

**Description:**
Defines and implements the structured context format that agents use to resume work across sessions. Replaces prose-based conversation summaries with structured manifests that can be loaded in <2K tokens.

**Deliverables:**
- `software/atomik_sdk/pipeline/context/manifest.py` — Structured context manifest
- `software/atomik_sdk/pipeline/context/cache.py` — Artifact cache with TTL
- `software/atomik_sdk/pipeline/context/checkpoint.py` — Cross-session checkpoints
- Context manifest JSON schema specification

**Manifest structure:**

```json
{
  "version": "1.0",
  "project_state": {
    "phase": "4C",
    "schemas_registered": 4,
    "languages_supported": ["python", "rust", "c", "javascript", "verilog"],
    "last_pipeline_run": "2026-01-26T14:30:00Z"
  },
  "artifact_index": {
    "schemas": {
      "video-h264-delta": {"sha256": "...", "path": "...", "last_generated": "..."},
      "edge-sensor-imu": {"sha256": "...", "path": "...", "last_generated": "..."},
      "finance-price-tick": {"sha256": "...", "path": "...", "last_generated": "..."}
    },
    "generated": {
      "video-h264-delta/python": {"files": 3, "checksum": "...", "status": "verified"},
      "...": "..."
    }
  },
  "token_ledger": {
    "session_total": 0,
    "pipeline_runs": [],
    "budget_remaining": 80.00
  },
  "pending_actions": []
}
```

**Key design decisions:**
- **Structured, not narrative:** Machine-readable JSON, not human-readable prose
- **Delta updates:** Checkpoint updates only write changed fields, not full state
- **Artifact-indexed:** Every generated file tracked by content hash, not path alone
- **Token-aware:** Built-in token accounting across sessions

**Acceptance criteria:**
- Cold-start context loading uses <2K tokens
- Checkpoint creation is idempotent (same state = same checkpoint)
- Artifact cache correctly invalidates on schema change

### T4C.6: Token Budget & Model Router

**Agent:** SDK Agent (Sonnet 4.5)
**Estimated tokens:** 10K
**Dependencies:** T4C.1

**Description:**
Implements model selection logic and per-pipeline token accounting. Routes each subtask to the cheapest model that can handle it. Tracks cumulative token usage and enforces budget limits.

**Deliverables:**
- `software/atomik_sdk/pipeline/agents/router.py` — Model routing logic
- `software/atomik_sdk/pipeline/agents/token_budget.py` — Token accounting

**Routing rules:**

```
Task Classification → Model Selection:

DETERMINISTIC (lint, test, diff, report)     → No LLM (local execution)
MECHANICAL (known template, known fix)        → Haiku 4.5
GENERATIVE (code generation, new patterns)    → Sonnet 4.5
NOVEL (unknown error, architectural decision) → Opus 4.5 (with human approval)
```

**Token accounting:**
- Pre-execution estimate: Each stage declares estimated token cost
- Real-time tracking: Actual tokens consumed logged per stage
- Budget enforcement: Pipeline aborts if projected cost exceeds remaining budget
- Session ledger: Cumulative cost across pipeline runs persisted to checkpoint

**Acceptance criteria:**
- 80%+ of pipeline stages execute with 0 LLM tokens
- Token estimates within 30% of actual consumption
- Pipeline aborts cleanly when budget would be exceeded (no partial execution)

### T4C.7: Hardware-in-the-Loop Agent Stage

**Agent:** SDK Agent (Sonnet 4.5)
**Estimated tokens:** 15K
**Dependencies:** T4C.1, T4C.4, Phase 3 FPGA pipeline

**Description:**
Integrates the existing `scripts/fpga_pipeline.py` as a pipeline stage. Automates the flow from generated Verilog to RTL simulation, synthesis readiness check, and (when hardware is connected) FPGA programming and validation.

**Deliverables:**
- `software/atomik_sdk/pipeline/stages/hardware.py` — Hardware stage wrapper
- Integration with `scripts/fpga_pipeline.py` subprocess execution
- Hardware validation manifest format (JSON)

**Stage modes:**
- **Simulation only** (default): Run `iverilog` + `vvp` on generated testbenches. No hardware required.
- **Full hardware** (opt-in): Program FPGA, run hardware test suite, collect results. Requires Tang Nano 9K connected.
- **Synthesis check** (opt-in): Run Gowin synthesis scripts, verify timing closure. Requires Gowin EDA installed.

**Key behaviors:**
- Hardware stage is optional — pipeline succeeds without it
- Simulation failures block hardware programming (fail-fast)
- Hardware test results fed back into verification manifest
- COM port auto-detection via existing `fpga_pipeline.py` logic

**Acceptance criteria:**
- Simulation mode runs successfully on all 3 domain schemas' Verilog output
- Hardware mode (when hardware connected) programs and validates autonomously
- Stage produces structured manifest with pass/fail per test

### T4C.8: Pipeline CLI & Integration

**Agent:** SDK Agent (Sonnet 4.5)
**Estimated tokens:** 12K
**Dependencies:** T4C.1 through T4C.7

**Description:**
Extends `atomik-gen` CLI with pipeline subcommands. Integrates all stages into a cohesive user experience. Adds pipeline-specific options and reporting.

**Deliverables:**
- CLI extension: `atomik-gen pipeline` subcommand with options
- `atomik-gen pipeline status` — Show current pipeline state from checkpoint
- `atomik-gen pipeline run <schema>` — Execute full pipeline
- `atomik-gen pipeline diff <schema>` — Show what would change (dry run)
- Updated documentation for pipeline commands

**CLI interface:**

```bash
# Full autonomous pipeline execution
atomik-gen pipeline run sdk/schemas/domains/video-h264-delta.json

# Batch pipeline for all schemas in a directory
atomik-gen pipeline run sdk/schemas/domains/ --batch

# Dry run: show what would be generated/tested
atomik-gen pipeline diff sdk/schemas/domains/video-h264-delta.json

# Show pipeline state and token usage
atomik-gen pipeline status

# Pipeline with hardware validation enabled
atomik-gen pipeline run schema.json --hardware --com-port COM5

# Pipeline with explicit token budget
atomik-gen pipeline run schema.json --token-budget 15000

# Pipeline targeting specific languages only
atomik-gen pipeline run schema.json --languages python rust
```

**Options:**

| Option | Description | Default |
|--------|-------------|---------|
| `--batch` | Process all schemas in directory | single file |
| `--hardware` | Enable hardware-in-the-loop stage | disabled |
| `--sim-only` | Run RTL simulation but skip FPGA | disabled |
| `--com-port PORT` | Serial port for hardware validation | auto-detect |
| `--token-budget N` | Maximum tokens for this pipeline run | unlimited |
| `--skip-verify` | Skip lint/test verification stage | enabled |
| `--dry-run` | Show plan without executing | false |
| `--report FILE` | Write pipeline report to file | stdout |
| `--checkpoint DIR` | Directory for pipeline checkpoints | `.atomik/` |

**Acceptance criteria:**
- `atomik-gen pipeline run` succeeds end-to-end on all 3 domain schemas
- `atomik-gen pipeline diff` correctly reports "up to date" on unchanged schemas
- `atomik-gen pipeline status` displays token usage and artifact state
- Pipeline report includes per-stage timing, token usage, and pass/fail status

### T4C.9: Test Suite & Documentation

**Agent:** Validator Agent (Haiku 4.5), Documenter Agent (Haiku 4.5)
**Estimated tokens:** 8K
**Dependencies:** T4C.1 through T4C.8

**Description:**
Comprehensive test coverage for the pipeline system and updated documentation across the project.

**Deliverables:**
- `software/atomik_sdk/tests/test_pipeline_controller.py` — Controller tests
- `software/atomik_sdk/tests/test_pipeline_diff.py` — Diff detection tests
- `software/atomik_sdk/tests/test_pipeline_verify.py` — Verification stage tests
- `software/atomik_sdk/tests/test_pipeline_context.py` — Context/cache tests
- Updated `docs/SDK_DEVELOPER_GUIDE.md` — Pipeline architecture section
- Updated `docs/SDK_API_REFERENCE.md` — Pipeline CLI reference
- Updated `README.md` — Pipeline section in Developer Tooling
- Phase 4C completion report

**Test scenarios:**
- Pipeline end-to-end on existing domain schema (happy path)
- Pipeline with schema modification (differential regeneration)
- Pipeline with no changes (short-circuit)
- Pipeline with injected lint error (self-correction)
- Pipeline with injected test failure (diagnosis and escalation)
- Pipeline budget enforcement (abort on over-budget)
- Context checkpoint save/restore cycle
- Hardware stage simulation mode

**Acceptance criteria:**
- 95%+ test coverage on pipeline modules
- All existing 87+ tests continue to pass
- Documentation accurately reflects pipeline architecture and CLI

---

## 4. Token Efficiency Strategy

### 4.1 Token Budget Breakdown

| Task | Estimated Tokens | LLM Model | Local (0 tokens) |
|------|-----------------|-----------|-------------------|
| T4C.1 Controller | 20K | Sonnet | — |
| T4C.2 Diff Detection | 12K | Sonnet | Diff logic is local |
| T4C.3 Selective Generation | 15K | Sonnet | — |
| T4C.4 Self-Validating | 5K | Haiku (diagnosis only) | Lint/test is local |
| T4C.5 Context Protocol | 10K | Sonnet | — |
| T4C.6 Token Router | 10K | Sonnet | Routing logic is local |
| T4C.7 Hardware Stage | 15K | Sonnet | Simulation is local |
| T4C.8 CLI Integration | 12K | Sonnet | — |
| T4C.9 Tests & Docs | 8K | Haiku | — |
| **Total** | **107K** | | |
| **Contingency (15%)** | **+13K** | | |
| **Grand Total** | **~120K** | | **Budget: $80** |

### 4.2 Steady-State Pipeline Token Cost

Once Phase 4C infrastructure is built, the per-pipeline-run token cost drops dramatically:

| Scenario | Tokens | Cost |
|----------|--------|------|
| **New schema (full generation)** | ~12K | ~$0.18 |
| **Schema update (differential)** | ~4K | ~$0.06 |
| **No change (short-circuit)** | 0 | $0.00 |
| **Failure + self-correction** | ~6K | ~$0.09 |
| **Hardware validation** | 0 (local) | $0.00 |

### 4.3 Optimization Techniques

| Technique | Token Savings | Implementation |
|-----------|--------------|----------------|
| **Local-first execution** | 80% of stages | Lint, test, diff, report run without LLM |
| **Differential regeneration** | 50-90% per update | Only regenerate changed languages/files |
| **Structured context** | 70% context loading | JSON manifests vs. prose summaries |
| **Model tiering** | 60% per diagnosis | Haiku for known errors, Sonnet for generation |
| **Short-circuit detection** | 100% when unchanged | Hash comparison skips entire pipeline |
| **Template caching** | 30% per generation | Reuse common code patterns across schemas |

---

## 5. Agent Coordination Model

### 5.1 Agent Roles in Pipeline

| Agent | Pipeline Role | Model | Invocation Trigger |
|-------|--------------|-------|-------------------|
| **Pipeline Controller** | Orchestration | Local (no LLM) | CLI command |
| **Validator Agent** | Schema validation, failure diagnosis | Haiku 4.5 | Stage failure |
| **SDK Agent** | Code generation, infrastructure creation | Sonnet 4.5 | Generation stage |
| **Synthesis Agent** | Hardware synthesis analysis | Sonnet 4.5 | Hardware stage (synthesis check) |
| **Documenter Agent** | Report generation | Haiku 4.5 | Pipeline completion |

### 5.2 Artifact-Based Handoff Protocol

Agents never communicate through conversation context. All inter-stage communication flows through typed artifacts:

```
Stage N produces:
  → Manifest file (JSON): declares outputs, status, checksums
  → Artifact files: generated code, test results, etc.

Stage N+1 consumes:
  ← Previous stage manifest
  ← Referenced artifact files
  ← Pipeline checkpoint (for context restoration)
```

**Manifest schema:**

```json
{
  "stage": "generate",
  "status": "success",
  "timestamp": "2026-01-26T14:30:00Z",
  "tokens_consumed": 8200,
  "artifacts": [
    {
      "path": "generated_sdks/atomik/Video/Streaming/h264delta.py",
      "language": "python",
      "sha256": "abc123...",
      "action": "created"
    }
  ],
  "errors": [],
  "next_stage": "verify"
}
```

### 5.3 Escalation Chain

```
Local execution (0 tokens)
    │ fails
    ▼
Haiku diagnosis (2K tokens)
    │ fails
    ▼
Sonnet generation (8K tokens)
    │ fails
    ▼
Opus reasoning (20K tokens, requires human approval)
    │ fails
    ▼
Human intervention (pipeline paused)
```

---

## 6. Pipeline Stages

### 6.1 Stage 1: Intake & Validation

**Input:** Schema file path (or directory for batch)
**Output:** Validation manifest
**LLM tokens:** 0 (local SchemaValidator execution)

1. Load schema JSON
2. Validate against `specs/atomik_schema_v1.json` (JSON Schema Draft 7)
3. Run semantic validation (SchemaValidator rules)
4. Compute schema content hash
5. Produce validation manifest

### 6.2 Stage 2: Differential Analysis

**Input:** Validation manifest + pipeline checkpoint
**Output:** Diff manifest
**LLM tokens:** 0 (local hash comparison)

1. Load previous checkpoint for this schema (if exists)
2. Compare schema content hash with checkpoint
3. If unchanged: produce "up-to-date" manifest, pipeline short-circuits
4. If changed: compute structural diff (which sections changed)
5. Map changed sections to affected generators
6. Produce diff manifest listing generators to invoke

**Diff classification:**

| Change Type | Affected Generators | Example |
|-------------|-------------------|---------|
| `delta_fields` | All 5 languages | Added/removed a field |
| `operations` | All 5 languages | Added rollback support |
| `namespace` | All 5 languages | Changed namespace path |
| `hardware` | Verilog only | Changed DATA_WIDTH |
| `constraints` | Verilog only | Changed target frequency |
| `metadata` | Python, JavaScript | Changed description text |

### 6.3 Stage 3: Code Generation

**Input:** Diff manifest + schema file
**Output:** Generated artifacts + artifact manifest
**LLM tokens:** 0 for existing GeneratorEngine (local Python); LLM only if self-correction needed

1. Read diff manifest to determine which generators to invoke
2. Create GeneratorEngine with GeneratorConfig
3. Invoke selected generators
4. Compute SHA256 of each generated file
5. Produce artifact manifest

### 6.4 Stage 4: Verification

**Input:** Artifact manifest + generated files
**Output:** Verification manifest
**LLM tokens:** 0 for passing; 2-8K for failure diagnosis

1. For each language with generated output:
   - Run language-specific lint check
   - Run language-specific tests (if test runner available)
   - Record pass/fail per check
2. If all pass: produce success verification manifest
3. If failure:
   a. Classify error type against known error database
   b. If known: apply deterministic fix, re-verify
   c. If unknown: invoke diagnosis agent (Haiku → Sonnet escalation)
   d. Apply suggested fix, re-verify (max 2 retries)
   e. If still failing: mark as failed, include error context in manifest

### 6.5 Stage 5: Hardware Validation (Optional)

**Input:** Verification manifest + Verilog artifacts
**Output:** Hardware manifest
**LLM tokens:** 0 (local simulation/programming)

1. If `--hardware` or `--sim-only` flag:
   a. Compile Verilog with `iverilog`
   b. Run testbench with `vvp`
   c. If `--hardware` and board detected:
      - Program FPGA via `openFPGALoader`
      - Run hardware test suite via UART
   d. Produce hardware manifest with test results
2. If no hardware flags: skip stage, produce empty manifest

### 6.6 Stage 6: Report & Checkpoint

**Input:** All stage manifests
**Output:** Pipeline report (JSON) + updated checkpoint
**LLM tokens:** 0 (local aggregation)

1. Aggregate all stage manifests into pipeline report
2. Compute totals: files generated, tests passed, tokens consumed, time elapsed
3. Update pipeline checkpoint with new artifact hashes
4. Write report to file (if `--report` specified) or stdout
5. Exit with appropriate code (0 = success, 1 = validation fail, 2 = generation fail, etc.)

---

## 7. Deliverables

### 7.1 New Files

| File | Purpose | Est. Lines |
|------|---------|-----------|
| `software/atomik_sdk/pipeline/__init__.py` | Package init | 10 |
| `software/atomik_sdk/pipeline/controller.py` | Pipeline orchestrator | 300 |
| `software/atomik_sdk/pipeline/stages/__init__.py` | Stage base protocol | 50 |
| `software/atomik_sdk/pipeline/stages/validate.py` | Validation stage | 80 |
| `software/atomik_sdk/pipeline/stages/diff.py` | Diff detection stage | 150 |
| `software/atomik_sdk/pipeline/stages/generate.py` | Generation stage | 120 |
| `software/atomik_sdk/pipeline/stages/verify.py` | Verification stage | 200 |
| `software/atomik_sdk/pipeline/stages/hardware.py` | Hardware stage | 150 |
| `software/atomik_sdk/pipeline/context/manifest.py` | Context manifests | 120 |
| `software/atomik_sdk/pipeline/context/cache.py` | Artifact cache | 100 |
| `software/atomik_sdk/pipeline/context/checkpoint.py` | Checkpoints | 100 |
| `software/atomik_sdk/pipeline/agents/router.py` | Model routing | 80 |
| `software/atomik_sdk/pipeline/agents/token_budget.py` | Token accounting | 100 |
| `software/atomik_sdk/pipeline/agents/self_correct.py` | Self-correction | 150 |
| `software/atomik_sdk/pipeline/reports/pipeline_report.py` | Report generation | 80 |
| **Total new source** | | **~1,790** |

### 7.2 Modified Files

| File | Changes |
|------|---------|
| `software/atomik_sdk/cli.py` | Add `pipeline` subcommand group |
| `software/pyproject.toml` | Add pipeline dependencies (if any) |
| `README.md` | Pipeline section in Developer Tooling |
| `docs/SDK_DEVELOPER_GUIDE.md` | Pipeline architecture section |
| `docs/SDK_API_REFERENCE.md` | Pipeline CLI reference |

### 7.3 Test Files

| File | Tests | Coverage |
|------|-------|----------|
| `test_pipeline_controller.py` | 8-10 | Controller dispatch, stage ordering |
| `test_pipeline_diff.py` | 6-8 | Diff detection, change classification |
| `test_pipeline_verify.py` | 5-7 | Verification, self-correction |
| `test_pipeline_context.py` | 5-7 | Manifest, cache, checkpoint |
| **Total new tests** | **24-32** | |

---

## 8. Validation Gates

| Gate | Metric | Threshold |
|------|--------|-----------|
| Pipeline end-to-end | Completes on all 3 domain schemas | 100% |
| Differential accuracy | Correctly identifies change vs. no-change | 100% |
| Self-correction | Handles injected lint errors without LLM | 3/3 known classes |
| Token efficiency | Full pipeline run per schema | <15K tokens |
| Short-circuit | Unchanged schema uses 0 tokens | 0 |
| Test coverage | Pipeline module coverage | >95% |
| Existing tests | All pre-existing tests pass | 87+ pass |
| Hardware simulation | Verilog testbenches pass in simulation | 3/3 |
| Context checkpoint | Save/restore preserves full state | Round-trip verified |
| CLI integration | All pipeline subcommands functional | 100% |

---

## 9. Risk Analysis

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Self-correction loops (retry forever) | Medium | Medium | Hard cap at 2 retries + escalation |
| Diff misclassification (misses a change) | Low | High | Conservative default: full regeneration if uncertain |
| Token budget exceeded during build | Low | Medium | 15% contingency allocation |
| Hardware stage blocks on missing tools | Medium | Low | Hardware stage is optional; graceful skip |
| Cross-platform CLI differences | Medium | Medium | Test on Windows (MSYS2) primary environment |
| Checkpoint corruption | Low | High | Atomic writes + backup checkpoint |

---

## 10. Integration with Previous Phases

| Phase | Integration Point |
|-------|-------------------|
| **Phase 1** (Lean4 proofs) | Generated code preserves proven algebraic properties; pipeline verification checks XOR algebra invariants |
| **Phase 2** (Benchmarks) | Domain schema constraints informed by benchmark results; pipeline could trigger re-benchmarking on schema change |
| **Phase 3** (FPGA) | Hardware stage wraps Phase 3 infrastructure (fpga_pipeline.py, test_hardware.py); RTL simulation validates generated Verilog |
| **Phase 4A** (Generator) | Pipeline wraps GeneratorEngine as its generation stage; all 5 language generators used unchanged |
| **Phase 4B** (Domains) | 3 domain schemas serve as pipeline test corpus; differential detection validates against existing generated output |

---

## Execution Schedule

| Step | Tasks | Dependencies | Est. Tokens |
|------|-------|-------------|-------------|
| 1 | T4C.1 (Controller) + T4C.5 (Context) + T4C.6 (Router) | Phase 4B complete | 40K |
| 2 | T4C.2 (Diff) + T4C.3 (Generation) | Step 1 | 27K |
| 3 | T4C.4 (Verify) + T4C.7 (Hardware) | Step 2 | 20K |
| 4 | T4C.8 (CLI) + T4C.9 (Tests & Docs) | Step 3 | 20K |
| **Total** | **9 tasks** | | **~107K + 13K contingency** |

Steps 1 and 2 contain independent tasks that can execute in parallel within each step.

---

## Success Criteria

Phase 4C is complete when:

1. `atomik-gen pipeline run sdk/schemas/domains/` succeeds end-to-end with all 3 domain schemas
2. `atomik-gen pipeline diff` correctly reports "up to date" when re-run with no changes (0 tokens)
3. Injecting a schema change triggers selective regeneration (measurably fewer files than full generation)
4. Injecting a lint error triggers self-correction without human intervention
5. Pipeline token cost for a new domain schema is under 15K tokens
6. All 87+ existing tests continue to pass alongside 24+ new pipeline tests
7. Pipeline checkpoint enables cold-start context restoration in <2K tokens
8. Documentation updated across README, Developer Guide, and API Reference

---

**Document Version:** 1.0
**Date:** January 26, 2026
**Author:** Claude Opus 4.5
**Phase:** 4C - Autonomous Token-Efficient Pipelines
**Status:** PLANNED
