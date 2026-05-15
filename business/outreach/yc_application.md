# Y Combinator Summer 2026 Application — ATOMiK

> **Publication status: APPLICATION DRAFT / REVIEW REQUIRED.**
> This draft contains older performance, funding, and maturity language. Rewrite
> against current evidence labels before submission.

## Company

**Company name:** Rockwell Industries (converting to Delaware C-Corp)
**Company URL:** github.com/MatthewHRockwell/ATOMiK
**If you have a demo, link here:** [3-node FPGA demo video — to be recorded]

## Founders

**Founder:** Matthew H. Rockwell
**Email:** matthew.h.rockwell@gmail.com
**Role:** Solo founder — designed the math, wrote the proofs, built prototype hardware, and built the SDK

---

## Application Questions

### Describe what your company does in 50 characters or less.

Formally verified hardware IP for state compute.

### What is your company going to make?

Licensable hardware IP cores that replace full-state computation with XOR-based delta accumulation. One operation, one clock cycle, 287 LUTs, 1.8 mW, mathematically proven correct with 108 machine-verified proofs. We license RTL to chip designers — the ARM model applied to state management.

### Why did you pick this idea to work on? Do you have domain expertise in this area? How do you know people need what you're making?

Every computing system — databases, trading engines, IoT sensors, AI inference — copies full state on every update. That's 120x–30,720x more memory traffic than necessary. I formalized an alternative in Lean4 (108 proofs), then built it in silicon on a $13.50 FPGA to prove it works.

Domain expertise: I designed the RTL, proved the math, wrote the SDK, deployed the SoC, and filed the patent. All of it, solo, for $225.

People need this because memory bandwidth is the bottleneck killing edge AI, real-time trading, and IoT at scale. The edge AI chip market is projected to hit $39B by 2030, and no one is shipping formally verified IP blocks. Every hardware bug that reaches silicon costs $1M+. We eliminate entire classes of bugs by mathematical construction.

### What's new about what you're making? What substitutes do people resort to today?

**Today's substitutes:**
- Event sourcing (Kafka): O(N) replay to reconstruct state. Breaks at scale.
- CRDTs: Software-only, microsecond latency, custom merge per data type.
- Traditional FPGA accelerators: Months of custom RTL, no formal verification, no reuse.

**What's new:**
- O(1) state reconstruction (single XOR, 10.6 ns) — not O(N) replay
- 120x–30,720x memory traffic reduction vs. full-state approaches
- 108 machine-verified proofs in Lean4 — not tested, *proven*
- 287 LUTs, 1.8 mW — runs on hardware that costs $13.50
- 1,056 Mops/s throughput from 16 parallel banks
- A single reusable primitive that works across all state management domains

No one has formally verified a hardware computing primitive to this degree and put it on real silicon. Patent pending.

### Who writes code, or does other technical work on your product? Was any of it done by a non-founder?

I wrote everything. Every Lean4 proof (108), every line of SystemVerilog RTL, every line of SDK code, every test. 80/80 hardware tests, 353 SDK tests, 5 languages. No contractors, no co-founder, no AI-generated RTL. Total cost: $225 on a consumer laptop.

### How long have each of you been working on this? How much of that has been full-time?

~6 months, full-time. From first Lean4 proof to production SoC running on real FPGA hardware with a 5-language SDK and patent application filed.

### How far along are you?

**Prototype hardware and proof artifacts.**

- 108 formal proofs (Lean4, machine-verified)
- Production SoC on Tang Nano 9K FPGA ($13.50) — 287 LUTs, 1.8 mW
- v3 SoC: custom RV64I CPU + delta-state engine + HDMI output
- 1,056 Mops/s throughput (16 parallel banks, validated)
- 120x–30,720x memory traffic reduction (benchmarked)
- 80/80 hardware tests, 353 SDK tests passing
- 5-language SDK (Python, Rust, C, JavaScript, Verilog)
- Patent pending
- 2 papers published (Zenodo), 1 under peer review at Scientific Reports (Springer Nature)
- Zynq port (ALINX AX7020) in progress — ARM+FPGA SoC integration
- 3-node VC demo complete

