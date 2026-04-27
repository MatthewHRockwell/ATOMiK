# ATOMiK Demo Build Brief
## Principal Systems Architect / Demo Director Implementation Plan

---

# SECTION 1 — DEMO THESIS

**Investor takeaway:** ATOMiK eliminates the fundamental waste in how every computing system discovers and propagates state change — and it does this in hardware that developers can target with the compilers they already use.

**Product takeaway:** ATOMiK is licensable compute IP that sits under agent memory, replicated state, edge sync, and continuous monitoring. It reduces compute, energy, and bandwidth by knowing what changed instead of rediscovering it. The product is the IP. The board is the proof.

**Technical takeaway:** ATOMiK's delta-state algebra (formally verified, 108 Lean4 theorems) enables O(1) change detection, selective propagation, and dynamic execution shaping — where the hardware concentrates compute on active state regions and ignores idle ones. This is not a cache. It is not a filter. It is a new execution primitive.

**One-sentence demo promise:**

> "Press this button, watch the system respond, see how much work disappears, and see that the code is still ordinary C built with GCC."

---

# SECTION 2 — TOP-LEVEL DEMO CONCEPT

## The ATOMiK Pulse Instrument

The demo is a tabletop instrument. The board is visible, mounted, and lit. The investor interacts directly.

### What the investor physically does:

1. **Presses a key or button** to inject a state change event (modify specific buffers)
2. **Watches three surfaces react simultaneously** — HDMI hero, browser control plane, LCD endpoint
3. **Sees the binary algebra** — actual 1s and 0s showing initial XOR delta = current
4. **Sees the waste contrast** — software scans everything (orange, full), ATOMiK acts on change only (blue, sparse)
5. **Triggers the compiler lane** (press `d`) — sees standard C + GCC = ATOMiK hardware instructions
6. **Triggers the adoption forecast** (press `f`) — sees year-by-year market capture
7. **Triggers burst mode** (press `b`) — watches 3 seconds of rapid state churn, ATOMiK stays sparse
8. **Triggers corruption detection** (press `c`) — watches ATOMiK catch a single tampered byte

### What changes on the board:

The ATOMiK hardware adapter at 0xF0020000 executes real LOAD, ACCUM, READ operations. The NaxRiscv RV64GC CPU drives the adapter via MMIO. The fingerprints are real. The detection is real. Nothing is mocked.

### What changes on HDMI:

**Split-screen head-to-head.** Left half = SOFTWARE (all orange, scans everything). Right half = ATOMiK (only changed buffers lit blue, rest dark). Below the comparison: live binary algebra showing the XOR operation on real data. Below that: metrics panel with percentage avoided, dollar savings at two scales ($K/yr at 1K servers, $B/yr global TAM), speedup factor, flow bars.

### What changes on LCD:

Replica endpoint showing abbreviated buffer strip (8 colored blocks with names), data avoided percentage, and "Only deltas propagated."

### What changes in browser:

ATOMiK Control Plane — three-panel dashboard. Left: state map with per-buffer status, change counts, dot indicators. Center: hero cards (data avoided %, synced count, cycles), delta flow bar, event log. Right: economics (compute avoided, savings at both scales, speedup, data volume breakdown, system info).

### What changes in compiler lane:

Press `d`: HDMI shows `#include "atomik.h"`, the four core API calls, the GCC build command, and "No new language. No new compiler." — holds 5 seconds. This is the same code that compiled and ran on the live board (atomik_example.c, confirmed working).

### What the audience should feel at each moment:

| Moment | Feeling |
|--------|---------|
| First keypress | "It's real — the board just responded" |
| Split screen comparison | "Software is doing way more work than it needs to" |
| Binary algebra | "I can see the math happening — this isn't magic, it's algebra" |
| Burst mode | "Even under continuous load, ATOMiK stays sparse" |
| Corruption detection | "It caught a single tampered byte — this is security too" |
| Compiler lane | "Wait, this is just C? And GCC? That changes everything." |
| Adoption forecast | "The market size is real. This isn't a science project." |
| Browser updating | "Multiple endpoints, one source of truth — this is infrastructure" |

