# ATOMiK Demo Recording Script

## Overview

**Duration targets:**
- 3-minute highlight reel (LinkedIn/Twitter)
- 10-minute full demo (investor meetings)

**Software**: OBS Studio for screen capture + voiceover

---

## 3-Minute Highlight Reel

### 0:00–0:20 — Hook

**Visual**: Dashboard starting up, 3 node cards appearing.

**Voiceover**: "What if you could process 1 billion state updates per second on a $10 chip? Not in simulation — on real hardware, with mathematical proofs guaranteeing correctness. This is ATOMiK."

### 0:20–0:50 — Problem + Solution

**Visual**: Slide 2 (Problem) then Slide 3 (Solution) from pitch deck.

**Voiceover**: "Every system managing state — databases, trading engines, sensor networks — wastes resources copying full state on every update. ATOMiK replaces full-state copies with XOR delta accumulation. One operation. One clock cycle. 10.6 nanoseconds."

### 0:50–1:30 — Live Demo: Scaling

**Visual**: TUI dashboard running Act 3 (Parallel Scaling).

**Voiceover**: "Three FPGA nodes, each with a different number of parallel banks. Same workload. Watch the throughput scale linearly. 4 banks: 324 million operations per second. 8 banks: 540 million. 16 banks: over 1 billion. On a $10 chip. Linear scaling, no diminishing returns."

### 1:30–2:10 — Live Demo: Distributed Merge

**Visual**: TUI dashboard running Act 5 (Distributed Merge).

**Voiceover**: "Grand finale. Three nodes each process a different subset of data. The host merges the results with a single XOR. The answer is identical to processing everything on one node. This proves lock-free distributed computing — no locks, no consensus, no coordination overhead. Just mathematics."

### 2:10–2:40 — Technical Depth

**Visual**: Slides showing 92 proofs, SDK pipeline, architecture diagram.

**Voiceover**: "92 mathematical proofs formally verified in Lean4. A schema-driven SDK generating code in 5 languages with 242 automated tests. 80 hardware tests passing. This isn't a prototype — it's a complete, validated computing architecture."

### 2:40–3:00 — Call to Action

**Visual**: Contact slide.

**Voiceover**: "ATOMiK. Delta-state computing in silicon. Patent pending. Ready for the next phase."

---

## 10-Minute Full Demo

### 0:00–0:30 — Hook

Same as highlight reel opening. Establish the "1 billion ops on $10" hook.

### 0:30–2:00 — Problem/Solution (Slides 2-4)

**Visual**: Pitch deck slides 2, 3, 4.

**Voiceover**: Walk through the state management problem in detail. Explain XOR delta algebra. Show the Traditional vs ATOMiK comparison diagram.

- "Traditional systems copy 64 bytes to change 1 bit."
- "ATOMiK stores only the change — the delta."
- "XOR is the perfect operator: single-cycle, no carry chains, naturally parallel, self-inverse."

### 2:00–3:30 — Mathematical Foundation (Slide 5)

**Visual**: Slide 5 (Mathematical Foundation) + Lean4 proof snippets.

**Voiceover**: "This isn't just engineering intuition. We have 92 theorems formally verified in Lean4 — the same proof system used for the Fields Medal. Closure, associativity, commutativity, identity, self-inverse. These properties guarantee that the algebra works regardless of execution order, parallelism, or scale."

### 3:30–5:00 — Live Demo: Acts 1-2

**Visual**: TUI running Acts 1 and 2.

**Voiceover (Act 1)**: "Let's see it in action. Load an initial state, accumulate deltas, read the result. Every operation: one clock cycle. The roundtrip is perfect — every bit preserved."

**Voiceover (Act 2)**: "Self-inverse: apply a delta, the state changes. Apply the same delta again — the state returns to the original. Instant undo. No checkpoints. No transaction logs. No rollback journals."

### 5:00–6:30 — Live Demo: Act 3 (Scaling)

**Visual**: TUI running Act 3, throughput chart filling in.

**Voiceover**: "Now the scaling demo. Same workload on all three nodes. 4 parallel banks: 324 million ops. 8 banks: 540 million. 16 banks: breaks 1 billion operations per second. Linear scaling — throughput equals N times frequency. And this is on a $10 FPGA using only 20% of its logic."

### 6:30–7:30 — Live Demo: Act 4 (Domain Applications)

**Visual**: TUI running Act 4.

**Voiceover**: "The same algebra powers real applications. Node 1 processes a finance tick stream — 5 price updates, then instantly undoes the last one. Node 2 fuses three sensor streams — IMU, gyroscope, GPS — order doesn't matter. Node 3 runs a 200-delta burst at peak throughput."

### 7:30–8:30 — Live Demo: Act 5 (Distributed Merge)

**Visual**: TUI running Act 5, merge result appearing.

**Voiceover**: "Grand finale. We partition 6 deltas across 3 nodes. Each processes its subset independently. The host XOR-merges the three partial results. Then we verify: a single node processing all 6 deltas produces the exact same answer. Commutativity proven. This is the foundation for distributed state management without locks, consensus, or coordination."

### 8:30–9:15 — Technical Depth (Slides 8-10)

**Visual**: Architecture diagram, SDK pipeline, market applications.

**Voiceover**: "The architecture: N parallel XOR banks with a binary merge tree. Zero carry chains. The SDK: one JSON schema generates implementations in Python, Rust, C, JavaScript, and Verilog — 19 files per schema, 242 tests. Applications span finance, IoT, video, databases, digital twins, and gaming."

### 9:15–10:00 — Business + CTA (Slides 12-15)

**Visual**: Competitive moat, business model, team, contact.

**Voiceover**: "Patent pending. 92 formal proofs that competitors can't hand-wave. Hardware-validated silicon, not just simulation. A complete platform — math, hardware, SDK, pipeline, 6 phases delivered. We're seeking seed funding to take this from proof-of-concept to product."

---

## B-Roll Shot List

1. **Board close-up**: 3x Tang Nano 9K boards with power LEDs lit
2. **UART activity**: Status LEDs blinking during demo
3. **TUI dashboard**: Full-screen terminal running all 5 acts
4. **Web dashboard**: Browser view with Chart.js throughput visualization
5. **Code scrolling**: Lean4 proof files, Verilog RTL, Python SDK
6. **Slide deck**: Key slides (problem, scaling chart, architecture)

## Camera Notes

- Screen capture at 1920x1080, 30fps
- Dark room for board close-ups (LEDs visible)
- Use OBS Studio scene transitions between slides and live demo
- Record voiceover separately for clean audio, mix in post
