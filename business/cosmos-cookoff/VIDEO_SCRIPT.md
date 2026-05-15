# ATOMiK Cosmos Cookoff Demo Video Script

> **Publication status: ARCHIVED SUBMISSION SCRIPT / DO NOT RECORD AS CURRENT COPY.**
> This script contains old performance, timing, and physical-AI claims. Replace
> them with evidence-labeled current copy before any public use.

**Target length:** 2:30 - 2:50 (under 3 min limit)
**Format:** Screen recording + FPGA board camera + voice narration

---

## Scene 1: Hook (0:00 - 0:15)

**Visual:** HDMI monitor showing ATOMiK test card on Tang Nano 9K, camera shot of the $13.50 FPGA board

**Narration:**
"Physical AI systems need to reason about state changes in the real world. But how do you verify those state changes are correct — in hardware, in real-time, for thirteen dollars and fifty cents? This is ATOMiK."

---

## Scene 2: The Problem (0:15 - 0:35)

**Visual:** Diagram showing traditional state tracking (full memory copies, software checksums) vs ATOMiK (XOR accumulator, delta reconstruction)

**Narration:**
"Traditional state verification copies entire memory buffers and runs software checksums. It's slow, it's variable-latency, and it leaks timing information. ATOMiK takes a completely different approach — state is never stored, only reconstructed. Current state equals initial state XOR'd with a running accumulator of deltas. Order doesn't matter. Latency is deterministic. And we've proven the math — 108 theorems in Lean4."

---

## Scene 3: Live Hardware Demo (0:35 - 1:30)

**Visual:** Terminal session showing UART interaction with the FPGA, HDMI output visible on monitor

**Narration + Demo Flow:**

### 3a: Boot and test card (0:35 - 0:50)
"Here's ATOMiK running on a Tang Nano 9K — a Gowin GW1NR-9 FPGA that costs thirteen fifty retail. The CPU is a custom RV64I core running at 21.6 megahertz. The HDMI output runs on a separate 25.2 megahertz pixel clock — standard 640x480 at 60 hertz."

**Show:** Board plugged in, HDMI test card on monitor, terminal connected

### 3b: ATOMiK operations (0:50 - 1:10)
"Let's run the hardware test suite."

**Show:** Run test commands via UART:
- `X` — ATOMiK self-test (9/9 PASS)
- `P` — Phase 2 regression (10/10 PASS)
- `V` — Display pipeline verification (6/6 PASS)

"Nine out of nine ATOMiK tests pass. The delta accumulator, state reconstruction, and change detection all verified in hardware. The display pipeline runs its own verification — six out of six, showing the delta-driven display is working through the clock domain crossing bridge."

### 3c: Performance numbers (1:10 - 1:30)
"Here are the real numbers from hardware."

**Show:** Performance benchmark output (`R` command), highlight key metrics:
- Change detection: 76-80% faster than software
- Memory traffic reduction: 7,670x to 916,000x
- Deterministic latency: stdev <= 0.5 cycles

"Change detection is 76 to 80 percent faster than software byte comparison. Memory traffic is reduced by up to 916,000 times. And every operation completes in exactly the same number of cycles — standard deviation under half a cycle. No timing side channels."

---

## Scene 4: Cosmos Reason 2 Connection (1:30 - 2:10)

**Visual:** Architecture diagram showing Cosmos Reason 2 + ATOMiK integration

**Narration:**
"So where does Cosmos Reason 2 fit? Cosmos is the brain — it observes the physical world and reasons about what's happening. ATOMiK is the verification layer — it mathematically proves the state transitions are consistent."

**Show:** Diagram:
```
Sensors/Cameras → [Cosmos Reason 2] → "State X changed"
                         ↓
                  [ATOMiK Hardware] → "Verified: delta consistent"
                         ↓
                  [Actuator Command] → "Safe to act"
```

"In a physical AI system — a robot arm, an autonomous vehicle, an industrial controller — Cosmos reasons about what it sees. ATOMiK provides ground truth. A single register read tells you whether any state has changed. The XOR delta tells you exactly which bits. And the algebra guarantees correctness regardless of sensor arrival order."

"This matters because physical AI can't afford to act on corrupted state. ATOMiK gives you hardware-speed verification with mathematical proof — on a thirteen dollar FPGA."

---

## Scene 5: Results Summary + Close (2:10 - 2:45)

**Visual:** Results table, repo URL, board shot

**Narration:**
"To summarize: ATOMiK runs on a thirteen dollar fifty FPGA. It provides 76 to 80 percent faster change detection than software. Memory traffic reduction up to 916,000x. Deterministic latency with no timing side channels. 108 Lean4 theorems proving the math. A custom RV64I CPU with ATOMiK fused directly into the instruction set. And a complete SDK generating verified code in five languages."

**Show:** GitHub repo URL: `github.com/MatthewHRockwell/ATOMiK`

"ATOMiK — hardware-verified state reasoning for physical AI. The code, the proofs, and the hardware are all open source."

---

## Production Notes

### Recording Setup
1. **Screen capture**: OBS or similar, 1920x1080, terminal + HDMI capture side-by-side
2. **Camera**: Phone camera on board showing LEDs, HDMI cable, size reference
3. **Audio**: Clear voice narration, no background music needed
4. **Terminal font**: Large, high-contrast (white on black, 16pt+)

### Before Recording
1. Fix UART baud rate (V3-022) so terminal output is legible
2. Run full test suite once to confirm all PASS
3. Have architecture diagrams ready (can use draw.io or slides)
4. Test HDMI capture device if using one (otherwise point camera at monitor)

### Key Visuals to Prepare
1. ATOMiK architecture diagram (delta-state algebra visual)
2. Cosmos Reason 2 + ATOMiK integration diagram
3. Performance comparison bar chart (ATOMiK vs software)
4. Board photo with size reference (coin or ruler)

### Editing
- Hard cut between scenes (no fancy transitions needed)
- Text overlays for key numbers (76-80% faster, 916,000x reduction, $13.50)
- Terminal output should be readable — zoom if needed
