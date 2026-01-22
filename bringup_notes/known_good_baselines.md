# Known-Good Baselines — ATOMiK (Tang Nano 9K)

Purpose: Maintain a small set of “gold standard” configurations that are known to:
- Build cleanly (synth + P&R + STA),
- Program and run reliably on hardware,
- Provide a stable rollback point during optimization.

---

## Baseline Definition Requirements
A baseline is “known good” only if all are true:
1) Synthesis completes with no fatal errors.
2) P&R completes and generates bitstream.
3) Timing Analysis shows:
   - No setup violations
   - No hold violations
   - Relevant clocks constrained correctly
4) Hardware validation passes:
   - PLL lock (if used) stable
   - UART stable (115200)
   - Loader behavior repeatable
   - Core enable behavior repeatable
   - Visible “alive” signature (LED or telemetry)

---

## Baseline Index (Current)

| Baseline ID | Date | Fabric Clock | PLL | STA Status | HW Status | Notes |
|------------|------|-------------:|-----|-----------:|----------:|------|
| BL-81-PLL  |      | 81 MHz | Yes | Pass | Pass | First PLL-closed baseline |
| BL-27-NO-PLL |   | 27 MHz | No  | Pass | Pass | Debug baseline |
| BL-96-PLL  |      | 96 MHz | Yes |     |     | Candidate baseline |
| BL-108-PLL |      | 108 MHz | Yes |     |     | “Over 100” milestone candidate |

---

# Baseline Record Template (Copy/Paste)

## Baseline: BL-81-PLL

**Date created:** 2026-01-21  
**Git/Tag (if applicable):** `v81_pll_timing_clean` *(recommended tag name)*  
**Device:** GW1NR-LV9QN88PC6/I5 (Tang Nano 9K)  
**Toolchain:** Gowin IDE (Education) v1.9.11.03  

---

### Clocking

- **sys_clk (reference):**
  - Source: Top-level port `sys_clk`
  - Frequency (Clock Summary): **27.000 MHz**
  - Period: 37.037 ns

- **fabric clock name:** `atomik_clk`
- **fabric clock source:** PLL `CLKOUT`
- **PLL module:** `atomik_pll_81m`
- **fabric clock frequency (Clock Summary):** **80.998 MHz**
- **fabric clock period:** 12.346 ns
- **PLL lock used to gate reset:** **Yes**

**Clock object used in SDC:**

```yaml
gen_pll.u_pll/rpll_inst/CLKOUT
```
---

### Constraints

- **SDC file:** `constraints/timing_constraints.sdc`

**create_clock definitions:**
- `sys_clk`: 37.037 ns (27.000 MHz)
- `atomik_clk`: 12.346 ns (~80.998 MHz)

**IO delays:**
- `uart_rx`: max 2.0 ns (relative to `atomik_clk`)
- `uart_tx`: max 2.0 ns (relative to `atomik_clk`)

**False paths:**
- Reset (`sys_rst_n`): Yes
- LEDs (`led[*]`): Yes

---

### STA Snapshot (Post-P&R)

**Setup Timing**
- Violations: **0**
- Endpoints: **0**
- TNS: **0.000**

**Hold Timing**
- Violations: **0**
- Endpoints: **0**
- TNS: **0.000**

**Max Frequency Summary**
- `atomik_clk`
  - Constraint: **80.998 MHz**
  - **Actual Fmax:** **81.061 MHz**
  - Logic levels on critical path: **8**

- `sys_clk`
  - Constraint: **27.000 MHz**
  - Actual Fmax: ~**200 MHz**
  - *(High Fmax due to minimal logic in sys_clk domain; not an operating frequency)*

**Generated clocks present (unused by fabric logic):**
- `gen_pll.u_pll/rpll_inst/CLKOUTP.default_gen_clk`
- `gen_pll.u_pll/rpll_inst/CLKOUTD.default_gen_clk`
- `gen_pll.u_pll/rpll_inst/CLKOUTD3.default_gen_clk`

No timing paths reported for unused generated clocks (expected).

---

### Hardware Validation

- Bitstream programmed successfully: **Yes**
- PLL lock stable: **Yes**
- UART stable @ 115200 baud: **Yes**
- BIOS / genome loader functional: **Yes**
- Core enable behavior: **Yes**

