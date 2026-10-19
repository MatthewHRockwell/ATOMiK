# ATOMiK Workloads Surface — Live on AX7020 Screenshot Caption

Suggested caption:

> ATOMiK's Workloads surface (real-time telemetry aggregation) running on the AX7020 over HDMI, captured live from /dev/fb0. The on-screen throughput is LIVE_MEASURED from the parallel-bank engine: 800 Mevents/s at 8 banks, with 1/2/4/8 throughput scaling and a byte-identical result across all bank configurations (order-independent, lock-free). The first customer-facing workload demo proven end-to-end on hardware.

Short label for slides or proof cards:

`LIVE_MEASURED throughput on hardware — NOT a customer workload benchmark or interactive demo`

Usage rules (read before quoting):

- **Scope the numbers tightly.** Only the throughput figures — 800 Mevents/s, the 1/2/4/8x scaling, and "byte-identical across configs" — are LIVE_MEASURED (they come from the parallel-bank engine at 0xF0021000, fed via a board-side daemon writing /tmp/atomik_bench_live.txt that the surface reads). Everything else on screen (Resource Fabric STATE/SYNC/AGENT/EVENT/VISUAL values, and the top-bar temperature / efficiency / predictive readouts) is DERIVED or SCENARIO and must NOT be quoted as measured customer, power, or thermal proof.
- **Not an interactive demo.** This is a static framebuffer capture of the surface rendering. It does NOT prove an interactive session, and it does NOT prove USB keyboard/mouse input — both remain in flight and are not board-confirmed. Do not say "live interactive demo" or "plug in a keyboard and use it."
- **What it DOES support:** the Workloads / telemetry-aggregation surface renders on AX7020 hardware driven by real measured parallel-bank throughput. Pair with `PARALLEL_BANKS_HARDWARE_VALIDATED.md` for the underlying measurement.
- Does not prove production maturity, customer workload value, battery, heat, cooling, water, power, uptime, or footprint outcomes.
