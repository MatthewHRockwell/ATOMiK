# ATOMiK Funding Playbook

**Prioritized by least dilution first, cross-ranked by relevance to ATOMiK's technology space.**

---

## Prerequisite: Incorporation

ATOMiK is **not yet incorporated**. Nearly every program below requires a legal entity.

**Action:** Incorporate a **Delaware C-Corp** before applying to any program.

| Step | Detail |
|------|--------|
| Formation service | Stripe Atlas (~$500) or Clerky (~$500) |
| Timeline | <1 week |
| Structure | Delaware C-Corp — standard for SBIR eligibility, SAFE notes, VC priced rounds, and patent assignment |
| Follow-up | Obtain EIN from IRS, register on SAM.gov for UEI number, assign IP from founder to company, open business bank account |

Delaware C-Corp is required for:
- SBIR/STTR eligibility (US small business, 51%+ US-owned, <500 employees)
- Y Combinator / accelerator SAFEs
- VC priced rounds
- Patent assignment from founder to entity

---

## Tier 1: Non-Dilutive Grants (0% Equity)

### 1. NSF SBIR Phase I

| Field | Detail |
|-------|--------|
| Agency | National Science Foundation |
| Amount | $275,000 |
| Relevance | **HIGH** — "Semiconductors" and "Novel AI Hardware" are explicit topic areas. Formal verification + FPGA IP = strong fit. |
| Status | Paused (SBIR reauthorization lapse). Monitor seedfund.nsf.gov. Expected to resume 2026. |
| Phase II | Up to $1M upon Phase I success |

**ATOMiK pitch angle:** Hardware-accelerated delta-state algebra with 92 machine-verified proofs — a novel computing primitive with applications in AI inference, sensor fusion, and secure state management. Frame under "Semiconductors and Photonics" or "Novel AI Hardware" topics.

### 2. DoD SBIR/STTR

| Field | Detail |
|-------|--------|
| Agency | Department of Defense (Army, DARPA, Navy) |
| Amount | $50K–$250K (Phase I) |
| Relevance | **HIGH** — Army topic "Open Source, High Assurance Hardware and Software Co-Design" directly matches FPGA + formal verification. DARPA PROVERS (formal methods) and SSITH (hardware security) are aligned. |
| Status | Paused pending reauthorization. Monitor dodsbirsttr.mil. |

**ATOMiK pitch angle:** Formally verified hardware for assured state management at the tactical edge. 92 Lean4 proofs guarantee correctness properties (commutativity, idempotence, self-inverse) that eliminate classes of hardware bugs. Single-cycle operation at ~20mW enables deployment on resource-constrained platforms.

### 3. DOE SBIR Phase I

| Field | Detail |
|-------|--------|
| Agency | Department of Energy, Office of Science |
| Amount | $200K–$250K |
| Relevance | **MEDIUM** — Energy-efficient computing topics. ATOMiK's 95–100% memory traffic reduction and ~20mW power consumption map to energy efficiency narratives. |
| Status | FY2026 topics at science.osti.gov/sbir |

**ATOMiK pitch angle:** Delta-state accumulation eliminates 95–100% of memory bus traffic compared to full-state copy architectures. At ~20mW on a $10 FPGA, ATOMiK demonstrates a path to orders-of-magnitude energy reduction in state-heavy workloads (sensor fusion, database replication, digital twins).

### 4. CHIPS Act R&D BAA

| Field | Detail |
|-------|--------|
| Agency | Department of Commerce, CHIPS Program Office |
| Amount | Varies (project-based) |
| Relevance | **MEDIUM** — Semiconductor R&D. BAA solicits proposals for microelectronics technology. Caveat: may require equity/royalty return on large awards. |
| Status | Concept plans accepted through Nov 2026. apply@chips.gov |

**ATOMiK pitch angle:** Novel XOR-based computing primitive that achieves linear throughput scaling in 7% LUT utilization — extreme area efficiency for a fully formally verified hardware block. Applicable as a co-processor IP block for next-generation American semiconductor designs.

### 5. NASA SBIR

| Field | Detail |
|-------|--------|
| Agency | NASA |
| Amount | $150K (Phase I) |
| Relevance | **LOW–MEDIUM** — Radiation-tolerant computing, formal verification for flight systems. Niche but defensible. |
| Status | Program Year 2026 hub active at sbir.nasa.gov |