---

# SECTION 3 — "VIRTUAL PROCESSOR" DEMOIZATION

## The core insight

"Virtual processor personalities" is not a separate feature to build. It is what ATOMiK already does, reframed.

When ATOMiK tracks 8 state buffers and only 3 are active, the system is already doing dynamic execution shaping: compute concentrates on the 3 active regions and skips the 5 idle ones. The "processor personality" changes every cycle based on which buffers are hot.

Traditional systems have a fixed execution pattern: scan all 8, every time, regardless. ATOMiK's pattern is workload-shaped: it adapts per-cycle based on actual state activity.

**This IS the virtual processor.** The visualization just needs to make it visible.

## What makes it legible to an investor:

Show **execution lanes** — 8 vertical lanes on the HDMI, one per buffer. In the SOFTWARE view, all 8 lanes are always active (orange pulses every cycle). In the ATOMiK view, only active lanes pulse (blue). Idle lanes go dark.

When the workload changes (different buffers become active), the ATOMiK lanes reconfigure. The investor sees: "The processor is reshaping itself around what's actually happening."

## Three candidate metaphors:

### 1. Execution Lanes (recommended)
8 vertical lanes. Active lanes pulse with data flowing down. Idle lanes dark. When workload shifts, different lanes light up. This is the most intuitive — it looks like traffic lanes opening and closing based on demand.

**Pro:** Instantly legible. No CS knowledge needed. Maps directly to the buffer model we already have.
**Con:** Could look like a simple filter rather than a processor.

### 2. Pipeline Morphing
A horizontal pipeline that physically reconfigures — stages appear and disappear, bypass paths form around idle buffers. Animated tubes that route around inactive state.

**Pro:** More architecturally accurate. Shows dynamic reconfiguration.
**Con:** Harder to render with our pixel font. Could look messy. Requires more animation code.

### 3. State Cells Regrouping
8 cells that physically move — active cells cluster together, inactive cells drift to the periphery. The "processor footprint" visually shrinks when fewer buffers are active.

**Pro:** Very dramatic visual. Shows the processor literally getting smaller/cheaper.
**Con:** Complex animation. Could look gimmicky. Hard to do well with our rendering primitives.

## Decision: Execution Lanes

Execution Lanes wins because:
1. It maps 1:1 to our existing 8-buffer model
2. It's immediately legible ("lanes opening and closing")
3. It can be implemented with our existing rect() and text() primitives
4. It makes the "virtual processor" claim concrete without overclaiming
5. The contrast between "all lanes always on" (software) and "only active lanes on" (ATOMiK) IS the demo

## How to keep it from sounding like hand-waving:

1. Never say "virtual processor" without immediately showing the behavior
2. Frame it as "workload-shaped execution" — the hardware adapts to what's actually happening
3. Show the binary algebra alongside — the math is the proof
4. Show the compiler lane — the adoption path is the credibility
5. Show the cost savings — the consequence is the business case

## Concrete implementation:

Add an "execution lanes" visualization to the HDMI display. 8 vertical columns, each 200px tall. SOFTWARE side: all 8 columns always pulsing orange. ATOMiK side: only changed columns pulse blue, rest are dark/minimal. The columns show the buffer name at top and a visual activity indicator (filled vs empty bar that changes each cycle).

This replaces or augments the current buffer strip with something more dynamic and visually striking.

---

# SECTION 4 — ADOPTION / GCC COMPATIBILITY LANE

## What exists now (real, working):

- `atomik.h` — dual-backend header (MMIO for Zynq, custom instructions for native CPU)
- `atomik_example.c` — compiles with `riscv64-linux-gnu-gcc -O2`, runs on live hardware, correctly detects buffer changes
- Disassembly shows `.insn` custom instructions (opcode 0x0B) in the RISC-V binary
- The MMIO path is fully functional on the Zynq board

## What the audience sees (press `d`):

Full-screen overlay on HDMI:

