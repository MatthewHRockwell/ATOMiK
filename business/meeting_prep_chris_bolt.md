# Meeting Prep: Chris Bolt — Thursday

**Context:** Chris reviewed the pitch deck and shared it with investor partners. He wants to discuss potentially funding the entire $3M–$4M seed round. A demo board is being shipped.

---

## Meeting Objectives

1. **Confirm interest level** — Is this a solo check or syndicated? What's his fund size/thesis?
2. **Establish valuation range** — The deck says $3M–$4M for 15–25% equity (implies $12M–$27M pre-money). Know your floor.
3. **Agree on next steps** — Term sheet timeline, due diligence process, legal structure (SAFE vs. priced round)
4. **Address the C-Corp question** — Entity is currently Rockwell Industries, LLC (California). VC investment typically requires Delaware C-Corp. Have a plan ready.

---

## Run of Show (Suggested 45–60 min)

| Time | Section | Goal |
|------|---------|------|
| 0–5 min | Rapport / catch-up | Reconnect personally. Don't rush into the pitch. |
| 5–15 min | Quick walkthrough | Hit the 3 pillars: math, hardware, market timing. He's read the deck — don't re-present it. Focus on what's NOT in the deck. |
| 15–25 min | Demo board discussion | Walk through what he'll see on HDMI. Reference the guide. If he has it connected, walk him through live. |
| 25–40 min | His questions | This is the most important section. Let him drive. |
| 40–50 min | Deal structure | Valuation, instrument, timeline, use of funds |
| 50–60 min | Next steps | What he needs to move forward. Offer data room access. |

---

## Key Talking Points (What's NOT in the Deck)

### 1. Why this is fundable at seed, not pre-seed
- Technical risk is retired — working silicon, not a slide deck
- The $225 spend is real and verifiable (3x $13.50 boards + cables)
- 92 proofs don't regress — mathematical truth is permanent IP
- The demo board he's holding IS the product (not a prototype of a prototype)

### 2. The AI-augmented development thesis
- One founder + AI produced output that typically requires 5–10 engineers and 12–18 months
- This is itself a proof point: AI tooling changes the economics of hardware development
- Post-funding, a small team (3–5) with the same AI-augmented model = exponential output
- The $225→$3M capital efficiency ratio is a story investors remember

### 3. What the money buys (be specific)
- **$1.2M–$1.6M Hardware R&D (40%)**: Xilinx Zynq port is already in progress (synthesis validated, 287 LUT). Mid-range FPGA with N=64+ banks → >4 Gops/s. ASIC feasibility study with foundry partner (28nm). Dev boards and lab equipment.
- **$750K–$1M SDK & Platform (25%)**: Production-harden the SDK. Build vertical modules (HFT tick processor, sensor fusion core, streaming transform). Developer documentation and community.
- **$600K–$800K Business Dev (20%)**: Patent prosecution (provisional → utility). Strategic partnerships. Pilot customer support. Conference presence.
- **$450K–$600K Team (15%)**: FPGA/ASIC engineer (first hire), application engineer. Both can be hired at competitive rates outside SF.

### 4. The 18-month milestone map
- **Month 1–3**: C-Corp conversion, patent prosecution, Zynq full deployment
- **Month 3–6**: Mid-range FPGA port (N=64+), SDK production hardening
- **Month 6–9**: First vertical modules (HFT, sensor fusion), begin customer outreach
- **Month 9–12**: 2 pilot design wins, ASIC feasibility study initiated
- **Month 12–18**: ASIC feasibility complete, SDK community at 500+, continuation patents filed
- **Month 18**: Series A ready with revenue-generating customers and ASIC economics

---

## Anticipated Questions & Answers

### Deal Structure

**Q: What instrument are you raising on?**
A: Open to discussion. A priced seed round or a SAFE with a valuation cap both work. For a full-round commitment, a priced round with board seat makes sense — it aligns incentives and gives you governance rights. If you prefer a SAFE for speed, we can do that with a cap in the $16M–$20M range (based on AI seed median pre-money of $17.9M per PitchBook).

**Q: What's your pre-money valuation expectation?**
A: The deck shows $3M–$4M for 15–25% equity. That implies a range of $12M–$27M pre-money. Given working hardware, 92 proofs, and the AI seed premium (42% over non-AI per PitchBook), a $16M–$20M pre-money is defensible. But I'm more focused on the right partner than maximizing valuation at seed.

**Q: What's the current cap table?**
A: 100% founder. No prior investors, no option pool allocated yet. Clean cap table. Post-funding, we'd create a standard 10–15% option pool for team hires.

**Q: What's the entity structure?**
A: Currently Rockwell Industries, LLC (California), active since May 2023. Plan is to convert to Delaware C-Corp — standard statutory conversion. This is a 2–4 week process. Can be initiated immediately upon term sheet.

**Q: Do you have other investors in the round?**
A: You're the first serious conversation. If you want to take the full round, that's ideal — single lead with a clean cap table, fast close, aligned decision-making. If you want to syndicate with your partners who've seen the deck, that works too.