**Alive signature:**
- Heartbeat LED: **Yes**
- Loader activity LED: **Yes**
- Core debug LED: **Yes**

Observed behavior is repeatable across resets and reprogramming.

---

### Files Included in Baseline

- `rtl/atomik_top.v`
- `rtl/atomik_core.v`
- `rtl/uart_genome_loader.v`
- `rtl/pll/atomik_pll_81m.v`
- `constraints/atomik_constraints.cst`
- `constraints/timing_constraints.sdc`
- Timing report: `reports/timing/ATOMiK_tr_content.html`

---

### Notes / Rationale

This is the **first PLL-based, timing-clean baseline** for ATOMiK on the Tang Nano 9K.

Key properties:
- Fabric logic is fully clocked from a constrained PLL output.
- Timing closure is real (post-P&R, zero violations).
- UART loader is PLL-safe and stable.
- Reset sequencing (POR + PLL lock) is correct.
- No reliance on swept or constant logic to fake closure.

This baseline serves as:
- A rollback point for all future frequency scaling work.
- The reference configuration for documentation and external communication.
- The foundation for attempts to exceed **100 MHz**.

---

### Next Intended Step

Attempt controlled frequency scaling using:
- `atomik_pll_96m`
- then `atomik_pll_108m` (first **>100 MHz** milestone)

Following the procedure defined in:
- `bringup_notes/frequency_sweeps.md`

---

## Baseline: BL-96-PLL (Skeleton)

**Date created:** YYYY-MM-DD  
**Git/Tag (if applicable):** `v96_pll_candidate`  
**Device:** GW1NR-LV9QN88PC6/I5 (Tang Nano 9K)  
**Toolchain:** Gowin IDE ___  

---

### Clocking

- **sys_clk (reference):**
  - Source: Top-level port `sys_clk`
  - Frequency (Clock Summary): **27.000 MHz**
  - Period: 37.037 ns

- **fabric clock name:** `atomik_clk`
- **fabric clock source:** PLL `CLKOUT`
- **PLL module:** `atomik_pll_96m`
- **fabric clock target frequency:** **96.000 MHz**
- **fabric clock target period:** **10.417 ns**
- **PLL lock used to gate reset:** Yes/No

**Clock object used in SDC:**

```yaml
gen_pll.u_pll/rpll_inst/CLKOUT
```
---

### Constraints

- **SDC file:** `constraints/timing_constraints.sdc`

**create_clock definitions:**
- `sys_clk`: 37.037 ns (27.000 MHz)
- `atomik_clk`: **10.417 ns** (96.000 MHz)

**IO delays:**
- `uart_rx`: max 2.0 ns (relative to `atomik_clk` *or* `sys_clk` if UART is moved to 27 MHz domain)
- `uart_tx`: max 2.0 ns (relative to `atomik_clk` *or* `sys_clk` if UART is moved to 27 MHz domain)

**False paths:**
- Reset (`sys_rst_n`): Yes
- LEDs (`led[*]`): Yes

---

### STA Snapshot (Post-P&R)

**Setup Timing**
- Violations: ___
- Endpoints: ___
- TNS: ___

**Hold Timing**
- Violations: ___
- Endpoints: ___
- TNS: ___

**Max Frequency Summary**
- `atomik_clk`
  - Constraint: **96.000 MHz**
  - Actual Fmax: ___ MHz
  - Logic levels on critical path: ___

- `sys_clk`
  - Constraint: **27.000 MHz**
  - Actual Fmax: ___ MHz
  - *(Not an operating frequency unless sys_clk clocks fabric logic)*

**Generated clocks present (if any):**
- ___

---

### Hardware Validation

- Bitstream programmed successfully: Yes/No
- PLL lock stable: Yes/No
- UART stable @ 115200 baud: Yes/No
- BIOS / genome loader functional: Yes/No
- Core enable behavior: Yes/No

**Alive signature:**
- Heartbeat LED: Yes/No
- Loader activity LED: Yes/No
- Core debug LED: Yes/No
- Other: ___

Observed behavior is repeatable across resets and reprogramming: Yes/No

---

### Files Included in Baseline

