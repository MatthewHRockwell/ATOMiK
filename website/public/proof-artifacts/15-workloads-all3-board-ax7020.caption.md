# All Three Workloads Scenarios — Live on AX7020 Screenshot Caption

Suggested caption:

> All three ATOMiK Workloads scenarios on the AX7020 over HDMI, captured live
> from /dev/fb0 with the production typography: Edge Telemetry Sync (2,048 B ->
> 160 B, 92% less data moved), Control-Update Coalescing (256 -> 32 writes, 87%
> fewer), and Parallel Aggregation (65,537 -> 8,193 cycles, 8x faster). The two
> memory scenarios are adapter-verified (exact 64-bit round-trips on the ATOMiK
> delta-state adapter @0xF0020000); the parallel scenario is measured on the
> parallel-bank engine @0xF0021000.

Short label for slides/proof cards:

`Three customer workloads, board-measured + adapter-verified — NOT universal multipliers`

Usage rules (read before quoting):

- **Pattern-dependent ratios.** 92% is at 5% telemetry change density; 87% at
  this update locality; 8x is 1-vs-8 lanes. Real ratios scale with the workload
  — these are NOT universal multipliers and must never be blended with the
  retired 7,670x-916,000x software-suite range.
- **Verification is falsification-tested:** the harness requires nonzero values
  to round-trip and fails against mock all-zero hardware.
- Static framebuffer captures, not an interactive session; no USB-input claim.
- Fabric lane / top-bar values remain DERIVED/SCENARIO — quote only the three
  workload comparison figures.
