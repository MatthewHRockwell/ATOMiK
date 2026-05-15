# Meeting Prep: Jeremy Mueller — Alliance for SoCal Innovation

> **Publication status: INTERNAL_PREP / NOT PUBLIC CLAIM COPY.**
> This file contains meeting notes, rough talk tracks, and older metric language.
> Do not quote performance, customer, production, pricing, or fundraising claims
> from this file unless each claim is reconciled with `results/claims_registry.yaml`
> and the current evidence-label docs.

**Date:** Thursday, March 12, 2026 at 3:00 PM PDT  
**Format:** Zoom (link from Calendly invite)  
**Contact:** jeremy@alliancesocal.org

---

## What Is the Alliance / SoCal Venture Pipeline (SVP)?

- **Non-profit** that matches investor-ready startups with VC investors for Seed and Series A
- **$224M+ raised** by SVP startups since 2021, **$4.8M average** raise
- **350+ VC network** — personal introductions, not cold emails
- **No fees, no equity, no catch** — funded by sponsors and a federal grant
- They recently launched a **Catalyst Fund** that can invest directly
- Two-committee vetting process: if you get in, VCs take the intro seriously
- Recent pipeline companies include aerospace/defense, medtech, biotech, AI — **deep tech fits perfectly**

**This is potentially the single most valuable connection for ATOMiK's fundraising.**

---

## Meeting Objectives

1. **Get accepted into the SVP program** — this is the primary goal
2. **Understand their process** — timeline, what they need, evaluation criteria
3. **Build rapport with Jeremy** — he's your champion inside the organization
4. **Ask about the Catalyst Fund** — could they invest directly?
5. **Get intel** — which VCs in their network invest in semiconductor/hardware IP?

---

## What Jeremy Already Knows

From your email exchange, you sent him:
- ATOMiK overview (delta-state computation, formal proofs, FPGA hardware)
- Your background as sole founder
- That you're raising a seed round

He reached out to YOU — that's a warm signal. He saw something worth scheduling a call about.

---

## Your 2-Minute Pitch (Rehearse This)

> "ATOMiK is a hardware IP core that eliminates redundant memory operations in real-time embedded systems. Instead of reading and writing full state buffers every cycle, it tracks only what changed — using XOR delta accumulation that's formally proven correct with 108 machine-checked theorems.
>
> The result: 120x to 30,000x memory traffic reduction on validated workloads, deterministic 2-cycle latency, and it fits in 287 logic cells — less than 1% of a mid-range FPGA.
>
> I built the entire thing — hardware, SDK, proofs, papers — for $225 on a $13 dev board. It's Patent Pending, published on Zenodo, and under peer review at Scientific Reports.
>
> The business model is ARM-style IP licensing. Edge and embedded is the beachhead — sensor fusion, industrial controllers, autonomous systems. $50B+ TAM in edge AI alone.
>
> I'm raising $3-4M seed to hire the first two engineers, run an ASIC feasibility study, and get the first paid customer evaluations going."

---

## Likely Questions & Answers

### "Why haven't you raised yet?"
"I've been heads-down building. The technology needed to be proven first — formally verified, running on silicon, benchmarked. That's done. Now I'm converting from LLC to C-Corp and actively fundraising. Chris Bolt is interested in the full round, and I'm building out the pipeline."

### "Are you SoCal-based?"
"I'm in Santa Rosa (NorCal), but the company can register in SoCal if that's a requirement. Happy to discuss what makes sense." *(Check if they require SoCal — the name implies it, but ask early)*

### "Do you have customers?"
"Not yet — honest answer. The seed funds both product hardening and the first customer conversations. I have 9 validated workloads with statistical significance, and the SDK makes integration straightforward, but no paying customer today."

### "What's your competitive advantage?"
"Three things no one else has: (1) 108 machine-checked proofs — this isn't tested, it's *proven*, (2) working hardware on two FPGA families for $225, and (3) a schema-driven SDK that generates integration code in 5 languages. The math is published. The hardware works. The moat is the formal verification — it would take years to replicate."

### "Solo founder — isn't that risky?"
"Yes, and I'm transparent about it. The seed funds the first two hires: an FPGA/ASIC engineer and an applications engineer. The repo is structured for reproducibility — `make` builds hardware, `lake build` checks proofs, `pytest` runs 353 tests. I'm also forming an advisory board targeting semiconductor, HFT, and IP licensing veterans."

### "What about Ubitium?"
"Ubitium is building a chip. We're licensing IP. They're trying to replace GPUs; we're making existing embedded systems faster. Different market, different model. ARM doesn't compete with NVIDIA — they complement each other."

---

## Key Numbers to Have Ready

