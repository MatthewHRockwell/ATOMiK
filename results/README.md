# Results and Benchmark Artifacts

This directory is for recorded ATOMiK proof artifacts. Do not place projected
or illustrative marketing numbers here unless they are clearly labeled.

The current public result package is the AX7020 performance matrix from
2026-05-09. New measured results should be added only when the raw artifact,
interpretation notes, and evidence label are ready together.

## Required Fields

Each benchmark or validation package should include:

- workload
- platform
- hardware
- software version
- date
- command
- raw output
- interpretation
- evidence label
- commit hash when available

## Evidence Labels

Use the public evidence framework in
[docs/evidence-labels.md](../docs/evidence-labels.md):

- `LIVE_MEASURED`
- `HARDWARE_VALIDATED`
- `SOFTWARE_VALIDATED`
- `SYNTHESIS_VALIDATED`
- `PROJECTED`
- `CONCEPTUAL`
- `ROADMAP`

## Current Artifacts

| Artifact | Label | Notes |
|---|---|---|
| `perf_matrix_ax7020_20260509.txt` | `LIVE_MEASURED` | Raw AX7020 board run output. See `docs/perf/20260509_matrix_interpretation.md` for interpretation and caveats. |
| `claims_registry.yaml` | registry | Public source of truth for claim labels, artifact paths, and publication status. |