```
COMPILER LANE

#include "atomik.h"

atomik_load(slot, initial_state);
atomik_accum(delta);
uint64_t current = atomik_read(slot);

riscv64-linux-gnu-gcc -O2 example.c -o example

→ ATOMiK hardware ops in the binary

No new language.
No new compiler.
```

This holds for 5 seconds then returns to the dashboard.

## What is real vs mocked vs deferred:

| Element | Status |
|---------|--------|
| atomik.h header | REAL — exists, compiles, dual-backend |
| atomik_example.c | REAL — runs on live hardware, confirmed correct output |
| GCC compilation | REAL — standard riscv64-linux-gnu-gcc produces working binary |
| Custom instructions in binary | REAL (native path) / MMIO equivalent (Zynq path) |
| Live execution on board | REAL — tested, output matches expected |
| Full upstream compiler backend | DEFERRED — not yet upstream, uses .insn + extended asm |
| Intrinsics | DEFERRED — could be added but .insn is sufficient |

## How to frame honestly:

Say: "This is standard C, compiled with GCC, targeting ATOMiK through a header and inline assembly. The RISC-V ISA reserves custom opcode space specifically for extensions like this. A first-class compiler backend is on the roadmap, but the adoption path is already real and buildable today."

Do NOT say: "We have full compiler support." That's not yet true.

## What makes it undeniable:

The `atomik_example.c` binary runs ON THE BOARD and produces correct output. That's not a mock. That's not a slide. That's a working program using ATOMiK hardware through a standard compiler workflow.

## Later work that would strengthen it:

1. LLVM/Clang backend with ATOMiK intrinsics
2. First-class `-march=rv64gc_xatomik` extension
3. Auto-vectorization of XOR fingerprint loops into ATOMiK instructions
4. IDE integration (VS Code extension with ATOMiK hints)

---

# SECTION 5 — SYSTEM ARCHITECTURE FOR THE DEMO

## Existing components reused as-is:

| Component | Status |
|-----------|--------|
| RK-ZYNQ7020-F board | Working |
| NaxRiscv RV64GC @ 100 MHz | Working |
| Ubuntu 24.04 initramfs | Working |
| ATOMiK adapter at 0xF0020000 | Working |
| HDMI 1920x1080 framebuffer via HP0 | Working |
| SPI LCD 320x172 (ST7789V) | Working |
| JTAG boot (95s) | Working |
| PS IOP mapping (GP1 → 0xE0000000) | Working |
| font_8x16.h | Working |
| L2 eviction flush | Working |

## Components to modify:

| Component | Change |
|-----------|--------|
| atomik_live.c | Add execution lanes visualization, tighten layout, add new key handlers |
| bridge.py | Add latency tracking, per-buffer change history, connection uptime |
| Browser HTML | Add sparkline chart, animation on state change, responsive improvements |
| LCD update | Widen buffer strip, add abbreviated names (done), add visual rhythm |
| demo_launch.sh | Verify single-command start still works after all changes |

## New components to build:

| Component | Purpose | Effort |
|-----------|---------|--------|
| Execution lanes HDMI viz | "Virtual processor" visualization | 2-4 hours |
| Browser sparkline chart | Time-series data avoided % | 1-2 hours |
| Browser connection latency | Proves liveness | 1 hour |
| Adoption forecast slide (done) | Year-by-year TAM | Done |
| Physical button wiring | When buttons arrive | 2-3 hours |
| Power telemetry integration | Real energy measurement | 4-8 hours (needs hardware) |

## Components that can be deferred:

| Component | Why defer |
|-----------|-----------|
| USB keyboard/mouse | Needs interrupt routing — deep kernel work |
| PS Ethernet | Not on critical path for demo |
| Multi-bank ATOMiK scaling | Strengthens claim but not needed for first meeting |
| LLVM backend | Later — .insn is sufficient now |
| Video recording | Do after visual polish is complete |

---

# SECTION 6 — MILESTONES

## Milestone 1: Demo Story Lock

**Objective:** Finalize the narrative arc, key messages, and screen sequence.

**Why it matters:** Everything else is built around the story. Changing the story after building screens wastes work.