| Metric | Value |
|--------|-------|
| Memory traffic reduction | 120x–30,720x |
| FPGA resource usage | 287 LUT (0.54% of Zynq) |
| Power | 1.8 mW |
| Formal proofs | 108 Lean4 theorems |
| SDK tests | 353 passing, 5 languages |
| Peak throughput | 1,056 Mops/s (16 banks) |
| Total build cost | $225 |
| Target raise | $3-4M seed |
| Target use of proceeds | 2 engineers + ASIC feasibility + customer pilots |

---

## Questions to Ask Jeremy

1. "Is there a geographic requirement for SoCal? I'm based in Santa Rosa."
2. "What does the evaluation committee look for? What makes a strong application?"
3. "What's the typical timeline from application to VC introductions?"
4. "Are there VCs in your network focused on semiconductor IP, hardware, or deep tech?"
5. "Can you tell me about the Catalyst Fund — would ATOMiK be a fit for direct investment?"
6. "What's the best way to frame a solo-founder hardware company for your committees?"
7. "Are there other resources or programs through the Alliance I should know about?"

---

## Materials to Have Open During Call

- [ ] ATOMiK one-pager (`business/one_pager/atomik_one_pager.md`)
- [ ] Pitch deck (`business/pitch_deck/investor_deck_full.md`)
- [ ] Benchmark data (Appendix A from Chris Bolt due diligence doc)
- [ ] GitHub repo: github.com/MatthewHRockwell/ATOMiK
- [ ] Zenodo papers (DOIs ready to share)

---

## ⚠️ Watch Outs

- **Don't oversell.** Jeremy is a gatekeeper — he needs to trust that you're real, not hyped. The numbers speak for themselves.
- **SoCal requirement** — find out early if this is a blocker. If so, discuss options.
- **Don't badmouth competitors** — position ATOMiK as complementary, not adversarial.
- **Follow up immediately** — send a thank-you email within 1 hour of the call with any materials he requests.

---

## Post-Meeting Actions

1. Send thank-you email with requested materials
2. Submit SVP application (if he directs you to)
3. Update MEMORY.md with outcomes
4. If accepted, prepare for VC committee presentation
5. Ask if he can intro you to specific VCs in the interim

---

*This meeting could open the door to 350+ VCs who take warm intros seriously. Treat it accordingly.*

---

## Additional Insights from Materials Review (Added March 11)

### Strengths to Emphasize
- **v3 SoC is brand new (March 2026)** — custom 64-bit RISC-V CPU with HDMI demo. This is extremely fresh and shows rapid iteration. Mention that v2 was February, v3 was March — two production SoCs in one month.
- **The 6.4× memcpy speedup story** is compelling: v2 was actually 12% *slower* than software due to CDC overhead. v3 with direct-wire integration flipped that to 6.4× faster. This shows engineering learning and iteration, not just blind building.
- **The deck has a full data room checklist** — if Jeremy asks "do you have X for due diligence," the answer is almost certainly yes. Mention the data room is ready.
- **Comparable companies are well-researched** — Positron AI ($51.6M Series A for FPGA inference) is the strongest comp for SVP context. Ubitium ($3.7M seed, no hardware demos) shows ATOMiK is further along on less capital.

### Potential Weak Spots to Prepare For
- **SoCal geographic requirement** — this could be a blocker. Lead with this question early. Have a plan: willingness to register in SoCal, establish a presence, or travel for committee presentations.
- **No revenue / no customers yet** — be honest and frame it: "Technical risk is retired. The seed funds commercial validation." The $4.8M SVP average raise suggests their companies are typically pre-revenue or early-revenue.
- **Solo founder concern** — the AI-augmented development narrative is strong but unusual. Frame it as: "I built what most teams of 5-10 build, for 1/10,000th the cost. The seed hires the first two engineers."
- **The one-pager includes v3 SoC details** — it's up to date. Good to go.

### Materials Status
- [x] One-pager: Current, includes v3 SoC metrics ✅
- [x] Pitch deck: Comprehensive (10-slide partner + 23-slide IC + exec summary + data room) ✅
- [x] Application brief: Created at `business/outreach/alliance_socal_application_brief.md` ✅
- [x] Follow-up email template: Created at `business/outreach/jeremy_followup_template.md` ✅
- [ ] Benchmark appendix from Chris Bolt due diligence — have this accessible during call

### Tactical Notes for the Call
- **Open with gratitude** that Jeremy reached out — reinforce the warm signal
- **Ask the SoCal question within the first 5 minutes** — don't waste 30 minutes if it's a geographic blocker
- **Offer to ship a $13.50 board** — "You can hold the proof in your hand" is a memorable close
- **Name-drop the peer review** — "Under peer review at Scientific Reports (Springer Nature)" adds credibility
- **If he asks about the Catalyst Fund**, express strong interest — a direct investment from the Alliance plus SVP introductions would be ideal
- **Close by asking**: "What would make me a strong application? What should I emphasize for your committee?"
