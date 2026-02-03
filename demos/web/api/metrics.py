"""REST endpoints for project metrics and demo data."""

from __future__ import annotations

from pathlib import Path

PROJECT_METRICS = {
    "throughput_gops": 1.056,
    "hardware_tests": "80/80",
    "formal_proofs": 92,
    "sdk_tests": 314,
    "sdk_languages": 5,
    "device_cost": 10,
    "dev_cost": 225,
    "lut_utilization": 7,
    "clock_mhz": 94.5,
    "latency_ns": 10.6,
    "parallel_banks": 16,
    "phases_complete": 6,
    "tam_b": 614,
    "sam_b": 12,
    "som_m": 80,
}

TRACTION_TIMELINE = [
    {"date": "2023-05", "event": "Project started"},
    {"date": "2026-01-25", "event": "Phase 1-3: Proofs + Hardware validated"},
    {"date": "2026-01-26", "event": "Phase 4: SDK + Domain SDKs complete"},
    {"date": "2026-01-27", "event": "Phase 5-6: Agentic pipeline + 1 Gops/s"},
    {"date": "2026-01-31", "event": "Robustness hardening, 314 tests"},
    {"date": "2026-02-02", "event": "Funding automation + demo website"},
]

PROOF_CATEGORIES = [
    {"category": "Core algebra", "count": 12},
    {"category": "Commutativity & associativity", "count": 8},
    {"category": "Self-inverse", "count": 6},
    {"category": "Parallel merge", "count": 10},
    {"category": "State reconstruction", "count": 8},
    {"category": "Turing completeness", "count": 15},
    {"category": "Hardware correspondence", "count": 12},
    {"category": "Edge cases", "count": 21},
]

SYNTHESIS_CONFIGS = [
    {"banks": 1, "freq_mhz": 94.5, "throughput_mops": 94.5, "lut": 477},
    {"banks": 2, "freq_mhz": 94.5, "throughput_mops": 189.0, "lut": 600},
    {"banks": 4, "freq_mhz": 81.0, "throughput_mops": 324.0, "lut": 745},
    {"banks": 8, "freq_mhz": 67.5, "throughput_mops": 540.0, "lut": 1126},
    {"banks": 16, "freq_mhz": 66.0, "throughput_mops": 1056.0, "lut": 1779},
]


def get_metrics() -> dict:
    return PROJECT_METRICS


def get_traction() -> list[dict]:
    return TRACTION_TIMELINE


def get_proofs() -> list[dict]:
    return PROOF_CATEGORIES


def get_synthesis() -> list[dict]:
    return SYNTHESIS_CONFIGS