**Inputs:** This build brief, ChatGPT's feedback, Gemini's feedback, user's vision.

**Tasks:**
- Write final one-paragraph demo story
- Lock the key sequence: idle → interact → compare → algebra → compiler → forecast → summary
- Lock the three remembered messages
- Get explicit user sign-off

**Deliverable:** Signed-off story document.

**Proof of completion:** User says "this is the story."

**Risks:** Story changes after screens are built. **Mitigation:** Lock story FIRST, refuse scope changes until first full run-through.

## Milestone 2: Interaction Model Lock

**Objective:** Define exactly how the investor interacts with the system.

**Why it matters:** The interaction model drives the entire UX design.

**Tasks:**
- Decide: keyboard keys vs physical buttons vs hybrid
- Map each key/button to an action
- Define the response time budget (< 200ms from keypress to screen update)
- Define the bridge latency budget (< 500ms from keypress to browser update)

**Deliverable:** Interaction map document + verified response times.

**Proof of completion:** Every key produces the correct response within budget.

**Risks:** Physical buttons not available yet. **Mitigation:** Ship with keyboard-via-browser as primary, add buttons when hardware arrives.

## Milestone 3: HDMI Hero Redesign

**Objective:** Implement the execution lanes visualization and tighten layout.

**Why it matters:** The HDMI is the cinematic surface — it must look premium, not like an engineering console.

**Tasks:**
- Replace buffer strips with execution lane columns
- Add animated activity indicators per lane
- Verify no visual overlaps at all screen states
- Verify all text is legible on a 24"+ monitor
- Test with real data across multiple interaction patterns

**Deliverable:** Updated atomik_live.c with execution lanes rendering.

**Proof of completion:** Screen capture showing clean layout at idle, 2-of-8 changed, all-8 changed, and post-reset states.

**Risks:** Execution lanes look too complex or cluttered. **Mitigation:** Start simple (filled bars), add detail only if it helps clarity.

## Milestone 4: Browser Control Plane Redesign

**Objective:** Make the browser feel like a product, not a dev tool.

**Tasks:**
- Add sparkline chart (data avoided % over last 20 cycles)
- Add connection latency display
- Add per-buffer change history count
- Add connection uptime counter
- Add animation/transition on state changes
- Color-code event log entries by type
- Verify responsive layout on tablet-size screens

**Deliverable:** Updated bridge.py with production browser UI.

**Proof of completion:** Screenshot of browser showing all new elements with live data.

**Risks:** Over-engineering the browser. **Mitigation:** Keep it calm and operational. No animations that compete with HDMI.

## Milestone 5: LCD Endpoint Redesign

**Objective:** Make LCD feel like a real edge endpoint, not a debug display.

**Tasks:**
- Add visual pulse on buffer change
- Optimize SPI update for changed blocks only
- Show data avoided % prominently

**Deliverable:** Updated LCD rendering in atomik_live.c.

**Proof of completion:** Photo of LCD showing clean, readable output.

**Risks:** SPI bitbang too slow for animation. **Mitigation:** Keep LCD updates minimal — change only what changed.

## Milestone 6: Compiler Compatibility Lane

**Objective:** Make the adoption story undeniable.

**Tasks:**
- Polish the `d` key overlay text and layout
- Add a brief disassembly highlight (show the .insn opcode)
- Verify atomik_example.c still runs correctly
- Prepare a side-by-side: "standard C" → "ATOMiK binary" → "runs on board"

**Deliverable:** Polished compiler lane overlay.

**Proof of completion:** Investor-grade screenshot of compiler lane screen.

**Risks:** Disassembly is too technical. **Mitigation:** Show only 2-3 lines, huge font, with annotation arrows.

## Milestone 7: Virtual Processor Visualization

**Objective:** Make "workload-shaped execution" visible.

**Tasks:**
- Implement execution lanes on the ATOMiK side of the split screen
- Show lanes activating/deactivating as different buffers change
- Add subtle animation (pulse or fill) on active lanes
- Test with various interaction patterns

**Deliverable:** Execution lanes rendering integrated into draw_content().

