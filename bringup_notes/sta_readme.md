# STA Readme — How to Read Gowin Timing Reports (ATOMiK / Tang Nano 9K)

This document is the “do not lie to yourself” guide for interpreting Gowin Timing Analyzer output. It is written to prevent common misreads (especially confusing “Actual Fmax” with “clock is running at X”).

The goal: every frequency claim you make (81 MHz, 108 MHz, etc.) should be defensible from:
1) Post-P&R STA results, and
2) A repeatable hardware validation step.

---

## 1) The Three Tables You Must Read Every Time

In a Gowin timing report, you must always capture:

1) **Clock Summary**
2) **Max Frequency Summary**
3) **Total Negative Slack (TNS) Summary**

If you do not have these three, you do not have timing truth.

---

## 2) Clock Summary — “What clocks exist, and what did we constrain?”

**Clock Summary answers:**
- What clocks does STA know about?
- What are their periods/frequencies?
- What exact **object** is each clock attached to?

### What to look for (ATOMiK example)
From the report:

- `sys_clk` (Base) is the 27 MHz reference clock on the top-level port:
  - Period: 37.037 ns
  - Frequency: 27.000 MHz
  - Object: `sys_clk`

- `atomik_clk` (Base) is your fabric clock on the PLL output pin:
  - Period: 12.346 ns
  - Frequency: ~80.998 MHz
  - Object: `gen_pll.u_pll/rpll_inst/CLKOUT`

You may also see Generated clocks such as:
- `gen_pll.u_pll/rpll_inst/CLKOUTP.default_gen_clk`
- `.../CLKOUTD.default_gen_clk`
- `.../CLKOUTD3.default_gen_clk`

These are normal if the PLL wrapper exposes those outputs (even if you do not use them in logic).

### Practical rule
If your actual design logic runs on PLL output, you must see a fabric clock entry like:
- `atomik_clk` attached to `.../CLKOUT` (or the net it drives)

If you only see `sys_clk` at 27 MHz and nothing else, your PLL-clocked logic is not properly constrained and STA is not telling you the truth.

---

## 3) Max Frequency Summary — “How fast could this design go?”

**Max Frequency Summary answers:**
- Given the placed-and-routed design, what is the estimated maximum frequency each clock domain *could* run before failing setup timing?

### The most common misread
**Actual Fmax is not the operating clock frequency.**

Example from your report:
- `sys_clk` constraint: 27.000 MHz
- `sys_clk` Actual Fmax: ~200.184 MHz

This does NOT mean sys_clk is “running at 200 MHz.”
It means: *the logic timed in the sys_clk domain is so small/fast that it could theoretically run at ~200 MHz.*

Your actual sys_clk is still 27 MHz, as defined by your input oscillator and Clock Summary.

### How to use Actual Fmax properly
- If you constrain `atomik_clk` at 108 MHz and STA shows Actual Fmax 104 MHz, your design does not close at 108 MHz.
- If you constrain `atomik_clk` at 108 MHz and STA shows Actual Fmax 130 MHz with no violations, your design likely has margin.

But you still must check violations/TNS.

---

## 4) Total Negative Slack Summary — “Did timing actually fail?”

**TNS Summary answers:**
- Are there setup or hold violations?
- How many endpoints are violating?
- How much total negative slack exists?

### “Pass” criteria
A clock domain is considered timing-clean if:
- Setup TNS = 0.000 and endpoints = 0
- Hold  TNS = 0.000 and endpoints = 0

Your report shows zeros across the board.

### Important nuance: “Endpoints = 0”
If a clock shows TNS=0 but endpoints=0, that can mean:
- There were no timed endpoints for that clock (clock exists but not used), OR
- The tool didn’t propagate timing to those paths due to missing constraints.

Therefore:
- Always correlate **Clock Summary** + **Path tables** when something looks suspicious.

---

## 5) “No timing paths to get frequency of …” — what that really means

Your report includes messages like:
- “No timing paths to get frequency of gen_pll.u_pll/rpll_inst/CLKOUTP.default_gen_clk!”

Meaning:
- The tool sees the generated clock object,
- But it cannot find synchronous timing paths (register-to-register paths) in that domain to evaluate.

This is common if that PLL output is not used to clock any sequential elements.

It is not an error. It is only a warning sign if you expected logic to be clocked by that output.

---

## 6) Setup vs Hold — how to interpret in practice

### Setup violations
- Typically happen when logic depth is too large for the chosen period.
- Fixes usually involve:
  - Pipelining,
  - Reducing comparator width / restructuring arithmetic,
  - Registering control signals,
  - Improving fanout (buffering/replication) or placement constraints.

### Hold violations
- Usually happen when paths are too fast relative to clock skew/uncertainty.
- Fixes typically involve:
  - Adding delay (register stages) or tool hold-fixing options,
  - Avoiding gating glitches / reset issues.

Hold violations are less common in clean synchronous RTL unless there are CDC/reset problems.

---

## 7) Reading the Path Slacks Table — finding the real bottleneck

When you are pushing >100 MHz, you will live in:
- “Setup Paths Table”
- “Path Summary / Data Arrival / Data Required”

You should capture:
- The startpoint register and endpoint register,
- The total logic levels,
- The named nets/modules on the path.

### Practical workflow
1) Identify Path1 (worst setup path).
2) Determine if it is:
   - In `atomik_core`,
   - In the UART loader,
   - In reset/gating/clock enable logic,
   - Or across module boundaries.
3) Change only the logic that shortens that specific path.
4) Re-run at the same frequency. Confirm the bottleneck moved or improved.

---

## 8) Clocking + Constraints Checklist (Before Every Sweep Run)

### Must-haves
- `sys_clk` constrained at 27 MHz on `[get_ports {sys_clk}]`
- `atomik_clk` constrained at the PLL output object used by fabric logic.
  - Example from your report:
    - `gen_pll.u_pll/rpll_inst/CLKOUT`

### UART constraints
If UART RX is sampled in the fabric domain, tie IO delays to `atomik_clk`.
If UART logic is moved to sys_clk domain, tie them to `sys_clk`.

### Comment syntax
Gowin SDC uses `#` comments.
Do not use `//` comments in SDC files.

---

## 9) “Passing STA” is necessary, not sufficient

Before claiming a new frequency milestone (e.g., 108 MHz):
- STA must pass with no setup/hold violations for `atomik_clk`.
- Hardware must pass a repeatable validation:
  - PLL lock stable,
  - UART stable (no phantom chars/deletes),
  - BIOS load success,
  - core_enable behavior repeatable,
  - Alive signature (LED/telemetry) repeatable.

If either fails, the frequency is not production-credible.

---

## 10) Recording Results for Documentation / LinkedIn

When publishing a frequency milestone, record the following from STA:
- Clock Summary row for `atomik_clk` (period, frequency, object)
- Max Frequency Summary row for `atomik_clk` (constraint + Actual Fmax)
- Total Negative Slack Summary for `atomik_clk` (setup + hold)

Avoid stating “sys_clk is running at 200 MHz” when you mean “sys_clk Fmax is ~200 MHz.”

---

## Appendix A — Known Objects from Current Report (for copy/paste)

From your timing report:
- Reference clock object:
  - `sys_clk`
- Fabric clock object:
  - `gen_pll.u_pll/rpll_inst/CLKOUT`

Generated clocks present (may be unused):
- `gen_pll.u_pll/rpll_inst/CLKOUTP.default_gen_clk`
- `gen_pll.u_pll/rpll_inst/CLKOUTD.default_gen_clk`
- `gen_pll.u_pll/rpll_inst/CLKOUTD3.default_gen_clk`
