# FPGA Resource Budget & Scaling Reference (Xilinx Zynq-7020)

**Target:** XC7Z020-CLG400 (Zynq-7020, PYNQ-Z2 or similar board)
**Date:** March 7, 2026
**Status:** Pre-board. All ATOMiK values marked ~ are estimates pending Vivado synthesis.

This is the go-to document for answering "can we fit X on the Zynq PL?" It mirrors the Gowin `RESOURCE_BUDGET_GUIDE.md` structure. All projections are extrapolated from Gowin synthesis data and Xilinx architecture specifications.

---

## Table of Contents

1. [XC7Z020 PL Resource Inventory](#1-xc7z020-pl-resource-inventory)
2. [ATOMiK Scaling Projections](#2-atomik-scaling-projections)
3. [AXI4-Lite Wrapper Overhead](#3-axi4-lite-wrapper-overhead)
4. [Scenario Planning](#4-scenario-planning)
5. [PS-Side Resources](#5-ps-side-resources)
6. [Gowin vs Zynq Comparison Summary](#6-gowin-vs-zynq-comparison-summary)
7. ["Can We Fit X?" Decision Table](#7-can-we-fit-x-decision-table)
8. [Xilinx Synthesis Attributes](#8-xilinx-synthesis-attributes)
9. [LUT6 vs LUT4 Mapping Notes](#9-lut6-vs-lut4-mapping-notes)
10. [Vivado Resource Reporting](#10-vivado-resource-reporting)

---

## 1. XC7Z020 PL Resource Inventory

Total PL (programmable logic) resources on the XC7Z020-CLG400 (from DS187 and UG474):

| Resource | Total Available | Unit | Notes |
|----------|:--------------:|------|-------|
| **LUT6** | 53,200 | units | 6-input lookup tables (within CLB slices) |
| **Flip-Flops (FF)** | 106,400 | units | D-type, 8 per slice (2 per LUT) |
| **CLB Slices** | 13,300 | units | 4 LUT6 + 8 FF each |
| **BRAM36** | 140 | blocks | 36 Kbit each, 4.9 Mbit total |
| **BRAM18** | 280 | blocks | Each BRAM36 splits into 2 BRAM18 |
| **DSP48E1** | 220 | units | 25x18 multiply-accumulate |
| **MMCM** | 4 | units | Mixed-mode clock manager |
| **PLL** | 2 | units | Phase-locked loop |
| **BUFG** | 32 | units | Global clock buffers |
| **IO (HR)** | 200 | pins | High-range I/O (CLG400 package) |
| **GTP** | 0 | units | No transceivers on XC7Z020 |

### LUT6 vs LUT4 -- Why This Matters

Each Xilinx LUT6 can implement **any Boolean function of 6 inputs**. Gowin's LUT4 handles 4 inputs. The implications:

| Characteristic | Gowin LUT4 | Xilinx LUT6 |
|---------------|:----------:|:----------:|
| Inputs | 4 | 6 |
| Functions per LUT | 1 | 1 (6-input) or 2 (independent 5-input via O5/O6) |
| XOR-64 tree depth | ~6 levels (LUT4) | ~4 levels (LUT6) |
| Effective capacity ratio | 1x (baseline) | ~2x per LUT (conservative) |

A single LUT6 can compute two independent functions of up to 5 inputs each using the O5 and O6 dual outputs. For ATOMiK's XOR operations (2-input XOR chains), a LUT6 can pack ~2 XOR operations vs 1 per LUT4. The effective logic capacity of XC7Z020 is roughly **12x the GW1NR-9** (53,200 LUT6 at ~2x density vs 8,640 LUT4).

### Resource Relationships

- 1 CLB Slice = 4 LUT6 + 8 FF + carry chain + MUX
- 1 BRAM36 = 36 Kbit = 512 x 72-bit, 1K x 36-bit, 2K x 18-bit, 4K x 9-bit, 8K x 4-bit, 16K x 2-bit, or 32K x 1-bit
- 1 BRAM36 can be split into 2 independent BRAM18 (18 Kbit each)
- BRAM supports true dual-port (two independent read/write ports)
- DSP48E1: 25x18 signed multiply + 48-bit accumulate + pattern detect
- BRAM total: 140 x 36 Kbit = 4,900 Kbit = 612.5 KB (vs Gowin's 468 Kbit = 58.5 KB)

---

## 2. ATOMiK Scaling Projections

All values in this section are **~estimates** extrapolated from Gowin synthesis. Actual Vivado results will differ.

### 2.1 Extrapolation Methodology

Gowin baseline (from actual synthesis sweeps):
- Single-bank ATOMiK = 477 LUT4, 537 FF
- Scaling formula: `LUT4 ~ 470 + 87*N` (where N = number of banks)

LUT4-to-LUT6 mapping rationale:
- ATOMiK is dominated by 64-bit XOR trees (2-input operations)
- Each LUT6 O5/O6 can implement two independent XOR operations
- Mapping ratio: ~0.5 LUT6 per LUT4 for XOR-dominated logic (conservative: ~0.6 for control logic overhead)
- Applied ratio: **0.5x** for accumulator datapath, **0.7x** for control/mux logic, **~0.55x blended**

### 2.2 Scaling Table

| Banks (N) | Gowin LUT4 | ~Xilinx LUT6 | ~LUT6 % | ~FF | ~BRAM36 | ~Fmax (MHz) |
|----------:|:----------:|:------------:|:-------:|:---:|:-------:|:-----------:|
| 1 | 477 | ~200 | ~0.4% | ~300 | 2 | ~200+ |
| 4 | 745 | ~350 | ~0.7% | ~450 | 2 | ~200 |
| 8 | 1,126 | ~550 | ~1.0% | ~600 | 2 | ~180 |
| 16 | 1,779 | ~900 | ~1.7% | ~900 | 2 | ~150 |
| 64 | ~5,900* | ~3,000 | ~5.6% | ~2,500 | 2 | ~120 |
| 256 | ~23,000* | ~12,000 | ~22.5% | ~9,000 | 2 | ~80 |

*N=64 and N=256 extrapolated from the scaling formula `LUT4 ~ 470 + 87*N`.

**BRAM usage**: Constant at 2 BRAM36 regardless of N. The state table (256 x 64-bit = 16 Kbit) plus metadata fits in 2 BRAM36 blocks. Bank accumulators are registers, not memories.

**~Fmax rationale**: Gowin achieves ~96 MHz for N=1 on 55nm. Artix-7 (28nm) is roughly 2-3x faster for equivalent logic. However, the XOR merge tree grows with N, adding combinational depth. Estimates assume pipeline registers can be added to maintain throughput at higher N.

### 2.3 Per-Bank Marginal Cost (~Estimate)

| Resource | Per Additional Bank | Source |
|----------|:-------------------:|--------|
| ~LUT6 | ~35 | 64 XOR (accumulator) packed into ~32 LUT6 + ~3 control |
| ~FF | ~64 | delta_accumulator register (same as Gowin) |
| BRAM | 0 | Accumulators are registers, not memories |
| DSP | 0 | No multiply in ATOMiK datapath |

**~Scaling formula (Xilinx):** `~LUT6 ~ 200 + 45*N`

### 2.4 Throughput Projections

| Banks (N) | ~Fmax (MHz) | ~Throughput (Mops/s) | vs Gowin Production |
|----------:|:-----------:|:--------------------:|:-------------------:|
| 1 | ~200 | ~200 | ~2.5x (vs 81 Mops/s) |
| 4 | ~200 | ~800 | ~10x |
| 16 | ~150 | ~2,400 | ~30x |
| 64 | ~120 | ~7,680 | ~95x |
| 256 | ~80 | ~20,480 | ~253x |

All throughput values are ~estimates. Gowin production reference: single-bank at 81 MHz = ~81 Mops/s.

---

## 3. AXI4-Lite Wrapper Overhead

The AXI4-Lite slave wrapper connects ATOMiK to the Zynq PS via the AXI interconnect. ~Estimates based on standard Xilinx AXI4-Lite slave templates:

### 3.1 Component Breakdown (~Estimate)

| Component | ~LUT6 | ~FF | Notes |
|-----------|:-----:|:---:|-------|
| AXI4-Lite slave FSM | ~100 | ~50 | AWREADY/WREADY/BVALID/ARREADY/RVALID handshake |
| Register file (10 regs) | ~50 | ~80 | Read mux + write decode for ATOMiK control/status |
| CDC bridge | ~30 | ~40 | Toggle-handshake (only if separate ATOMiK clock) |
| Interrupt logic | ~10 | ~5 | Single interrupt line to PS GIC |
| **Total (with CDC)** | **~190** | **~175** | |
| **Total (no CDC)** | **~160** | **~135** | Same-clock, Phase 1 |

### 3.2 Register Map (~Planned)

| Offset | Name | Width | Access | Description |
|:------:|------|:-----:|:------:|-------------|
| 0x00 | CTRL | 32 | R/W | Control: enable, reset, bank select |
| 0x04 | STATUS | 32 | R | Status: busy, done, error, bank count |
| 0x08 | DELTA_LO | 32 | R/W | Delta input (lower 32 bits) |
| 0x0C | DELTA_HI | 32 | R/W | Delta input (upper 32 bits) |
| 0x10 | ACC_LO | 32 | R | Accumulator read (lower 32 bits) |
| 0x14 | ACC_HI | 32 | R | Accumulator read (upper 32 bits) |
| 0x18 | STATE_LO | 32 | R | Reconstructed state (lower 32 bits) |
| 0x1C | STATE_HI | 32 | R | Reconstructed state (upper 32 bits) |
| 0x20 | REF_LO | 32 | R/W | Reference state (lower 32 bits) |
| 0x24 | REF_HI | 32 | R/W | Reference state (upper 32 bits) |

This maps cleanly to the existing Gowin Wishbone register layout but uses AXI4-Lite protocol instead.

### 3.3 Comparison with Gowin Bus Wrapper

| Aspect | Gowin (Wishbone) | Xilinx (AXI4-Lite) |
|--------|:----------------:|:------------------:|
| Protocol | Wishbone B4 pipelined | AXI4-Lite |
| ~LUT | ~50 (simple) | ~100 (more handshake logic) |
| Bus width | 32-bit | 32-bit |
| Burst | No | No (AXI4-Lite is single-beat) |
| Arbitration | 1:4 Wishbone mux | AXI Interconnect IP (free) |
| Integration effort | Manual RTL | Vivado IP Integrator (block design) |

---

## 4. Scenario Planning

### Scenario A: Bringup (N=1, AXI Wrapper, No HDMI from PL)

Target: Minimal ATOMiK on Zynq. Run ATOMiK on FCLK (no MMCM, no CDC).

| Component | ~LUT6 | ~BRAM36 | ~MMCM | Notes |
|-----------|:-----:|:-------:|:-----:|-------|
| ATOMiK N=1 | ~200 | 2 | 0 | Single-bank core |
| AXI4-Lite wrapper (no CDC) | ~160 | 0 | 0 | Same clock as AXI |
| **Total** | **~360** | **2** | **0** | |
| **Utilization** | **~0.7%** | **~1.4%** | **0%** | |
| **Remaining** | **~52,840** | **138** | **4+2** | |

Verdict: **Trivially fits.** Less than 1% of PL resources. The entire ATOMiK subsystem is smaller than a single AXI DMA IP core.

### Scenario B: Production (N=16, Linux, UIO Driver)

Target: Multi-bank ATOMiK with Linux userspace access via UIO or custom driver.

| Component | ~LUT6 | ~BRAM36 | ~MMCM | Notes |
|-----------|:-----:|:-------:|:-----:|-------|
| ATOMiK N=16 | ~900 | 2 | 0 | 16-bank core |
| AXI4-Lite wrapper + CDC | ~190 | 0 | 1 | Separate ATOMiK clock |
| **Total** | **~1,090** | **2** | **1** | |
| **Utilization** | **~2.1%** | **~1.4%** | **25%** | |
| **Remaining** | **~52,110** | **138** | **3+2** | |

Verdict: **Trivially fits.** 98% of PL resources remain unused.

### Scenario C: Maximum Scale (N=64, HDMI, DMA)

Target: Large-scale ATOMiK with 1080p HDMI output and DMA streaming.

| Component | ~LUT6 | ~BRAM36 | ~MMCM | Notes |
|-----------|:-----:|:-------:|:-----:|-------|
| ATOMiK N=64 | ~3,000 | 2 | 1 | 64-bank core, dedicated MMCM |
| AXI4-Lite wrapper + CDC | ~190 | 0 | 0 | Shares ATOMiK MMCM |
| HDMI 1080p pipeline | ~1,500 | 4 | 1 | Pixel + serializer clocks |
| AXI DMA engine | ~500 | 1 | 0 | Scatter-gather DMA to ATOMiK |
| **Total** | **~5,190** | **7** | **2** | |
| **Utilization** | **~9.8%** | **~5.0%** | **50%** | |
| **Remaining** | **~48,010** | **133** | **2+2** | |

Verdict: **Fits with enormous headroom.** 90% of LUT and 95% of BRAM remain available.

### Scenario D: Stress Test (N=256)

Target: Maximum parallelism, pushing bank count to extremes.

| Component | ~LUT6 | ~BRAM36 | ~MMCM | Notes |
|-----------|:-----:|:-------:|:-----:|-------|
| ATOMiK N=256 | ~12,000 | 2 | 1 | 256-bank core |
| AXI4-Lite wrapper + CDC | ~190 | 0 | 0 | |
| HDMI 1080p pipeline | ~1,500 | 4 | 1 | |
| AXI DMA engine | ~500 | 1 | 0 | |
| **Total** | **~14,190** | **7** | **2** | |
| **Utilization** | **~26.7%** | **~5.0%** | **50%** | |
| **Remaining** | **~39,010** | **133** | **2+2** | |

Verdict: **Fits -- still 73% LUT remaining.** Routing at 256 banks may challenge Fmax, but resource count is not the bottleneck.

### Scenario Comparison Chart

```
XC7Z020 LUT6 Budget (53,200 total)
======================================================================
Scenario A (N=1, bringup):
[|                                                                    ]  ~360  (0.7%)

Scenario B (N=16, production):
[||                                                                   ]  ~1,090 (2.1%)

Scenario C (N=64, HDMI+DMA):
[|||||                                                                ]  ~5,190 (9.8%)

Scenario D (N=256, stress):
[||||||||||||||                                                       ]  ~14,190 (26.7%)

For reference — Gowin GW1NR-9 FULL SoC (N=1):
[||||||||||||||||||||||                                               ]  3,838 LUT4 (44%)
(Note: Gowin total is 8,640 LUT4; shown here scaled to Xilinx bar for visual comparison)
======================================================================
```

---

## 5. PS-Side Resources

The Zynq PS (Processing System) provides resources that are entirely separate from the PL budget. These eliminate the most painful constraints of the Gowin bare-metal deployment.

| PS Resource | Specification | vs Gowin Equivalent |
|-------------|:------------:|:-------------------:|
| **CPU** | Dual Cortex-A9 @ 667 MHz | vs PicoRV32 @ 25.2 MHz (~53x per core) |
| **L1 Cache** | 32 KB I + 32 KB D per core | vs none |
| **L2 Cache** | 512 KB shared | vs none |
| **DDR3** | 512 MB - 1 GB (board-dependent) | vs 8 KB SRAM (~131,072x) |
| **OCM** | 256 KB on-chip | vs 8 KB SRAM (32x) |
| **OS** | Linux (full POSIX) | vs bare-metal firmware |
| **Peripherals** | 2x UART, 2x SPI, 2x I2C, 2x CAN, USB, GigE, SD | vs custom RTL per peripheral |
| **DMA** | 8-channel DMA controller | vs manual bus transactions |
| **Interrupt** | GIC with 16 shared PL interrupts | vs polled status register |
| **Timers** | 3x triple timer counters | vs fabric counter |

### Key Implications

| Gowin Constraint | Zynq Resolution |
|------------------|-----------------|
| 8 KB SRAM (stack overflow risk) | 1 GB DDR3 (effectively unlimited) |
| No OS (bare-metal only) | Full Linux userspace |
| SPI XIP boot (slow, ~1 MB/s) | SD card boot, DDR execution |
| Firmware size limit (~16 KB) | No practical limit |
| Custom UART/SPI RTL (200+ LUT each) | PS hard peripherals (0 PL resources) |
| printf via powers-of-10 table | Full libc with hardware multiply |
| No floating point | NEON SIMD + VFPv3 |

**The Zynq PS eliminates all software constraints.** The ATOMiK driver can be a standard Linux kernel module or UIO userspace driver, with full debugging, profiling, and toolchain support.

---

## 6. Gowin vs Zynq Comparison Summary

| Resource | GW1NR-9 (Gowin) | XC7Z020 (Zynq) | Ratio | Bottleneck on Zynq? |
|----------|:----------------:|:---------------:|:-----:|:-------------------:|
| **LUT** | 8,640 (4-input) | 53,200 (6-input) | ~12x effective | **No** |
| **FF** | 6,480 | 106,400 | 16.4x | **No** |
| **BRAM** | 468 Kbit (26 blocks) | 4,900 Kbit (140 blocks) | 10.5x | **No** |
| **DSP** | 20 (pMAC18) | 220 (DSP48E1) | 11x | **No** |
| **PLL/MMCM** | 2 (rPLL) | 4 MMCM + 2 PLL = 6 | 3x | **No** (was HARD LIMIT on Gowin) |
| **Clock buffers** | ~8 (shared routing) | 32 (BUFG) | 4x | **No** |
| **Memory** | 8 KB SRAM | 1 GB DDR3 | ~131,072x | **No** |
| **CPU** | Soft 25.2 MHz (RV32I) | Hard 667 MHz (Cortex-A9 x2) | ~53x per core | **No** |
| **I/O pins** | 88 (QN88P) | 200 (CLG400) | 2.3x | **No** |
| **Fabric speed** | 55nm | 28nm | ~2-3x Fmax | **No** |

### Where Does the Bottleneck Shift?

**The Zynq removes ALL resource bottlenecks.** The limiting factor shifts from hardware to:

| New Bottleneck | Description | Mitigation |
|---------------|-------------|------------|
| **AXI interconnect bandwidth** | PS-PL bridge is ~1.2 GB/s (GP) or ~4.8 GB/s (HP) | Use AXI HP port for DMA, GP for control |
| **Driver quality** | Linux driver overhead, context switch latency | UIO for latency-critical paths, mmap registers |
| **Linux scheduling** | Non-deterministic userspace latency | RT kernel or dedicated core isolation |
| **PS-PL latency** | ~10-20 AXI clock cycles per transaction | Batch operations, DMA for bulk transfer |
| **Software architecture** | Framework design, API quality | Focus engineering effort here |

---

## 7. "Can We Fit X?" Decision Table

| Question | Answer | Reasoning |
|----------|:------:|-----------|
| ATOMiK N=1? | **Yes, trivially** | ~200 LUT6 (~0.4%). Smaller than a UART IP. |
| ATOMiK N=16? | **Yes, trivially** | ~900 LUT6 (~1.7%). Less than 2% utilization. |
| ATOMiK N=64? | **Yes, easily** | ~3,000 LUT6 (~5.6%). Significant headroom. |
| ATOMiK N=256? | **Yes** | ~12,000 LUT6 (~22.5%). 73% remaining. |
| ATOMiK N=1024? | **Maybe** | ~46,000 LUT6 (~87%). Routing would be challenging. ~Fmax likely drops below 50 MHz. |
| HDMI 1080p from PL? | **Yes** | ~1,500 LUT6 (~3%), 1 MMCM. Fractional divide enables exact 148.5 MHz. |
| Linux framebuffer? | **Yes** | PS-side via VDMA + AXI. No PL logic for software rendering. |
| DMA to ATOMiK? | **Yes** | ~500 LUT6 (~1%). Or use Xilinx AXI DMA IP. |
| Second MMCM for HDMI? | **Yes** | 4 MMCM + 2 PLL available. Was impossible on Gowin (0 remaining). |
| Third MMCM? | **Yes** | Still 1 MMCM + 2 PLL remaining. |
| AXI HP port (high bandwidth)? | **Yes** | 4 HP ports available, ~4.8 GB/s aggregate. 0 LUT cost (hard block). |
| Hardware multiply (in ATOMiK path)? | **Yes** | 220 DSP48E1 available. |
| Second ATOMiK instance? | **Yes** | Even 2x N=64 is ~6,000 LUT6 (11%). |
| All of the above simultaneously? | **Yes** | Scenario D (N=256 + HDMI + DMA) is 26.7% LUT. Add second instance and it is still < 50%. |

---

## 8. Xilinx Synthesis Attributes

Equivalent to the Gowin `syn_*` attributes documented in the Gowin RESOURCE_BUDGET_GUIDE.md.

### 8.1 Attribute Reference

| Attribute | Applies To | Effect | Gowin Equivalent |
|-----------|-----------|--------|-----------------|
| `(* DONT_TOUCH = "TRUE" *)` | module, wire, reg | Prevents optimization/removal. Combines Gowin's `syn_keep` + `syn_preserve` into one. | `syn_keep` + `syn_preserve` |
| `(* KEEP = "TRUE" *)` | wire, reg | Prevents signal from being optimized away. Less strict than DONT_TOUCH (allows replication). | `syn_keep` |
| `(* KEEP_HIERARCHY = "YES" *)` | module | Prevents cross-boundary optimization. Useful for per-module resource reporting. | (no direct equivalent) |
| `(* ram_style = "block" *)` | reg array | Forces BRAM inference. | `syn_ramstyle = "block_ram"` |
| `(* ram_style = "distributed" *)` | reg array | Forces distributed RAM (LUTRAM). | `syn_ramstyle = "distributed"` |
| `(* ram_style = "register" *)` | reg array | Forces plain register implementation. | (no direct equivalent) |
| `(* use_dsp = "yes" *)` | multiply op | Forces DSP48E1 inference. | `syn_dspstyle = "dsp"` |
| `(* use_dsp = "no" *)` | multiply op | Forces LUT implementation. | `syn_dspstyle = "logic"` |
| `(* max_fanout = N *)` | reg | Limits fanout to N, tool inserts buffer tree. | `syn_maxfan` |
| `(* MARK_DEBUG = "TRUE" *)` | wire, reg | Adds ILA (Integrated Logic Analyzer) debug probe. | (no direct equivalent) |
| `(* fsm_encoding = "one_hot" *)` | reg | Forces FSM encoding style. Also: "sequential", "gray", "auto". | `syn_encoding` |

### 8.2 ATOMiK-Specific Attribute Usage

```verilog
// Prevent Vivado from optimizing the XOR accumulator tree
// (equivalent to syn_keep on merged_acc_comb in Gowin)
(* DONT_TOUCH = "TRUE" *) wire [63:0] merged_acc_comb;

// Force the state table into BRAM
(* ram_style = "block" *) reg [63:0] state_table [0:255];

// Prevent DSP inference on XOR operations
// (Vivado generally won't map XOR to DSP, but be explicit)
(* use_dsp = "no" *) wire [63:0] xor_result = acc ^ delta;

// Debug probe for ILA during bringup
(* MARK_DEBUG = "TRUE" *) wire [63:0] debug_accumulator;
(* MARK_DEBUG = "TRUE" *) wire        debug_valid;
```

### 8.3 Key Differences from Gowin

| Gowin Pattern | Xilinx Equivalent | Notes |
|--------------|-------------------|-------|
| `(* syn_keep = 1 *)` | `(* KEEP = "TRUE" *)` | Xilinx uses string "TRUE"/"FALSE" |
| `(* syn_preserve = 1 *)` | `(* DONT_TOUCH = "TRUE" *)` | DONT_TOUCH is stronger (no replication) |
| `defparam inst.PARAM = val;` | `#(.PARAM(val))` in instantiation | Vivado does not support `defparam` |
| `(* syn_ramstyle = "block_ram" *)` | `(* ram_style = "block" *)` | Different attribute name and value |
| `(* syn_dspstyle = "dsp" *)` | `(* use_dsp = "yes" *)` | Different attribute name |
| `(* syn_maxfan = 32 *)` | `(* max_fanout = 32 *)` | Same concept, different name |

---

## 9. LUT6 vs LUT4 Mapping Notes

### 9.1 Why Direct LUT Count Comparisons Are Misleading

A design that uses 477 LUT4 on Gowin will NOT use 477 LUT6 on Xilinx. The mapping depends on the logic function complexity:

| Logic Type | LUT4 Count | ~LUT6 Count | Mapping Ratio | Reason |
|-----------|:----------:|:-----------:|:-------------:|--------|
| 2-input XOR chain | 64 | ~32 | 0.5x | LUT6 O5/O6 dual output packs 2 XOR |
| 4:1 MUX | 3 | 1 | 0.33x | LUT6 implements 4:1 MUX directly |
| 3-input AND/OR | 1 | 0.5 | 0.5x | Packed with another function via O5/O6 |
| 6-input function | 4 | 1 | 0.25x | LUT6 handles natively |
| FSM decode | varies | ~0.6x | 0.6x | Wider input eliminates cascade stages |
| Carry chain (add/sub) | 1 per bit | 1 per bit | 1.0x | Carry primitives are 1:1 |
| Memory (LUTRAM) | 16x1 per LUT | 64x1 per LUT | 0.25x | LUT6 has 4x LUTRAM density |

### 9.2 ATOMiK-Specific Mapping

ATOMiK logic composition (from Gowin synthesis):

| ATOMiK Component | Gowin LUT4 | ~Xilinx LUT6 | Mapping Ratio | Dominant Logic |
|-----------------|:----------:|:------------:|:-------------:|---------------|
| XOR accumulator (per bank) | ~65 | ~32 | ~0.5x | 64-bit XOR tree |
| State reconstruction | ~50 | ~25 | ~0.5x | 64-bit XOR |
| Bank select MUX | ~30 | ~15 | ~0.5x | N:1 MUX |
| Control FSM | ~80 | ~55 | ~0.7x | State decode, sequencing |
| Bus interface | ~50 | ~35 | ~0.7x | Address decode, read mux |
| CDC bridge | ~50 | ~35 | ~0.7x | Synchronizers, toggle logic |
| Misc (reset, status) | ~30 | ~20 | ~0.7x | Combinational |

**Blended ratio for ATOMiK: ~0.55x** (datapath-heavy design benefits significantly from LUT6).

---

## 10. Vivado Resource Reporting

### 10.1 Where to Find Resource Numbers

After synthesis and implementation, Vivado produces:

| Report | Location | Contains |
|--------|----------|----------|
| Utilization (post-synth) | `project.runs/synth_1/*_utilization_synth.rpt` | LUT, FF, BRAM, DSP after synthesis |
| Utilization (post-impl) | `project.runs/impl_1/*_utilization_placed.rpt` | Final utilization after placement |
| Timing (post-route) | `project.runs/impl_1/*_timing_summary_routed.rpt` | WNS, TNS, Fmax per clock domain |
| Power | `project.runs/impl_1/*_power_routed.rpt` | Estimated power consumption |
| IO | `project.runs/impl_1/*_io_placed.rpt` | Pin assignments and standards |

### 10.2 Vivado Utilization Report Format

```
+----------------------------+-------+-------+-----------+-------+
| Site Type                  |  Used | Fixed | Available | Util% |
+----------------------------+-------+-------+-----------+-------+
| Slice LUTs                 |   360 |     0 |     53200 |  0.68 |
|   LUT as Logic             |   340 |     0 |     53200 |  0.64 |
|   LUT as Memory            |    20 |     0 |     17400 |  0.11 |
| Slice Registers            |   475 |     0 |    106400 |  0.45 |
| Block RAM Tile             |     2 |     0 |       140 |  1.43 |
| DSPs                       |     0 |     0 |       220 |  0.00 |
+----------------------------+-------+-------+-----------+-------+
```

### 10.3 Vivado Timing Report Format

```
------------------------------------------------------------------------------------------------
| Clock Summary
| Clock      | Waveform(ns)  | Period(ns) | Frequency(MHz) | Sources
| clk_fpga_0 | {0.000 5.000} | 10.000     | 100.000        | PS7/FCLK_CLK0
| clk_atomik | {0.000 2.500} |  5.000     | 200.000        | u_mmcm/CLKOUT0
------------------------------------------------------------------------------------------------

Timing Summary:
  WNS(ns)  TNS(ns)  TNS Failing Endpoints  WHS(ns)  THS(ns)
  -------  -------  ---------------------  -------  -------
    1.234    0.000                      0    0.045    0.000
```

- **WNS > 0**: Setup timing met (positive slack)
- **TNS = 0**: No setup violations
- **WHS > 0**: Hold timing met
- **THS = 0**: No hold violations

### 10.4 TCL Commands for Resource Queries

```tcl
# After opening a synthesized or implemented design:

# Get utilization summary
report_utilization -hierarchical

# Get utilization for a specific module
report_utilization -cells [get_cells u_atomik]

# Get timing for a specific clock
report_timing -from [get_clocks clk_atomik] -to [get_clocks clk_atomik] -max_paths 10

# Get critical path
report_timing_summary -delay_type max -max_paths 5

# Check clock interaction (useful for CDC verification)
report_clock_interaction -delay_type max

# Get power estimate
report_power
```

### 10.5 Batch Mode (Non-Interactive)

```tcl
# vivado -mode batch -source build.tcl

# Open project
open_project project.xpr

# Run synthesis
launch_runs synth_1
wait_on_run synth_1

# Run implementation
launch_runs impl_1
wait_on_run impl_1

# Generate bitstream
launch_runs impl_1 -to_step write_bitstream
wait_on_run impl_1

# Export hardware (for SDK/PetaLinux)
write_hw_platform -fixed -include_bit project.xsa
```

---

## Appendix A: Zynq PS-PL Interface Bandwidth

| Port | Count | Width | Direction | ~Bandwidth | Use Case |
|------|:-----:|:-----:|:---------:|:----------:|----------|
| AXI GP (M) | 2 | 32-bit | PS -> PL | ~400 MB/s each | Control registers, low-bandwidth |
| AXI GP (S) | 2 | 32-bit | PL -> PS | ~400 MB/s each | PL-initiated DMA to PS memory |
| AXI HP | 4 | 64-bit | PL -> PS | ~1.2 GB/s each | High-bandwidth DMA, video |
| AXI ACP | 1 | 64-bit | PL -> PS | ~1.2 GB/s | Cache-coherent access to DDR |

**For ATOMiK**: AXI GP is sufficient for register-based control (read/write accumulator, load delta). AXI HP should be used if DMA streaming of deltas is implemented.

---

## Appendix B: Device Comparison (Potential Xilinx Targets)

| Device | LUT6 | FF | BRAM36 | DSP48 | MMCM | PS | Board | ~Price |
|--------|-----:|---:|-------:|------:|-----:|:--:|-------|-------:|
| XC7Z010 | 17,600 | 35,200 | 60 | 80 | 2 | Yes | PYNQ-Z1 | ~$65 |
| **XC7Z020** | **53,200** | **106,400** | **140** | **220** | **4** | **Yes** | **PYNQ-Z2** | **~$120** |
| XC7A35T | 20,800 | 41,600 | 50 | 90 | 5 | No | CMOD A7, Arty A7-35T | ~$75 |
| XC7A100T | 63,400 | 126,800 | 135 | 240 | 6 | No | Arty A7-100T, Nexys A7 | ~$150 |
| XC7Z045 | 218,600 | 437,200 | 545 | 900 | 8 | Yes | ZC706 | ~$2,000 |

The XC7Z020 (PYNQ-Z2) offers the best balance of Zynq PS integration, PL capacity, and cost for ATOMiK development. The XC7A35T is a viable no-PS alternative for pure RTL testing.

---

## Appendix C: Revision History

| Date | Change |
|------|--------|
| 2026-03-07 | Initial version. Pre-board estimates from Gowin synthesis data and Xilinx datasheets. |

---

*References: DS187 (Zynq-7000 Data Sheet), UG474 (7 Series CLB User Guide), UG473 (7 Series Memory Resources), UG479 (7 Series DSP48E1), UG585 (Zynq-7000 TRM), Gowin RESOURCE_BUDGET_GUIDE.md (measured synthesis data)*
