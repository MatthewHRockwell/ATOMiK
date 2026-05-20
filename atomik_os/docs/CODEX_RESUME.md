# ATOMiK Desk — Codex/ChatGPT Resume Prompt
# Updated: 2026-05-20 post v0.39-I

---

## BEFORE YOU SEND THIS — ATTACH THESE FILES

Attach all of the following images to your message:

1. **The screenshot to review:**
   `/home/mattrock/Projects/ATOMiK/atomik_os/docs/screenshots/deploy_v0.39-I_rail.png`

2. **Concept reference images (use for fidelity comparison):**
   - `/home/mattrock/Projects/ATOMiK/docs/design/concept_images/07_atomik_desk_landing.png`
   - `/home/mattrock/Projects/ATOMiK/docs/design/concept_images/02_document_surface.png`
   - `/home/mattrock/Projects/ATOMiK/docs/design/concept_images/06_adaptive_mode.png`
   - `/home/mattrock/Projects/ATOMiK/docs/design/assistant/atomik_assistant_final.png`

   (Concept 01 is an OLD screenshot — do NOT use it as a target. 02–07 are the real targets.)

---

## ROLE

You are the visual design reviewer for **ATOMiK Desk**, a graphical OS shell
rendered in C directly to a 1920×1080 framebuffer on a Xilinx Zynq FPGA board
(ALINX AX7020, ~$200 board). No GPU, no X11, no compositor — a single C process
writing pixels directly to `/dev/fb0`.

Your job: review SIGSTOP-captured framebuffer screenshots against the concept
images, score each surface, and give a tight next-slice directive.

---

## WHAT ATOMiK IS

ATOMiK is a **delta-state compute architecture**: `state = initial XOR accumulator`.
It is NOT a traditional desktop. The shell is a live demo of the hardware
architecture running on itself.

Three personalities:
- **STATE** — coalesces repeated writes before commit. Headline metric: `ops_logical / ops_issued` (compression ratio).
- **SYNC** — propagates deltas from edge to remote replica. Headline: `bytes_avoided` or `ops_issued`.
- **AGENT** — retains hot context, parks cold. Headline: `ops_issued` ("N HOT").

---

## DESIGN SYSTEM — HARD CONSTRAINTS

**Semantic color grammar (NEVER violate):**
- Cyan → hardware / STATE
- Green → savings / SYNC
- Violet → agent / AGENT
- Amber → contention / waste
- Dim slate → idle / decorative

**Class discipline:**
- **Class A**: every visible number from a real live producer. No fake stats, no placeholders.
- **Class B**: decorative chrome (waveforms, halos, backgrounds). Fine as long as no number claim.
- **Class C: FORBIDDEN** — fake metrics, canned values, placeholder numbers.

**Layered-stroke doctrine:** No naked 1 px lines. Every stroke has an alpha halo
(8/4/2 px stack for heavier lines; glow via pre-rendered Class B assets).

**AA font atlas:**
- `atomik_14` → small labels
- `atomik_18` → UI text
- `atomik_28` → display / big metrics
- `atomik_36` → brand / wordmark

---

## CURRENT SCORES (post v0.39-I, 2026-05-20)

| Surface     | Score | Status                              |
|-------------|-------|-------------------------------------|
| Pulse Bar   | 92    | v0.39-I shipped — **YOUR VERDICT PENDING** |
| Cap Rail    | 93    | PARKED                              |
| Hero        | 92    | parked                              |
| Fabric      | 93    | parked                              |
| Background  | 94    | parked                              |
| Typography  | 92    | parked                              |
| Composition | 94    | parked                              |
| Atom        | 95    | PARKED                              |
| **Overall** | **94** | **Target: 95**                     |

---

## WHAT CHANGED IN v0.39-I (the screenshot you're reviewing)

**Problem v0.39-I fixed:**
In v0.39-H, the Pulse Bar's integrated metric readout showed "3 HOT" (violet /
AGENT personality color) while the top badge and Hero both showed SYNC as the
active personality. Two competing stories on the same screen.

**The fix:**
The metric now reads `perf_last_for(fabric_active())` — it pulls from the
**currently active personality's** batch, not the most-recent-any-personality
batch. If the active personality has no batch data yet, the metric region is
left empty (waveform-only). This is Class A: never borrow another
personality's number.

**What v0.39-H introduced that carries forward:**
- 200 px unified pulse well (was 128 px).
- Left ~64 px: AA `atomik_14` personality metric in semantic color.
- Right ~136 px: 8/4/2 layered halo glow waveform (`draw_event_pulse_glow`).
- Waveform color stays **cyan** (system-level event stream, personality-neutral).
- "PULSE" label + separate mini-readout removed.

---

## SURFACE STATE REFERENCE

**PULSE BAR (top of screen, 80 px tall):**
Dev path left→right: `ATOMiK` wordmark | active personality badge (semantic color) |
`DATA` chip | `MODE` chip | event pulse well (200 px) | wallet / cpu% / uptime / version

**CAP RAIL (left sidebar, parked at 93):**
80×100 px cells with line-art icons + AA labels.
"SYSTEM" header above cells. Thin divider before Atom cell.
6 cells: About | Monitor | Terminal | Files | Notes | Atom.

**HERO (center canvas, 92):**
Three personality fields (STATE / SYNC / AGENT) at equal spacing.
Central core node with orbital rings.
Bezier energy links from each field to the core.

**RESOURCE FABRIC (right panel, 93):**
5-lane card: STATE | SYNC | AGENT | EVENT | VISUAL.
AA capsule chip + freshness chip + stacked big-metric per lane. Class A only.

**ATOM ASSISTANT (overlay, parked at 95):**
160 px asset with chromakey cutout. Three concentric halo rings.
SUCCESS state = cyan core + emerald outer rim.
Source-aware title above windows: "active" / "workload detected" / "data live".

---

## SCORING HISTORY (anchor points)

| Version    | Overall | Notes                                          |
|------------|---------|------------------------------------------------|
| v0.38-J++  | ~65     | typography first pass                          |
| v0.38-K3   | ~72     | Fabric AA chips + stacked metric               |
| v0.38-L2   | ~80     | Hero recomposition                             |
| v0.39-E    | 93      | SUCCESS emerald rim; Atom → 95 (PARKED)        |
| v0.39-G    | 94      | Cap Rail → 93 (PARKED)                         |
| v0.39-I    | 94+?    | **Awaiting your verdict**                      |

---

## WHAT I NEED FROM YOU

**1. Score the full rubric** (all 8 surfaces + Overall).

**2. Verdict on Pulse Bar specifically:**
- Does the metric now match the active personality shown in the top badge?
- If the metric region is empty (no active-personality batch yet), does the
  waveform-only well read as "instrument waiting" or "broken / empty"?
- Should the waveform color track the active personality, or stay cyan?

**3. Next-slice directive:**
- If Pulse Bar crosses 93, name the smallest change to push Overall from 94 → 95.
  Remaining laggards: Hero (92), Typography (92).
- If Pulse Bar is still 92, name v0.39-J as a tight, single-scope fix.
- Or: is the desktop effectively "done" at 94 and the right move is to
  pivot off cosmetic polish entirely?

**Response format:**
1. Full rubric scores.
2. Verdict paragraph on the changed surface.
3. v0.39-J directive: name, scope (one paragraph), nothing more.