### Technology

**Q: XOR is trivial — what's the real IP here?**
A: XOR is trivial. The IP is the complete system: the algebraic framework (92 proofs establishing that XOR accumulation is computationally equivalent to traditional state management), the parallel bank architecture (N independent accumulators with combinational merge tree), the synthesis optimization (preventing carry-chain inference, +42% Fmax), and the schema-driven code generation pipeline. No one else has combined formal verification with hardware validation for a delta-state primitive. The patent covers the architecture, not the XOR gate.

**Q: Can a big company just copy this?**
A: They could build an XOR accumulator. They cannot easily replicate: (1) 92 machine-verified proofs — that's months of proof engineering, (2) the full SDK with 5-language code generation, (3) the patent claims, (4) the validation data (80/80 hardware tests, 353 SDK tests, statistical benchmarks). Time-to-replicate is 6–12+ months for a well-resourced team starting from our published work. By then we'll have design wins and continuation patents.

**Q: How do you know the 916,000x number is real?**
A: It's measured, not estimated. The benchmark streams a workload through both a conventional store-and-forward path and the ATOMiK delta path, counting bytes moved. The methodology is in the repository — anyone can reproduce it. The 916,000x is the high end (streaming workload); the low end is 7,670x (smaller working set). We report both.

**Q: What about larger data widths — 128-bit, 256-bit?**
A: The architecture is parametric. DATA_WIDTH is a Verilog parameter. 128-bit and 256-bit are wire-width changes — the math is identical. The SDK schemas already define widths up to 256 bits. Resource cost scales linearly with width (2x width = 2x LUTs for the XOR).

**Q: Does this work outside FPGAs?**
A: Yes. The architecture is vendor-agnostic Verilog. We've already validated synthesis on both Gowin (Tang Nano 9K) and Xilinx (Zynq-7020). GPU implementation is straightforward — XOR reduction is a natural CUDA kernel. ASIC implementation would give 10–100x power and performance improvement over FPGA. The IP licensing model means we don't manufacture — we license the design.

**Q: What about the solo founder risk?**
A: Three mitigations: (1) Every deliverable is documented and reproducible — the proofs are machine-checked, the RTL is synthesizable, the SDK generates code from schemas. Nothing depends on tribal knowledge. (2) First two hires (FPGA engineer + application engineer) are budgeted in the raise. (3) The AI-augmented model means each person produces 5–10x traditional output. A 3-person team post-funding is equivalent to 15–30 traditional engineers.

### Market & Business

**Q: Who's the first customer?**
A: HFT is the highest-value beachhead. Tick-to-trade latency at 10.6 ns is compelling. Instant trade reversal (self-inverse) eliminates rollback infrastructure. We'd target Tier 2/3 HFT firms first (more accessible, still spending millions on latency) rather than Citadel/Jane Street.

**Q: What's the pricing for IP licensing?**
A: Comparable to ARM Cortex-M licensing: $500K–$5M per design, plus per-unit royalty ($0.01–$1.00 scaled to device ASP). ARM does $4B/year at 97% gross margin on this model. We start smaller — 15 design wins at $3M average = $45M in Year 5.

**Q: Why not just build a chip company?**
A: Chip companies require $50M+ to tape out and have 18–24 month design cycles. IP licensing generates revenue on other people's silicon. ARM proved this model — $4B revenue with zero manufacturing. We complement ARM (they do compute, we do state management), we don't compete with them.

**Q: What if nobody adopts delta-state computing?**
A: Adoption risk is the biggest risk — we're transparent about that. Mitigation: the SDK abstracts the paradigm. Developers write a JSON schema and get working code in 5 languages. They don't need to understand XOR algebra. The developer experience is "define your state, get a library that tracks changes for free." The hardware acceleration is invisible to the application layer.

**Q: What are the exit paths?**
A: Three paths: (1) Strategic acquisition by FPGA vendor (AMD/Xilinx, Lattice, Intel/Altera) — they acquire novel IP architectures. (2) Strategic acquisition by AI chip company (NVIDIA, Cerebras) — state management is complementary to compute. (3) IPO/growth path if we become the standard delta-state primitive (ARM model). Comparable exits: NVIDIA acquired Groq for $20B, Cerebras targeting $20B IPO, Tenstorrent at $2.6B.

---

## Numbers to Have Ready

