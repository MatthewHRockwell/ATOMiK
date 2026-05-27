#!/usr/bin/env python3
"""Generate the current ATOMiK investor deck.

The Markdown files in this directory are the reviewable copy source. This script
renders the meeting deck as a controlled-layout PowerPoint so pitch artifacts do
not inherit stale metrics from older generator scripts.
"""

from pathlib import Path
import shutil
import subprocess
import sys

try:
    from pptx import Presentation
    from pptx.util import Inches, Pt
    from pptx.dml.color import RGBColor
    from pptx.enum.text import PP_ALIGN, MSO_ANCHOR
    from pptx.enum.shapes import MSO_SHAPE
    from PIL import Image
except ModuleNotFoundError:
    Presentation = None

    def RGBColor(r, g, b):
        return (r, g, b)

    def Inches(value):
        return value

    def Pt(value):
        return value

    class _FallbackEnum:
        LEFT = CENTER = RIGHT = TOP = 0
        RECTANGLE = ROUNDED_RECTANGLE = 0

    PP_ALIGN = _FallbackEnum()
    MSO_ANCHOR = _FallbackEnum()
    MSO_SHAPE = _FallbackEnum()
    Image = None

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[1]
SOURCE = ROOT / "slides.md"
OUT_MAIN = ROOT / "ATOMiK_Investor_Deck.pptx"
OUT_AGGIE = ROOT / "ATOMiK_Aggie_Angel_Deck.pptx"
LIVE_SCREENSHOT = REPO / "website/public/09-current-live-atomik-desk-v039k.png"

BG = RGBColor(0x07, 0x0B, 0x12)
PANEL = RGBColor(0x0D, 0x14, 0x20)
PANEL2 = RGBColor(0x10, 0x1A, 0x29)
BORDER = RGBColor(0x1D, 0x32, 0x4A)
TEXT = RGBColor(0xF4, 0xF8, 0xFF)
MUTED = RGBColor(0x9F, 0xB1, 0xC7)
FAINT = RGBColor(0x6F, 0x80, 0x97)
CYAN = RGBColor(0x22, 0xD3, 0xEE)
GREEN = RGBColor(0x22, 0xC5, 0x5E)
BLUE = RGBColor(0x4F, 0x8F, 0xFF)
VIOLET = RGBColor(0xA7, 0x8B, 0xFA)
AMBER = RGBColor(0xF5, 0x9E, 0x0B)
RED = RGBColor(0xEF, 0x44, 0x44)

SLIDE_W = Inches(13.333)
SLIDE_H = Inches(7.5)
FONT = "Aptos"


def rgb_tuple(c):
    return (c[0], c[1], c[2]) if isinstance(c, tuple) else c


def set_bg(slide, color=BG):
    fill = slide.background.fill
    fill.solid()
    fill.fore_color.rgb = color


def text_box(slide, x, y, w, h, text, size=18, color=TEXT, bold=False,
             align=PP_ALIGN.LEFT, anchor=MSO_ANCHOR.TOP, font=FONT):
    box = slide.shapes.add_textbox(x, y, w, h)
    tf = box.text_frame
    tf.clear()
    tf.word_wrap = True
    tf.margin_left = 0
    tf.margin_right = 0
    tf.margin_top = 0
    tf.margin_bottom = 0
    tf.vertical_anchor = anchor
    p = tf.paragraphs[0]
    p.alignment = align
    p.space_after = Pt(0)
    run = p.add_run()
    run.text = text
    run.font.name = font
    run.font.size = Pt(size)
    run.font.bold = bold
    run.font.color.rgb = color
    return box


def add_paragraphs(slide, x, y, w, h, lines, size=18, color=MUTED, bullet=False,
                   line_gap=4, accent=None):
    box = slide.shapes.add_textbox(x, y, w, h)
    tf = box.text_frame
    tf.clear()
    tf.word_wrap = True
    tf.margin_left = 0
    tf.margin_right = 0
    tf.margin_top = 0
    tf.margin_bottom = 0
    for idx, line in enumerate(lines):
        p = tf.paragraphs[0] if idx == 0 else tf.add_paragraph()
        p.space_after = Pt(line_gap)
        if bullet:
            p.level = 0
            p.text = ""
            run = p.add_run()
            run.text = "- "
            run.font.color.rgb = accent or CYAN
            run.font.size = Pt(size)
            run.font.name = FONT
            run = p.add_run()
            run.text = line
        else:
            run = p.add_run()
            run.text = line
        run.font.name = FONT
        run.font.size = Pt(size)
        run.font.color.rgb = color
    return box


