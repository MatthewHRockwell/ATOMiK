# Revised Revenue Model — Bottoms-Up with Three Scenarios

*Prepared for due diligence — March 2026*

---

## Why the Original Model Needed Revision

The original projection (Y1 $0 → Y5 $80M) was top-down: TAM × market share = revenue. Chris correctly identified that it lacks bottoms-up math — specifically, customer acquisition timelines, realistic sales cycles, and per-deal economics.

This revision builds from the unit: one IP license deal.

---

## Unit Economics: One IP License Deal

Based on ARM, CEVA, and Imagination Technologies comparable licensing structures:

| Component | Range | Notes |
|-----------|-------|-------|
| **Upfront license fee** | $250K–$2M | Per-design, depends on customer size and use case |
| **Annual maintenance** | 15–20% of license | Updates, support, new IP revisions |
| **Per-unit royalty** | $0.01–$1.00 | Scales with device ASP; starts at $0.10 typical |
| **Integration support (NRE)** | $50K–$200K | One-time, first deployment only |

**Blended ACV (Annual Contract Value) at steady state:**
- Small customer (IoT/edge): $300K–$500K/year (license + maintenance + royalty)
- Mid-market (industrial, automotive): $500K–$1M/year
- Large customer (HFT, data center): $1M–$3M/year

---

## Sales Cycle Reality

IP licensing has long sales cycles. Being honest about this:

| Customer Type | Sales Cycle | Why |
|---------------|-------------|-----|
| **HFT firms** | 3–6 months | Fast procurement for latency-critical infrastructure. Budget holders are technical. Decision by CTO/infrastructure lead. |
| **Edge AI / IoT OEMs** | 9–15 months | Hardware design cycles are 12–18 months. IP evaluation happens early in design phase. |
| **Automotive / Industrial** | 12–24 months | Qualification cycles, safety certification, multi-vendor evaluation. |
| **Data center / Cloud** | 6–12 months | Large procurement teams, but strategic investments can move fast. |

**Implication:** First revenue is realistic at **Month 12–15** (HFT beachhead), not Month 6. The $0 in Y1 is correct.

---

## Customer Acquisition Funnel

Realistic conversion rates for semiconductor IP:

| Stage | Count | Conversion | Timeline |
|-------|-------|------------|----------|
| **Prospects identified** | 50 | — | Month 1–3 |
| **Initial contact** | 30 | 60% response | Month 3–6 |
| **Technical evaluation** | 12 | 40% proceed | Month 6–9 |
| **Pilot / proof-of-concept** | 6 | 50% proceed | Month 9–12 |
| **Design win (signed license)** | 3 | 50% close | Month 12–18 |

This assumes one salesperson (founder initially, then application engineer). The funnel produces **3 design wins in 18 months** — aggressive but achievable for a novel IP with clear differentiation.

---

## Three Scenarios

### Conservative: $15M in Year 5

*Assumes: slow adoption, HFT-only for first 3 years, no royalty ramp*

| Year | New Customers | Cumulative | Avg ACV | Revenue | Cumulative |
|------|--------------|------------|---------|---------|------------|
| Y1 | 0 | 0 | — | **$0** | $0 |
| Y2 | 2 | 2 | $400K | **$800K** | $800K |
| Y3 | 4 | 6 | $500K | **$3M** | $3.8M |
| Y4 | 6 | 12 | $600K | **$7.2M** | $11M |
| Y5 | 8 | 20 | $750K | **$15M** | $26M |

**Assumptions:**
- HFT beachhead only for Y2–Y3, edge/IoT expansion in Y4
- ACV grows as product matures and royalties begin
- 90% gross margin (IP licensing)
- No ASIC revenue
- 20 customers by Y5 (all FPGA-based deployments)

### Base Case: $35M in Year 5

*Assumes: HFT + edge expansion, moderate royalty ramp, 1 large deal in Y4*