**Proof of completion:** Video or screen captures showing lanes reconfiguring across 3 different workload patterns.

**Risks:** Looks too abstract. **Mitigation:** Tie each lane to a named buffer — the concreteness of "agent.ctx" or "model.wt" grounds the visualization.

## Milestone 8: Operator Hardening

**Objective:** One-command start, clean recovery, zero operator anxiety.

**Tasks:**
- Verify demo_launch.sh works end-to-end
- Add --quick mode for restarting without reboot
- Test reset (r key) at every screen state
- Test recovery from bridge disconnect
- Prepare fallback: pre-recorded video of the demo
- Write operator checklist

**Deliverable:** Tested launch script + checklist.

**Proof of completion:** Cold boot to fully running demo in under 3 minutes with zero manual intervention.

## Milestone 9: Investor-Ready Rehearsal Package

**Objective:** Everything needed to walk into a meeting.

**Tasks:**
- Record HDMI capture of full demo sequence
- Record camera shot of physical board setup
- Print one-page leave-behind
- Rehearse 90-second script 5 times
- Prepare 3-minute extended version
- Verify fallback video is ready
- Pack spare cables and spare SD card

**Deliverable:** Complete meeting kit.

**Proof of completion:** Successful dry run in front of at least one non-technical person who can retell the story afterward.

---

# SECTION 7 — CRITICAL PATH

## Execution order:

1. **Story lock** (30 min) — must happen first, everything else depends on it
2. **HDMI hero redesign** (4-6 hours) — the hero surface drives the demo impact
3. **Execution lanes** (2-4 hours) — builds on HDMI redesign, makes "virtual processor" visible
4. **Browser polish** (2-3 hours) — can happen in parallel with HDMI
5. **Compiler lane polish** (1-2 hours) — small effort, high impact
6. **LCD tightening** (1 hour) — small effort
7. **Operator hardening** (2 hours) — must happen before rehearsal
8. **Recording** (2-3 hours) — must happen last, after everything is stable
9. **Rehearsal** (2 hours) — absolute last step

## What should wait:
- Physical buttons (blocked on hardware procurement)
- Power telemetry (blocked on analyzer purchase)
- USB keyboard (blocked on interrupt routing — deep kernel work)

## What is optional polish:
- Sound cues
- LED light bars
- Custom enclosure
- Mobile-responsive browser

## What is dangerous scope creep:
- Trying to build a real "virtual processor" in RTL
- Adding inference/ML capabilities to the live demo
- Building a multi-board networked demo
- Implementing a custom compiler backend

## Single most likely stall:
**Visual overlap bugs on the HDMI.** Every time we change the layout, something covers something else. The fix: define a pixel-level layout map BEFORE coding, verify in a spreadsheet, then implement. Don't eyeball coordinates.

---

# SECTION 8 — SCREEN-BY-SCREEN DESIGN

## Screen 0: Idle / Attract

**Purpose:** Board is on, waiting for interaction.
**Key visual:** Dark background, subtle ATOMiK wordmark pulsing slowly, "LIVE ON HARDWARE" badge.
**Text:** "Press any key to begin" centered, dim.
**Animates:** Wordmark subtle pulse (brightness oscillation).
**Data:** None.
**Never clutter:** No metrics, no numbers, no buffers. Just the brand and the invitation.

## Screen 1: Interactive Dashboard (default)

**Purpose:** The main working state. Head-to-head comparison.
**Key visual:** Split screen — SOFTWARE (left, orange) vs ATOMiK (right, blue). Execution lanes on ATOMiK side. Binary algebra below.
**Text:** Buffer names, SCAN/SYNC/SKIP labels, metrics.
**Animates:** Lanes activate/deactivate on each keypress. Binary digits change. Metrics update.
**Data:** All live from ATOMiK hardware.
**Never clutter:** No more than 3 lines of event log visible. No hex dumps. No register addresses.

## Screen 2: Burst Mode (press `b`)

