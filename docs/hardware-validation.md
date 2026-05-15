# Hardware Validation

This page separates live hardware proof, hardware validation, synthesis output,
and roadmap work.

## Live / Hardware-Backed

| Artifact | Label | Notes |
|---|---|---|
| `website/public/08-current-live-atomik-desk-v038i.png` | `HARDWARE_VALIDATED` | Current ATOMiK Desk v0.38-I prototype UI running on live hardware. |
| `website/public/07-current-live-atomik-desk-v038g.png` | `HARDWARE_VALIDATED` | Earlier ATOMiK Desk v0.38-G prototype UI running on live hardware. |
| `website/public/01-current-live-atomik-desk.jpg` | `HARDWARE_VALIDATED` | Earlier ATOMiK Desk prototype running on live hardware. |
| `docs/LINUX_USERSPACE_PROOF.md` | `HARDWARE_VALIDATED` | Linux userspace to FPGA accelerator validation path. |
| `results/perf_matrix_ax7020_20260509.txt` | `LIVE_MEASURED` | Raw AX7020 board run output with interpretation caveats in `docs/perf/`. |

## Synthesis / Toolchain Output

Synthesis ceilings are useful, but they are not live-board measurements. Public
copy must label them as `SYNTHESIS_VALIDATED` unless a matching hardware run is
recorded.

Relevant source:

- `docs/HARDWARE_SYNTHESIS.md`

## Roadmap Hardware Work

Roadmap hardware and compiler integration language should be phrased as planned
work unless the repo contains a dated artifact showing the end-to-end path.

Examples:

- Future standard-C / GCC adoption lane: `ROADMAP` until end-to-end artifact is
  published.
- Productized Resource Fabric workload reallocation: `ROADMAP` unless live demo
  artifacts prove the behavior.
- Longer-term agent/context surfaces: `ROADMAP` / `CONCEPTUAL`.

## Public Caption Rule

Use:

> Current ATOMiK Desk prototype running on live hardware.

For the current UI upgrade track, use:

> ATOMiK Desk v0.38-I prototype UI running on live hardware.

Do not infer benchmark, uptime, allocation, maturity, or commercial product
claims from the screenshot.

Avoid:

- commercial product
- production deployment
- customer validated
- unqualified speedup
- hardware verified for synthesis-only claims