**ATOMiK pitch angle:** XOR-based accumulation is inherently radiation-tolerant (single-bit upsets are self-correcting via commutative merge). 92 formal proofs provide flight-system-grade assurance without simulation gaps.

### SBIR/STTR Application Requirements (All Agencies)

**Eligibility:**
- Delaware C-Corp (51%+ US-owned, <500 employees)
- DUNS/UEI number (SAM.gov registration — allow 2–4 weeks)
- Principal Investigator (PI) — founder

**Proposal components:**
- Technical proposal (problem statement, approach, Phase I objectives, milestones)
- Budget and budget justification
- Commercialization plan
- Biographical sketch of PI
- Current/pending support documentation

**ATOMiK's SBIR strengths:**
- 92 formal proofs = strong "preliminary data" section
- Working FPGA hardware = de-risked feasibility
- 353 tests across 5 languages = engineering maturity
- $225 total development spend = extreme capital efficiency narrative
- Defense applications (sensor fusion, secure state management) = DoD relevance
- Patent pending = IP defensibility

---

## Tier 2: Zero-Equity Programs (0% Equity)

### 6. NVIDIA Inception

| Field | Detail |
|-------|--------|
| Operator | NVIDIA |
| Cost | Free. No equity. |
| Benefits | Up to $100K cloud credits, preferred GPU pricing, Deep Learning Institute training, VC network access, co-marketing opportunities |
| Relevance | **MEDIUM** — ATOMiK's AI inference angle fits. Frame as "state management accelerator for AI inference pipelines." No FPGA-specific track but AI hardware angle works. |
| Requirements | Website, 1+ developer, incorporated, <10 years old |
| Apply | nvidia.com/startups |
| Timeline | No deadline — rolling acceptance |

**ATOMiK pitch angle:** Hardware-accelerated state management for AI inference pipelines. Delta-state accumulation reduces memory bandwidth by 95–100%, enabling faster model serving at the edge.

### 7. Creative Destruction Lab (CDL)

| Field | Detail |
|-------|--------|
| Operator | CDL (University of Toronto / global sites) |
| Cost | Free. No equity. |
| Benefits | 10-month objective-setting program. Mentorship from scientists and investors. Access to deep-tech founder network. |
| Relevance | **MEDIUM–HIGH** — "New Compute" stream (Berlin site) covers non-von-Neumann architectures. Quantum/deep-tech founder network. |
| Requirements | Application with technical description and founder background |
| Timeline | Apply summer 2026 for 2026/2027 cohort at creativedestructionlab.com |

**ATOMiK pitch angle:** A new computing primitive — delta-state algebra in silicon. Not an incremental improvement to existing architectures but a mathematically novel approach with 92 formal proofs. Fits CDL's mandate to support science-based ventures.

### 8. NSF I-Corps

| Field | Detail |
|-------|--------|
| Operator | National Science Foundation |
| Amount | $50K for customer discovery |
| Relevance | **HIGH** — Validates commercial assumptions. Strong signal for follow-on SBIR. |
| Requirements | 3-person team (technical lead + mentor + entrepreneurial lead). Typically follows an SBIR/STTR award or NSF connection. 7-week program. |
| Timeline | Multiple cohorts per year. Check icorps.nsf.gov |

**ATOMiK pitch angle:** Use $50K to validate which vertical (HFT, IoT, video processing, database replication) has the highest willingness-to-pay for delta-state hardware IP. 100+ customer discovery interviews.

---

## Tier 3: Low-Dilution Accelerators (3–10% Equity)

### 9. Silicon Catalyst

| Field | Detail |
|-------|--------|
| Terms | $150K on admission + access to millions in EDA/IP/fab in-kind from TSMC, Synopsys, ARM, and others. 24-month incubation. |
| Relevance | **HIGHEST** — The only semiconductor-specific accelerator. FPGA IP companies in portfolio (e.g., Chevin Technology). Strategic partners (ARM, TSMC, GlobalFoundries, NXP) participate in selection. |
| Requirements | Application with IC/chip design details, technology node, TAM/SAM/SOM analysis |
| Timeline | Next cohort deadline TBD (Jan 2026 deadline passed). Apply at siliconcatalyst.com/application |

**ATOMiK pitch angle:** Formally verified XOR accumulator IP targeting ASIC integration. 92 Lean4 proofs, working FPGA prototype on Tang Nano 9K, 7% LUT utilization (single bank). Seeking EDA tool access and fab partner introductions to move from FPGA to ASIC.

### 10. Alchemist Accelerator

