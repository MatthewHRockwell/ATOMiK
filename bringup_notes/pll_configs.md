# PLL Configurations — Tang Nano 9K (GW1NR-9 rPLL)

Purpose: Track every PLL configuration used during ATOMiK bring-up so frequency sweeps are reproducible and results are defensible.

---

## Board & Device Context
- Board: Tang Nano 9K
- Device: GW1NR-LV9QN88PC6/I5 (GW1NR-9C)
- Reference oscillator: 27 MHz (sys_clk input)

---

## Rules for PLL Config Management
1) Prefer one PLL IP per target frequency (e.g., `atomik_pll_81m`, `atomik_pll_108m`) to avoid accidental drift.
2) Always record:
   - Input frequency (FCLKIN)
   - IDIV / FBDIV / ODIV selections (or VCO divide settings as exposed)
   - Whether dynamic divisors are enabled
   - Output clock used (CLKOUT vs CLKOUTP)
   - Reset polarity and lock usage
3) Ensure `lock` gates the internal reset release for fabric logic.

---

## PLL Instance Interface Convention (Recommended)
For each PLL wrapper module:
- Inputs:
  - `clkin`  (27 MHz ref)
  - `reset`  (active-high reset into PLL)
- Outputs:
  - `clkout` (fabric clock)
  - `lock`   (PLL lock indicator)

Example wrapper signature:
`module atomik_pll_XXXm (clkout, lock, reset, clkin);`

---

## PLL Config Table

| Name / Module | Target MHz | FCLKIN | IDIV | FBDIV | ODIV | Notes |
|--------------|-----------:|-------:|-----:|------:|-----:|------|
| atomik_pll_81m    | 81.0  | 27 |     |      |     | Baseline PLL bring-up |
| atomik_pll_94p5m  | 94.5  | 27 | 1   | 6    | 8   | Current timing-clean baseline (UI-legal; replaced 96 MHz target) |
| atomik_pll_96m    | 96.0  | 27 |     |      |     | UI target only; not reachable in rPLL UI (do not use unless proven achievable) |
| atomik_pll_108m   | 108.0 | 27 |     |      |     | Candidate >100 MHz milestone (UI-dependent) |
| atomik_pll_120m   | 120.0 | 27 |     |      |     | Candidate stretch (UI-dependent) |
| atomik_pll_135m   | 135.0 | 27 |     |      |     | Optional (UI-dependent) |
| atomik_pll_150m   | 150.0 | 27 |     |      |     | Optional (UI-dependent) |

Fill in divisors based on generated wrapper `defparam` lines. based on generated wrapper `defparam` lines.

---

## Stored Config Blocks (Copy/Paste)

### atomik_pll_81m
- File: `rtl/pll/atomik_pll_81m.v`
- Output used: CLKOUT
- Reset polarity into PLL: active-high (`RESET(reset)`)

Defparams (record from file):
- FCLKIN = "___"
- DYN_IDIV_SEL = ___ ; IDIV_SEL = ___
- DYN_FBDIV_SEL = ___ ; FBDIV_SEL = ___
- DYN_ODIV_SEL = ___ ; ODIV_SEL = ___
- DUTYDA_SEL / PSDA_SEL = ___
- CLKFB_SEL = ___
- DEVICE = ___

Notes:
- Lock must be stable before releasing fabric reset.
- Ensure SDC defines `atomik_clk` on PLL CLKOUT object.


### atomik_pll_94p5m
- File: `rtl/pll/atomik_pll_94p5m.v`
- Output used: CLKOUT
- Reset polarity into PLL: active-high (`RESET(reset)`)

Defparams (from generated wrapper):
- FCLKIN = "27"
- DYN_IDIV_SEL = "false" ; IDIV_SEL = 1
- DYN_FBDIV_SEL = "false" ; FBDIV_SEL = 6
- DYN_ODIV_SEL = "false" ; ODIV_SEL = 8
- DYN_DA_EN = "false"
- PSDA_SEL = "0000" ; DUTYDA_SEL = "1000"
- CLKFB_SEL = "internal"
- DEVICE = "GW1NR-9C"

Notes:
- This is the timing-clean 94.5 MHz fabric clock baseline.
- rPLL UI frequency selection is quantized; do not assume 96.0 MHz is reachable even if math suggests it.
### atomik_pll_108m
(same template)

---

## Common Pitfalls
- Timing constraints still set to 27 MHz while fabric is PLL-clocked: leads to meaningless STA.
- PLL lock not gating reset: can cause intermittent boot / UART weirdness.
- Using CLKOUTP for timing while fabric clocks CLKOUT (or vice versa): inconsistent reports.
- Dynamic divisor settings left enabled unintentionally: makes reproducibility harder.