**Purpose:** Show ATOMiK under sustained load.
**Key visual:** Same split screen but updating rapidly. History ribbon filling up. Execution lanes flickering.
**Text:** "BURST: rapid state changes" banner.
**Animates:** 3 seconds of rapid updates, 5 per second.
**Data:** Live, randomized buffer mutations.
**Never clutter:** Don't add more metrics during burst — the visual rhythm IS the message.

## Screen 3: Corruption Detection (press `c`)

**Purpose:** Show tamper detection / integrity verification.
**Key visual:** One buffer flashes red on the ATOMiK side. "TAMPERED" label. Then "DETECTED" in green.
**Text:** "1 byte corrupted → detected in O(1)" — this is the security story.
**Animates:** Brief red flash, then green confirmation.
**Data:** Real corruption injected and detected via ATOMiK fingerprint.
**Never clutter:** This should feel like an alert, not a dashboard.

## Screen 4: Compiler Lane (press `d`)

**Purpose:** Adoption credibility.
**Key visual:** Dark background, large monospace code in center. `#include "atomik.h"`, API calls, build command.
**Text:** "Same C. Standard GCC. No new language. No new compiler."
**Animates:** Nothing — this should feel still and confident, like reading a blueprint.
**Data:** Static (the code is the data).
**Never clutter:** No metrics, no graphs. Just code and the message.

## Screen 5: Adoption Forecast (press `f`)

**Purpose:** Market size and revenue projection.
**Key visual:** Year-by-year table with visual bars growing rightward.
**Text:** 2027-2033 rows with adoption, rev/server, TAM captured, cumulative.
**Animates:** Nothing — table is static, bars provide visual weight.
**Data:** Projected (clearly labeled as forecast).
**Never clutter:** No engineering metrics. This is a business screen.

## Screen 6: Summary / Freeze Frame

**Purpose:** Memory anchor — what the investor remembers.
**Key visual:** ATOMiK wordmark large, three proof chips (% less data, flat query cost, live on hardware), buyer sentence.
**Text:** "Hardware that knows what changed. We help state-heavy systems move less data and react faster. The board is the proof. The product is the IP."
**Animates:** Nothing. This is the final frame. Hold indefinitely.
**Data:** Final cumulative metrics from the session.
**Never clutter:** Maximum 5 lines of text on this screen. Every word earns its place.

---

# SECTION 9 — BROWSER REPLICA PRODUCTIZATION

## Product identity:

**Name:** ATOMiK Control Plane

**It should intentionally differ from HDMI.** The HDMI is cinematic theater. The browser is operational confidence. Think: what a customer would actually log into to manage their ATOMiK-enabled infrastructure.

## Layout (already implemented, minor refinements needed):

**Top bar:** ATOMiK logo, "Control Plane" title, LIVE/DISCONNECTED status pill, EDGE-01 node badge.

**Left panel (280px):** State Map. 8 buffer rows with dot indicator, name, size, delta status, change count per buffer.

**Center panel (flexible):** Hero cards (Data Avoided %, Synced Count, Cycles), delta propagation flow bar, event log with timestamps and color-coded entries.

**Right panel (300px):** Economics (compute avoided, savings at both scales, speedup), data volume (SW/HW/avoided KB), system info (node, replica, link, board, CPU, clock, build hash).

**Bottom bar:** Key hints, "Only changed state crosses the wire."

## What should be interactive:

- Keyboard forwarding (1-8, a, r, b, c, d, f, v, q) — already working
- Clicking a buffer row could highlight it on HDMI (future)

## What should be passive:

- All metrics update automatically from event stream
- Event log scrolls automatically
- Status pill changes color on connect/disconnect

## What should feel customer-facing:

- The typography, spacing, and color palette
- The event log format ("agent.ctx updated", "delta apply: 31% of software path")
- The economics section (this is what the CFO sees)
- The system info section (this is what the ops team sees)

## How the browser helps investors understand the company:

The browser IS the product surface. When the investor sees the HDMI, they think "cool demo." When they see the browser, they think "I can imagine logging into this. I can imagine my team using this. This is a real product."

---

# SECTION 10 — PHYSICAL BOARD PRESENCE

