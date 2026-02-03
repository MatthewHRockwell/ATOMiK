# Phase 4C Roadmap: Autonomous Token-Efficient Pipelines for Agentic Execution

**Version:** 2.0
**Date:** January 27, 2026
**Status:** PLANNED
**Budget:** $130 (estimated ~200K tokens)
**Primary Agents:** SDK Agent (Sonnet 4.5), Synthesis Agent (Opus 4.5), Validator Agent (Haiku 4.5)
**Dependencies:** Phase 4A (Generator Framework), Phase 4B (Domain SDKs), Phase 3 (FPGA Hardware)

---

## Executive Summary

Phase 4C transforms ATOMiK's existing generation and validation tooling into an autonomous, self-coordinating pipeline system with **mandatory hardware execution** and **comprehensive performance metrics**. The pipeline drives every domain schema from JSON definition through code generation, software verification, RTL simulation, FPGA synthesis, on-device validation, and performance benchmarking — all without human intervention.

Three domain hardware demonstrators (Video, Edge Sensor, Finance) deploy the Phase 4B domain SDKs onto the Tang Nano 9K FPGA with real peripheral I/O, collecting synthesis metrics (LUT utilization, Fmax, timing margins) and runtime performance data (throughput, latency, operations/second) to produce a quantitative performance profile for each domain.

**Core thesis:** The ATOMiK pipeline currently requires human orchestration between steps (schema authoring, validation, generation, testing, hardware deployment). Phase 4C replaces this manual coordination with an autonomous agent pipeline that selects the right model tier for each subtask, caches intermediate artifacts, self-corrects on failure, **validates on real FPGA hardware**, and **reports quantitative performance metrics** — achieving sub-15K token cost per new domain schema end-to-end.

---

## Table of Contents