| Field | Detail |
|-------|--------|
| Terms | ~$36K for ~5% equity. Deep Tech track (Chicago/UChicago) offers $50K. 6-month B2B/enterprise program. |
| Relevance | **HIGH** — B2B enterprise focus matches IP licensing model. Hardware welcome. Deep tech track specifically for foundational technology. SF + Chicago hubs. |
| Timeline | Next class began Jan 2026. Apply for next cohort at alchemistaccelerator.com |

**ATOMiK pitch angle:** B2B IP licensing play — delta-state hardware blocks for chip designers and system integrators. Proven architecture (1 Gops/s on $10 FPGA), 5-language SDK for customer integration, patent pending.

### 11. HAX (SOSV)

| Field | Detail |
|-------|--------|
| Terms | Up to $250K–$500K for ~10% equity. 6-month hardware-specific program. Shenzhen + Newark locations. |
| Relevance | **HIGH** — Premier hardware accelerator. 348+ portfolio companies, $2.5B raised, $8.6B cumulative value. Prototyping resources and supply chain support. |
| Timeline | Rolling applications at hax.co |

**ATOMiK pitch angle:** Working hardware on $10 FPGA — not a slide deck, not a simulation. 1 Gops/s throughput, 92 formal proofs, $225 total development cost. Ready for HAX's hardware-to-market pipeline.

### 12. Y Combinator

| Field | Detail |
|-------|--------|
| Terms | $500K on standard SAFE terms. 3-month batch. |
| Relevance | **HIGH** — Actively funding semiconductor/hardware startups (Inversion Semiconductor, SigmanticAI in recent batches). Brand signal for follow-on funding. |
| Requirements | Application at ycombinator.com. Video demo. |
| Timeline | Batches: Winter (January start) and Summer (June start) |

**ATOMiK pitch angle:** Hardware that works, math that's proven, software that ships. 92 formal proofs, 1 Gops/s on a $10 chip, 353 passing tests, $225 total spend. The IP licensing business model scales without manufacturing risk.

### 13. Techstars

| Field | Detail |
|-------|--------|
| Terms | $120K for 6% equity. 3-month program. |
| Relevance | **MEDIUM** — Broad network but less hardware-specific. Good for commercial traction phase. |
| Timeline | Spring 2026 programs at techstars.com |

**ATOMiK pitch angle:** Novel computing IP with working hardware and multi-language SDK. Seeking commercial pilot customers in HFT, IoT, or database replication verticals.

### Accelerator Application Requirements (General)

- Incorporated entity (C-Corp preferred)
- Application with: team, technology, traction, market, ask
- Demo/video (ATOMiK has 3-node FPGA demo ready)
- For Silicon Catalyst specifically: IC/chip design details, technology node, TAM/SAM/SOM

**ATOMiK's accelerator strengths:**
- Working silicon on $10 FPGA (most applicants have no hardware)
- 92 formal proofs (unmatched in semiconductor startups)
- $225 total development cost (extreme capital efficiency story)
- 5-language SDK (commercial surface area)
- Patent pending

---

## Tier 4: Strategic/Defense Investors (Equity Terms Vary)

### 14. In-Q-Tel (IQT)

| Field | Detail |
|-------|--------|
| Type | CIA/Intelligence Community strategic VC |
| Relevance | **HIGH** — Formal verification + hardware security + edge computing = intelligence community fit. 750+ investments. |
| Engagement | IQT scouts startups — no public application. Engage through iqt.org or get introduced via Silicon Catalyst, HAX, or defense-tech conferences. |

**ATOMiK pitch angle:** Formally verified hardware for assured state management in contested environments. Self-inverse property enables instant rollback (no checkpoints). Commutative merge enables lock-free multi-sensor fusion at the tactical edge. 92 Lean4 proofs — not tested, proven.

### 15. Shield Capital

| Field | Detail |
|-------|--------|
| Type | Defense-focused VC |
| Relevance | **MEDIUM–HIGH** — Dual-use hardware, cybersecurity thesis. |
| Engagement | Through network introductions or defense-tech events |

**ATOMiK pitch angle:** Dual-use hardware IP — same delta-state architecture serves commercial IoT and defense sensor fusion. Formally verified correctness properties eliminate classes of hardware vulnerabilities.

### Engagement Approach for Tier 4