## Minimum viable:

Board sits on a clean dark surface (black mousepad or felt). Cables routed behind. HDMI to a 24"+ monitor. Laptop open with browser. LCD visible on the board. Presenter types in browser window.

**Cost:** $0 (use what exists).

## Premium version:

### Mounting:
- Matte black acrylic base (laser-cut or 3D printed), 12" x 8"
- Board mounted with standoffs, visible but organized
- All cables route through a slot in the back of the base
- LCD positioned at a slight angle toward the viewer

### Interaction:
- 3 illuminated arcade buttons (when available) mounted in the base front edge:
  - Blue: INJECT CHANGE
  - Orange: SOFTWARE / ATOMiK toggle (or burst mode)
  - Green: BUILD & RUN (compiler lane)
- Connected to 40-pin header via simple GPIO wiring

### Light columns (when available):
- Two 8" diffused RGB LED strips mounted vertically behind the board
- Left strip: orange (software activity)
- Right strip: blue (ATOMiK activity)
- Driven by a small MCU (Arduino Nano) reading UART events from the board

### Power telemetry (when available):
- Joulescope JS220 inline on 12V rail
- Small monitor or tablet showing real-time power draw
- This is the evidence that makes the energy claim undeniable

### Display:
- 24-27" monitor for HDMI (clean stand, no visible brand)
- 10" tablet for browser control plane (on a small stand next to the board)
- Board LCD visible between them

**Estimated cost:** $1,500-2,500 for the full premium rig.
**Estimated build time:** 1-2 weekends for enclosure + wiring.

## Recommendation:

Start with minimum viable. The demo content matters more than the enclosure. Add premium elements incrementally as the story stabilizes. The last thing to cut is power telemetry. The first thing to cut is fancy audio.

---

# SECTION 11 — 90-SECOND INVESTOR SCRIPT

> **0-8s:**
> "Every computing system manages state. Databases, agents, replicas, edge devices — they all constantly check what changed and move data to keep things in sync."

> **8-15s:**
> "The problem is waste. Software checks everything, every time, even when almost nothing changed."
> *(Gesture to HDMI: SOFTWARE side, all orange)*

> **15-25s:**
> "ATOMiK is a hardware primitive that knows what changed without rediscovering it. Watch."
> *(Press key — ATOMiK side lights up, only 2-3 of 8 lanes active)*

> **25-35s:**
> "Same event. Software scanned every byte of every buffer. ATOMiK acted on only what mattered. You can see the actual binary algebra happening — initial state, XOR the delta, equals the current state."
> *(Point to binary grids)*

> **35-45s:**
> "This isn't a canned animation. These buffers are live on the board. The LCD endpoint and the browser control plane are updating from the same real hardware operations."
> *(Gesture to LCD and browser)*

> **45-55s:**
> "And this is still ordinary C, compiled with GCC. We're not asking anyone to learn a new language."
> *(Press `d` — compiler lane appears)*

> **55-68s:**
> "On these workloads, ATOMiK eliminates sixty-nine percent of compute, energy, and cost. At a thousand servers, that's thirty-four thousand dollars a year. At global scale, the addressable market is one point seven billion."
> *(Press `f` — adoption forecast)*

> **68-80s:**
> "The execution adapts to the workload. Only active state regions get compute. Idle state costs nothing. The hardware shapes itself around what's actually happening."

> **80-90s:**
> "ATOMiK is a live hardware execution primitive for state-heavy systems. The board is the proof. The product is the IP. And the adoption path starts with the compilers developers already use."

---

# SECTION 12 — BIGGEST RISKS

## 1. Visual confusion

**Risk:** Too many panels, numbers, and animations make the screen unreadable.
**How it fails:** Investor stares at the screen and doesn't know where to look.
**Mitigation:** Strict visual hierarchy. One hero element per screen state. Everything else is supporting cast.

## 2. Overclaiming "virtual processors"