1. [Design Principles](#1-design-principles)
2. [Architecture Overview](#2-architecture-overview)
3. [Task Breakdown](#3-task-breakdown)
4. [Token Efficiency Strategy](#4-token-efficiency-strategy)
5. [Agent Coordination Model](#5-agent-coordination-model)
6. [Pipeline Stages](#6-pipeline-stages)
7. [Performance Metrics](#7-performance-metrics)
8. [Deliverables](#8-deliverables)
9. [Validation Gates](#9-validation-gates)
10. [Risk Analysis](#10-risk-analysis)
11. [Integration with Previous Phases](#11-integration-with-previous-phases)

---

## 1. Design Principles

### 1.1 Autonomous by Default

Every pipeline stage must be executable without human input. Human intervention is an escalation path, not a requirement. The pipeline should:

- Accept a schema file (or directory) as its sole input
- Produce validated, tested, lint-clean output in all target languages
- Synthesize, program, and validate generated RTL on FPGA hardware
- Collect and report performance metrics at every stage
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
| **Metrics** | All collection and aggregation is local; LLM only for anomaly analysis |
| **Reporting** | Structured JSON, not narrative prose |

### 1.3 Hardware-First Validation

Software verification is necessary but not sufficient. Every generated Verilog module must pass RTL simulation and, when hardware is connected, on-device FPGA validation. Hardware is not optional — it is the primary proof of correctness for the RTL pipeline.

- **RTL simulation** is always mandatory (iverilog + vvp)
- **FPGA synthesis** runs when Gowin EDA is available
- **On-device validation** runs when Tang Nano 9K is connected
- Pipeline reports clearly distinguish simulation-only vs. hardware-validated results

### 1.4 Metrics-Driven

Every pipeline run produces quantitative data. Performance is not anecdotal — it is measured, recorded, and trended.

- **Pipeline metrics:** Token cost, generation time, differential savings, self-correction rate
- **Hardware metrics:** LUT/FF utilization, Fmax, timing margins, power estimates
- **Runtime metrics:** Operations/second, latency per operation, throughput
- **Quality metrics:** Test pass rate, lint error count, lines of code generated

### 1.5 Artifact-Driven Coordination

Agents communicate through artifacts on disk, not through conversation context. Every inter-agent handoff is a file (JSON manifest, generated code, test result) — never a prompt injection or context window relay.

### 1.6 Fail-Fast, Self-Correct, Then Escalate

```
Stage fails -> Retry with diagnosis (max 2 retries) -> Escalate to higher-tier model -> Escalate to human
```

Routine failures (lint, test) are handled autonomously. Novel failures (new error classes, ambiguous requirements) escalate immediately.

---

## 2. Architecture Overview

### 2.1 Pipeline Topology

```
                         ┌──────────────────────────┐
                         │    Pipeline Controller    │
                         │    (Orchestrator Agent)    │
                         │                            │
                         │  - Schema intake            │
                         │  - Stage dispatch            │
                         │  - Token accounting           │
                         │  - Metrics aggregation        │
                         │  - Failure routing             │
                         └──────────┬─────────────────┘
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
     └──────────────┘    └──────────────┘    └──────────────┘
                                    │
                         ┌──────────┴──────────┐
                         ▼                     ▼
              ┌──────────────────┐  ┌──────────────────┐
              │  Hardware Stage  │  │  Metrics Stage   │
              │  (Mandatory)     │  │  (Mandatory)     │
              │                  │  │                  │
              │  RTL simulation  │  │  Token tracking  │
              │  FPGA synthesis  │  │  HW benchmarks   │
              │  On-device test  │  │  Trend analysis  │
              │  Perf profiling  │  │  Report output   │
              └──────────────────┘  └──────────────────┘
                         │                     │
                         └──────────┬──────────┘
                                    ▼
                         ┌──────────────────┐
                         │  Pipeline Report  │
                         │  + Checkpoint     │
                         │  + Metrics CSV    │
                         └──────────────────┘
```

### 2.2 Agent-to-Model Routing

| Pipeline Stage | Default Model | Escalation Model | Token Budget |
|----------------|--------------|-------------------|--------------|
| Schema validation | Local (no LLM) | Haiku 4.5 | 0 / 2K |
| Diff detection | Local (no LLM) | -- | 0 |
| Code generation | Local (GeneratorEngine) | Sonnet 4.5 | 0 / 8K |
| Lint and format | Local (no LLM) | Haiku 4.5 | 0 / 1K |
| Test execution | Local (no LLM) | Sonnet 4.5 | 0 / 5K |
| Failure diagnosis | Haiku 4.5 | Sonnet 4.5 | 2K / 8K |
| RTL simulation | Local (iverilog) | -- | 0 |
| FPGA synthesis | Local (Gowin EDA) | -- | 0 |
| Hardware validation | Local (UART) | Sonnet 4.5 | 0 / 5K |
| Metrics collection | Local (no LLM) | -- | 0 |
| Report generation | Local (no LLM) | -- | 0 |

**Target: 85%+ of pipeline stages execute with zero LLM tokens** (deterministic local execution).

### 2.3 Directory Structure

```
software/atomik_sdk/
├── pipeline/                    # Autonomous pipeline system
│   ├── __init__.py
│   ├── controller.py            # Pipeline orchestrator
│   ├── stages/
│   │   ├── __init__.py
│   │   ├── validate.py          # Schema validation stage
│   │   ├── diff.py              # Differential change detection
│   │   ├── generate.py          # Code generation stage
│   │   ├── verify.py            # Test + lint verification stage
│   │   ├── hardware.py          # Hardware-in-the-loop stage (mandatory)
│   │   └── metrics.py           # Performance metrics collection stage
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
│   ├── metrics/
│   │   ├── __init__.py
│   │   ├── collector.py         # Metrics collection framework
│   │   ├── hardware_bench.py    # FPGA synthesis + runtime benchmarks
│   │   ├── pipeline_bench.py    # Pipeline efficiency benchmarks
│   │   └── reporter.py          # Metrics report generation (JSON + CSV)
│   └── reports/
│       ├── __init__.py
│       └── pipeline_report.py   # Structured JSON report generation
├── cli.py                       # EXTEND: Add pipeline + metrics subcommands
└── ...

demos/                           # Domain hardware demonstrators
├── video/
│   ├── video_demo_top.v         # Top-level: H.264 delta + camera interface
│   ├── camera_interface.v       # OV2640 camera module interface
│   ├── tb_video_demo.v          # Demo testbench
│   └── video_demo.cst           # Pin constraints for camera + HDMI
├── sensor/
│   ├── sensor_demo_top.v        # Top-level: IMU fusion + SPI interface
│   ├── spi_imu_interface.v      # MPU-6050/ICM-20948 SPI driver
│   ├── tb_sensor_demo.v         # Demo testbench
│   └── sensor_demo.cst          # Pin constraints for SPI + GPIO
├── finance/
│   ├── finance_demo_top.v       # Top-level: Price tick + UART stream
│   ├── tick_stream_interface.v  # High-speed tick ingestion
│   ├── tb_finance_demo.v        # Demo testbench
│   └── finance_demo.cst         # Pin constraints for dual UART
└── common/
    ├── perf_counter.v           # Cycle-accurate performance counter module
    ├── throughput_monitor.v     # Operations/second measurement
    └── latency_timer.v          # Per-operation latency measurement
```

---

## 3. Task Breakdown

### T4C.1: Pipeline Controller & Stage Dispatcher

**Agent:** SDK Agent (Sonnet 4.5)
**Estimated tokens:** 20K
**Dependencies:** Phase 4A GeneratorEngine, Phase 4B CLI tool

**Description:**
Core orchestrator that accepts a schema (or directory of schemas), dispatches work through a sequence of stages (including mandatory hardware and metrics stages), tracks progress, and produces a final report. Implements the fail-fast/self-correct/escalate pattern.

**Deliverables:**
- `software/atomik_sdk/pipeline/controller.py` -- Pipeline orchestration logic
- `software/atomik_sdk/pipeline/stages/__init__.py` -- Stage protocol (base class/interface)
- CLI extension: `atomik-gen pipeline <schema|directory>` subcommand

**Acceptance criteria:**
- Pipeline runs end-to-end on an existing domain schema with no manual steps
- Each stage produces a structured manifest (JSON) consumed by the next stage
- Hardware stage executes automatically (simulation always; FPGA when connected)
- Metrics collected and aggregated at every stage
- Pipeline exits with code 0 on success, structured error on failure

### T4C.2: Differential Change Detection

**Agent:** SDK Agent (Sonnet 4.5)
**Estimated tokens:** 12K
**Dependencies:** T4C.1

**Description:**
Detects what changed between a schema revision and the previously generated output. Produces a diff manifest that tells the generation stage which languages/files need regeneration. Compares schema content (not timestamps) using structural diff.

**Deliverables:**
- `software/atomik_sdk/pipeline/stages/diff.py` -- Schema diff engine
- Diff manifest format specification (JSON)

**Key behaviors:**
- **No change detected:** Pipeline short-circuits, reports "up to date," 0 tokens consumed
- **Field change:** Regenerate all languages (field names affect all emitters)
- **Metadata-only change:** Regenerate only affected files (e.g., description changes affect Python docstrings but not Verilog)
- **Hardware section change:** Regenerate only Verilog output + constraint files; trigger re-synthesis
- **New operation added:** Regenerate all languages (operations are cross-cutting)

**Acceptance criteria:**
- Correctly identifies "no change" for identical schema re-runs
- Correctly identifies field-level, metadata, and hardware changes
- Hardware-section changes trigger re-synthesis and re-validation
- Diff manifest consumed by generation stage to skip unchanged outputs

### T4C.3: Selective Generation Stage

**Agent:** SDK Agent (Sonnet 4.5)
**Estimated tokens:** 15K
**Dependencies:** T4C.1, T4C.2

**Description:**
Wraps GeneratorEngine with selective execution. Reads the diff manifest to determine which generators to invoke. Tracks per-file generation status and produces an artifact manifest with performance metrics (generation time, lines of code, file count).

**Deliverables:**
- `software/atomik_sdk/pipeline/stages/generate.py` -- Selective generation wrapper
- Artifact manifest format (JSON listing all generated files with checksums and metrics)

**Key behaviors:**
- Full generation: Invoked when no previous output exists or diff indicates cross-cutting change
- Selective generation: Only invoke generators for affected languages
- Artifact checksums: SHA256 of each generated file for downstream verification
- Idempotent: Running twice with same input produces identical output
- Metrics: Generation wall-clock time, lines of code per language, file count per language

**Acceptance criteria:**
- Selective generation produces identical output to full generation for unchanged languages
- Token savings measurable: metadata-only change uses <50% of full generation tokens
- Artifact manifest includes file path, language, checksum, generation timestamp, and line count

### T4C.4: Self-Validating Verification Stage

**Agent:** Validator Agent (Haiku 4.5) for diagnosis only
**Estimated tokens:** 5K (diagnosis budget; most verification is local)
**Dependencies:** T4C.1, T4C.3

**Description:**
Runs lint, type-check, and test execution against generated outputs. On failure, diagnoses the issue and attempts self-correction (up to 2 retries). Produces a verification manifest with per-language metrics.

**Deliverables:**
- `software/atomik_sdk/pipeline/stages/verify.py` -- Verification orchestrator
- `software/atomik_sdk/pipeline/agents/self_correct.py` -- Failure diagnosis and retry logic
- Verification manifest format (JSON)

**Verification checks (all local, no LLM):**

| Language | Lint | Type Check | Test |
|----------|------|------------|------|
| Python | `ruff check` | `mypy` (optional) | `pytest` |
| Rust | `cargo check` | Built-in | `cargo test` |
| C | Compiler warnings (`-Wall -Werror`) | Built-in | `make test` |
| JavaScript | `eslint` (optional) | -- | `npm test` |
| Verilog | `verilator --lint-only` | -- | `iverilog` + `vvp` |

**Metrics collected per language:**
- Lint errors found (before/after self-correction)
- Test pass/fail count
- Test execution time
- Self-correction attempts and outcomes

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
- Verification manifest includes per-language metrics

### T4C.5: Context Compression Protocol

**Agent:** SDK Agent (Sonnet 4.5)
**Estimated tokens:** 10K
**Dependencies:** T4C.1

**Description:**
Defines and implements the structured context format that agents use to resume work across sessions. Replaces prose-based conversation summaries with structured manifests that can be loaded in <2K tokens. Includes metrics history for trend analysis.

**Deliverables:**
- `software/atomik_sdk/pipeline/context/manifest.py` -- Structured context manifest
- `software/atomik_sdk/pipeline/context/cache.py` -- Artifact cache with TTL
- `software/atomik_sdk/pipeline/context/checkpoint.py` -- Cross-session checkpoints
- Context manifest JSON schema specification

**Manifest structure:**

```json
{
  "version": "2.0",
  "project_state": {
    "phase": "4C",
    "schemas_registered": 4,
    "languages_supported": ["python", "rust", "c", "javascript", "verilog"],
    "last_pipeline_run": "2026-01-27T14:30:00Z"
  },
  "artifact_index": {
    "schemas": {
      "video-h264-delta": {"sha256": "...", "path": "...", "last_generated": "..."},
      "edge-sensor-imu": {"sha256": "...", "path": "...", "last_generated": "..."},
      "finance-price-tick": {"sha256": "...", "path": "...", "last_generated": "..."}
    },
    "generated": {
      "video-h264-delta/python": {"files": 3, "checksum": "...", "status": "verified"},
      "video-h264-delta/verilog": {"files": 3, "checksum": "...", "status": "hw_validated"}
    }
  },
  "hardware_state": {
    "board_detected": true,
    "board_type": "Tang Nano 9K (GW1NR-9)",
    "last_programming": "2026-01-27T14:32:00Z",
    "com_port": "COM5"
  },
  "metrics_history": {
    "pipeline_runs": [
      {
        "timestamp": "2026-01-27T14:30:00Z",
        "schema": "video-h264-delta",
        "tokens_consumed": 0,
        "generation_time_ms": 1200,
        "hw_tests_passed": 10,
        "hw_tests_total": 10,
        "fmax_mhz": 94.9,
        "lut_percent": 7
      }
    ]
  },
  "token_ledger": {
    "session_total": 0,
    "budget_remaining": 130.00
  },
  "pending_actions": []
}
```

**Acceptance criteria:**
- Cold-start context loading uses <2K tokens
- Checkpoint creation is idempotent (same state = same checkpoint)
- Artifact cache correctly invalidates on schema change
- Metrics history persisted across sessions for trend analysis

### T4C.6: Token Budget & Model Router

**Agent:** SDK Agent (Sonnet 4.5)
**Estimated tokens:** 10K
**Dependencies:** T4C.1

**Description:**
Implements model selection logic and per-pipeline token accounting. Routes each subtask to the cheapest model that can handle it. Tracks cumulative token usage and enforces budget limits. Token efficiency is itself a tracked metric.

**Deliverables:**
- `software/atomik_sdk/pipeline/agents/router.py` -- Model routing logic
- `software/atomik_sdk/pipeline/agents/token_budget.py` -- Token accounting

**Routing rules:**

```
Task Classification -> Model Selection:

DETERMINISTIC (lint, test, diff, report, metrics)  -> No LLM (local execution)
MECHANICAL (known template, known fix)              -> Haiku 4.5
GENERATIVE (code generation, new patterns)          -> Sonnet 4.5
NOVEL (unknown error, architectural decision)       -> Opus 4.5 (with human approval)
```

**Token accounting metrics:**
- Pre-execution estimate vs. actual consumption (accuracy %)
- Cumulative cost per session, per schema, per pipeline run
- Model tier utilization breakdown (% local vs. Haiku vs. Sonnet vs. Opus)
- Cost-per-generated-line-of-code

**Acceptance criteria:**
- 85%+ of pipeline stages execute with 0 LLM tokens
- Token estimates within 30% of actual consumption
- Pipeline aborts cleanly when budget would be exceeded (no partial execution)
- Token efficiency metrics included in pipeline report

### T4C.7: Hardware-in-the-Loop Pipeline Stage (Mandatory)

**Agent:** Synthesis Agent (Opus 4.5)
**Estimated tokens:** 20K
**Dependencies:** T4C.1, T4C.4, Phase 3 FPGA pipeline

**Description:**
Mandatory pipeline stage that validates all generated Verilog through RTL simulation and, when hardware is available, FPGA programming and on-device test execution. Collects hardware performance metrics (utilization, Fmax, timing margins). This stage is **not optional** -- simulation always runs; FPGA execution runs whenever the board is detected.

**Deliverables:**
- `software/atomik_sdk/pipeline/stages/hardware.py` -- Hardware stage wrapper
- Integration with `hardware/scripts/fpga_pipeline.py` subprocess execution
- Hardware validation manifest format (JSON) with embedded metrics
- `demos/common/perf_counter.v` -- Cycle-accurate performance counter RTL module
- `demos/common/throughput_monitor.v` -- Operations/second measurement RTL module
- `demos/common/latency_timer.v` -- Per-operation latency measurement RTL module

**Stage execution flow:**

```
1. RTL Simulation (ALWAYS)
   ├── Compile generated Verilog with iverilog
   ├── Run testbench with vvp
   ├── Parse simulation output for pass/fail
   └── Collect: test count, pass rate, simulation time

2. FPGA Synthesis (when Gowin EDA available)
   ├── Run Gowin synthesis scripts
   ├── Parse synthesis report
   └── Collect: LUT%, FF%, BSRAM%, Fmax, timing slack

3. FPGA Programming (when Tang Nano 9K detected)
   ├── Program SRAM via openFPGALoader
   ├── Detect COM port
   └── Collect: programming time, board ID

4. On-Device Validation (when FPGA programmed)
   ├── Run hardware test suite via UART
   ├── Verify delta algebra properties in silicon
   └── Collect: test pass rate, per-test latency

5. Performance Profiling (when FPGA programmed)
   ├── Run throughput benchmark (operations/second)
   ├── Run latency benchmark (ns per operation)
   ├── Run power estimation (toggle rate analysis)
   └── Collect: ops/sec, latency_ns, estimated_mW
```

**Hardware metrics collected:**

| Metric | Source | Unit |
|--------|--------|------|
| LUT utilization | Synthesis report | % |
| FF utilization | Synthesis report | % |
| BSRAM utilization | Synthesis report | % |
| Max frequency (Fmax) | Timing analysis | MHz |
| Timing slack | Timing analysis | ns |
| Operations per second | Runtime benchmark | ops/s |
| Latency per operation | Runtime benchmark | ns |
| Power estimate | Toggle rate analysis | mW |
| Test pass rate | Hardware test suite | % |
| Programming time | openFPGALoader | seconds |

**Acceptance criteria:**
- RTL simulation runs on all 3 domain schemas (mandatory, no skip)
- Simulation tests pass 100% on all domain Verilog modules
- FPGA synthesis succeeds when Gowin EDA is available
- On-device validation passes 10/10 tests when hardware is connected
- Hardware metrics manifest produced with all available metrics
- Pipeline report clearly indicates which validation level was achieved (sim-only vs. hw-validated)

### T4C.8: Performance Metrics Framework

**Agent:** SDK Agent (Sonnet 4.5)
**Estimated tokens:** 18K
**Dependencies:** T4C.1, T4C.7

**Description:**
Unified metrics collection, storage, and reporting framework. Collects metrics from every pipeline stage, stores them in CSV for trend analysis, and generates structured JSON reports. Provides a `metrics` CLI subcommand for querying and comparing metrics across runs.

**Deliverables:**
- `software/atomik_sdk/pipeline/metrics/collector.py` -- Metrics collection API
- `software/atomik_sdk/pipeline/metrics/hardware_bench.py` -- Hardware benchmark runner
- `software/atomik_sdk/pipeline/metrics/pipeline_bench.py` -- Pipeline efficiency benchmarks
- `software/atomik_sdk/pipeline/metrics/reporter.py` -- Report generation (JSON + CSV)
- Metrics CSV schema specification
- CLI extension: `atomik-gen metrics` subcommand

**Metrics taxonomy:**

#### Pipeline Efficiency Metrics

| Metric | Description | Unit | Collection Point |
|--------|-------------|------|-----------------|
| `pipeline_total_time` | End-to-end pipeline wall-clock time | ms | Controller |
| `generation_time` | Code generation stage time | ms | Generate stage |
| `verification_time` | Lint + test execution time | ms | Verify stage |
| `hardware_time` | Simulation + synthesis + programming time | ms | Hardware stage |
| `tokens_consumed` | Total LLM tokens used this run | count | Token budget |
| `tokens_saved` | Tokens saved via differential/short-circuit | count | Diff stage |
| `token_efficiency` | tokens_saved / (tokens_saved + tokens_consumed) | % | Report |
| `self_correction_count` | Number of self-correction attempts | count | Verify stage |
| `self_correction_success` | Self-corrections that resolved the issue | count | Verify stage |
| `files_generated` | Total files produced | count | Generate stage |
| `lines_generated` | Total lines of code generated | count | Generate stage |
| `diff_type` | Type of change detected (none/field/metadata/hardware) | enum | Diff stage |

#### Hardware Synthesis Metrics

| Metric | Description | Unit | Source |
|--------|-------------|------|--------|
| `lut_used` | Logic elements consumed | count | Gowin synthesis |
| `lut_available` | Total logic elements on device | count | Device spec |
| `lut_utilization` | lut_used / lut_available | % | Computed |
| `ff_used` | Flip-flops consumed | count | Gowin synthesis |
| `ff_utilization` | ff_used / ff_available | % | Computed |
| `bsram_used` | Block SRAM consumed | count | Gowin synthesis |
| `fmax_achieved` | Maximum clock frequency achieved | MHz | Timing analysis |
| `fmax_target` | Target clock frequency from schema | MHz | Schema hardware section |
| `timing_slack` | Worst-case timing slack | ns | Timing analysis |
| `timing_met` | All timing constraints satisfied | bool | Timing analysis |

#### Runtime Performance Metrics

| Metric | Description | Unit | Source |
|--------|-------------|------|--------|
| `ops_per_second` | Delta operations per second | ops/s | Hardware benchmark |
| `load_latency` | LOAD operation latency | ns | Latency timer |
| `accumulate_latency` | ACCUMULATE operation latency | ns | Latency timer |
| `reconstruct_latency` | RECONSTRUCT operation latency | ns | Latency timer |
| `rollback_latency` | ROLLBACK operation latency | ns | Latency timer |
| `throughput_mbps` | Data throughput (DATA_WIDTH * ops/s) | Mbps | Computed |
| `power_estimate` | Estimated dynamic power | mW | Toggle rate |
| `energy_per_op` | Energy per delta operation | pJ | Computed |

#### Quality Metrics

| Metric | Description | Unit | Source |
|--------|-------------|------|--------|
| `sim_tests_passed` | RTL simulation tests passed | count | iverilog |
| `sim_tests_total` | RTL simulation tests total | count | iverilog |
| `hw_tests_passed` | On-device tests passed | count | UART test suite |
| `hw_tests_total` | On-device tests total | count | UART test suite |
| `sw_tests_passed` | Software tests passed (all languages) | count | pytest/cargo/etc. |
| `lint_errors` | Lint errors detected (pre-correction) | count | ruff/verilator |
| `lint_clean` | All lint checks pass (post-correction) | bool | Verify stage |

**Metrics storage format (CSV):**

```csv
timestamp,schema,pipeline_time_ms,tokens_consumed,tokens_saved,files_generated,lines_generated,lut_pct,ff_pct,fmax_mhz,timing_slack_ns,ops_per_sec,latency_ns,power_mw,sim_pass,hw_pass,sw_pass
2026-01-27T14:30:00Z,video-h264-delta,4500,0,0,19,850,12,14,94.9,0.049,94500000,10.6,45,10/10,10/10,87/87
2026-01-27T14:31:00Z,edge-sensor-imu,3800,0,0,19,720,7,9,94.9,0.049,94500000,10.6,32,10/10,10/10,87/87
2026-01-27T14:32:00Z,finance-price-tick,4100,0,0,19,780,8,10,94.9,0.049,94500000,10.6,38,10/10,10/10,87/87
```

**CLI interface:**

```bash
# Show metrics for the last pipeline run
atomik-gen metrics show

# Show metrics for a specific schema
atomik-gen metrics show --schema video-h264-delta

# Compare metrics across schemas
atomik-gen metrics compare

# Export metrics history to CSV
atomik-gen metrics export --output metrics_history.csv

# Show hardware metrics only
atomik-gen metrics hardware

# Show token efficiency trend
atomik-gen metrics tokens
```

**Acceptance criteria:**
- All metrics from taxonomy collected during pipeline run
- Metrics persisted to CSV for historical trend analysis
- `atomik-gen metrics show` displays formatted metrics table
- `atomik-gen metrics compare` shows side-by-side domain comparison
- Hardware metrics match Phase 3 baselines (7% LUT, 9% FF, 94.5 MHz)

### T4C.9: Video Domain Hardware Demonstrator

**Agent:** Synthesis Agent (Opus 4.5)
**Estimated tokens:** 20K
**Dependencies:** T4C.7, T4C.8, Phase 4B Video schema

**Description:**
Deploys the Video H.264 Delta domain SDK onto the Tang Nano 9K with a camera module interface. Demonstrates real-time frame delta accumulation from a camera feed, measuring throughput, latency, and resource utilization specific to the 256-bit video processing pipeline.

**Hardware configuration:**
- **Board:** Tang Nano 9K (Gowin GW1NR-LV9QN88PC6/I5)
- **Peripheral:** OV2640 camera module (DVP interface) or simulated frame source
- **Data width:** 256-bit (matching schema `frame_delta` field)
- **Target frequency:** 150 MHz (per schema constraint)

**Deliverables:**
- `demos/video/video_demo_top.v` -- Top-level integration
- `demos/video/camera_interface.v` -- Camera/frame source interface
- `demos/video/tb_video_demo.v` -- Simulation testbench
- `demos/video/video_demo.cst` -- Pin constraints
- Video domain metrics report (JSON)

**Performance targets (from schema constraints):**

| Metric | Target | Measurement Method |
|--------|--------|--------------------|
| Data width | 256-bit | RTL parameter |
| Target Fmax | 150 MHz | Synthesis timing |
| Max latency | 33 ms (30fps) | Latency timer |
| Memory budget | 256 MB | Resource report |
| Rollback depth | 512 frames | Functional test |
| Throughput | 256b * Fmax = 38.4 Gbps peak | Throughput monitor |

**Acceptance criteria:**
- RTL simulation passes all video-specific tests
- Synthesis succeeds on GW1NR-9 (may not reach 150 MHz target on this device -- document actual Fmax)
- On-device delta accumulation and reconstruction verified
- 256-bit XOR operations confirmed single-cycle
- Performance metrics collected and reported

### T4C.10: Edge Sensor Domain Hardware Demonstrator

**Agent:** Synthesis Agent (Opus 4.5)
**Estimated tokens:** 18K
**Dependencies:** T4C.7, T4C.8, Phase 4B Edge Sensor schema

**Description:**
Deploys the Edge Sensor IMU Fusion domain SDK onto the Tang Nano 9K with an SPI-connected IMU sensor (or simulated sensor data). Demonstrates low-power delta accumulation from motion sensor data, measuring power-optimized performance specific to the 64-bit edge computing pipeline.

**Hardware configuration:**
- **Board:** Tang Nano 9K (Gowin GW1NR-LV9QN88PC6/I5)
- **Peripheral:** MPU-6050 or ICM-20948 IMU over SPI (or simulated)
- **Data width:** 64-bit (matching schema `motion_delta` field)
- **Target frequency:** 100 MHz (per schema constraint)
- **Optimization:** Power

**Deliverables:**
- `demos/sensor/sensor_demo_top.v` -- Top-level integration
- `demos/sensor/spi_imu_interface.v` -- SPI sensor interface
- `demos/sensor/tb_sensor_demo.v` -- Simulation testbench
- `demos/sensor/sensor_demo.cst` -- Pin constraints
- Sensor domain metrics report (JSON)

**Performance targets (from schema constraints):**

| Metric | Target | Measurement Method |
|--------|--------|--------------------|
| Data width | 64-bit | RTL parameter |
| Target Fmax | 100 MHz | Synthesis timing |
| Max latency | 10 ms | Latency timer |
| Power budget | 500 mW | Toggle rate analysis |
| Rollback depth | 1024 samples | Functional test |
| Memory budget | 16 MB | Resource report |

**Acceptance criteria:**
- RTL simulation passes all sensor-specific tests
- Synthesis meets 100 MHz timing on GW1NR-9
- On-device motion delta accumulation verified
- Power estimate within 500 mW budget
- Alert flag bitmask delta operations verified
- Performance metrics collected and reported

### T4C.11: Finance Domain Hardware Demonstrator

**Agent:** Synthesis Agent (Opus 4.5)
**Estimated tokens:** 18K
**Dependencies:** T4C.7, T4C.8, Phase 4B Finance schema

**Description:**
Deploys the Financial Price Tick domain SDK onto the Tang Nano 9K with a high-speed UART tick stream interface (or simulated market data). Demonstrates high-throughput delta accumulation for price tick processing, measuring throughput and deep rollback capability specific to the 64-bit financial computing pipeline.

**Hardware configuration:**
- **Board:** Tang Nano 9K (Gowin GW1NR-LV9QN88PC6/I5)
- **Peripheral:** Dual UART for tick ingestion + status output (or simulated)
- **Data width:** 64-bit (matching schema `price_delta` field)
- **Target frequency:** 400 MHz (per schema constraint -- will document achieved vs. target on GW1NR-9)
- **Optimization:** Speed

**Deliverables:**
- `demos/finance/finance_demo_top.v` -- Top-level integration
- `demos/finance/tick_stream_interface.v` -- Tick data ingestion
- `demos/finance/tb_finance_demo.v` -- Simulation testbench
- `demos/finance/finance_demo.cst` -- Pin constraints
- Finance domain metrics report (JSON)

**Performance targets (from schema constraints):**

| Metric | Target | Measurement Method |
|--------|--------|--------------------|
| Data width | 64-bit | RTL parameter |
| Target Fmax | 400 MHz (device limit ~95 MHz) | Synthesis timing |
| Max latency | 1 ms | Latency timer |
| Rollback depth | 4096 ticks | Functional test |
| Memory budget | 512 MB | Resource report |
| Throughput | 64b * Fmax = peak | Throughput monitor |
| Parallel accumulation | Enabled | Schema config |

**Note:** The GW1NR-9 cannot reach 400 MHz. The demonstrator will run at the device's maximum achievable frequency and document the gap. The schema constraint represents the target for production-grade FPGAs (e.g., Xilinx Ultrascale+).

**Acceptance criteria:**
- RTL simulation passes all finance-specific tests
- Synthesis succeeds at maximum achievable frequency
- Deep rollback (4096 entries) verified in simulation
- Three-field accumulation (price, volume, flags) verified
- Performance metrics collected with actual vs. target comparison

### T4C.12: Pipeline CLI & Integration

**Agent:** SDK Agent (Sonnet 4.5)
**Estimated tokens:** 15K
**Dependencies:** T4C.1 through T4C.11

**Description:**
Extends `atomik-gen` CLI with pipeline and metrics subcommands. Integrates all stages (including mandatory hardware and metrics) into a cohesive user experience.

**Deliverables:**
- CLI extension: `atomik-gen pipeline` subcommand with options
- CLI extension: `atomik-gen metrics` subcommand with options
- `atomik-gen pipeline status` -- Show current pipeline state from checkpoint
- `atomik-gen pipeline run <schema>` -- Execute full pipeline (including hardware)
- `atomik-gen pipeline diff <schema>` -- Show what would change (dry run)
- `atomik-gen demo <domain>` -- Run a domain hardware demonstrator
- Updated documentation for all commands

**CLI interface:**

```bash
# Full autonomous pipeline execution (hardware mandatory)
atomik-gen pipeline run sdk/schemas/domains/video-h264-delta.json

# Batch pipeline for all schemas in a directory
atomik-gen pipeline run sdk/schemas/domains/ --batch

# Dry run: show what would be generated/tested/synthesized
atomik-gen pipeline diff sdk/schemas/domains/video-h264-delta.json

# Show pipeline state, hardware status, and metrics summary
atomik-gen pipeline status

# Specify COM port for hardware validation
atomik-gen pipeline run schema.json --com-port COM5

# Pipeline with explicit token budget
atomik-gen pipeline run schema.json --token-budget 15000

# Pipeline targeting specific languages only
atomik-gen pipeline run schema.json --languages python rust verilog

# Run a domain hardware demonstrator
atomik-gen demo video
atomik-gen demo sensor --com-port COM5
atomik-gen demo finance --report finance_metrics.json

# Show metrics for last pipeline run
atomik-gen metrics show
atomik-gen metrics compare
atomik-gen metrics export --output metrics.csv
atomik-gen metrics hardware
```

**Options (pipeline):**

| Option | Description | Default |
|--------|-------------|---------|
| `--batch` | Process all schemas in directory | single file |
| `--com-port PORT` | Serial port for hardware validation | auto-detect |
| `--token-budget N` | Maximum tokens for this pipeline run | unlimited |
| `--skip-synthesis` | Skip FPGA synthesis (simulation still runs) | false |
| `--sim-only` | RTL simulation only (no FPGA programming) | false |
| `--dry-run` | Show plan without executing | false |
| `--report FILE` | Write pipeline report to file | stdout |
| `--checkpoint DIR` | Directory for pipeline checkpoints | `.atomik/` |
| `--metrics-csv FILE` | Append metrics to CSV file | `.atomik/metrics.csv` |

**Options (demo):**

| Option | Description | Default |
|--------|-------------|---------|
| `--com-port PORT` | Serial port for FPGA | auto-detect |
| `--sim-only` | Simulation only | false |
| `--report FILE` | Write demo metrics report | stdout |
| `--flash` | Program to persistent flash | SRAM |

**Acceptance criteria:**
- `atomik-gen pipeline run` succeeds end-to-end on all 3 domain schemas
- `atomik-gen pipeline diff` correctly reports "up to date" on unchanged schemas
- `atomik-gen pipeline status` displays hardware status, token usage, and last metrics
- `atomik-gen demo video|sensor|finance` runs the corresponding demonstrator
- `atomik-gen metrics show` displays formatted metrics table
- Pipeline report includes per-stage timing, token usage, hardware metrics, and pass/fail status

### T4C.13: Test Suite, Benchmarks & Documentation

**Agent:** Validator Agent (Haiku 4.5), Documenter Agent (Haiku 4.5)
**Estimated tokens:** 12K
**Dependencies:** T4C.1 through T4C.12

**Description:**
Comprehensive test coverage for the pipeline system, hardware demonstrators, and metrics framework. Updated documentation across the project. Final performance benchmark report comparing all 3 domains.

**Deliverables:**
- `software/atomik_sdk/tests/test_pipeline_controller.py` -- Controller tests
- `software/atomik_sdk/tests/test_pipeline_diff.py` -- Diff detection tests
- `software/atomik_sdk/tests/test_pipeline_verify.py` -- Verification stage tests
- `software/atomik_sdk/tests/test_pipeline_context.py` -- Context/cache tests
- `software/atomik_sdk/tests/test_pipeline_hardware.py` -- Hardware stage tests
- `software/atomik_sdk/tests/test_pipeline_metrics.py` -- Metrics framework tests
- Updated `docs/SDK_DEVELOPER_GUIDE.md` -- Pipeline architecture section
- Updated `docs/SDK_API_REFERENCE.md` -- Pipeline + metrics CLI reference
- Updated `README.md` -- Pipeline and hardware demos section
- Phase 4C completion report with full performance benchmark data
- Cross-domain performance comparison report

**Test scenarios:**
- Pipeline end-to-end on existing domain schema (happy path)
- Pipeline with schema modification (differential regeneration)
- Pipeline with no changes (short-circuit, 0 tokens)
- Pipeline with injected lint error (self-correction)
- Pipeline with injected test failure (diagnosis and escalation)
- Pipeline budget enforcement (abort on over-budget)
- Context checkpoint save/restore cycle
- Hardware stage RTL simulation (all 3 domains)
- Hardware stage FPGA programming and validation (when hardware available)
- Metrics collection completeness (all taxonomy fields populated)
- Metrics CSV export and re-import
- Demo execution (simulation mode) for all 3 domains
- Performance counter RTL verification

**Cross-domain performance comparison:**

| Metric | Video (256b) | Sensor (64b) | Finance (64b) | Unit |
|--------|-------------|-------------|---------------|------|
| LUT utilization | TBD | TBD | TBD | % |
| FF utilization | TBD | TBD | TBD | % |
| Fmax achieved | TBD | TBD | TBD | MHz |
| Fmax target | 150 | 100 | 400 | MHz |
| Ops/second | TBD | TBD | TBD | M ops/s |
| Latency | TBD | TBD | TBD | ns |
| Throughput | TBD | TBD | TBD | Gbps |
| Power estimate | TBD | TBD | TBD | mW |
| Rollback depth | 512 | 1024 | 4096 | entries |
| Pipeline time | TBD | TBD | TBD | ms |
| Tokens consumed | TBD | TBD | TBD | count |

**Acceptance criteria:**
- 95%+ test coverage on pipeline and metrics modules
- All existing 87+ tests continue to pass
- 30+ new tests covering pipeline, hardware, and metrics
- Cross-domain performance comparison table fully populated
- Documentation accurately reflects pipeline architecture, hardware demos, and metrics CLI

---

## 4. Token Efficiency Strategy

### 4.1 Token Budget Breakdown

| Task | Estimated Tokens | LLM Model | Local (0 tokens) |
|------|-----------------|-----------|-------------------|
| T4C.1 Controller | 20K | Sonnet | -- |
| T4C.2 Diff Detection | 12K | Sonnet | Diff logic is local |
| T4C.3 Selective Generation | 15K | Sonnet | -- |
| T4C.4 Self-Validating | 5K | Haiku (diagnosis only) | Lint/test is local |
| T4C.5 Context Protocol | 10K | Sonnet | -- |
| T4C.6 Token Router | 10K | Sonnet | Routing logic is local |
| T4C.7 Hardware Stage | 20K | Opus | Simulation is local |
| T4C.8 Metrics Framework | 18K | Sonnet | Collection is local |
| T4C.9 Video Demo | 20K | Opus | Synthesis is local |
| T4C.10 Sensor Demo | 18K | Opus | Synthesis is local |
| T4C.11 Finance Demo | 18K | Opus | Synthesis is local |
| T4C.12 CLI Integration | 15K | Sonnet | -- |
| T4C.13 Tests & Docs | 12K | Haiku | -- |
| **Total** | **193K** | | |
| **Contingency (15%)** | **+29K** | | |
| **Grand Total** | **~222K** | | **Budget: $130** |

### 4.2 Steady-State Pipeline Token Cost

Once Phase 4C infrastructure is built, the per-pipeline-run token cost drops dramatically:

| Scenario | Tokens | Cost | Hardware |
|----------|--------|------|----------|
| **New schema (full pipeline)** | ~12K | ~$0.18 | Sim + FPGA |
| **Schema update (differential)** | ~4K | ~$0.06 | Selective re-synth |
| **No change (short-circuit)** | 0 | $0.00 | Skipped |
| **Failure + self-correction** | ~6K | ~$0.09 | Re-verify |
| **Hardware validation only** | 0 (local) | $0.00 | Full HW suite |
| **Metrics collection** | 0 (local) | $0.00 | Benchmarks |
| **Demo execution** | 0 (local) | $0.00 | Full demo |

### 4.3 Optimization Techniques

| Technique | Token Savings | Implementation |
|-----------|--------------|----------------|
| **Local-first execution** | 85% of stages | Lint, test, diff, report, metrics, hardware all run without LLM |
| **Differential regeneration** | 50-90% per update | Only regenerate changed languages/files |
| **Structured context** | 70% context loading | JSON manifests vs. prose summaries |
| **Model tiering** | 60% per diagnosis | Haiku for known errors, Sonnet for generation |
| **Short-circuit detection** | 100% when unchanged | Hash comparison skips entire pipeline |
| **Template caching** | 30% per generation | Reuse common code patterns across schemas |
| **Local synthesis** | 100% hardware stages | FPGA tools run locally, no LLM needed |

---

## 5. Agent Coordination Model

### 5.1 Agent Roles in Pipeline

| Agent | Pipeline Role | Model | Invocation Trigger |
|-------|--------------|-------|-------------------|
| **Pipeline Controller** | Orchestration, metrics aggregation | Local (no LLM) | CLI command |
| **Validator Agent** | Schema validation, failure diagnosis | Haiku 4.5 | Stage failure |
| **SDK Agent** | Code generation, infrastructure creation | Sonnet 4.5 | Generation stage |
| **Synthesis Agent** | RTL generation, hardware demos, synthesis analysis | Opus 4.5 | Hardware stage, demo tasks |
| **Documenter Agent** | Report generation, documentation updates | Haiku 4.5 | Pipeline completion |

### 5.2 Artifact-Based Handoff Protocol

Agents never communicate through conversation context. All inter-stage communication flows through typed artifacts:

```
Stage N produces:
  -> Manifest file (JSON): declares outputs, status, checksums, metrics
  -> Artifact files: generated code, test results, synthesis reports
  -> Metrics entries: appended to pipeline metrics ledger

Stage N+1 consumes:
  <- Previous stage manifest
  <- Referenced artifact files
  <- Pipeline checkpoint (for context restoration)
```

**Manifest schema (v2 with metrics):**

```json
{
  "stage": "hardware",
  "status": "success",
  "timestamp": "2026-01-27T14:32:00Z",
  "tokens_consumed": 0,
  "duration_ms": 12500,
  "artifacts": [
    {
      "path": "sdk/generated/rtl/video/streaming/atomik_video_streaming_h264_delta.v",
      "language": "verilog",
      "sha256": "abc123...",
      "action": "validated"
    }
  ],
  "metrics": {
    "sim_tests_passed": 10,
    "sim_tests_total": 10,
    "hw_tests_passed": 10,
    "hw_tests_total": 10,
    "lut_utilization_pct": 12,
    "ff_utilization_pct": 14,
    "fmax_mhz": 94.9,
    "timing_slack_ns": 0.049,
    "ops_per_second": 94500000,
    "latency_ns": 10.6
  },
  "validation_level": "hw_validated",
  "errors": [],
  "next_stage": "metrics"
}
```

### 5.3 Escalation Chain

```
Local execution (0 tokens)
    | fails
    v
Haiku diagnosis (2K tokens)
    | fails
    v
Sonnet generation (8K tokens)
    | fails
    v
Opus reasoning (20K tokens, requires human approval)
    | fails
    v
Human intervention (pipeline paused)
```

---

## 6. Pipeline Stages

### 6.1 Stage 1: Intake & Validation

**Input:** Schema file path (or directory for batch)
**Output:** Validation manifest
**LLM tokens:** 0 (local SchemaValidator execution)
**Metrics:** Schema complexity (field count, operation count, data width)

1. Load schema JSON
2. Validate against `specs/atomik_schema_v1.json` (JSON Schema Draft 7)
3. Run semantic validation (SchemaValidator rules)
4. Compute schema content hash
5. Extract schema complexity metrics
6. Produce validation manifest

### 6.2 Stage 2: Differential Analysis

**Input:** Validation manifest + pipeline checkpoint
**Output:** Diff manifest
**LLM tokens:** 0 (local hash comparison)
**Metrics:** Change type, affected generators count, estimated regeneration scope

1. Load previous checkpoint for this schema (if exists)
2. Compare schema content hash with checkpoint
3. If unchanged: produce "up-to-date" manifest, pipeline short-circuits
4. If changed: compute structural diff (which sections changed)
5. Map changed sections to affected generators
6. If hardware section changed: flag for re-synthesis
7. Produce diff manifest listing generators to invoke

**Diff classification:**

| Change Type | Affected Generators | Hardware Impact | Example |
|-------------|-------------------|----------------|---------|
| `delta_fields` | All 5 languages | Re-synthesize | Added/removed a field |
| `operations` | All 5 languages | Re-synthesize | Added rollback support |
| `namespace` | All 5 languages | Re-synthesize | Changed namespace path |
| `hardware` | Verilog only | Re-synthesize | Changed DATA_WIDTH |
| `constraints` | Verilog only | Re-synthesize | Changed target frequency |
| `metadata` | Python, JavaScript | No impact | Changed description text |

### 6.3 Stage 3: Code Generation

**Input:** Diff manifest + schema file
**Output:** Generated artifacts + artifact manifest
**LLM tokens:** 0 for existing GeneratorEngine (local Python); LLM only if self-correction needed
**Metrics:** Generation time, files generated, lines of code per language

1. Read diff manifest to determine which generators to invoke
2. Create GeneratorEngine with GeneratorConfig
3. Invoke selected generators (timing each)
4. Compute SHA256 of each generated file
5. Count lines of code per file
6. Produce artifact manifest with generation metrics

### 6.4 Stage 4: Verification

**Input:** Artifact manifest + generated files
**Output:** Verification manifest
**LLM tokens:** 0 for passing; 2-8K for failure diagnosis
**Metrics:** Test pass rate, lint error count, self-correction attempts, verification time

1. For each language with generated output:
   - Run language-specific lint check
   - Run language-specific tests (if test runner available)
   - Record pass/fail per check with timing
2. If all pass: produce success verification manifest
3. If failure:
   a. Classify error type against known error database
   b. If known: apply deterministic fix, re-verify
   c. If unknown: invoke diagnosis agent (Haiku -> Sonnet escalation)
   d. Apply suggested fix, re-verify (max 2 retries)
   e. If still failing: mark as failed, include error context in manifest

### 6.5 Stage 5: Hardware Validation (Mandatory)

**Input:** Verification manifest + Verilog artifacts
**Output:** Hardware manifest with performance metrics
**LLM tokens:** 0 (local simulation/synthesis/programming)
**Metrics:** Full hardware metrics taxonomy (see Section 7)

1. **RTL Simulation** (ALWAYS -- cannot be skipped):
   a. Compile generated Verilog with `iverilog`
   b. Run testbench with `vvp`
   c. Parse simulation output for pass/fail and performance data
   d. Collect: test count, pass rate, simulation time
2. **FPGA Synthesis** (when Gowin EDA is available):
   a. Run Gowin synthesis scripts
   b. Parse synthesis report for utilization and timing
   c. Collect: LUT%, FF%, BSRAM%, Fmax, timing slack
3. **FPGA Programming** (when Tang Nano 9K is detected):
   a. Program SRAM via `openFPGALoader`
   b. Auto-detect COM port
   c. Collect: programming time, board ID
4. **On-Device Validation** (when FPGA is programmed):
   a. Run hardware test suite via UART (10 tests)
   b. Verify delta algebra properties in silicon
   c. Collect: test pass rate, per-test timing
5. **Performance Profiling** (when FPGA is programmed):
   a. Run throughput benchmark (operations/second)
   b. Run latency benchmark (ns per operation)
   c. Run power estimation (toggle rate analysis)
   d. Collect: ops/sec, latency_ns, estimated_mW, energy_per_op
6. Produce hardware manifest with all collected metrics
7. Tag validation level: `simulation_only`, `synthesized`, `hw_validated`, `hw_benchmarked`

### 6.6 Stage 6: Metrics Collection & Reporting

**Input:** All stage manifests
**Output:** Pipeline report (JSON) + metrics CSV row + updated checkpoint
**LLM tokens:** 0 (local aggregation)

1. Aggregate all stage manifests into pipeline report
2. Compute totals: files generated, tests passed, tokens consumed, time elapsed
3. Compute derived metrics: token efficiency %, cost-per-line, throughput
4. Append metrics row to CSV history file
5. Update pipeline checkpoint with new artifact hashes and metrics
6. Generate formatted report (summary table + detailed JSON)
7. Write report to file (if `--report` specified) or stdout
8. Exit with appropriate code (0 = success, 1 = validation fail, 2 = generation fail, 3 = hardware fail)

---

## 7. Performance Metrics

### 7.1 Per-Domain Benchmark Targets

Based on domain schema constraints and Phase 3 hardware baseline:

| Metric | Video (256b) | Sensor (64b) | Finance (64b) | Phase 3 Baseline |
|--------|-------------|-------------|---------------|-----------------|
| **DATA_WIDTH** | 256-bit | 64-bit | 64-bit | 64-bit |
| **Target Fmax** | 150 MHz | 100 MHz | 400 MHz | 94.5 MHz |
| **Device Fmax** | ~80-95 MHz* | 94.5 MHz | ~94.5 MHz* | 94.5 MHz |
| **LUT budget** | <25% | <10% | <12% | 7% |
| **FF budget** | <30% | <12% | <15% | 9% |
| **Max latency** | 33 ms | 10 ms | 1 ms | 10.6 ns/op |
| **Rollback depth** | 512 | 1024 | 4096 | N/A |
| **Power budget** | N/A | 500 mW | N/A | N/A |
| **Optimization** | Speed | Power | Speed | -- |

*GW1NR-9 maximum Fmax is ~95 MHz; 256-bit datapath may reduce this.

### 7.2 Phase 3 Hardware Baseline

All domain demonstrators are compared against the Phase 3 validated baseline:

| Metric | Phase 3 Actual | Source |
|--------|---------------|--------|
| Clock frequency | 94.5 MHz | PLL from 27 MHz crystal |
| Fmax achieved | 94.9 MHz | Gowin timing analysis |
| LUT utilization | 7% (579/8640) | Synthesis report |
| FF utilization | 9% (537/6693) | Synthesis report |
| CLS utilization | 10% (417/4320) | Synthesis report |
| Operations/second | 94.5 M ops/s | Single-cycle operation |
| Latency per op | 10.6 ns | 1/94.5 MHz |
| Hardware tests | 10/10 (100%) | UART test suite |
| Timing slack | +0.049 ns | Timing analysis |

### 7.3 Metrics Collection Architecture

```
┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│  Pipeline    │  │  Hardware    │  │  Runtime     │
│  Metrics     │  │  Metrics     │  │  Metrics     │
│              │  │              │  │              │
│  Tokens      │  │  LUT/FF %   │  │  Ops/sec     │
│  Time (ms)   │  │  Fmax (MHz) │  │  Latency     │
│  Files/Lines │  │  Slack (ns) │  │  Throughput  │
│  Diff type   │  │  Power (mW) │  │  Energy/op   │
└──────┬──────┘  └──────┬──────┘  └──────┬──────┘
       │                │                │
       └────────────────┼────────────────┘
                        ▼
              ┌──────────────────┐
              │  Metrics         │
              │  Collector       │
              │                  │
              │  Normalize       │
              │  Validate        │
              │  Aggregate       │
              └────────┬─────────┘
                       │
          ┌────────────┼────────────┐
          ▼            ▼            ▼
   ┌───────────┐ ┌───────────┐ ┌───────────┐
   │ JSON      │ │ CSV       │ │ Checkpoint │
   │ Report    │ │ History   │ │ Update     │
   └───────────┘ └───────────┘ └───────────┘
```

### 7.4 Metrics Report Format

**Summary report (stdout):**

```
ATOMiK Pipeline Report -- video-h264-delta
==========================================

Pipeline Efficiency
  Total time:           4,500 ms
  Tokens consumed:      0
  Tokens saved:         12,000 (short-circuit)
  Token efficiency:     100%
  Files generated:      19
  Lines of code:        850

Hardware Validation     [HW_VALIDATED]
  RTL simulation:       10/10 tests passed
  FPGA synthesis:       SUCCESS
  On-device tests:      10/10 tests passed

Synthesis Metrics
  LUT utilization:      12% (1,037/8,640)
  FF utilization:       14% (937/6,693)
  Fmax achieved:        89.2 MHz (target: 150 MHz)
  Timing slack:         +0.032 ns

Runtime Performance
  Operations/second:    89,200,000
  Latency per op:       11.2 ns
  Throughput:           22.9 Gbps (256-bit @ 89.2 MHz)
  Power estimate:       45 mW

Quality
  SW tests passed:      87/87
  Lint errors:          0
  Self-corrections:     0
```

---

## 8. Deliverables

### 8.1 New Files

| File | Purpose | Est. Lines |
|------|---------|-----------|
| `software/atomik_sdk/pipeline/__init__.py` | Package init | 10 |
| `software/atomik_sdk/pipeline/controller.py` | Pipeline orchestrator | 350 |
| `software/atomik_sdk/pipeline/stages/__init__.py` | Stage base protocol | 50 |
| `software/atomik_sdk/pipeline/stages/validate.py` | Validation stage | 80 |
| `software/atomik_sdk/pipeline/stages/diff.py` | Diff detection stage | 150 |
| `software/atomik_sdk/pipeline/stages/generate.py` | Generation stage | 120 |
| `software/atomik_sdk/pipeline/stages/verify.py` | Verification stage | 200 |
| `software/atomik_sdk/pipeline/stages/hardware.py` | Hardware stage (mandatory) | 250 |
| `software/atomik_sdk/pipeline/stages/metrics.py` | Metrics collection stage | 150 |
| `software/atomik_sdk/pipeline/context/manifest.py` | Context manifests | 120 |
| `software/atomik_sdk/pipeline/context/cache.py` | Artifact cache | 100 |
| `software/atomik_sdk/pipeline/context/checkpoint.py` | Checkpoints | 100 |
| `software/atomik_sdk/pipeline/agents/router.py` | Model routing | 80 |
| `software/atomik_sdk/pipeline/agents/token_budget.py` | Token accounting | 100 |
| `software/atomik_sdk/pipeline/agents/self_correct.py` | Self-correction | 150 |
| `software/atomik_sdk/pipeline/metrics/collector.py` | Metrics collection API | 200 |
| `software/atomik_sdk/pipeline/metrics/hardware_bench.py` | Hardware benchmarks | 180 |
| `software/atomik_sdk/pipeline/metrics/pipeline_bench.py` | Pipeline benchmarks | 120 |
| `software/atomik_sdk/pipeline/metrics/reporter.py` | Report generation | 200 |
| `software/atomik_sdk/pipeline/reports/pipeline_report.py` | JSON report | 100 |
| `demos/common/perf_counter.v` | Performance counter RTL | 80 |
| `demos/common/throughput_monitor.v` | Throughput measurement RTL | 100 |
| `demos/common/latency_timer.v` | Latency measurement RTL | 80 |
| `demos/video/video_demo_top.v` | Video demo top-level | 200 |
| `demos/video/camera_interface.v` | Camera interface | 150 |
| `demos/video/tb_video_demo.v` | Video demo testbench | 200 |
| `demos/video/video_demo.cst` | Video pin constraints | 30 |
| `demos/sensor/sensor_demo_top.v` | Sensor demo top-level | 180 |
| `demos/sensor/spi_imu_interface.v` | SPI IMU interface | 200 |
| `demos/sensor/tb_sensor_demo.v` | Sensor demo testbench | 180 |
| `demos/sensor/sensor_demo.cst` | Sensor pin constraints | 25 |
| `demos/finance/finance_demo_top.v` | Finance demo top-level | 180 |
| `demos/finance/tick_stream_interface.v` | Tick stream interface | 150 |
| `demos/finance/tb_finance_demo.v` | Finance demo testbench | 180 |
| `demos/finance/finance_demo.cst` | Finance pin constraints | 25 |
| **Total new source** | | **~4,670** |

### 8.2 Modified Files

| File | Changes |
|------|---------|
| `software/atomik_sdk/cli.py` | Add `pipeline`, `metrics`, `demo` subcommand groups |
| `software/pyproject.toml` | Add pipeline dependencies (if any) |
| `README.md` | Pipeline, hardware demos, and metrics sections |
| `docs/SDK_DEVELOPER_GUIDE.md` | Pipeline architecture and metrics sections |
| `docs/SDK_API_REFERENCE.md` | Pipeline + metrics + demo CLI reference |

### 8.3 Test Files

| File | Tests | Coverage |
|------|-------|----------|
| `test_pipeline_controller.py` | 8-10 | Controller dispatch, stage ordering |
| `test_pipeline_diff.py` | 6-8 | Diff detection, change classification |
| `test_pipeline_verify.py` | 5-7 | Verification, self-correction |
| `test_pipeline_context.py` | 5-7 | Manifest, cache, checkpoint |
| `test_pipeline_hardware.py` | 6-8 | Hardware stage, simulation mode |
| `test_pipeline_metrics.py` | 6-8 | Metrics collection, reporting, CSV |
| **Total new tests** | **36-48** | |

---

## 9. Validation Gates

| Gate | Metric | Threshold |
|------|--------|-----------|
| Pipeline end-to-end | Completes on all 3 domain schemas | 100% |
| Differential accuracy | Correctly identifies change vs. no-change | 100% |
| Self-correction | Handles injected lint errors without LLM | 3/3 known classes |
| Token efficiency | Full pipeline run per schema | <15K tokens |
| Short-circuit | Unchanged schema uses 0 tokens | 0 |
| Test coverage | Pipeline + metrics module coverage | >95% |
| Existing tests | All pre-existing tests pass | 87+ pass |
| New tests | Pipeline + hardware + metrics tests | 36+ pass |
| **RTL simulation** | **All 3 domain testbenches pass** | **100%** |
| **FPGA synthesis** | **All 3 domains synthesize on GW1NR-9** | **100%** |
| **On-device validation** | **Hardware tests pass when board connected** | **10/10 per domain** |
| **Timing closure** | **All domains meet device Fmax** | **Slack >= 0 ns** |
| **Performance metrics** | **All taxonomy metrics collected** | **100% populated** |
| **Metrics baseline** | **Results compared to Phase 3 baseline** | **Report generated** |
| Context checkpoint | Save/restore preserves full state + metrics | Round-trip verified |
| CLI integration | All pipeline + metrics + demo subcommands | 100% functional |
| Demo simulation | All 3 domain demos pass simulation | 100% |
| Cross-domain report | Performance comparison table populated | All TBD filled |

---

## 10. Risk Analysis

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Self-correction loops (retry forever) | Medium | Medium | Hard cap at 2 retries + escalation |
| Diff misclassification (misses a change) | Low | High | Conservative default: full regeneration if uncertain |
| Token budget exceeded during build | Low | Medium | 15% contingency allocation |
| 256-bit datapath fails timing on GW1NR-9 | High | Medium | Document actual Fmax; design for production FPGA scaling |
| Peripheral hardware unavailable (camera, IMU) | High | Low | All demos include simulated data source mode |
| Gowin EDA not installed on build machine | Medium | Medium | Synthesis stage graceful skip; simulation always runs |
| COM port detection fails on Windows | Medium | Medium | Manual `--com-port` override; existing fpga_pipeline.py logic |
| Cross-platform CLI differences | Medium | Medium | Test on Windows (MSYS2) primary environment |
| Checkpoint corruption | Low | High | Atomic writes + backup checkpoint |
| Metrics drift (Phase 3 baseline not reproducible) | Low | Medium | Baseline frozen from Phase 3 completion report |
| Power estimation inaccuracy | High | Low | Clearly label as estimate; toggle-rate analysis is approximate |

---

## 11. Integration with Previous Phases

| Phase | Integration Point |
|-------|-------------------|
| **Phase 1** (Lean4 proofs) | Generated code preserves proven algebraic properties; hardware demos verify XOR algebra in silicon; metrics confirm algebraic invariants hold at-speed |
| **Phase 2** (Benchmarks) | Domain schema constraints informed by benchmark results; pipeline metrics extend Phase 2 data with per-domain hardware measurements |
| **Phase 3** (FPGA) | Hardware stage wraps Phase 3 infrastructure (fpga_pipeline.py, test_hardware.py); Phase 3 metrics serve as performance baseline for all domain comparisons |
| **Phase 4A** (Generator) | Pipeline wraps GeneratorEngine as its generation stage; all 5 language generators used unchanged |
| **Phase 4B** (Domains) | 3 domain schemas serve as pipeline test corpus and demo inputs; generated Verilog is the starting point for domain demonstrators |

---

## Execution Schedule

| Step | Tasks | Dependencies | Est. Tokens |
|------|-------|-------------|-------------|
| 1 | T4C.1 (Controller) + T4C.5 (Context) + T4C.6 (Router) | Phase 4B complete | 40K |
| 2 | T4C.2 (Diff) + T4C.3 (Generation) | Step 1 | 27K |
| 3 | T4C.4 (Verify) + T4C.7 (Hardware) + T4C.8 (Metrics) | Step 2 | 43K |
| 4 | T4C.9 (Video Demo) + T4C.10 (Sensor Demo) + T4C.11 (Finance Demo) | Step 3 | 56K |
| 5 | T4C.12 (CLI) + T4C.13 (Tests & Docs & Benchmarks) | Step 4 | 27K |
| **Total** | **13 tasks** | | **~193K + 29K contingency** |

Steps within each group contain independent tasks that can execute in parallel.

---

## Success Criteria

Phase 4C is complete when:

1. `atomik-gen pipeline run sdk/schemas/domains/` succeeds end-to-end with all 3 domain schemas
2. `atomik-gen pipeline diff` correctly reports "up to date" when re-run with no changes (0 tokens)
3. Injecting a schema change triggers selective regeneration (measurably fewer files than full generation)
4. Injecting a lint error triggers self-correction without human intervention
5. Pipeline token cost for a new domain schema is under 15K tokens
6. **RTL simulation passes 100% on all 3 domain schemas**
7. **FPGA synthesis succeeds for all 3 domains on GW1NR-9**
8. **On-device validation passes 10/10 tests per domain (when hardware connected)**
9. **All performance metrics taxonomy fields populated for each domain**
10. **Cross-domain performance comparison report generated with actual measurements**
11. **Performance metrics compared against Phase 3 baseline**
12. All 87+ existing tests continue to pass alongside 36+ new pipeline/metrics tests
13. Pipeline checkpoint enables cold-start context restoration in <2K tokens
14. `atomik-gen demo video|sensor|finance` executes corresponding demonstrator
15. Documentation updated across README, Developer Guide, and API Reference

---

**Document Version:** 2.0
**Date:** January 27, 2026
**Author:** Claude Opus 4.5
**Phase:** 4C - Autonomous Token-Efficient Pipelines for Agentic Execution
**Status:** PLANNED