- `rtl/atomik_top.v`
- `rtl/atomik_core.v`
- `rtl/uart_genome_loader.v`
- `rtl/pll/atomik_pll_96m.v`
- `constraints/atomik_constraints.cst`
- `constraints/timing_constraints.sdc`
- Timing report: `reports/timing/<96MHz_report>.html`

---

### Notes / Rationale

- What changed vs BL-81-PLL:
  - PLL target updated to 96 MHz
  - Constraints updated for 10.417 ns period
  - RTL changes (if any): ___

- Warnings observed: ___
- Critical path location (if known): ___

---

### Next Intended Step

- If pass: attempt BL-108-PLL
- If fail: identify worst setup path and apply one targeted optimization

---

## Baseline: BL-108-PLL (Skeleton)

**Date created:** YYYY-MM-DD  
**Git/Tag (if applicable):** `v108_pll_candidate`  
**Device:** GW1NR-LV9QN88PC6/I5 (Tang Nano 9K)  
**Toolchain:** Gowin IDE ___  

---

### Clocking

- **sys_clk (reference):**
  - Source: Top-level port `sys_clk`
  - Frequency (Clock Summary): **27.000 MHz**
  - Period: 37.037 ns

- **fabric clock name:** `atomik_clk`
- **fabric clock source:** PLL `CLKOUT`
- **PLL module:** `atomik_pll_108m`
- **fabric clock target frequency:** **108.000 MHz**
- **fabric clock target period:** **9.259 ns**
- **PLL lock used to gate reset:** Yes/No

**Clock object used in SDC:**

```yaml
gen_pll.u_pll/rpll_inst/CLKOUT
```
---

### Constraints

- **SDC file:** `constraints/timing_constraints.sdc`

**create_clock definitions:**
- `sys_clk`: 37.037 ns (27.000 MHz)
- `atomik_clk`: **9.259 ns** (108.000 MHz)

**IO delays:**
- `uart_rx`: max 2.0 ns (relative to `atomik_clk` *or* `sys_clk` if UART is moved to 27 MHz domain)
- `uart_tx`: max 2.0 ns (relative to `atomik_clk` *or* `sys_clk` if UART is moved to 27 MHz domain)

**False paths:**
- Reset (`sys_rst_n`): Yes
- LEDs (`led[*]`): Yes

---

### STA Snapshot (Post-P&R)

**Setup Timing**
- Violations: ___
- Endpoints: ___
- TNS: ___

**Hold Timing**
- Violations: ___
- Endpoints: ___
- TNS: ___

**Max Frequency Summary**
- `atomik_clk`
  - Constraint: **108.000 MHz**
  - Actual Fmax: ___ MHz
  - Logic levels on critical path: ___

- `sys_clk`
  - Constraint: **27.000 MHz**
  - Actual Fmax: ___ MHz
  - *(Not an operating frequency unless sys_clk clocks fabric logic)*

**Generated clocks present (if any):**
- ___

---

### Hardware Validation

- Bitstream programmed successfully: Yes/No
- PLL lock stable: Yes/No
- UART stable @ 115200 baud: Yes/No
- BIOS / genome loader functional: Yes/No
- Core enable behavior: Yes/No

**Alive signature:**
- Heartbeat LED: Yes/No
- Loader activity LED: Yes/No
- Core debug LED: Yes/No
- Other: ___

Observed behavior is repeatable across resets and reprogramming: Yes/No

---

### Files Included in Baseline

- `rtl/atomik_top.v`
- `rtl/atomik_core.v`
- `rtl/uart_genome_loader.v`
- `rtl/pll/atomik_pll_108m.v`
- `constraints/atomik_constraints.cst`
- `constraints/timing_constraints.sdc`
- Timing report: `reports/timing/<108MHz_report>.html`

---

### Notes / Rationale

- What changed vs BL-96-PLL:
  - PLL target updated to 108 MHz
  - Constraints updated for 9.259 ns period
  - RTL changes (if any): ___

- Warnings observed: ___
- Critical path location (if known): ___

---

### Next Intended Step

- If pass: attempt BL-120-PLL
- If fail: apply a single critical-path reduction (pipeline / down-counter / clock-domain split)
