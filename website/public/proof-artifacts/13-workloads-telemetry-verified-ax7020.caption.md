# ATOMiK Verified Telemetry-Sync Savings — Live on AX7020 Screenshot Caption

Suggested caption:

> ATOMiK's redesigned Workloads surface on the AX7020 over HDMI, captured live from /dev/fb0. The Edge Telemetry Sync comparison is LIVE_MEASURED and hardware-verified: a 256-region state tick moves 2,048 bytes the conventional way vs 160 bytes as ATOMiK deltas — 92% less data moved — and every delta was verified on the ATOMiK adapter (exact 64-bit LOAD/ACCUM/READ round-trips at 0xF0020000), with the same final state on both sides.

Short label for slides or proof cards:

`LIVE_MEASURED + adapter-verified savings — NOT a universal multiplier or interactive demo`

Usage rules (read before quoting):

- **Scope the numbers tightly.** The comparison figures (2,048 B vs 160 B, 92% less data moved) are measured and adapter-verified for THIS deterministic test pattern at 5% change density on one tick. The ratio scales with change density — it is NOT a universal multiplier, and must never be blended with the retired 7,670x–916,000x software-suite range.
- **The verification is falsification-tested.** The harness requires nonzero values round-tripped through the adapter and provably fails against mock all-zero hardware; verified=1 means real silicon round-trips matched exactly.
- **Not an interactive demo.** Static framebuffer capture. Does NOT prove an interactive session or USB keyboard/mouse input (still in flight). Do not say "live interactive demo" or "plug in a keyboard."
- Resource Fabric lane values and top-bar temperature/efficiency/predictive readouts remain DERIVED/SCENARIO — never quote them as measured.
- Does not prove production maturity, customer workload value, battery, heat, cooling, water, power, uptime, or footprint outcomes.
