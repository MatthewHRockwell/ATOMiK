# FPGA Resource Budget & Optimization Reference

**Target:** Tang Nano 9K (GW1NR-LV9QN88PC6/I5)
**Date:** February 14, 2026
**Status:** Production SoC deployed, planning expansion peripherals

This is the go-to document for answering "can we fit X on the FPGA?" All numbers are from actual synthesis/P&R results unless marked as estimates.

---

## Table of Contents

1. [GW1NR-9 Resource Inventory](#1-gw1nr-9-resource-inventory)
2. [Current SoC Utilization (Measured)](#2-current-soc-utilization-measured)
3. [PicoRV32 Configuration Reference](#3-picorv32-configuration-reference)
4. [ATOMiK Bank Scaling Reference](#4-atomik-bank-scaling-reference)
5. [Planned Peripheral Budget](#5-planned-peripheral-budget)
6. [Critical Resource Bottlenecks](#6-critical-resource-bottlenecks)
7. [Resource Optimization Strategies](#7-resource-optimization-strategies)
8. [Optimization Decision Matrix](#8-optimization-decision-matrix)
9. [Gowin EDA Resource Reporting](#9-gowin-eda-resource-reporting)
10. [gw_sh TCL Commands for Resource Queries](#10-gw_sh-tcl-commands-for-resource-queries)
11. [Quick-Reference: "Can We Fit X?"](#11-quick-reference-can-we-fit-x)

---

## 1. GW1NR-9 Resource Inventory

Total resources available on the GW1NR-LV9QN88PC6/I5 (from DS117-2.9.3E):

| Resource | Total Available | Unit | Notes |
|----------|:--------------:|------|-------|
| **LUT4** | 8,640 | units | 4-input lookup tables |
| **Flip-Flops (FF)** | 6,480 | units | D-type, within CLS |
| **CLS** | 4,320 | units | Configurable Logic Slices (2 LUT4 + 2 FF each) |
| **Shadow SRAM (SSRAM)** | 17,280 | bits | Distributed RAM within CLS |
| **Block SRAM (BSRAM)** | 26 | blocks | 18 Kbit each, 468 Kbit total |
| **User Flash** | 608 | Kbit | One-time programmable, non-volatile |
| **PSRAM** | 64 | Mbit | Embedded pseudo-static RAM (HyperRAM interface) |
| **DSP (pMAC18)** | 20 | units | 18x18 multiplier-accumulator blocks |
| **PLL (rPLL)** | 2 | units | Reconfigurable phase-locked loop |
| **IO Pins** | 88 | pins | QN88P package (not all user-accessible) |

### Resource Relationships

- 1 CLS = 2 LUT4 + 2 FF (but packing is not always 100%)
- 1 BSRAM = 18 Kbit = 2,048 x 9-bit or 1,024 x 18-bit or 512 x 36-bit
- BSRAM supports true dual-port (two independent read/write ports)
- DSP supports 18x18 signed multiply, optional 54-bit accumulator
- User Flash is read-only at runtime (programmed at configuration time)
- PSRAM requires a controller IP (not trivial, high latency, shared with FPGA config)

---

## 2. Current SoC Utilization (Measured)

Production SoC: PicoRV32 + ATOMiK (N=1) + HDMI + UART + GPIO + SPI Flash.

### 2.1 Summary Table

| Resource | Used | Available | Utilization | Remaining |
|----------|-----:|----------:|:-----------:|----------:|
| **LUT** | 3,838 | 8,640 | **44%** | **4,802** |
| **ALU** | 707 | -- | -- | -- |
| **FF** | ~2,080 | 6,480 | ~32% | ~4,400 |
| **CLS** | 3,103 | 4,320 | **72%** | **1,217** |
| **BSRAM** | 12 | 26 | **47%** | **14** |
| **PLL** | 2 | 2 | **100%** | **0** |
| **DSP** | 0 | 20 | 0% | **20** |

**CLS utilization (72%) is higher than LUT utilization (44%)** because Gowin's P&R does not always achieve perfect LUT+FF packing into CLS. CLS is the physical container; LUT and FF are the logical resources within it. When routing pressure is high, CLS fills up before LUTs are exhausted.

### 2.2 Per-Subsystem Breakdown (Measured)

| Subsystem | LUT | FF (est.) | BSRAM | PLL | Notes |
|-----------|----:|----------:|------:|----:|-------|
| **PicoRV32 CPU core** | ~1,800 | ~900 | 0 | 0 | RV32I, no multiply |
| **SPI Flash controller** | ~300 | ~150 | 0 | 0 | XIP, single-SPI mode |
| **SRAM (8 KB)** | ~50 | ~30 | 12 | 0 | 12 BSRAM = 8 KB + boot ROM |
| **UART** | ~200 | ~100 | 0 | 0 | 115200 baud, TX+RX |
| **GPIO** | ~50 | ~30 | 0 | 0 | 7 pins |
| **HDMI output** | ~700 | ~400 | 0 | 1 | 126 MHz serializer PLL |
| **HDMI PLL divider** | ~50 | ~30 | 0 | 0 | clk_p = 25.2 MHz |
| **Bus interconnect** | ~250 | ~150 | 0 | 0 | 1:4 Wishbone mux |
| **ATOMiK core (N=1)** | ~230 | ~150 | 0 | 1 | Single-bank @ 81 MHz |
| **CDC bridge** | ~50 | ~40 | 0 | 0 | Toggle-handshake |
| **Reset/misc** | ~110 | ~100 | 0 | 0 | Sync, POR, heartbeat |
| **Total** | **3,838** | **~2,080** | **12** | **2** | |

### 2.3 ATOMiK Marginal Contribution

From A/B comparison (picotiny with vs without ATOMiK):

| Resource | Baseline (picotiny) | With ATOMiK | Delta |
|----------|----:|-----:|------:|
| LUT | 3,608 | 3,838 | **+230** (2.7%) |
| FF (est.) | 1,930 | ~2,080 | **+~150** |
| BSRAM | 12 | 12 | +0 |
| PLL | 1 | 2 | **+1** |

The ATOMiK core itself (single-bank accumulator + CDC bridge + bus wrapper) costs only 230 LUT. Most of the SoC budget is consumed by PicoRV32 and peripherals.

---

## 3. PicoRV32 Configuration Reference

Resource usage varies significantly based on PicoRV32 configuration parameters and attached peripherals. Values from IPUG914 (Gowin PicoRV32 Resource Statistics):

### 3.1 PicoRV32 Core Configurations (No Peripherals)

| Configuration | LUT | FF | BSRAM | DSP | Key Parameters |
|---------------|----:|---:|------:|----:|----------------|
| **Minimum** | 2,764 | 1,833 | 8 | 0 | RV32I, no IRQ, no MUL |
| **Default** | 5,321 | 3,173 | 32 | 2 | RV32IMC, IRQ, MUL |
| **Maximum** | 6,210 | 3,477 | 32 | 2 | All optional features |

### 3.2 PicoRV32 With Peripherals

| Configuration | LUT | FF | BSRAM | DSP | Peripherals |
|---------------|----:|---:|------:|----:|-------------|
| Default + UART/GPIO/I2C | 6,804 | 4,228 | 32 | 2 | Common I/O |
| Default + All Default | 8,330 | 5,070 | 32 | 2 | Everything at defaults |
| Maximum + All Maximum | 8,594 | 5,278 | 32 | 2 | Everything maxed out |

### 3.3 ATOMiK SoC Configuration (Actual)

Our production picotiny-based SoC uses a **customized minimal** PicoRV32:

| Parameter | Setting | Resource Impact |
|-----------|---------|-----------------|
| ISA | RV32I (no M, no C) | Saves ~500 LUT, 2 DSP vs RV32IMC |
| IRQ | Disabled | Saves ~200 LUT |
| Multiply | Disabled | Saves 2 DSP + ~300 LUT |
| Compressed | Disabled | Saves ~200 LUT |
| SRAM | 8 KB (12 BSRAM) | vs 32 BSRAM in "Default" |

This is why our SoC fits in 3,838 LUT despite the Gowin reference showing 5,321+ LUT for "Default" PicoRV32.

---

## 4. ATOMiK Bank Scaling Reference

All values from actual synthesis sweeps (Gowin EDA V1.9.11.03).

### 4.1 Standalone ATOMiK (Without PicoRV32 SoC)

| Banks (N) | LUT | LUT % | FF | ALU | Fmax (MHz) | Throughput |
|----------:|----:|------:|---:|----:|----------:|-----------:|
| 1 | 477 | 5.5% | 537 | 40 | 96.0 | 94.5 Mops/s |
| 2 | 616 | 7.1% | 602 | 40 | 95.8 | 189.0 Mops/s |
| 4 | 745 | 8.6% | 731 | 40 | 81.1 | 324.0 Mops/s |
| 8 | 1,126 | 13.0% | 988 | 40 | 67.9 | 540.0 Mops/s |
| 16 | 1,779 | 20.6% | 1,501 | 40 | 63.7 | 864.0 Mops/s |
| 32 | 3,213 | 37.2% | -- | -- | 52.4 | Timing fail |
| 64 | 5,939 | 68.7% | -- | -- | 41.6 | Timing fail |

### 4.2 Per-Bank Marginal Cost

| Resource | Per Additional Bank | Source |
|----------|:-------------------:|--------|
| LUT | ~65 | 64 XOR (accumulator) + 1 control |
| FF | 64 | delta_accumulator register |
| ALU | 0 | Pure LUT (syn_keep enforced) |

**Scaling formula:** `Total LUT ~ 470 + 65*N + merge_overhead(N)`

Merge overhead grows with log2(N) but is absorbed into LUT4 packing. In practice, N=1 to N=16 follows near-linear LUT growth.

### 4.3 ATOMiK-in-SoC Budget Estimates

To estimate total SoC resources with multi-bank ATOMiK, add the ATOMiK delta to the picotiny baseline:

| SoC Configuration | Est. LUT | LUT % | Est. BSRAM | Timing Feasible? |
|-------------------|:--------:|------:|:----------:|:----------------:|
| picotiny + ATOMiK N=1 (current) | 3,838 | 44% | 12 | Yes (deployed) |
| picotiny + ATOMiK N=4 | ~4,100 | 47% | 12 | Likely at 67.5 MHz |
| picotiny + ATOMiK N=8 | ~4,500 | 52% | 12 | Likely at 54 MHz |
| picotiny + ATOMiK N=16 | ~5,400 | 63% | 12 | Marginal at 54 MHz |

These are estimates. SoC integration adds routing pressure that may shift Fmax downward. N=4 at 81 MHz had a -0.454 ns TNS violation during production testing, which is why N=1 was chosen.

---

## 5. Planned Peripheral Budget

### 5.1 Expansion Peripheral Estimates

| Peripheral | Est. LUT | Est. FF | BSRAM | DSP | PLL | Notes |
|------------|:--------:|:-------:|:-----:|:---:|:---:|-------|
| **HDMI text mode** (80x25) | 500-1,000 | 200-400 | 2-4 | 0 | 0 | Font ROM + character buffer |
| **SPI master (MAX3421E)** | 100-200 | 60-100 | 0 | 0 | 0 | USB HID via external IC |
| **SPI master (inter-board)** | 100-200 | 60-100 | 0 | 0 | 0 | Multi-board delta sync |
| **SD card SPI controller** | 150-300 | 80-150 | 1-2 | 0 | 0 | SPI mode, sector buffer in BSRAM |
| **Timer/counter** | 50-100 | 30-60 | 0 | 0 | 0 | Interval timer, watchdog |
| **DMA engine** | 300-500 | 150-250 | 0 | 0 | 0 | Burst transfer to ATOMiK |
| **Interrupt controller** | 100-200 | 50-100 | 0 | 0 | 0 | Priority encoder, mask register |

### 5.2 HDMI Text Mode Breakdown

The HDMI text mode is the largest planned addition. Estimated subcomponent costs:

| Component | Est. LUT | Est. FF | BSRAM | Notes |
|-----------|:--------:|:-------:|:-----:|-------|
| Character buffer (80x25) | 0 | 0 | 1 | 2,000 bytes = 1 BSRAM (18 Kbit) |
| Font ROM (8x16, 128 chars) | 0 | 0 | 1-2 | 16 KB = 1-2 BSRAM |
| Pixel pipeline | 200-400 | 100-200 | 0 | Character lookup + pixel shift |
| Timing generator | 150-300 | 80-150 | 0 | H/V sync, blanking, position |
| Cursor logic | 30-50 | 20-30 | 0 | Blink timer, position register |
| Color attribute | 50-100 | 30-50 | 0-1 | Optional: per-char color |
| **Subtotal** | **430-850** | **230-430** | **2-4** | |

### 5.3 SPI Controller Breakdown

A single SPI master is reusable for MAX3421E, inter-board, and SD card with a chip-select mux:

| Component | Est. LUT | Est. FF | Notes |
|-----------|:--------:|:-------:|-------|
| SPI shift register | 30-50 | 20-30 | 8-bit TX/RX shift |
| Clock divider | 20-30 | 15-20 | Programmable baud rate |
| Control FSM | 40-60 | 20-30 | CPOL/CPHA modes |
| TX/RX FIFO (small) | 30-60 | 20-40 | Optional: 4-8 entry |
| Bus interface | 30-50 | 15-25 | Wishbone or valid/ready |
| **Subtotal** | **150-250** | **90-145** | Shared across all SPI devices |

If sharing one SPI controller across all three SPI peripherals (MAX3421E, inter-board, SD card), only one instance plus a CS mux (~20-30 LUT) is needed.

### 5.4 Scenario Planning

#### Scenario A: OS Shell (Minimum Viable)

Target: HDMI text output + USB keyboard input + UART.

| Component | LUT | BSRAM | Status |
|-----------|----:|------:|--------|
| Current SoC (measured) | 3,838 | 12 | Deployed |
| HDMI text mode | +700 | +3 | Estimate |
| SPI master (shared) | +200 | 0 | Estimate |
| Timer/counter | +75 | 0 | Estimate |
| **Total** | **~4,813** | **15** | |
| **Remaining** | **3,827 (44%)** | **11 (42%)** | |

Verdict: **Fits comfortably.**

#### Scenario B: Full Peripheral Suite

Target: OS shell + SD card + inter-board + DMA + interrupts.

| Component | LUT | BSRAM | Status |
|-----------|----:|------:|--------|
| Scenario A total | 4,813 | 15 | From above |
| SD card (uses shared SPI) | +50 | +1 | CS mux + sector buffer |
| Inter-board (uses shared SPI) | +30 | 0 | CS mux only |
| DMA engine | +400 | 0 | Estimate |
| Interrupt controller | +150 | 0 | Estimate |
| **Total** | **~5,443** | **16** | |
| **Remaining** | **3,197 (37%)** | **10 (38%)** | |

Verdict: **Fits with headroom for ATOMiK bank upgrade.**

#### Scenario C: Full Suite + ATOMiK N=4

Target: Everything from Scenario B + multi-bank ATOMiK.

| Component | LUT | BSRAM | Status |
|-----------|----:|------:|--------|
| Scenario B total | 5,443 | 16 | From above |
| ATOMiK upgrade N=1 to N=4 | +270 | 0 | 4 banks - 1 bank delta |
| **Total** | **~5,713** | **16** | |
| **Remaining** | **2,927 (34%)** | **10 (38%)** | |

Verdict: **Fits, but timing closure at 81 MHz is not guaranteed for N=4. May need to drop ATOMiK to 67.5 MHz.**

#### Scenario D: Maximum Density (Absolute Limit)

Target: Full suite + ATOMiK N=8. Pushing the device hard.

| Component | LUT | BSRAM | Status |
|-----------|----:|------:|--------|
| Scenario B total | 5,443 | 16 | From above |
| ATOMiK upgrade N=1 to N=8 | +650 | 0 | Estimate |
| **Total** | **~6,093** | **16** | |
| **Remaining** | **2,547 (29%)** | **10 (38%)** | |

Verdict: **Physically fits, but routing congestion will be severe. ATOMiK Fmax likely drops to 54 MHz. CLS utilization will exceed 85%, making P&R slow and results non-deterministic. Not recommended unless throughput is critical.**

---

## 6. Critical Resource Bottlenecks

### 6.1 PLL: 2/2 Used -- HARD LIMIT

| PLL | Current Assignment | Frequency | Output |
|-----|-------------------|-----------|--------|
| PLL 1 (HDMI) | HDMI serializer | 126.0 MHz | clk_p (25.2 MHz via CLKDIV) |
| PLL 2 (ATOMiK) | ATOMiK core | 81.0 MHz | clk_atomik |

**No additional PLLs are available.** Any new clock frequency must be derived from existing PLL outputs via:
- CLKDIV primitive (integer division only: /2, /4, /8)
- Fabric-based clock divider (adds jitter, wastes LUT)
- CLKOUTD output of existing PLL (secondary divider, DYN_SDIV_SEL)

**Implication:** Inter-board SPI, SD card SPI, and any other peripherals must run on clocks derivable from 25.2 MHz or 81 MHz. The SPI controller can use a fabric clock divider since SPI is not timing-critical.

### 6.2 BSRAM: 12/26 Used -- Moderate Pressure

| BSRAM Allocation | Blocks | Capacity |
|------------------|:------:|----------|
| Boot ROM | 2 | 4 KB |
| SRAM (data/stack) | 10 | 8 KB (with overhead) |
| **Used** | **12** | |
| **Remaining** | **14** | **252 Kbit = 31.5 KB** |

Planned BSRAM consumers:

| Consumer | Blocks | Remaining After |
|----------|:------:|:---------------:|
| Font ROM (8x16, 128 chars) | 1-2 | 12-13 |
| Character buffer (80x25) | 1 | 11-12 |
| Color attribute buffer | 0-1 | 10-12 |
| SD card sector buffer (512 B) | 1 | 9-11 |
| SPI RX/TX buffers | 0-1 | 8-11 |
| **Worst-case planned total** | **6** | **8** |

8 remaining BSRAM blocks (144 Kbit = 18 KB) is adequate headroom for unforeseen needs. BSRAM is not the binding constraint.

### 6.3 LUT: 44% Used -- Primary Expansion Resource

With 4,802 LUT remaining, the binding constraint on LUT is not total count but **CLS packing efficiency** and **routing congestion**. In practice:

- Below 60% LUT utilization: Synthesis is predictable, timing closure reliable.
- 60-75% LUT: Routing becomes harder, Fmax may drop 5-15%.
- Above 75% LUT: P&R times increase dramatically, results become non-deterministic.

**Rule of thumb:** Keep LUT utilization below 70% (6,048 LUT) for reliable builds. That means ~2,210 LUT budget for all additions.

### 6.4 CLS: 72% Used -- Watch This Metric

CLS is the physical container for LUT+FF pairs. At 72% CLS with only 44% LUT, the P&R tool is not packing efficiently. This is common when:
- Many signals have high fanout (spread across the die)
- Clock domain crossing paths force placement constraints
- Routing congestion prevents co-location of LUT+FF pairs

**CLS is currently the tightest physical constraint**, not LUT. Adding ~1,200 LUT of peripherals will push CLS toward 85-90%, which is the practical ceiling.

### 6.5 DSP: 0/20 Used -- Completely Free

All 20 DSP blocks are available. These are valuable for:
- 18x18 multiply (replaces ~200 LUT per multiplier)
- Multiply-accumulate (MAC) operations
- Fixed-point arithmetic
- CRC computation (with some creativity)

Using DSP for multiplication instead of LUT is strongly recommended wherever multiply operations are needed (e.g., coordinate calculation in HDMI, baud rate generation, timer comparisons with large constants).

---

## 7. Resource Optimization Strategies

### 7.1 BSRAM Inference

Gowin EDA can infer BSRAM from behavioral Verilog if the code matches supported patterns.

**Recommended: Use the `syn_ramstyle` attribute to force BSRAM:**

```verilog
// Force BSRAM inference for a 2KB memory
(* syn_ramstyle = "block_ram" *) reg [7:0] framebuffer [0:2047];

// Force distributed RAM (SSRAM) for small memories
(* syn_ramstyle = "distributed" *) reg [7:0] small_lut [0:15];

// Let the tool decide
reg [7:0] auto_mem [0:255];  // Tool chooses based on size
```

**Rules for BSRAM inference:**
- Memory depth >= 64 entries generally maps to BSRAM
- Single-port or true dual-port patterns are inferred
- Synchronous read (registered output) infers more reliably than async read
- Reset on memory contents prevents BSRAM inference (remove initial blocks for memory arrays)

### 7.2 DSP Macro Inference

Gowin synthesis infers DSP blocks for multiply operations:

```verilog
// This will infer a pMAC18 DSP block
wire [35:0] product = a[17:0] * b[17:0];

// Force DSP inference
(* syn_dspstyle = "dsp" *) wire [35:0] product = a[17:0] * b[17:0];

// Prevent DSP inference (use LUT instead)
(* syn_dspstyle = "logic" *) wire [35:0] product = a[17:0] * b[17:0];
```

**When to use DSP vs LUT for multiply:**
- Use DSP: Any 18-bit or smaller multiply (saves ~200 LUT per instance)
- Use LUT: When all 20 DSP blocks are consumed, or for very small multiplies (4-bit)
- Our SoC uses 0/20 DSP — we have plenty of headroom

### 7.3 Clock Domain Separation

Keeping ATOMiK on a separate 81 MHz PLL prevents it from creating timing pressure on the 25.2 MHz CPU domain:

- ATOMiK paths only need to close at 81 MHz within the ATOMiK domain
- CPU paths only need to close at 25.2 MHz
- CDC bridge paths are false-pathed in SDC (handled by synchronizers)

**Best practice for new peripherals:** Run new peripherals in the 25.2 MHz CPU domain. Do NOT create additional clock domains without a PLL (which we cannot — both PLLs are used).

### 7.4 Register Retiming

Gowin synthesis supports automatic register retiming to balance combinational logic across pipeline stages:

```verilog
// Enable retiming for a module
(* syn_allow_retiming = 1 *) module my_pipeline (...);
```

This is particularly useful for the HDMI pixel pipeline, where the character-lookup-to-pixel-shift path may be long.

### 7.5 Gowin Synthesis Attributes Reference

| Attribute | Applies To | Effect |
|-----------|-----------|--------|
| `syn_keep` | wire/reg | Prevents optimization/merging of signal |
| `syn_preserve` | reg | Prevents register removal during optimization |
| `syn_maxfan` | reg | Limits fanout (tool inserts buffer trees) |
| `syn_ramstyle` | reg array | Forces BSRAM (`"block_ram"`) or SSRAM (`"distributed"`) |
| `syn_dspstyle` | multiply | Forces DSP (`"dsp"`) or LUT (`"logic"`) |
| `syn_allow_retiming` | module | Enables automatic register retiming |
| `syn_encoding` | reg | FSM encoding: `"one-hot"`, `"sequential"`, `"gray"` |

**ATOMiK-specific usage:** The `syn_keep` attribute on `merged_acc_comb` and `state_recon` wires in `atomik_parallel_acc.v` prevents Gowin from mapping XOR operations to ALU carry chains, reducing logic levels from 9-12 to 6-7 and improving Fmax by up to 42%.

### 7.6 Fanout Management

High-fanout signals (reset, clock enable) cause routing congestion:

```verilog
// Limit fanout to 32 — tool inserts buffer tree automatically
(* syn_maxfan = 32 *) reg global_enable;
```

For the ATOMiK SoC, the CPU reset signal and ATOMiK clock enable are the highest-fanout nets. If CLS utilization becomes critical, adding `syn_maxfan` to these signals can improve packing.

---

## 8. Optimization Decision Matrix

### 8.1 Memory Type Selection

| Criteria | Use BSRAM | Use Distributed (SSRAM) | Use External (PSRAM/Flash) |
|----------|:---------:|:----------------------:|:--------------------------:|
| Size > 512 bytes | Yes | No | Consider |
| Size <= 64 bytes | No | Yes | No |
| Needs dual-port | Yes | Possible (small) | No |
| Latency-critical | Yes (1 cycle) | Yes (0 cycle async) | No (10+ cycles) |
| BSRAM blocks available | Check budget | Always available | Always available |
| Read-only (constants) | Yes | Small tables only | Large lookup tables |

**Decision flowchart for a new memory:**

```
Is the memory > 2 KB?
  YES -> Is BSRAM available? -> YES -> Use BSRAM
                               -> NO  -> Use PSRAM (if controller exists) or split
  NO  -> Is the memory > 64 entries?
           YES -> Prefer BSRAM (saves LUT), use distributed if BSRAM scarce
           NO  -> Use distributed RAM (registers/SSRAM)
```

### 8.2 Arithmetic Implementation

| Operation | Use DSP | Use LUT/ALU | Notes |
|-----------|:-------:|:-----------:|-------|
| 18-bit multiply | Yes | No | Saves ~200 LUT |
| 32-bit multiply | Yes (2 DSP) | Only if DSP exhausted | 4 partial products |
| Addition/subtraction | No | Yes (ALU carry chain) | ALU is free, efficient |
| XOR (ATOMiK paths) | No | Yes (pure LUT) | syn_keep prevents ALU |
| Compare | No | Yes | Subtraction via ALU |
| Shift | No | Yes | Barrel shifter in LUT |

### 8.3 When to Trade LUT for BSRAM (and Vice Versa)

| Situation | Action |
|-----------|--------|
| LUT > 65%, BSRAM < 60% | Move lookup tables from distributed to BSRAM |
| LUT < 50%, BSRAM > 80% | Move small memories from BSRAM to distributed |
| Both constrained | Evaluate whether PSRAM controller is worth the LUT cost |
| Neither constrained | Use BSRAM for anything > 64 entries (better timing) |

---

## 9. Gowin EDA Resource Reporting

### 9.1 Where to Find Resource Numbers

After synthesis and P&R, Gowin EDA produces several report files:

| File | Location | Contains |
|------|----------|----------|
| `*.rpt.txt` | `impl/pnr/` | P&R summary (LUT, FF, BSRAM, DSP, IO) |
| `*.tr` | `impl/pnr/` | Timing report (Fmax, WNS, TNS, critical paths) |
| `*.rpt.html` | `impl/pnr/` | Visual resource report |
| `*_syn_rsc.xml` | `impl/gwsynthesis/` | Per-module synthesis resource breakdown |
| `*.synlog` | `impl/gwsynthesis/` | Synthesis log (warnings, inferred primitives) |

### 9.2 Reading the P&R Resource Report

The `*.rpt.txt` file contains a table like:

```
Resource Usage Summary:

  ----------------------------------------------------------
  Resources           | Usage   | Coverage
  ----------------------------------------------------------
  Logic               |  3838/8640  | 45%
    --LUT,bindLUT     |  3131      |
    --bindLUT         |  707       |
  Register            |  2080/6480  | 33%
    --bindREG         |  2080      |
  CLS                 |  3103/4320  | 72%
  ALU                 |  707       |
  BSRAM               |  12/26     | 47%
  PLL                 |  2/2       | 100%
  ----------------------------------------------------------
```

Key relationships:
- **Logic** = LUT (used as logic) + bindLUT (used as route-through or constant)
- **ALU** = Arithmetic Logic Units in carry-chain mode (counted within Logic)
- **CLS** = Physical slices consumed (the real packing metric)
- **Register** = Total flip-flops used (subset of what CLS can hold)

### 9.3 Reading the Timing Report

The `*.tr` file shows:

```
Clock Summary:
  clk_atomik: Period = 12.346 ns, Fmax = 100.167 MHz
  clk_p:      Period = 39.683 ns, Fmax = 30.591 MHz

Setup Timing Summary:
  Domain    | WNS (ns) | TNS (ns) | Paths
  clk_atomik|   2.199  |   0.000  | 1234
  clk_p     |   7.053  |   0.000  | 5678
```

- **WNS > 0**: Timing is met (positive slack = headroom)
- **TNS = 0**: No setup violations anywhere in the domain
- **Fmax**: 1000 / (period - WNS) = maximum achievable frequency

### 9.4 Per-Module Resource Breakdown

The `*_syn_rsc.xml` file (generated during synthesis) provides hierarchical resource data:

```xml
<module name="picotiny" reg="2080" alu="707" lut="3131">
  <module name="cpu" reg="900" alu="650" lut="1800">
  </module>
  <module name="atomik_bus_wrapper" reg="150" alu="0" lut="180">
    <module name="u_core" reg="100" alu="0" lut="130">
    </module>
  </module>
</module>
```

This is the most useful file for answering "which module is consuming resources?"

---

## 10. gw_sh TCL Commands for Resource Queries

Gowin's `gw_sh` (command-line synthesis tool) supports TCL scripting for batch synthesis and resource analysis.

### 10.1 Basic Synthesis and Resource Extraction

```tcl
# Open project
open_project project/picotiny.gprj

# Run synthesis
run_synthesis

# Run place & route
run_pnr

# Reports are generated in impl/ directory automatically
```

### 10.2 Project Configuration via TCL

```tcl
# Set synthesis options
set_option -synthesis_tool gowinsynthesis
set_option -top_module picotiny
set_option -verilog_std sysv2017

# Set P&R options
set_option -timing_driven 1
set_option -use_mspi_as_gpio 1
set_option -use_sspi_as_gpio 1

# Run everything
run all
```

### 10.3 Automated Sweep Script Pattern

From `hardware/scripts/phase6_hw_sweep.py`, the pattern for batch resource extraction:

```python
import subprocess, re

def run_synthesis(project_file):
    """Run gw_sh synthesis and extract resources."""
    cmd = [
        "/opt/gowin/IDE/bin/gw_sh",
        "synth.tcl"
    ]
    env = {
        "LD_PRELOAD": "/lib/x86_64-linux-gnu/libstdc++.so.6",
        "LD_LIBRARY_PATH": "/opt/gowin/IDE/lib:/lib/x86_64-linux-gnu",
    }
    result = subprocess.run(cmd, env=env, capture_output=True, text=True)
    return result

def parse_pnr_report(rpt_path):
    """Parse resource utilization from P&R report."""
    with open(rpt_path) as f:
        text = f.read()
    lut = int(re.search(r'Logic\s+\|\s+(\d+)/8640', text).group(1))
    bsram = int(re.search(r'BSRAM\s+\|\s+(\d+)/26', text).group(1))
    cls = int(re.search(r'CLS\s+\|\s+(\d+)/4320', text).group(1))
    return {"lut": lut, "bsram": bsram, "cls": cls}

def parse_timing_report(tr_path):
    """Parse Fmax from timing report."""
    with open(tr_path) as f:
        text = f.read()
    fmax = float(re.search(r'Fmax\s*=\s*([\d.]+)', text).group(1))
    return fmax
```

### 10.4 Useful gw_sh One-Liners

```bash
# Synthesis only (faster, gives LUT/FF estimate but no final placement)
LD_PRELOAD=/lib/x86_64-linux-gnu/libstdc++.so.6 \
LD_LIBRARY_PATH=/opt/gowin/IDE/lib:/lib/x86_64-linux-gnu \
/opt/gowin/IDE/bin/gw_sh synth_only.tcl

# Full build (synthesis + P&R + timing)
LD_PRELOAD=/lib/x86_64-linux-gnu/libstdc++.so.6 \
LD_LIBRARY_PATH=/opt/gowin/IDE/lib:/lib/x86_64-linux-gnu \
/opt/gowin/IDE/bin/gw_sh synth.tcl

# Note: gw_sh close_project is NOT a valid command (harmless error at end of .tcl)
```

---

## 11. Quick-Reference: "Can We Fit X?"

### Decision Table

| Question | Answer | Reasoning |
|----------|--------|-----------|
| Can we add HDMI text mode? | **Yes** | ~700 LUT + 3 BSRAM. Well within budget. |
| Can we add USB HID (MAX3421E)? | **Yes** | ~200 LUT for SPI master. External IC handles USB. |
| Can we add SD card? | **Yes** | Shares SPI master + 1 BSRAM for sector buffer. |
| Can we add all three above? | **Yes** | ~1,000 LUT total. Scenario A shows 44% remaining. |
| Can we upgrade to ATOMiK N=4? | **Yes, with caveats** | +270 LUT. May need to reduce ATOMiK clock to 67.5 MHz. |
| Can we upgrade to ATOMiK N=8? | **Marginal** | +650 LUT. CLS will be tight (>85%). Fmax drops. |
| Can we upgrade to ATOMiK N=16? | **Not with full peripherals** | N=16 alone is 1,779 LUT + SoC would exceed 75% LUT. |
| Can we add a second PLL? | **No** | 2/2 PLLs used. Hard device limit. |
| Can we use PSRAM (64 Mbit)? | **With effort** | Need HyperRAM controller (~500 LUT). Shared with config. |
| Can we add hardware multiply (RV32M)? | **Yes** | Uses 2 DSP blocks (20 available). Saves ~300 LUT vs soft. |
| Can we add DMA? | **Yes** | ~400 LUT. Worth it for ATOMiK delta streaming. |
| Can we run PicoRV32 at RV32IMC? | **Not recommended** | Adds ~1,500 LUT + 2 DSP. Pushes total past 65% LUT. |
| Can we add a second UART? | **Yes** | ~200 LUT. Minimal impact. |
| Can we fit everything (full suite)? | **Yes** | Scenario B: ~5,443 LUT (63%). Tight but feasible. |

### Resource Budget Summary Diagram

```
GW1NR-9 LUT Budget (8,640 total)
================================================================
[||||||||||||||||||||        ] PicoRV32 + peripherals  3,608 (42%)
[||                          ] ATOMiK N=1 + CDC          230  (3%)
[                            ] -- Current total:       3,838 (44%)
[.......                     ] Planned peripherals    ~1,000 (12%)
[....                        ] Contingency / future      500  (6%)
[                            ] -- Planned total:      ~5,338 (62%)
[               .............]  Available headroom:    3,302 (38%)
================================================================

GW1NR-9 BSRAM Budget (26 total)
================================================================
[||||||||||||                ] Current SoC                  12 (47%)
[......                      ] Planned additions          ~5  (19%)
[               .............]  Available headroom:         9 (35%)
================================================================

GW1NR-9 PLL Budget (2 total)
================================================================
[||||||||||||||||||||||||||||] HDMI PLL + ATOMiK PLL    2/2 (100%)
================================================================
HARD LIMIT: No additional PLLs available.

GW1NR-9 DSP Budget (20 total)
================================================================
[                            ] Current usage:            0/20 (0%)
[............................] All 20 available for future use
================================================================
```

---

## Appendix A: GW1NR-9 vs Other Gowin Devices

If the GW1NR-9 becomes too constrained, these are the next-tier Gowin devices:

| Device | LUT4 | FF | BSRAM | PLL | DSP | Package | Notes |
|--------|-----:|---:|------:|----:|----:|---------|-------|
| GW1NR-9 (current) | 8,640 | 6,480 | 26 | 2 | 20 | QN88P | Tang Nano 9K |
| GW2A-18 | 20,736 | 15,552 | 46 | 4 | 48 | QN88P | Tang Primer 20K |
| GW2A-55 | 55,296 | 41,472 | 118 | 8 | 120 | -- | No cheap eval board |

The GW2A-18 (Tang Primer 20K, ~$30) would provide 2.4x LUT, 2x PLL, and 2.3x DSP, resolving all current bottlenecks.

---

## Appendix B: Revision History

| Date | Change |
|------|--------|
| 2026-02-14 | Initial version. Production SoC data from deployment. |

---

*References: DS117-2.9.3E (GW1NR-9 Datasheet), IPUG914 (PicoRV32 Resource Statistics), SUG113-1.1E (Gowin FPGA Design Guide)*
