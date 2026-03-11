# Y Combinator Summer 2026 Application — ATOMiK

## Company

**Company name:** Rockwell Industries (converting to Delaware C-Corp)
**Company URL:** github.com/MatthewHRockwell/ATOMiK
**If you have a demo, link here:** [3-node FPGA demo video — to be recorded]

## Founders

**Founder:** Matthew H. Rockwell
**Email:** matthew.h.rockwell@gmail.com
**Role:** Solo founder — built everything

---

## Application Questions

### Describe what your company does in 50 characters or less.

Delta-state computation in silicon. Hardware IP.

### What is your company going to make?

Hardware IP cores that replace full-state computation with XOR-based delta accumulation. One operation, one clock cycle, mathematically proven correct. We license RTL to chip designers (ARM model).

### Why did you pick this idea to work on? Do you have domain expertise in this area? How do you know people need what you're making?

I was working on state synchronization problems and realized the entire stack — from databases to distributed systems to hardware — copies full state on every update. That's insane. XOR gives you the same result in one cycle with zero carry chains.

I formalized the math in Lean4 (92 proofs), then built it in silicon to prove it's not just theory. Domain expertise: I designed the RTL, proved the math, wrote the SDK, and deployed the SoC. All of it.

People need this because state management is the bottleneck in HFT (microseconds matter), IoT (power matters), and databases (replay cost matters). I've validated interest with investors in the semiconductor space.

### What's new about what you're making? What substitutes do people resort to today?

**Today's substitutes:**
- Event sourcing (Kafka): O(N) replay to reconstruct state. Breaks at scale.
- CRDTs: Software-only, microsecond latency, custom merge per data type.
- Traditional FPGA accelerators: Months of custom RTL per application, no formal proofs.

**What's new:**
- O(1) state reconstruction (single XOR, 10.6 ns)
- 92 machine-verified proofs — not tested, *proven*
- A single reusable primitive that works across all state management domains
- Hardware-accelerated, not software

No one has formally verified a hardware computing primitive to this degree and put it on real silicon.

### Who writes code, or does other technical work on your product? Was any of it done by a non-founder?

I wrote everything. Every line of Lean4 proof, every line of SystemVerilog RTL, every line of SDK code, every test. 92 proofs, 80/80 hardware tests, 353 SDK tests, 5 languages. No contractors, no co-founder, no AI-generated RTL. Total cost: $225.

### How long have each of you been working on this? How much of that has been full-time?

~6 months, full-time. From first Lean4 proof to production SoC deployment on real FPGA hardware.

### How far along are you?

**Production.** Not prototyping — deployed.

- 92 formal proofs (Lean4, machine-verified)
- Production SoC running on Tang Nano 9K FPGA ($13.50)
- v3 SoC with custom RV64I CPU + HDMI output
- 1 Gops/s throughput (16 parallel banks, validated)
- 80/80 hardware tests, 353 SDK tests
- 5-language SDK (Python, Rust, C, JavaScript, Verilog)
- Patent pending
- 3-node VC demo complete

### How many active users or customers do you have? How many are paying? What is your monthly revenue?

Pre-revenue. Building IP licensing pipeline. Have warm investor interest (seed-stage). No paying customers yet — incorporating entity now, then pursuing commercial pilots.

### If you have revenue, what was your revenue for the last full calendar month?

$0. Pre-revenue.

### How will you make money?

IP licensing (ARM model):
1. **RTL core licenses** — chip designers integrate our delta-state blocks
2. **Vertical accelerator modules** — pre-built for HFT, IoT, video, database
3. **SDK subscriptions** — schema-driven code generation for 5+ languages
4. **Professional services** — custom integration for enterprise customers

Semiconductor IP licensing is a proven model (ARM: $2.7B revenue, ~95% gross margin). We're the ARM of state management.

### If you've applied previously, what's changed?

First application.

### Why did you pick this particular idea to work on?

State management is the silent bottleneck in computing. Every database, every distributed system, every real-time application copies full state on every update. XOR-based delta accumulation eliminates this — mathematically, provably, in hardware.

I picked this because I could see the entire path from proof to silicon, and I could build it alone. Most semiconductor ideas require a team and millions. This one required math and a $13.50 FPGA.

### What do you understand about your business that other companies in it just don't get?

Formal verification isn't overhead — it's the product. Other hardware companies test their designs. We prove ours. 92 Lean4 theorems create a moat that scales with mathematical complexity, not capital. You can't throw money at replicating machine-verified proofs; you need the expertise and the time.

Also: the semiconductor industry is desperate for formally verified IP blocks. Every hardware bug that makes it to silicon costs millions. We eliminate entire classes of bugs by construction.

### Who would use your product?

1. **Chip designers** integrating state management into SoCs (our core customer)
2. **HFT firms** needing single-cycle tick processing with instant rollback
3. **IoT/edge companies** doing multi-sensor fusion at microcontroller power budgets
4. **Database companies** wanting O(1) state reconstruction instead of O(N) replay
5. **Game studios** doing order-independent multiplayer state sync

### How do or will you get users?

Phase 1: Direct outreach to semiconductor design houses and FPGA integrators using the open-source repo as proof of capability. Apache 2.0 for evaluation, commercial license for production.

Phase 2: Conference presence (Hot Chips, DAC, FCCM) and technical blog posts targeting hardware engineers.

Phase 3: YC network + accelerator demo days for enterprise introductions.

### What's your burn rate?

Effectively $0 in company costs. Personal living expenses only. I built everything on a consumer laptop and $13.50 FPGAs.

### How long can you go before funding runs out?

Need funding now for living expenses and ASIC tape-out work. The technology is built; the founder needs to eat.

### If you are applying with the same idea as a previous batch, did anything change?

N/A — first application.

### Anything else you want us to know?

I built a semiconductor IP company — formal proofs, working silicon, production SoC, 5-language SDK, patent pending — for $225 total. On a consumer laptop. By myself.

That's either insane or the most capital-efficient hardware startup you've seen. I think it's both.
