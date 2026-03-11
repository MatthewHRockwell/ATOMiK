# ASIC Strategy Clarification — Seed Funds Feasibility, Not Tapeout

*Prepared for due diligence — March 2026*

---

## Key Point

The seed round does **not** fund an ASIC tapeout. It funds an ASIC **feasibility study** — a design exercise that produces the economic model for Series A. Revenue begins on FPGA, not custom silicon.

---

## What the Seed Buys (ASIC-Related)

Of the $1.2M–$1.6M Hardware R&D budget (40% of raise):

| Item | Cost | Deliverable |
|------|------|-------------|
| Zynq FPGA port (in progress) | ~$50K | Production-grade Xilinx deployment, customer-facing |
| Mid-range FPGA (N=64+ banks) | ~$100K | >4 Gops/s demonstrator for customer pilots |
| **ASIC feasibility study** | **$150K–$300K** | Die size estimate, power/perf projections, foundry selection, cost model |
| Dev boards, lab equipment | ~$50K | Engineering infrastructure |

The feasibility study is a paper-and-simulation exercise with a foundry partner. It produces:
- Gate-level synthesis results (area, power, timing) at target node
- Die size and yield estimate
- Per-unit cost projection at volume (10K, 100K, 1M units)
- Foundry recommendation (TSMC, GlobalFoundries, Samsung)
- Go/no-go recommendation for Series A ASIC investment

---

## ASIC Cost Reality

| Stage | Cost | When | Funded By |
|-------|------|------|-----------|
| **Feasibility study** (RTL → gate-level synthesis) | $150K–$300K | Seed (month 9–18) | This raise |
| **Shuttle run** (multi-project wafer, limited dies) | $200K–$500K | Series A (month 18–24) | Series A |
| **Full tapeout** (28nm, production mask set) | $2M–$5M | Series A/B (month 24–36) | Series A/growth |
| **Advanced node** (7nm, if warranted) | $30M+ | Series B+ | Not relevant at seed |

The seed raise covers step 1 only. Steps 2–3 are Series A decisions informed by step 1's results.

---

## Why FPGA-First Is the Revenue Strategy

ATOMiK follows the ARM model: license IP that runs on **customer silicon**, not our own.

```
Revenue Path:

  Seed (Y0–Y1.5)          Series A (Y1.5–Y3)         Growth (Y3+)
  ─────────────           ──────────────────         ──────────────
  FPGA IP licensing  ──→  FPGA + ASIC IP       ──→  ASIC IP at scale
  $0 → $500K              $500K → $5M                $5M → $80M

  No ASIC needed          Shuttle validates           Production ASIC
  for first revenue       economics                   if market warrants
```

**FPGA IP licensing starts generating revenue in Y2** — no custom silicon required. Customers integrate the ATOMiK IP core into their existing FPGA designs. This is identical to how ARM licenses Cortex cores to chip designers.

The ASIC path is a **cost optimization**, not a revenue gate:
- FPGA: ~$0.50–$5.00 per unit (ATOMiK IP portion)
- ASIC: ~$0.01–$0.10 per unit at volume
- ASIC makes sense when unit volumes exceed ~100K/year

---

## Current FPGA Progress (Already Done)

| Platform | Status | Utilization |
|----------|--------|-------------|
| **Gowin GW1NR-9** (Tang Nano 9K) | Production deployed, all tests passing | 477 LUT (5.5%) single-bank |
| **Xilinx Zynq-7020** (AX7020) | Synthesis validated, awaiting board | 287 LUT (0.54%) |

The Zynq port demonstrates that ATOMiK IP is **vendor-portable** — a key requirement for IP licensing. The core synthesized on Xilinx with zero RTL changes, using only 0.54% of the target FPGA's resources.

### Zynq Software Stack (Ready)
- libatomik C library: 33/33 tests passing
- Python bindings: 35/35 tests passing
- UIO device tree overlay for Linux integration
- Build system: `vivado -mode batch -source vivado/build.tcl`

---

## What ASIC Feasibility Answers

The $150K–$300K study produces specific answers for the Series A pitch:

| Question | Why It Matters |
|----------|---------------|
| What is the die size at 28nm? | Determines unit cost and yield |
| What is the power at 28nm? | Key for edge/IoT market (battery life) |
| What is the max frequency? | Performance ceiling (10x–100x over FPGA) |
| What is the NRE for a shuttle? | Series A budget planning |
| Which foundry? | Risk assessment, lead time, minimum order |
| Is a dedicated ASIC warranted? | Or is FPGA-only the right model? |

The answer might be "ASIC isn't worth it — stay FPGA-only." That's a valid outcome. The feasibility study de-risks the Series A decision, not the seed.

---

## Comparable IP Companies

| Company | Model | Revenue | Custom Silicon? |
|---------|-------|---------|-----------------|
| **ARM** | License CPU IP cores | $4.0B (FY2025) | No — runs on customer silicon |
| **Imagination Technologies** | License GPU IP cores | ~$200M | No |
| **Cadence/Synopsys** | License EDA + IP | $6B+ each | No |
| **CEVA** | License DSP/AI IP | ~$150M | No |

None of these companies manufacture chips. They license designs. ATOMiK follows the same model — the ASIC path is an option for power/cost optimization, not a requirement for the business.

---

*The seed round retires commercial risk (first design wins) and initiates ASIC feasibility. It does not commit to a tapeout. That decision is made at Series A with data from the feasibility study.*