def rect(slide, x, y, w, h, fill=PANEL, border=BORDER, radius=True):
    shape_type = MSO_SHAPE.ROUNDED_RECTANGLE if radius else MSO_SHAPE.RECTANGLE
    s = slide.shapes.add_shape(shape_type, x, y, w, h)
    s.fill.solid()
    s.fill.fore_color.rgb = fill
    s.line.color.rgb = border
    s.line.width = Pt(0.75)
    if radius:
        try:
            s.adjustments[0] = 0.08
        except Exception:
            pass
    return s


def accent_bar(slide, x, y, w, color):
    s = slide.shapes.add_shape(MSO_SHAPE.RECTANGLE, x, y, w, Pt(3))
    s.fill.solid()
    s.fill.fore_color.rgb = color
    s.line.fill.background()
    return s


def header(slide, n, label="Investor deck | May 2026 | matthew.h.rockwell@gmail.com"):
    text_box(slide, Inches(0.42), Inches(0.23), Inches(1.4), Inches(0.25),
             "ATOMiK", 14, CYAN, True)
    text_box(slide, Inches(1.82), Inches(0.24), Inches(6.0), Inches(0.25),
             label, 9, FAINT)
    text_box(slide, SLIDE_W - Inches(0.9), Inches(0.24), Inches(0.5), Inches(0.25),
             f"{n:02d}", 9, FAINT, align=PP_ALIGN.RIGHT)
    accent_bar(slide, Inches(0.42), Inches(0.6), SLIDE_W - Inches(0.84), BORDER)


def title(slide, kicker, heading, body=None):
    text_box(slide, Inches(0.75), Inches(0.93), Inches(4.8), Inches(0.28),
             kicker.upper(), 10, CYAN, True)
    text_box(slide, Inches(0.75), Inches(1.24), Inches(10.8), Inches(0.9),
             heading, 34, TEXT, True)
    if body:
        text_box(slide, Inches(0.77), Inches(2.1), Inches(10.2), Inches(0.55),
                 body, 15, MUTED)


def card(slide, x, y, w, h, label, body, color=CYAN, body_size=12):
    rect(slide, x, y, w, h, PANEL)
    accent_bar(slide, x + Inches(0.18), y + Inches(0.16), Inches(0.62), color)
    text_box(slide, x + Inches(0.18), y + Inches(0.36), w - Inches(0.36), Inches(0.3),
             label, 14, TEXT, True)
    text_box(slide, x + Inches(0.18), y + Inches(0.77), w - Inches(0.36), h - Inches(0.9),
             body, body_size, MUTED)


def pill(slide, x, y, text, color=GREEN, w=Inches(2.0)):
    rect(slide, x, y, w, Inches(0.34), PANEL2, BORDER)
    text_box(slide, x + Inches(0.12), y + Inches(0.08), w - Inches(0.24), Inches(0.18),
             text, 8.5, color, True, align=PP_ALIGN.CENTER)


def image_fit(slide, path, x, y, w, h):
    with Image.open(path) as im:
        iw, ih = im.size
    scale = min(w / iw, h / ih)
    nw = int(iw * scale)
    nh = int(ih * scale)
    return slide.shapes.add_picture(str(path), x + int((w - nw) / 2), y + int((h - nh) / 2), width=nw, height=nh)


def source_note(slide, text):
    text_box(slide, Inches(0.75), SLIDE_H - Inches(0.55), Inches(11.9), Inches(0.25),
             text, 7.5, FAINT)