| Year | New Customers | Cumulative | Avg ACV | Revenue | Cumulative |
|------|--------------|------------|---------|---------|------------|
| Y1 | 0 | 0 | — | **$0** | $0 |
| Y2 | 3 | 3 | $500K | **$1.5M** | $1.5M |
| Y3 | 7 | 10 | $600K | **$6M** | $7.5M |
| Y4 | 12 | 22 | $750K | **$16.5M** | $24M |
| Y5 | 15 | 37 | $950K | **$35M** | $59M |

**Assumptions:**
- HFT (Y2), edge/IoT (Y3), automotive/industrial (Y4)
- 1 large deal ($2M+) in Y4 from data center or automotive
- Royalty stream begins Y3 as first deployments ship product
- ACV rises with maturity and vertical-specific modules
- 37 customers by Y5 across 3 verticals

### Bull Case: $80M in Year 5

*Assumes: ARM-trajectory adoption, ASIC licensing begins Y4, platform standard*

| Year | New Customers | Cumulative | Avg ACV | Revenue | Cumulative |
|------|--------------|------------|---------|---------|------------|
| Y1 | 0 | 0 | — | **$0** | $0 |
| Y2 | 5 | 5 | $500K | **$2.5M** | $2.5M |
| Y3 | 15 | 20 | $700K | **$14M** | $16.5M |
| Y4 | 25 | 45 | $900K | **$40M** | $56.5M |
| Y5 | 30 | 75 | $1.1M | **$80M** | $136.5M |

**Assumptions:**
- Rapid multi-vertical adoption (HFT + edge + streaming + automotive)
- ASIC IP licensing begins Y4 ($2M–$5M per design)
- Developer ecosystem creates pull (SDK community 500+ by month 18)
- 2+ large deals ($3M+) per year starting Y4
- 75 customers by Y5 — approaching platform-standard status

---

## What Each Scenario Implies for Series A

| Scenario | Y2 Revenue | Series A Timing | Series A Size | Implied Valuation |
|----------|-----------|-----------------|---------------|-------------------|
| Conservative | $800K | Month 18–24 | $10M–$15M | $40M–$60M |
| Base | $1.5M | Month 15–18 | $15M–$25M | $60M–$100M |
| Bull | $2.5M | Month 12–15 | $25M–$40M | $100M+ |

**The seed investment thesis:** Even the conservative scenario returns 3–5x at Series A (entry at $16M–$20M pre-money, Series A at $40M–$60M). The base case returns 5–8x. The bull case is a venture-scale outcome.

---

## Key Assumptions Requiring Validation (Seed Period)

These are the specific hypotheses the seed capital tests:

| Hypothesis | Validation Metric | Timeline |
|-----------|-------------------|----------|
| HFT firms will pay for delta-state IP | 1 signed LOI | Month 9–12 |
| IP licensing model works for this category | 1 completed deal | Month 12–18 |
| Edge/IoT is a viable second vertical | 3 technical evaluations initiated | Month 12–15 |
| Developer adoption creates pull | SDK community reaches 200+ | Month 12 |
| Team can scale with AI-augmented model | 3-person team ships on schedule | Month 6 |

**If all 5 validate → base case or better.**
**If 3 of 5 validate → conservative case (still a good seed return).**
**If <3 validate → pivot or wind down (seed capital preserved for 18 months).**

---

## Comparison to Original Model

| | Original | Revised (Base) | Change |
|--|---------|---------------|--------|
| Y2 | $500K | $1.5M | More aggressive (added NRE revenue) |
| Y3 | $5M | $6M | Similar |
| Y4 | $20M | $16.5M | More conservative (longer sales cycles) |
| Y5 | $80M | $35M | Significantly more conservative |
| Customers Y5 | 100+ | 37 | Realistic funnel math |

The base case is less dramatic than the original but more defensible. The bull case ($80M) is preserved as an upside scenario, not the baseline.

---

*Revenue projections are estimates based on comparable IP licensing companies (ARM, CEVA, Imagination). Actual results depend on market adoption, sales execution, and competitive dynamics. All scenarios assume successful Zynq deployment and at least 2 design wins in the first 18 months.*