IQT and defense VCs don't have standard application processes. Best paths:
1. Get into Silicon Catalyst or HAX first, then get introduced
2. Present at defense-tech conferences (DARPA Demo Days, DIU events)
3. Publish in defense-adjacent venues (formal verification for hardware assurance)
4. Cold outreach with 1-pager + demo video through iqt.org

---

## Tier 5: Semiconductor/Hardware-Focused VCs (Priced Round)

| # | Firm | Stage | Check Size | Relevance | Notes |
|---|------|-------|-----------|-----------|-------|
| 16 | **Eclipse Ventures** | Seed–A | $1–10M | HIGH | Semiconductor, manufacturing, deep tech. Operating experience in chip industry. |
| 17 | **Lux Capital** | Seed–A | $1–15M | HIGH | Deep tech thesis. Hardware, materials, compute. |
| 18 | **DCVC (Data Collective)** | Seed–A | $1–10M | HIGH | Computational approaches, deep tech, AI infrastructure. |
| 19 | **Playground Global** | Seed | $1–5M | HIGH | Hardware, systems, computing architecture. |
| 20 | **Intel Capital** | Seed–A | $1–10M | MEDIUM | Strategic. Semiconductor ecosystem investments. |
| 21 | **Qualcomm Ventures** | Seed–A | $1–5M | MEDIUM | Edge computing, low-power hardware. |
| 22 | **Samsung NEXT** | Seed | $1–5M | MEDIUM | Hardware innovation, semiconductor ecosystem. |
| 23 | **Cantos Ventures** | Pre-seed/Seed | $250K–2M | HIGH | Deep tech, hardware, small checks. Good for earliest rounds. |
| 24 | **Khosla Ventures** | Seed | $1–10M | MEDIUM | Breakthrough tech thesis. AI infrastructure. |
| 25 | **a16z START** | Pre-seed | $500K SAFE | MEDIUM–HIGH | Technical founders building foundational tech. Fast, clean terms. |

### VC Engagement Approach

1. **Warm intros** through accelerator networks (Silicon Catalyst, YC, Alchemist all have VC demo days)
2. **Cold outreach** with 1-pager (`business/one_pager/atomik_one_pager.md`) + demo video
3. **Target list:** 20–30 firms from the table above plus sector-adjacent funds
4. **Process:** Expect 5–10 meetings from 20–30 outreach, aim for 2–3 term sheets
5. **Timing:** Run VC outreach after at least one credibility signal (accelerator acceptance, SBIR award, or notable pilot customer)

**ATOMiK pitch angle for VCs:** IP licensing play in a $600B+ semiconductor market. Formally verified hardware IP that no competitor can replicate without years of mathematical work. Working silicon, not a slide deck. $225 → 1 Gops/s = the most capital-efficient hardware startup in existence.

---

## Recommended Execution Sequence

### Phase 0: Entity Formation (Week 1)

- [ ] Incorporate Delaware C-Corp via Stripe Atlas or Clerky (~$500)
- [ ] Obtain EIN from IRS (immediate upon incorporation)
- [ ] Register on SAM.gov for UEI number (required for SBIR — allow 2–4 weeks)
- [ ] Execute IP assignment agreement (founder → company)
- [ ] Open business bank account
- [ ] Set up cap table (100% founder pre-funding)

### Phase 1: Zero-Cost Programs (Weeks 2–4)

- [ ] Apply to **NVIDIA Inception** (free, immediate benefits, no deadline)
- [ ] Apply to **Silicon Catalyst** next open cohort (highest-value semiconductor accelerator)
- [ ] Register interest with **CDL** for 2026/2027 cohort (summer 2026 applications)
- [ ] Record 2–3 minute demo video using `business/demo_recording/recording_script.md`

### Phase 2: Grant Preparation (Weeks 2–8)

- [ ] Monitor SBIR reauthorization status (sbir.gov, relevant industry newsletters)
- [ ] Draft NSF SBIR Phase I proposal targeting "Semiconductors" or "Novel AI Hardware" topic
- [ ] Draft DoD SBIR proposal targeting Army "High Assurance Hardware" topic or DARPA formal methods
- [ ] Complete SAM.gov registration (started in Phase 0)
- [ ] Prepare CHIPS Act R&D BAA concept plan
- [ ] Prepare NSF I-Corps application (identify mentor and entrepreneurial lead)

### Phase 3: Accelerator Applications (Weeks 4–8)

- [ ] Apply to **Y Combinator** Summer 2026 batch
- [ ] Apply to **HAX** (rolling admissions)
- [ ] Apply to **Alchemist** next cohort
- [ ] Submit SBIR proposals as soon as reauthorization passes
- [ ] Record 2-minute pitch video for accelerator applications