### How many active users or customers do you have? How many are paying? What is your monthly revenue?

Pre-revenue. Technology is built; now converting entity to Delaware C-Corp and building licensing pipeline. Warm investor interest at the seed stage. Open-source repo serves as proof of capability and lead generation for commercial licenses.

### If you have revenue, what was your revenue for the last full calendar month?

$0. Pre-revenue — IP licensing deals are in the $50K–$500K range and require a corporate entity, which we're establishing now.

### How will you make money?

IP licensing (ARM model):
1. **RTL core licenses** — chip designers integrate delta-state blocks into their SoCs ($50K–$500K/license + royalties)
2. **Vertical accelerator modules** — pre-built for HFT, IoT, video, database ($100K–$1M)
3. **SDK subscriptions** — schema-driven code generation for 5+ languages ($5K–$50K/yr)
4. **Royalties** — per-unit on silicon containing our IP (ARM charges $0.01–$2/chip)

Semiconductor IP licensing is a proven model. ARM: $2.7B revenue, ~95% gross margin. Synopsys/Cadence IP divisions: $1B+ each. We're bringing formal verification to a market that desperately needs it.

### If you've applied previously, what's changed?

First application.

### Why did you pick this particular idea to work on?

I could see the entire path from mathematical proof to silicon, and I could build it alone for $225. Most semiconductor ideas require a team of 20 and $10M. This one required one person who understood both formal methods and hardware design.

The timing is right: edge AI is exploding (Jetson, NPUs in every phone, RISC-V custom silicon), and every one of those chips needs state management. But no one is shipping formally verified IP. The semiconductor industry loses billions annually to hardware bugs that formal verification would have caught. We're the first to prove a computing primitive correct *before* it ships.

### What do you understand about your business that other companies in it just don't get?

Formal verification isn't overhead — it's the product. Other hardware companies test their designs with simulation (and still ship bugs). We prove ours correct with 108 machine-verified theorems. That's a moat that scales with mathematical complexity, not headcount or capital.

The semiconductor industry is moving toward custom silicon (Apple, Google, Amazon all designing their own chips). Every one of them needs verified IP blocks they can trust. We're building the verified primitive layer — the atoms of state computation.

### Who would use your product?

1. **Chip designers** at companies building custom SoCs (Apple, Qualcomm, MediaTek, startups)
2. **HFT firms** needing single-cycle tick processing with instant rollback
3. **Edge AI companies** doing inference at microcontroller power budgets (1.8 mW)
4. **Database companies** wanting O(1) state reconstruction instead of O(N) replay
5. **IoT platforms** doing multi-sensor fusion with lock-free concurrency

### How do or will you get users?

1. **Open-source first**: Apache 2.0 repo is live. Engineers evaluate, then companies license for production.
2. **Conference circuit**: Hot Chips, DAC, FCCM — where chip designers go to find IP.
3. **Published research**: 2 papers on Zenodo, 1 under review at Springer Nature. Academic credibility drives industry adoption.
4. **YC network**: Direct intros to hardware teams at YC companies building custom silicon.
5. **Pilot programs**: Free evaluation licenses → paid production licenses with royalties.

### What's your burn rate?

Effectively $0 in company costs. $225 total spend to date. Personal living expenses are the only burn.

### How long can you go before funding runs out?

Need funding now. The technology is built and proven. The founder needs runway to incorporate, pursue licenses, and prepare for ASIC tape-out.

### If you are applying with the same idea as a previous batch, did anything change?

N/A — first application.

### Anything else you want us to know?

**On being a solo founder:** I know YC prefers teams. Here's my case: I built a semiconductor IP company — 108 formal proofs, working silicon, production SoC, 5-language SDK, patent pending, 2 published papers — for $225 on a consumer laptop. Solo. In 6 months.

That's not a risk factor. That's evidence I can execute at a pace that makes teams jealous. And I'm not trying to stay solo — seed funding goes toward hiring a verification engineer and an applications engineer. The hard part (proving the math and building the first silicon) is done. Now I need people to help scale it.

YC would be investing in the most capital-efficient hardware startup you've ever seen, with working product, formal proofs, and a patent — not a pitch deck.
