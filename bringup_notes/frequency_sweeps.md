# Frequency Sweeps — ATOMiK on Tang Nano 9K (GW1NR-LV9)

Purpose: Establish a disciplined, reproducible process to scale ATOMiK frequency under safe operating conditions while maintaining timing truth (post-P&R STA) and functional stability.

---

## Safety & Ground Rules

### Definition of “safe”
- Default board conditions (no over-volting, no unconventional cooling assumptions).
- PLL input remains the onboard 27 MHz oscillator.
- Only claim a frequency as “supported” if:
  - Post-P&R STA shows no setup/hold violations for the relevant clock domain(s),
  - The bitstream runs stably on hardware using a repeatable functional check.

### Timing truth rules
- Always constrain clocks in SDC to match the active implementation:
  - `sys_clk` = 27 MHz reference clock on `sys_clk` port.
  - `atomik_clk` = PLL `CLKOUT` object (or net/pin) used to clock the fabric.
- Never interpret “Actual Fmax” as the operating clock.
- Record both the constraint and the achieved slack (or, if only Fmax is shown, record “Actual Fmax”).

### Functional truth rules
- At each frequency, run the same minimal hardware validation:
  1) UART link check (PuTTY at 115200).
  2) BIOS genome load succeeds (or if bypassing gating, a documented debug mode is enabled).
  3) LED or UART telemetry provides a repeatable “alive” signature.

---

## Sweep Procedure (Repeatable)

### Step 1 — Choose target frequency
Select next target in sequence (recommended):
- 81 MHz (baseline PLL bring-up)
- 96 MHz
- 108 MHz (first “over 100” milestone)
- 120 MHz
- 135 MHz (optional stretch)
- 150 MHz (optional stretch; expect redesign/CDC/pipelining)

### Step 2 — Generate / select PLL variant
- Create a dedicated PLL IP instance per target (recommended) OR update a single PLL config (faster, but less reproducible).
- Ensure PLL wrapper has:
  - `clkin` connected to 27 MHz input.
  - `clkout` used for fabric clock (`atomik_clk`).
  - `lock` exposed and used to qualify reset release.

Record PLL params (see `pll_configs.md`).

### Step 3 — Update timing constraints (SDC)
Update/confirm:
- `create_clock` on `sys_clk` port (27 MHz).
- `create_clock` for the PLL output clock object used to drive the fabric.

Period calculation:
- Period(ns) = 1000 / F(MHz)

Waveform recommendation:
- `{0 Period/2}`

Example reference (replace object path with your clock pin/net):
- 108 MHz: period 9.259 ns, half 4.629 ns

### Step 4 — Run Gowin flow
Run:
1) Synthesis
2) Place & Route
3) Timing Analysis (STA)

Collect:
- Timing Summaries screenshot (Clock Summary, Max Frequency Summary, TNS)
- P&R report (if available) for utilization / critical path hints

### Step 5 — Hardware validation
Program FPGA and validate:
- PLL lock indicator behaves as expected (if mapped).
- UART behavior is stable at 115200 (no random characters, no phantom deletions).
- Genome load + core enable works (or document bypass mode).
- “Alive” signature is repeatable (LED heartbeat or UART telemetry).

### Step 6 — Record results
Use the run log tables below. Do not skip fields.

---

## Sweep Run Log

### Run Template (copy per run)
**Run ID:** YYYY-MM-DD__FREQMHz__TAG  
**Target Fabric Clock (atomik_clk):** ___ MHz  
**PLL module:** ___  
**SDC period for atomik_clk:** ___ ns  
**Tool:** Gowin IDE version ___  
**Device:** GW1NR-LV9QN88PC6/I5 (Tang Nano 9K)

**STA Results**
- Clock Summary:
  - sys_clk: ___ MHz (expected 27.0)
  - atomik_clk: ___ MHz (expected target)
- Max Frequency Summary:
  - atomik_clk constraint: ___ MHz
  - atomik_clk Actual Fmax: ___ MHz
- Slack / Violations:
  - Setup violated endpoints: ___
  - Hold violated endpoints: ___
  - atomik_clk TNS: ___

**Hardware Results**
- Bitstream programmed: Yes/No
- PLL lock stable: Yes/No
- UART stable (115200): Yes/No
- Genome load success: Yes/No
- Core enable: Yes/No
- Observed signature: (describe)

**Notes**
- Any warnings (PLL mismatch, unconstrained paths, etc.)
- Any design changes relative to baseline
- Next action / hypothesis

---

## Results Summary Table

| Target MHz | PLL Out MHz (STA) | atomik_clk Constraint MHz | atomik_clk Actual Fmax MHz | Setup Violations | Hold Violations | HW Pass? | Notes |
|-----------:|-------------------:|--------------------------:|---------------------------:|----------------:|----------------:|:--------:|------|
| 81         |                    |                           |                            |                 |                 |          |      |
| 96         |                    |                           |                            |                 |                 |          |      |
| 108        |                    |                           |                            |                 |                 |          |      |
| 120        |                    |                           |                            |                 |                 |          |      |
| 135        |                    |                           |                            |                 |                 |          |      |
| 150        |                    |                           |                            |                 |                 |          |      |

---

## Interpretation Notes (Do Not Misread STA)

### “Actual Fmax” means:
Given the placed-and-routed design, the estimated maximum frequency that would meet setup timing in that domain.

### “Clock Summary” shows:
What clocks exist and what the tool believes their frequencies are based on constraints and generated clocks.

### “No timing paths to get frequency…”
Typically means a generated clock exists but no synchronous endpoints are currently timed under it (or it is unused). This can be normal.

---

## Escalation Strategy When a Target Fails

If timing fails at target frequency:

1) Confirm constraints match the real clock source object.
2) Confirm the core is not being optimized/swept due to gating.
3) Identify critical path:
   - Compare logic levels, cone depth, wide compares, fanout.
4) Apply one change at a time:
   - Pipeline seed update path
   - Replace wide comparator with down-counter
   - Split clocks: UART loader @ 27 MHz, core @ atomik_clk
5) Re-run at last passing frequency before reattempting higher target.

Stop when functional stability is not repeatable, even if STA passes.
