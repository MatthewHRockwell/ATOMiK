# ATOMiK Verified Control-Coalescing + Self-Driving Demo — Live on AX7020 Screenshot Caption

Suggested caption:

> Control-Update Coalescing on the AX7020, captured live from /dev/fb0 while the demo was self-driving (AUTO): 256 control updates touching 32 registers coalesce into 32 net delta writes — 87% fewer writes — verified on the ATOMiK adapter with exact 64-bit round-trips, final register state identical. The untethered board cycles all three workload scenarios on its own (paired capture workloads_demo_cycleB shows a different scenario ~15s later).

Short label for slides or proof cards:

`LIVE_MEASURED + adapter-verified; self-driving = AUTO-CYCLING, NOT interactive input`

Usage rules (read before quoting):

- **Scope the numbers tightly.** 256 → 32 writes (87% fewer) is measured and adapter-verified for this deterministic update pattern; the ratio depends on update locality.
- **"Self-driving" means the surface auto-cycles scenarios with no input attached** (proven by two captures showing different scenarios). It does NOT mean interactive input: USB keyboard/mouse remain in flight; never present this as an interactive session.
- Resource Fabric lane values and top-bar readouts remain DERIVED/SCENARIO — never quote them as measured.
- Does not prove production maturity, customer workload value, battery, heat, cooling, water, power, uptime, or footprint outcomes.