def add_deck():
    prs = Presentation()
    prs.slide_width = SLIDE_W
    prs.slide_height = SLIDE_H

    # 1
    s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s); header(s, 1)
    title(s, "Pre-seed  ·  $2.0M  ·  Post-money SAFE", "Make change the unit of compute",
          "State-aware compute architecture that reduces wasted state movement for constrained edge, embedded, and AI workloads.")
    pill(s, Inches(0.77), Inches(2.86), "HARDWARE_VALIDATED on Zynq FPGA", GREEN, Inches(2.75))
    pill(s, Inches(3.7), Inches(2.86), "16/16 algebraic proofs pass", CYAN, Inches(2.35))
    text_box(s, Inches(0.77), Inches(3.4), Inches(5.0), Inches(0.65),
             "Every constrained system wastes resources rediscovering what changed. ATOMiK tracks meaningful change instead — reducing bytes moved, operations repeated, and state scanned.", 15, MUTED)
    image_fit(s, LIVE_SCREENSHOT, Inches(6.15), Inches(1.2), Inches(6.55), Inches(3.7))
    card(s, Inches(0.75), Inches(4.9), Inches(3.85), Inches(1.4), "IP thesis", "Architecture + implementation + workload evidence → licensing or strategic acquisition", CYAN, 10)
    card(s, Inches(4.88), Inches(4.9), Inches(3.85), Inches(1.4), "Live proof", "ATOMiK Desk v0.39-K running on Zynq hardware today", GREEN, 10)
    card(s, Inches(9.01), Inches(4.9), Inches(3.7), Inches(1.4), "18-month gate", "$2M funds measured customer proof + IP diligence + ASIC feasibility", VIOLET, 10)

    # 2
    s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s); header(s, 2)
    title(s, "The problem", "Constrained systems pay a hidden tax on unchanged state")
    bullets = [
        "Data-center energy: 415 TWh in 2024 → projected 945 TWh by 2030. (IEA)",
        "U.S. alone: 176 TWh in 2023 → 325–580 TWh projected by 2028. (LBNL)",
        "66 billion liters of direct cooling water consumed in U.S. data centers in 2023.",
        "Edge and embedded teams hit hard limits on battery, heat, and bandwidth — before data center scale.",
        "In every case: the system processes state it already knows. Most of it didn't change.",
    ]
    add_paragraphs(s, Inches(0.9), Inches(2.1), Inches(6.5), Inches(3.2), bullets, 15.5, MUTED, True, 7, AMBER)
    card(s, Inches(7.6), Inches(2.0), Inches(4.65), Inches(1.15), "Battery / heat", "Redundant state movement burns power and generates thermal load in sealed devices.", GREEN, 11)
    card(s, Inches(7.6), Inches(3.35), Inches(4.65), Inches(1.15), "Bandwidth", "Full-state sync over constrained links moves more than necessary, every tick.", BLUE, 11)
    card(s, Inches(7.6), Inches(4.7), Inches(4.65), Inches(1.15), "Latency", "Rebuilding or replaying state instead of tracking delta adds delay where it hurts.", VIOLET, 11)
    source_note(s, "IEA Energy and AI (2024); LBNL 2024 US Data Center Report. Market framing only — not an ATOMiK savings claim.")

    # 3
    s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s); header(s, 3)
    title(s, "End-game value", "Customers do not buy delta-state algebra")
    values = [
        ("Bytes avoided", "Less state movement against the current baseline", GREEN),
        ("Update latency", "Faster local state paths where the workload fits", CYAN),
        ("Bandwidth pressure", "Fewer full-state transfers when measured", BLUE),
        ("Operations coalesced", "Less repeated work in change-heavy paths", VIOLET),
        ("Power/thermal proxy", "Follow-on measurement when instrumentation exists", AMBER),
    ]
    for i, (a,b,c) in enumerate(values):
        card(s, Inches(0.85 + (i % 3) * 4.1), Inches(2.05 + (i // 3) * 1.75), Inches(3.6), Inches(1.28), a, b, c, 11)
    text_box(s, Inches(5.0), Inches(5.65), Inches(7.2), Inches(0.55),
             "Battery, cooling, water, and footprint remain evaluation targets until measured.", 17, TEXT, True)

    # 4
    s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s); header(s, 4)
    title(s, "The primitive", "Make change the unit of compute")
    rect(s, Inches(1.1), Inches(2.2), Inches(11.1), Inches(1.05), PANEL2)
    text_box(s, Inches(1.42), Inches(2.53), Inches(10.4), Inches(0.35),
             "state = reference_state XOR accumulated_delta", 24, CYAN, True, align=PP_ALIGN.CENTER)
    steps = [("LOAD", "Start from a known reference state", CYAN), ("ACCUM", "Accumulate compact deltas", GREEN), ("READ", "Reconstruct only when needed", BLUE), ("SWAP", "Commit clean epoch boundaries", VIOLET)]
    for i, (a,b,c) in enumerate(steps):
        card(s, Inches(0.85 + i * 3.12), Inches(4.0), Inches(2.65), Inches(1.35), a, b, c, 11)

    # 5
    s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s); header(s, 5)
    title(s, "Fit", "ATOMiK has a wedge, not a universal claim")
    add_paragraphs(s, Inches(0.9), Inches(2.0), Inches(5.6), Inches(3.6), [
        "writes or updates dominate reads",
        "state changes are sparse",
        "bandwidth is expensive",
        "latency and power budgets are tight",
        "rollback, sync, or context retention create overhead",
    ], 17, MUTED, True, 7, GREEN)
    rect(s, Inches(7.0), Inches(2.15), Inches(4.8), Inches(2.5), PANEL2)
    text_box(s, Inches(7.35), Inches(2.45), Inches(4.1), Inches(0.4), "Not the pitch", 16, AMBER, True)
    add_paragraphs(s, Inches(7.35), Inches(3.0), Inches(4.1), Inches(1.15), [
        "a general-purpose CPU replacement",
        "a claimed thermal result before measurement",
        "a commercial desktop product",
    ], 13, MUTED, True, 5, AMBER)

    # 6
    s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s); header(s, 6)
    title(s, "Use cases", "Different industries feel the same waste in different budgets")
    cases = [
        ("Edge / embedded", "battery, enclosure heat, intermittent links, reliability", "measure bytes moved, latency, bandwidth, power proxy", CYAN),
        ("AI at the edge", "context/state movement, memory pressure", "measure context retained and transfer avoided", VIOLET),
        ("Remote / industrial", "weight, wattage, packet budget, field runtime", "measure runtime proxy and update cost", AMBER),
        ("Data center / infrastructure", "power bill, cooling, water pressure, density", "measure bytes moved and power/thermal path", GREEN),
    ]
    for i, (name,pain,metric,color) in enumerate(cases):
        x = Inches(0.85 + (i % 2) * 6.05); y = Inches(2.0 + (i // 2) * 1.85)
        rect(s, x, y, Inches(5.45), Inches(1.45), PANEL)
        accent_bar(s, x + Inches(0.18), y + Inches(0.16), Inches(0.72), color)
        text_box(s, x + Inches(0.18), y + Inches(0.36), Inches(5.0), Inches(0.28), name, 15, TEXT, True)
        text_box(s, x + Inches(0.18), y + Inches(0.75), Inches(5.0), Inches(0.22), f"Pain: {pain}", 10.5, MUTED)
        text_box(s, x + Inches(0.18), y + Inches(1.05), Inches(5.0), Inches(0.22), f"Target: {metric}", 10.5, FAINT)

    # 7
    s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s); header(s, 7)
    title(s, "Live proof", "Current demo surface running on Zynq hardware")
    rect(s, Inches(0.75), Inches(2.0), Inches(11.85), Inches(4.25), PANEL2)
    image_fit(s, LIVE_SCREENSHOT, Inches(0.92), Inches(2.18), Inches(11.5), Inches(3.65))
    text_box(s, Inches(1.0), Inches(5.93), Inches(11.1), Inches(0.25),
             "HARDWARE_VALIDATED: ATOMiK Desk v0.39-K prototype UI running on live Zynq hardware. Not commercial product maturity or performance proof.", 8.5, MUTED)

    # 8
    s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s); header(s, 8)
    title(s, "Proof stack", "Compelling, but evidence-bounded")
    proofs = [
        ("Zynq Desk v0.39-K", "HARDWARE_VALIDATED", "current public proof image", GREEN),
        ("Linux userspace to FPGA", "HARDWARE_VALIDATED", "documented OS-to-bus path", CYAN),
        ("AX7020 board run matrix", "LIVE_MEASURED", "raw artifact with caveats", BLUE),
        ("Formal algebra", "SOFTWARE_VALIDATED", "proof work in repo", VIOLET),
        ("Standalone boot artifacts", "BUILD_ARTIFACT", "local build output exists; public power-on artifact gated", AMBER),
    ]
    for i, (name,label,status,color) in enumerate(proofs):
        y = Inches(1.95 + i * 0.78)
        rect(s, Inches(0.9), y, Inches(11.55), Inches(0.55), PANEL, BORDER, True)
        text_box(s, Inches(1.15), y + Inches(0.14), Inches(3.0), Inches(0.18), name, 10.5, TEXT, True)
        text_box(s, Inches(4.25), y + Inches(0.14), Inches(2.4), Inches(0.18), label, 9.3, color, True)
        text_box(s, Inches(6.85), y + Inches(0.14), Inches(5.25), Inches(0.18), status, 10.2, MUTED)

    # 9
    s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s); header(s, 9)
    title(s, "Return path", "Don't outspend incumbents. Become their strategic acquisition.")
    path = [
        ("1", "Measured proof\n(0–12 mo)", "One workload. One baseline. One metric. Artifact-backed, correctness-verified.", CYAN),
        ("2", "IP & diligence\n(6–18 mo)", "Patent conversion, prior-art review, counsel-reviewed packet.", GREEN),
        ("3", "Design partners\n(12–24 mo)", "Paid evaluations → design-partner agreements → IP licensing value.", BLUE),
        ("4", "Strategic outcome\n(18–36 mo)", "License, partner, or be acquired by a chip/platform company at IP premium.", VIOLET),
    ]
    for i, (num, head, body, color) in enumerate(path):
        x = Inches(0.75 + i * 3.1)
        rect(s, x, Inches(2.1), Inches(2.7), Inches(3.05), PANEL2)
        accent_bar(s, x + Inches(0.18), Inches(2.26), Inches(2.15), color)
        text_box(s, x + Inches(0.18), Inches(2.5), Inches(2.3), Inches(0.48), head, 13.5, TEXT, True)
        text_box(s, x + Inches(0.18), Inches(3.1), Inches(2.25), Inches(0.95), body, 10.5, MUTED)
    text_box(s, Inches(0.9), Inches(5.45), Inches(11.5), Inches(0.42),
             "Comparable IP architecture acquisitions in semiconductor history: $50M–$400M+ for validated compute primitives. ATOMiK's path: proof → IP value → strategic partner.", 13, FAINT, align=PP_ALIGN.CENTER)
    source_note(s, "Strategic outcome framing only. No acquisition is guaranteed or implied. Returns are speculative and subject to risk.")

    # 10
    s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s); header(s, 10)
    title(s, "Use of funds", "$2.0M target pre-seed | 18 months to measured proof")
    funds = [
        ("Engineering", "$600K | Zynq hardening, measurement tooling, first technical capacity", BLUE),
        ("Customer proof", "$400K | workload baselines, evaluation SOWs, measured artifacts", GREEN),
        ("IP / legal", "$300K | patent conversion, prior-art review, diligence packet", CYAN),
        ("ASIC feasibility", "$300K | mentor review, feasibility study, quoted path", VIOLET),
        ("Finance / GTM", "$250K | CFO, investor reporting, design-partner outreach", AMBER),
        ("Reserve", "$150K | runway buffer and unforeseen proof costs", FAINT),
    ]
    for i, (head,body,color) in enumerate(funds):
        card(s, Inches(0.85 + (i % 3) * 4.1), Inches(2.0 + (i // 3) * 1.55), Inches(3.65), Inches(1.16), head, body, color, 10.2)
    text_box(s, Inches(0.9), Inches(5.55), Inches(11.45), Inches(0.65), "Minimum close $1.25M. Stretch plan $2.75M. Final SAFE terms require CFO/counsel approval. No tape-out funding.", 14.2, FAINT, align=PP_ALIGN.CENTER)

    # 11
    s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s); header(s, 11)
    title(s, "Risks and gates", "Trust comes from saying exactly what is still unproven")
    risks = [
        ("Battery / power / thermal outcomes", "Evaluation targets until measured end to end", GREEN),
        ("Customer validation", "Pending; paid evaluations and design partners are the wedge", CYAN),
        ("ASIC economics", "Fund feasibility review before tape-out", VIOLET),
        ("Incumbent response", "Protect IP, build proof, pursue strategic conversations", AMBER),
    ]
    for i, (r,a,c) in enumerate(risks):
        x = Inches(0.9 + (i % 2) * 5.9); y = Inches(2.1 + (i // 2) * 1.65)
        card(s, x, y, Inches(5.35), Inches(1.22), r, a, c, 11)
    text_box(s, Inches(1.0), Inches(5.75), Inches(11.4), Inches(0.4), "The pitch is ambitious, but every public claim has a proof boundary.", 18, TEXT, True, align=PP_ALIGN.CENTER)

    # 12
    s = prs.slides.add_slide(prs.slide_layouts[6]); set_bg(s); header(s, 12)
    title(s, "The ask", "Raising $2.0M to become a strategic acquisition target")
    asks = [
        "$2.0M target pre-seed  ·  $1.25M minimum close  ·  $2.75M stretch",
        "Post-money SAFE  ·  final terms require CFO/counsel review",
        "18 months of runway to measured proof, IP diligence, and ASIC feasibility",
        "Design-partner introductions and customer workload access",
    ]
    add_paragraphs(s, Inches(0.9), Inches(2.2), Inches(7.7), Inches(2.2), asks, 16, MUTED, True, 9, GREEN)
    rect(s, Inches(8.75), Inches(2.1), Inches(3.55), Inches(2.8), PANEL2)
    text_box(s, Inches(9.05), Inches(2.38), Inches(2.95), Inches(0.3), "Why invest now?", 13, CYAN, True, align=PP_ALIGN.CENTER)
    text_box(s, Inches(8.95), Inches(2.82), Inches(3.2), Inches(1.7),
             "Proof is achievable at pre-seed scale. The gap between current hardware validation and defensible IP is $2M, not $20M. That gap is the entry point.", 13.5, TEXT, False, align=PP_ALIGN.CENTER)
    card(s, Inches(0.9), Inches(4.72), Inches(3.75), Inches(0.95), "Money in", "$2.0M pre-seed", CYAN, 13)
    card(s, Inches(4.85), Inches(4.72), Inches(3.75), Inches(0.95), "18-month output", "Measured proof + IP packet + ASIC feasibility", GREEN, 11)
    card(s, Inches(8.8), Inches(4.72), Inches(3.5), Inches(0.95), "Strategic upside", "Acquisition by chip/platform company at IP premium", VIOLET, 11)
    text_box(s, Inches(1.0), Inches(5.88), Inches(11.25), Inches(0.35), "matthew.h.rockwell@gmail.com  ·  First round still open  ·  Aggie Angel Network pitch  ·  May 2026", 9.5, FAINT, align=PP_ALIGN.CENTER)

    prs.save(OUT_MAIN)
    OUT_AGGIE.write_bytes(OUT_MAIN.read_bytes())


def pandoc_fallback():
    pandoc = shutil.which("pandoc")
    if not pandoc:
        print("ERROR: python-pptx or pandoc is required to generate the deck", file=sys.stderr)
        return 1
    cmd = [pandoc, str(SOURCE.name), "--from", "markdown", "--to", "pptx", "--slide-level", "1", "--output", str(OUT_MAIN.name)]
    subprocess.run(cmd, cwd=ROOT, check=True)
    OUT_AGGIE.write_bytes(OUT_MAIN.read_bytes())
    return 0


def main():
    if Presentation is None:
        print("python-pptx not found; falling back to Pandoc conversion", file=sys.stderr)
        return pandoc_fallback()
    add_deck()
    print(f"Wrote {OUT_MAIN}")
    print(f"Wrote {OUT_AGGIE}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