**Risk:** Saying "virtual processor personalities" without clear evidence sounds like marketing fluff.
**How it fails:** Technical investor asks "what exactly is the processor doing differently?" and the answer is vague.
**Mitigation:** Never say "virtual processor" without immediately showing execution lanes reconfiguring. The visual IS the proof. Frame it as "workload-shaped execution" not "virtual CPU."

## 3. Adoption credibility gap

**Risk:** Showing inline asm + .insn and claiming "same compiler" feels like a stretch.
**How it fails:** A developer in the room says "that's not real compiler support, that's inline assembly."
**Mitigation:** Be honest. Say "we target ATOMiK through a header and GCC's extended asm, using the RISC-V custom opcode space. A first-class backend is on the roadmap." The example runs on the board — that's the undeniable part.

## 4. Demo reliability

**Risk:** The board crashes, the UART garbles, the browser disconnects, or the DMA address gets corrupted.
**How it fails:** Presenter scrambles to fix it. Trust evaporates.
**Mitigation:** Pre-recorded fallback video. Operator checklist. One-command reset. Test the full sequence 10 times before the meeting.

## 5. Scope creep destroying timeline

**Risk:** Every improvement request adds another build-test-transfer cycle (10+ minutes each), and the demo is never "done."
**How it fails:** The team spends weeks polishing instead of meeting investors.
**Mitigation:** Define "investor-ready" as a specific milestone with specific criteria. When those criteria are met, STOP BUILDING AND START MEETING.

---

# SECTION 13 — MVP VS "UNBELIEVABLE" VERSION

## A. MVP Investor-Ready Version

**What's included:**
- Current atomik_live.c with all working features (head-to-head, binary algebra, compiler lane, adoption forecast, burst mode, corruption detection, reset)
- Browser control plane (current version)
- LCD replica
- Narration script
- One-page leave-behind
- demo_launch.sh working
- Pre-recorded fallback video

**What's excluded:**
- Physical buttons (keyboard-via-browser is primary input)
- Power telemetry
- Light columns
- Custom enclosure
- Execution lanes animation
- Browser sparkline chart
- Mobile responsive

**Effort:** 1-2 days of polish + recording + rehearsal.

**This is investor-ready NOW** with the fixes already in progress.

## B. Unbelievable Flagship Version

**What's included:**
Everything in MVP, plus:
- Execution lanes visualization on HDMI
- Physical 3-button control pod wired to 40-pin header
- Inline power telemetry (Joulescope JS220)
- Two physical light columns (SW orange / ATOMiK blue)
- Matte black acrylic mounting base
- Browser sparkline chart + latency display
- 10" tablet showing browser (dedicated display)
- HDMI recorder for clean capture
- Side display showing compiler lane (portrait orientation)
- Rehearsed 90-second and 3-minute scripts
- Professional one-page leave-behind (designed, printed)

**What's excluded:**
- USB keyboard (parked)
- Multi-board networking
- Real inference workload
- Custom compiler backend

**Effort:** 2-3 weeks focused work + ~$2,500 hardware.

**This is the version that makes investors fight over the check.**

## Recommendation:

**Ship MVP within 48 hours. Start Flagship build immediately after first investor meeting.**

The MVP already works. Don't let perfect be the enemy of live investor meetings. Use the first meeting to calibrate what the Flagship version needs to emphasize.

---

# SECTION 14 — FINAL RECOMMENDATION

## Which demo concept to build:

**ATOMiK Pulse Instrument** — the interactive, multi-surface, head-to-head live system that's already running on the board.

## What to build first:

1. Fix remaining visual overlaps (30 min)
2. Lock the narrative (30 min)
3. Record the fallback video (1 hour)
4. Rehearse the script (1 hour)
5. Meet an investor

## What to cut:

- USB keyboard bring-up
- Multi-board networking
- Custom compiler backend
- Anything that delays the first meeting by more than 48 hours

## The winning story:

ATOMiK is not a faster chip. It is a new execution primitive that eliminates wasted rediscovery of change. It works in hardware, it scales across endpoints, and developers can target it with the compilers they already use.

## One sentence for internal alignment:

> **"We make the waste visible, we make it disappear, and we prove the path to adoption — all in one board, all live, all real."**