| Metric | Value | Source |
|--------|-------|--------|
| Peak throughput | 1,056 Mops/s (16 banks, 66 MHz) | Hardware-validated synthesis sweep |
| Single-op latency | 10.6 ns (1 cycle @ 94.5 MHz) | Gowin STA |
| Memory traffic reduction | 7,670x to 916,000x | Python benchmarks (PERFORMANCE_COMPARISON.md) |
| Write-heavy speedup | +22% to +58% (p < 0.001) | Statistical benchmarks, 100 iterations |
| Formal proofs | 92 (Lean4, 0 sorry) | math/proofs/ |
| Hardware tests | 80/80 parallel bank sweep | UART validation suite |
| SDK tests | 353 (5 languages) | software/atomik_sdk/ |
| LUT utilization (N=1) | 579 / 8,640 (7%) | Gowin synthesis |
| LUT utilization (N=16) | 1,776 / 8,640 (20.6%) | Gowin synthesis |
| Power (ATOMiK core) | 1.8 mW | Gowin power analysis |
| Power (full SoC) | 62.2 mW | Gowin power analysis |
| Development cost | ~$225 | 3x Tang Nano 9K + cables |
| Board cost | $13.50 | Sipeed retail |
| AI seed median pre-money | $17.9M (42% premium) | PitchBook 2025 |
| AI Series A median | >$50M | PitchBook 2025 |
| US/Canadian startup funding 2025 | $280B (+46% YoY) | Crunchbase |
| Hyperscaler AI capex 2025 | $300B+ | Earnings calls |
| ARM FY2025 revenue | $4.0B at 97% GM | SEC filing |
| Lattice EV/Revenue | 21.6x ($10.7B / $489M) | Public market |

---

## Objection Handling

### "It's just one person"
*Reframe:* "One person with AI tools produced 92 proofs, working silicon, and a 5-language SDK for $225. The question isn't whether I can do more with a team — it's what a 3-person team can do with that same model. The first two hires are budgeted in the raise."

### "The market doesn't exist yet"
*Reframe:* "The memory wall is a $300B problem — DARPA, Intel, and every hyperscaler are spending to solve it. ATOMiK doesn't need a new market. It needs design wins in existing markets (HFT, edge AI, streaming) where memory bandwidth is already the bottleneck."

### "XOR is too simple to be defensible"
*Reframe:* "ARM's core product is also 'simple' — a processor instruction set. The IP value is in the complete verified architecture, not the gate. Our patent covers the system. Our 92 proofs establish mathematical novelty. Our SDK creates ecosystem lock-in. And our 80/80 hardware validation proves it works in silicon."

### "You need a co-founder"
*Reframe:* "I agree that the team needs to grow — that's 15% of the raise. But the right co-founder shows up when there's a funded company with working technology, not the other way around. The demo board in your hand is proof that the solo-plus-AI model works at this stage."

### "The valuation is too high for seed"
*Reframe:* "The AI seed median is $17.9M pre-money — 42% premium over non-AI. ATOMiK has working hardware, 92 proofs, and a full SDK. Most AI seeds fund a slide deck and a fine-tuned model. We've retired the technical risk. The remaining risk is commercial — and that's exactly what the seed capital addresses."

### "I'd want to see customers first"
*Reframe:* "Understood. The seed capital buys us 18 months to land 2 design wins. The Zynq port (already in progress) is the first step — it puts us on a platform that HFT and edge customers actually use. Would you be open to a milestone-based tranche structure? First tranche now for FPGA port and customer outreach, second tranche upon first LOI."

---

## Things to Bring / Have Ready

- [ ] Demo board shipped (confirm tracking)
- [ ] Demo Board Guide PDF (email to Chris before meeting)
- [ ] Pitch deck link (already sent)
- [ ] Data room access ready to share (business/data_room/)
- [ ] Financial model (business/data_room/01_financial/financial_model.md)
- [ ] Patent status summary (business/data_room/03_intellectual_property/patent_status.md)
- [ ] GitHub repo access (if he wants to inspect code/proofs)
- [ ] Entity conversion timeline (LLC → C-Corp, 2–4 weeks)
- [ ] Your availability for follow-up calls with his partners

---

## Pre-Meeting Checklist

- [ ] Confirm meeting time and format (video call? in-person?)
- [ ] Test your own demo board on a monitor — make sure it's running clean
- [ ] Send Demo Board Guide PDF to Chris ahead of meeting
- [ ] Prepare a clean data room link (Google Drive or similar) with key docs
- [ ] Research Chris's fund: thesis, typical check size, portfolio companies, stage focus
- [ ] Prepare 2–3 questions for HIM (shows you're evaluating fit, not desperate):
  - "What's your fund's typical check size at seed?"
  - "What does your portfolio support look like post-investment?"
  - "Have you invested in hardware/semiconductor IP before?"

---

## Red Lines (Know Before You Walk In)

- **Minimum valuation**: Know your floor pre-money. Below $12M, the dilution on a $4M raise exceeds 25%.
- **Board seat**: Acceptable at a priced round this size. One investor seat + one founder seat + one independent is standard.
- **Anti-dilution**: Standard weighted-average is fine. Full ratchet is a red flag.
- **Liquidation preference**: 1x non-participating is standard. Anything >1x or participating — push back.
- **C-Corp timeline**: Commit to conversion, but don't let it block the term sheet. Can close on SAFE pre-conversion if needed.
- **IP assignment**: Be prepared — he'll want confirmation that all IP is assigned to the company entity, not held personally. Have the assignment template ready (business/data_room/02_legal/ip_assignment_template.md).

---

*Last updated: March 10, 2026*