### Phase 4: VC Outreach (Weeks 8–16)

- [ ] Use accelerator acceptances and/or SBIR awards as credibility signals
- [ ] Build target list of 20–30 firms from Tier 5 table
- [ ] Seek warm intros through accelerator mentor networks
- [ ] Run structured fundraise process (back-to-back meetings over 2–3 weeks)
- [ ] Engage defense/strategic investors (Tier 4) through network introductions

---

## Application Materials Inventory

| Material | Status | Location / Action |
|----------|--------|-------------------|
| Investor deck (10-slide) | **Done** | `business/pitch_deck/investor_deck_full.md` and `.pptx` |
| 1-page executive summary | **Done** | Part 3 of `business/pitch_deck/investor_deck_full.md` |
| One-pager | **Done** | `business/one_pager/atomik_one_pager.md` |
| Demo video (3-min) | **Needed** | Record using `business/demo_recording/recording_script.md` |
| 2-min pitch video | **Needed** | For YC, Alchemist, HAX applications |
| SBIR technical proposal | **Needed** | 15-page technical narrative + budget justification |
| SAM.gov registration | **Needed** | Requires DUNS/UEI — allow 2–4 weeks |
| Delaware C-Corp | **Needed** | Prerequisite for all programs |
| IP assignment agreement | **Needed** | Founder → company assignment of all ATOMiK IP |
| Cap table | **Needed** | Simple: 100% founder pre-funding |

---

## Program-Specific Pitch Framing

| Audience | Frame ATOMiK As |
|----------|-----------------|
| NSF SBIR | Novel semiconductor architecture with 92 formal proofs — advancing the state of the art in verified hardware design |
| DoD SBIR | High-assurance hardware for edge computing in contested environments — formally verified state management for sensor fusion |
| DOE SBIR | Energy-efficient computing primitive — 95–100% memory traffic reduction, ~20mW power on $10 FPGA |
| CHIPS Act | American semiconductor IP — area-efficient (7% LUT) formally verified compute block for next-gen chip designs |
| NASA SBIR | Radiation-tolerant state management with flight-system-grade formal verification (92 Lean4 proofs) |
| NVIDIA Inception | State management accelerator for AI inference pipelines — reducing memory bandwidth in model serving |
| CDL | Science-based venture — new computing primitive grounded in delta-state algebra, not incremental improvement |
| Silicon Catalyst | FPGA-proven IP block ready for ASIC integration — seeking EDA access and fab partner introductions |
| Alchemist | B2B IP licensing play — hardware blocks for chip designers with 5-language SDK for customer integration |
| HAX | Working hardware, not a slide deck — $225 → 1 Gops/s on $10 FPGA, ready for hardware-to-market pipeline |
| YC | Most capital-efficient hardware startup — 92 proofs, 353 tests, working silicon, $225 total spend |
| In-Q-Tel | Assured hardware for intelligence community — formally verified, self-inverse (instant rollback), commutative (lock-free merge) |
| VCs | IP licensing in $600B+ semiconductor market — irreproducible formal verification moat, working silicon, patent pending |

---

## Key Dates and Deadlines

| Program | Deadline | Action |
|---------|----------|--------|
| NVIDIA Inception | Rolling (no deadline) | Apply immediately after incorporation |
| Silicon Catalyst | Next cohort TBD (Jan 2026 passed) | Monitor siliconcatalyst.com for next deadline |
| CDL | Summer 2026 | Apply for 2026/2027 cohort |
| Y Combinator | Summer batch ~March 2026 deadline | Apply for Summer 2026 batch |
| HAX | Rolling | Apply after incorporation |
| Alchemist | Next cohort ~Q3 2026 | Monitor alchemistaccelerator.com |
| Techstars | Spring 2026 apps open | Apply at techstars.com |
| NSF SBIR | TBD (pending reauthorization) | Monitor seedfund.nsf.gov |
| DoD SBIR | TBD (pending reauthorization) | Monitor dodsbirsttr.mil |
| DOE SBIR | FY2026 topics | Monitor science.osti.gov/sbir |
| CHIPS Act BAA | Concept plans through Nov 2026 | Submit concept plan after SAM.gov registration |
| NASA SBIR | Program Year 2026 | Monitor sbir.nasa.gov |

---

*Last updated: February 2026*
