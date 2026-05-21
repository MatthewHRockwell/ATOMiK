# ATOMiK Desk — Codex/ChatGPT Resume Prompt
# Updated: 2026-05-20 post v0.39-J

---

## BEFORE YOU SEND THIS — ATTACH THESE FILES

Attach all of the following images to your message:

1. **The screenshot to review:**
   `/home/mattrock/Projects/ATOMiK/atomik_os/docs/screenshots/deploy_v0.39-J_rail.png`

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
| Pulse Bar   | 93    | PARKED (v0.39-I verdict)            |
| Cap Rail    | 93    | PARKED                              |
| Hero        | 92    | v0.39-J shipped — **YOUR VERDICT PENDING** |
| Fabric      | 93    | parked                              |
| Background  | 94    | parked                              |
| Typography  | 92    | v0.39-J shipped — **YOUR VERDICT PENDING** |
| Composition | 94    | parked                              |
| Atom        | 95    | PARKED                              |
| **Overall** | **94** | **Target: 95**                     |

---

## WHAT CHANGED IN v0.39-J (the screenshot you're reviewing)

**Directive:** Hero Label Lift — raise Hero + Typography together by tightening
the three personality field labels. Make STATE/SYNC/AGENT readable from across
the room without moving any geometry.

**Three changes in `hero.c`:**

1. **Semantic halo behind each heading** — a 3-layer alpha rect (outer/mid/core
   at 14/28/50 alpha for active, 6/12/22 for idle) gives each label a soft
   colored glow in its personality color. The active field glows; idle fields
   are visibly subordinate.

2. **Sublabel contrast gap widened** — active sublabel lifts from `ATOMIK_FG_DIM`
   to `ATOMIK_FG` (full brightness). Idle sublabels step DOWN to
   `rgb(0x72, 0x7C, 0x94)` — darker than the old flat dim — so the active/idle
   gap is wide enough to read across the room.

3. **Heading font unchanged** — `FONT_AA_UI` (atomik_18) as directed. No
   geometry moved, no metrics added, no background changes.

**Previous slice context:**
- v0.39-I (Pulse Bar) scored **93** — PARKED.
- Hero was at **92**, Typography at **92**, the last two laggards before Overall 95.

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

**2. Verdict on Hero + Typography specifically:**
- Do the STATE/SYNC/AGENT headings now read as a clear active/idle hierarchy?
- Is the semantic halo subtle enough not to fight the energy field glow, or
  does it add visual noise?
- Does the active sublabel (full `ATOMIK_FG`) feel legible, or does it
  compete with the heading color?
- Does the idle sublabel step-down feel right, or is it now too dim to read?

**3. Next-slice directive:**
- If Hero + Typography both cross 93, name the smallest change to push
  Overall from 94 → 95.
- If still 92, name v0.39-K as a tight, single-scope fix.
- Or: is the desktop effectively "done" at 94 and the right move is to
  pivot off cosmetic polish entirely?

**Response format:**
1. Full rubric scores.
2. Verdict paragraph on Hero + Typography.
3. v0.39-K directive: name, scope (one paragraph), nothing more.
