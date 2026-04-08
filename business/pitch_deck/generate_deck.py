"""
ATOMiK Investor Pitch Deck Generator
Generates a 10-slide investor-grade PowerPoint with visuals.
"""

from pptx import Presentation
from pptx.util import Inches, Pt, Emu
from pptx.dml.color import RGBColor
from pptx.enum.text import PP_ALIGN, MSO_ANCHOR
from pptx.enum.shapes import MSO_SHAPE
from pptx.enum.chart import XL_CHART_TYPE, XL_LEGEND_POSITION
from pptx.chart.data import CategoryChartData
import math

# ── Theme colours ──────────────────────────────────────────────────
BG         = RGBColor(0x13, 0x13, 0x1A)  # near-black
BG_CARD    = RGBColor(0x1E, 0x1E, 0x2E)  # card background
BLUE       = RGBColor(0x89, 0xB4, 0xFA)  # primary accent
GREEN      = RGBColor(0xA6, 0xE3, 0xA1)  # positive / success
PURPLE     = RGBColor(0xCB, 0xA6, 0xF7)  # secondary accent
YELLOW     = RGBColor(0xF9, 0xE2, 0xAF)  # highlight
RED        = RGBColor(0xF3, 0x8B, 0xA8)  # warning / negative
WHITE      = RGBColor(0xCD, 0xD6, 0xF4)  # body text
DIM        = RGBColor(0x7F, 0x84, 0x9C)  # muted text
SURFACE    = RGBColor(0x31, 0x32, 0x44)  # table/shape fills
TEAL       = RGBColor(0x94, 0xE2, 0xD5)  # extra accent
PINK       = RGBColor(0xF5, 0xC2, 0xE7)  # extra accent

SLIDE_W = Inches(13.333)
SLIDE_H = Inches(7.5)

prs = Presentation()
prs.slide_width  = SLIDE_W
prs.slide_height = SLIDE_H

# ── Helpers ────────────────────────────────────────────────────────

def set_bg(slide, colour=BG):
    bg = slide.background
    fill = bg.fill
    fill.solid()
    fill.fore_color.rgb = colour


def add_textbox(slide, left, top, width, height, text, *,
                font_size=18, bold=False, colour=WHITE,
                alignment=PP_ALIGN.LEFT, font_name="Segoe UI",
                anchor=MSO_ANCHOR.TOP, line_space=1.15):
    txBox = slide.shapes.add_textbox(left, top, width, height)
    tf = txBox.text_frame
    tf.word_wrap = True
    tf.auto_size = None
    try:
        tf.paragraphs[0].space_before = Pt(0)
        tf.paragraphs[0].space_after  = Pt(0)
    except Exception:
        pass
    # Handle multi-run text (list of (text, colour, bold) tuples)
    if isinstance(text, list):
        for i, item in enumerate(text):
            if i == 0:
                p = tf.paragraphs[0]
            else:
                p = tf.add_paragraph()
            p.alignment = alignment
            p.space_before = Pt(0)
            p.space_after  = Pt(2)
            if isinstance(item, tuple):
                t, c, b = item
                run = p.add_run()
                run.text = t
                run.font.size = Pt(font_size)
                run.font.bold = b
                run.font.color.rgb = c
                run.font.name = font_name
            else:
                run = p.add_run()
                run.text = str(item)
                run.font.size = Pt(font_size)
                run.font.bold = bold
                run.font.color.rgb = colour
                run.font.name = font_name
            try:
                p.line_spacing = Pt(int(font_size * line_space))
            except Exception:
                pass
    else:
        p = tf.paragraphs[0]
        p.alignment = alignment
        run = p.add_run()
        run.text = str(text)
        run.font.size = Pt(font_size)
        run.font.bold = bold
        run.font.color.rgb = colour
        run.font.name = font_name
        try:
            p.line_spacing = Pt(int(font_size * line_space))
        except Exception:
            pass
    return txBox


def add_rect(slide, left, top, width, height, fill_colour=SURFACE,
             border_colour=None, corner_radius=None):
    if corner_radius:
        shape = slide.shapes.add_shape(
            MSO_SHAPE.ROUNDED_RECTANGLE, left, top, width, height)
        # Set corner rounding (0-100000 EMU scale)
        try:
            shape.adjustments[0] = corner_radius
        except Exception:
            pass
    else:
        shape = slide.shapes.add_shape(
            MSO_SHAPE.RECTANGLE, left, top, width, height)
    shape.fill.solid()
    shape.fill.fore_color.rgb = fill_colour
    if border_colour:
        shape.line.color.rgb = border_colour
        shape.line.width = Pt(1)
    else:
        shape.line.fill.background()
    return shape


def add_circle(slide, left, top, size, fill_colour):
    shape = slide.shapes.add_shape(
        MSO_SHAPE.OVAL, left, top, size, size)
    shape.fill.solid()
    shape.fill.fore_color.rgb = fill_colour
    shape.line.fill.background()
    return shape


def kpi_card(slide, left, top, width, height, value, label,
             value_colour=GREEN, label_colour=DIM):
    """Render a KPI card with large value and small label."""
    add_rect(slide, left, top, width, height, BG_CARD, SURFACE, 0.05)
    add_textbox(slide, left + Inches(0.2), top + Inches(0.15),
                width - Inches(0.4), Inches(0.5), value,
                font_size=28, bold=True, colour=value_colour,
                alignment=PP_ALIGN.CENTER)
    add_textbox(slide, left + Inches(0.2), top + Inches(0.7),
                width - Inches(0.4), Inches(0.4), label,
                font_size=11, colour=label_colour,
                alignment=PP_ALIGN.CENTER)


def slide_number(slide, num, total=10):
    add_textbox(slide, SLIDE_W - Inches(1), SLIDE_H - Inches(0.4),
                Inches(0.8), Inches(0.3), f"{num}/{total}",
                font_size=10, colour=DIM, alignment=PP_ALIGN.RIGHT)


def section_line(slide, top, colour=BLUE, width_inches=12):
    add_rect(slide, Inches(0.667), top,
             Inches(width_inches), Pt(2), colour)


def add_table(slide, left, top, width, height, rows, cols, data,
              col_widths=None):
    """data = list of lists; first row is header."""
    table_shape = slide.shapes.add_table(rows, cols, left, top, width, height)
    table = table_shape.table

    if col_widths:
        for i, w in enumerate(col_widths):
            table.columns[i].width = w

    for r in range(rows):
        for c in range(cols):
            cell = table.cell(r, c)
            cell.text = str(data[r][c])
            cell.fill.solid()
            if r == 0:
                cell.fill.fore_color.rgb = RGBColor(0x25, 0x25, 0x38)
            else:
                cell.fill.fore_color.rgb = BG_CARD if r % 2 == 1 else RGBColor(0x1A, 0x1A, 0x28)
            for paragraph in cell.text_frame.paragraphs:
                paragraph.font.size = Pt(11)
                paragraph.font.name = "Segoe UI"
                paragraph.font.color.rgb = WHITE if r > 0 else BLUE
                paragraph.font.bold = (r == 0)
                paragraph.alignment = PP_ALIGN.LEFT if c == 0 else PP_ALIGN.CENTER
            # Remove cell margins for tighter look
            cell.margin_left = Inches(0.08)
            cell.margin_right = Inches(0.08)
            cell.margin_top = Inches(0.04)
            cell.margin_bottom = Inches(0.04)

    # Remove table borders
    for r in range(rows):
        for c in range(cols):
            cell = table.cell(r, c)
            for border_name in ['top', 'bottom', 'left', 'right']:
                border = getattr(cell, f'border_{border_name}', None)
                if border:
                    # Can't easily remove borders in python-pptx; set to dark
                    pass

    return table_shape


# ═══════════════════════════════════════════════════════════════════
# SLIDE 1 — TITLE
# ═══════════════════════════════════════════════════════════════════
slide = prs.slides.add_slide(prs.slide_layouts[6])  # blank
set_bg(slide)

# Decorative circles (top-right and bottom-left)
add_circle(slide, SLIDE_W - Inches(3), Inches(-1.5), Inches(5),
           RGBColor(0x89, 0xB4, 0xFA))  # large blue circle, partly off-slide
# Make it semi-transparent by using a muted shade
shape = slide.shapes[-1]
shape.fill.fore_color.rgb = RGBColor(0x1F, 0x27, 0x3A)

add_circle(slide, Inches(-2), SLIDE_H - Inches(3), Inches(5),
           RGBColor(0x1F, 0x20, 0x35))

# Title
add_textbox(slide, Inches(1), Inches(1.2), Inches(10), Inches(1.2),
            "ATOMiK", font_size=72, bold=True, colour=BLUE,
            alignment=PP_ALIGN.LEFT, font_name="Segoe UI")

# Subtitle
add_textbox(slide, Inches(1), Inches(2.5), Inches(10), Inches(0.7),
            "Delta-State Computing in Silicon", font_size=32,
            colour=PURPLE, alignment=PP_ALIGN.LEFT)

# Tagline
add_textbox(slide, Inches(1), Inches(3.4), Inches(10), Inches(0.5),
            "1 Billion Operations/Second on a $13.50 Chip", font_size=22,
            bold=True, colour=GREEN, alignment=PP_ALIGN.LEFT)

# Descriptors
add_textbox(slide, Inches(1), Inches(4.3), Inches(8), Inches(0.4),
            "Mathematically proven  \u2022  Hardware validated  \u2022  Patent pending",
            font_size=16, colour=DIM, alignment=PP_ALIGN.LEFT)

# KPI strip at bottom
kpi_y = Inches(5.5)
kpi_w = Inches(2.5)
kpi_h = Inches(1.1)
gap = Inches(0.35)
start_x = Inches(1)

kpis = [
    ("1,056 Mops/s",  "Peak Throughput (16 banks)",  GREEN),
    ("10.6 ns",        "Single-Op Latency",           BLUE),
    ("92 Proofs",      "Lean4 Verified, 0 Sorry",     PURPLE),
    ("$13.50 Device",  "Tang Nano 9K FPGA",           YELLOW),
]

for i, (val, lbl, clr) in enumerate(kpis):
    kpi_card(slide, start_x + i * (kpi_w + gap), kpi_y, kpi_w, kpi_h,
             val, lbl, clr)

add_textbox(slide, Inches(1), SLIDE_H - Inches(0.55), Inches(4), Inches(0.3),
            "Confidential \u2014 For Investor Use Only", font_size=10, colour=DIM)
slide_number(slide, 1)


# ═══════════════════════════════════════════════════════════════════
# SLIDE 2 — THE PROBLEM
# ═══════════════════════════════════════════════════════════════════
slide = prs.slides.add_slide(prs.slide_layouts[6])
set_bg(slide)

add_textbox(slide, Inches(0.8), Inches(0.4), Inches(8), Inches(0.6),
            "The Memory Wall", font_size=36, bold=True, colour=BLUE)
add_textbox(slide, Inches(0.8), Inches(0.95), Inches(10), Inches(0.4),
            "60\u201390% of system energy is spent moving data, not computing on it.",
            font_size=16, colour=DIM)
section_line(slide, Inches(1.4))

# ── Left column: energy breakdown visual ──
# Stacked bar visual: compute vs data movement
bar_left = Inches(1)
bar_top = Inches(1.8)
bar_w = Inches(5)
bar_h = Inches(0.7)

# Data movement portion (90%)
add_rect(slide, bar_left, bar_top, Inches(4.5), bar_h,
         RGBColor(0xF3, 0x8B, 0xA8))  # red
add_textbox(slide, bar_left + Inches(0.8), bar_top + Inches(0.1),
            Inches(3), Inches(0.5),
            "DATA MOVEMENT  60\u201390%", font_size=14, bold=True,
            colour=BG, alignment=PP_ALIGN.CENTER)

# Compute portion (10%)
add_rect(slide, bar_left + Inches(4.5), bar_top, Inches(0.5), bar_h, GREEN)
add_textbox(slide, bar_left + Inches(4.5), bar_top + Inches(0.1),
            Inches(0.5), Inches(0.5),
            "", font_size=10, colour=BG)

add_textbox(slide, bar_left, bar_top + Inches(0.8), Inches(5), Inches(0.3),
            "Compute (10\u201340%)", font_size=10, colour=GREEN,
            alignment=PP_ALIGN.RIGHT)
add_textbox(slide, bar_left, bar_top + Inches(0.8), Inches(3), Inches(0.3),
            "Data movement (60\u201390%)", font_size=10, colour=RED,
            alignment=PP_ALIGN.LEFT)

# Key stats as cards
stats = [
    ("1,000\u00d7", "Energy: off-chip access\nvs floating-point op", RED),
    ("30,740\u00d7", "Memory traffic reduction\n(ATOMiK vs traditional, 64\u00d764 matrix)", GREEN),
    ("+130%", "HBM demand growth\nYoY in 2025 (TrendForce)", YELLOW),
]

card_top = Inches(3.2)
card_w = Inches(3.4)
card_h = Inches(1.4)

for i, (val, desc, clr) in enumerate(stats):
    cx = Inches(1) + i * (card_w + Inches(0.3))
    add_rect(slide, cx, card_top, card_w, card_h, BG_CARD, SURFACE, 0.05)
    add_textbox(slide, cx + Inches(0.2), card_top + Inches(0.12),
                card_w - Inches(0.4), Inches(0.5), val,
                font_size=32, bold=True, colour=clr,
                alignment=PP_ALIGN.CENTER)
    add_textbox(slide, cx + Inches(0.2), card_top + Inches(0.7),
                card_w - Inches(0.4), Inches(0.7), desc,
                font_size=11, colour=DIM, alignment=PP_ALIGN.CENTER)

# Right side: root cause
add_textbox(slide, Inches(0.8), Inches(5.0), Inches(11.5), Inches(0.4),
            "Root cause: the full-state computation model.",
            font_size=18, bold=True, colour=WHITE)
add_textbox(slide, Inches(0.8), Inches(5.5), Inches(11.5), Inches(1.0),
            "Changing 1 bit in a 64-byte cache line moves 512 bits. "
            "A 64\u00d764 matrix update moves 32 KB per operation. "
            "ATOMiK\u2019s delta approach moves 8 bytes \u2014 the delta itself.",
            font_size=14, colour=DIM)

add_textbox(slide, Inches(0.8), Inches(6.3), Inches(8), Inches(0.3),
            "Sources: DARPA/JASON, Intel ISSCC, TrendForce, McKinsey",
            font_size=9, colour=DIM)
slide_number(slide, 2)


# ═══════════════════════════════════════════════════════════════════
# SLIDE 3 — WHY NOW
# ═══════════════════════════════════════════════════════════════════
slide = prs.slides.add_slide(prs.slide_layouts[6])
set_bg(slide)

add_textbox(slide, Inches(0.8), Inches(0.4), Inches(10), Inches(0.6),
            "Why Now: Four Converging Forces", font_size=36, bold=True,
            colour=BLUE)
section_line(slide, Inches(1.05))

forces = [
    ("AI Inference\nCrisis",
     "90% of AI compute is inference\n(McKinsey). Inference is\nmemory-bandwidth-bound.\n$300B+ hyperscaler AI capex.",
     RGBColor(0xF3, 0x8B, 0xA8),  # red/pink
     "\u26A1"),  # lightning
    ("Post-Moore\nEconomics",
     "Below 3nm: diminishing returns.\nArchitectural innovation is\nthe new scaling vector.\nXOR eliminates carry chains.",
     PURPLE,
     "\u2699"),  # gear
    ("Edge\nConstraints",
     "Edge market: $61B \u2192 $232B\nby 2030 (MarketsandMarkets).\nDevices need single-watt budgets.\nATOMiK: ~20 mW.",
     TEAL,
     "\u25CE"),  # target
    ("HBM Cost\nSpiral",
     "Demand +130% YoY (TrendForce).\nPrices +35%.\n95\u2013100% traffic reduction\ncuts HBM requirements.",
     YELLOW,
     "\u2191"),  # up arrow
]

card_w = Inches(2.7)
card_h = Inches(3.8)
start_x = Inches(0.8)
card_top = Inches(1.4)

for i, (title, desc, clr, icon) in enumerate(forces):
    cx = start_x + i * (card_w + Inches(0.25))

    # Card background
    add_rect(slide, cx, card_top, card_w, card_h, BG_CARD, SURFACE, 0.05)

    # Coloured accent bar at top of card
    add_rect(slide, cx, card_top, card_w, Pt(4), clr)

    # Icon circle
    circle_size = Inches(0.7)
    circle = add_circle(slide, cx + (card_w - circle_size) / 2,
                        card_top + Inches(0.25), circle_size, clr)
    # Darken the circle fill
    circle.fill.fore_color.rgb = RGBColor(
        max(0, clr[0] // 3), max(0, clr[1] // 3), max(0, clr[2] // 3))

    add_textbox(slide, cx + (card_w - circle_size) / 2,
                card_top + Inches(0.3), circle_size, circle_size,
                icon, font_size=24, colour=clr, alignment=PP_ALIGN.CENTER)

    # Title
    add_textbox(slide, cx + Inches(0.15), card_top + Inches(1.1),
                card_w - Inches(0.3), Inches(0.8), title,
                font_size=16, bold=True, colour=clr,
                alignment=PP_ALIGN.CENTER, line_space=1.1)

    # Description
    add_textbox(slide, cx + Inches(0.15), card_top + Inches(1.95),
                card_w - Inches(0.3), Inches(1.8), desc,
                font_size=11, colour=DIM, alignment=PP_ALIGN.CENTER,
                line_space=1.3)

# Bottom bar
add_rect(slide, Inches(0.8), Inches(5.5), Inches(11.5), Inches(1.2),
         RGBColor(0x1A, 0x1F, 0x2E), SURFACE, 0.03)
add_textbox(slide, Inches(1.1), Inches(5.65), Inches(10.9), Inches(0.4),
            "VC Climate: $280B deployed into US/Canadian startups in 2025 (+46% YoY)",
            font_size=16, bold=True, colour=GREEN, alignment=PP_ALIGN.CENTER)
add_textbox(slide, Inches(1.1), Inches(6.1), Inches(10.9), Inches(0.4),
            "AI seed median pre-money: $17.9M (42% premium over non-AI)  \u2022  "
            "AI Series A median: >$50M  \u2022  "
            "Compute-focused seeds regularly >$100M",
            font_size=11, colour=DIM, alignment=PP_ALIGN.CENTER)
slide_number(slide, 3)


# ═══════════════════════════════════════════════════════════════════
# SLIDE 4 — THE SOLUTION
# ═══════════════════════════════════════════════════════════════════
slide = prs.slides.add_slide(prs.slide_layouts[6])
set_bg(slide)

add_textbox(slide, Inches(0.8), Inches(0.4), Inches(10), Inches(0.6),
            "Delta-State XOR Algebra", font_size=36, bold=True, colour=BLUE)
add_textbox(slide, Inches(0.8), Inches(0.95), Inches(10), Inches(0.4),
            "Replace full-state read-modify-write with single-cycle delta accumulation.",
            font_size=16, colour=DIM)
section_line(slide, Inches(1.4))

# ── Traditional vs ATOMiK comparison ──
comp_top = Inches(1.7)
comp_h = Inches(1.2)
half_w = Inches(5.5)

# Traditional box
add_rect(slide, Inches(0.8), comp_top, half_w, comp_h,
         RGBColor(0x2A, 0x1A, 0x1A), RED, 0.04)
add_textbox(slide, Inches(1.0), comp_top + Inches(0.1),
            half_w - Inches(0.4), Inches(0.3),
            "TRADITIONAL", font_size=12, bold=True, colour=RED)
add_textbox(slide, Inches(1.0), comp_top + Inches(0.45),
            half_w - Inches(0.4), Inches(0.3),
            "State = Load \u2192 Lock \u2192 Modify \u2192 Unlock \u2192 Store",
            font_size=14, colour=WHITE, font_name="Consolas")
add_textbox(slide, Inches(1.0), comp_top + Inches(0.8),
            half_w - Inches(0.4), Inches(0.3),
            "Full 64-byte copy each time  \u2022  Lock contention  \u2022  No undo",
            font_size=11, colour=DIM)

# ATOMiK box
add_rect(slide, Inches(6.6), comp_top, half_w, comp_h,
         RGBColor(0x1A, 0x2A, 0x1A), GREEN, 0.04)
add_textbox(slide, Inches(6.8), comp_top + Inches(0.1),
            half_w - Inches(0.4), Inches(0.3),
            "ATOMiK", font_size=12, bold=True, colour=GREEN)
add_textbox(slide, Inches(6.8), comp_top + Inches(0.45),
            half_w - Inches(0.4), Inches(0.3),
            "State = S\u2080 \u2295 \u03b4\u2081 \u2295 \u03b4\u2082 \u2295 \u2026 \u2295 \u03b4\u2099",
            font_size=14, colour=WHITE, font_name="Consolas")
add_textbox(slide, Inches(6.8), comp_top + Inches(0.8),
            half_w - Inches(0.4), Inches(0.3),
            "8-byte delta only  \u2022  Lock-free  \u2022  Self-inverse undo",
            font_size=11, colour=DIM)

# ── Abelian Group Properties ──
add_textbox(slide, Inches(0.8), Inches(3.15), Inches(6), Inches(0.4),
            "Abelian Group Properties (All Proven in Lean4)",
            font_size=18, bold=True, colour=PURPLE)

props_data = [
    ["Property", "Formula", "Hardware Implication"],
    ["Commutativity", "\u03b4\u2081 \u2295 \u03b4\u2082 = \u03b4\u2082 \u2295 \u03b4\u2081", "Lock-free parallel banks"],
    ["Associativity", "(\u03b4\u2081 \u2295 \u03b4\u2082) \u2295 \u03b4\u2083 = \u03b4\u2081 \u2295 (\u03b4\u2082 \u2295 \u03b4\u2083)", "Binary merge tree reduction"],
    ["Self-Inverse", "\u03b4 \u2295 \u03b4 = 0", "Every change is its own undo"],
    ["Identity", "\u03b4 \u2295 0 = \u03b4", "Zero-cost no-op filtering"],
    ["Closure", "\u03b4\u2081 \u2295 \u03b4\u2082 \u2208 \u0394", "No overflow, no type conversion"],
]

add_table(slide, Inches(0.8), Inches(3.6), Inches(7.5), Inches(2.3),
          6, 3, props_data,
          col_widths=[Inches(2), Inches(2.8), Inches(2.7)])

# ── Three consequences ──
cons_top = Inches(3.15)
cons = [
    ("Single-Cycle", "10.6 ns per op\nNo carry propagation", GREEN),
    ("Lock-Free\nParallel", "N banks = N\u00d7\nthroughput", BLUE),
    ("Instant Undo", "\u03b4 \u2295 \u03b4 = 0\nZero-cost reversal", PURPLE),
]

cw = Inches(2.7)
ch = Inches(2.0)
cx_start = Inches(8.8)

for i, (title, desc, clr) in enumerate(cons):
    cy = cons_top + i * (ch + Inches(0.15))
    add_rect(slide, cx_start, cy, cw, ch, BG_CARD, clr, 0.05)

    # Small coloured dot
    add_circle(slide, cx_start + Inches(0.15), cy + Inches(0.15),
               Inches(0.25), clr)
    add_textbox(slide, cx_start + Inches(0.18), cy + Inches(0.12),
                Inches(0.25), Inches(0.25), "\u2713",
                font_size=12, bold=True, colour=BG, alignment=PP_ALIGN.CENTER)

    add_textbox(slide, cx_start + Inches(0.5), cy + Inches(0.1),
                cw - Inches(0.6), Inches(0.5), title,
                font_size=14, bold=True, colour=clr)
    add_textbox(slide, cx_start + Inches(0.5), cy + Inches(0.65),
                cw - Inches(0.6), Inches(1.2), desc,
                font_size=11, colour=DIM, line_space=1.3)

# ── Crossover / honesty note ──
add_textbox(slide, Inches(0.8), Inches(6.3), Inches(11.5), Inches(0.7),
            [("Where it doesn\u2019t work: ", YELLOW, True),
             ("Write-heavy wins (+55% at 90% writes). Read-heavy loses (\u221232% at 90% reads). "
              "Crossover ~50%. Purpose-built for state-tracking, not general-purpose.", DIM, False)],
            font_size=11)

# ── Zynq footprint note ──
add_textbox(slide, Inches(0.8), Inches(6.85), Inches(11.5), Inches(0.3),
            "Footprint: 287 LUT on Xilinx Zynq (0.54%), 1.8 mW \u2014 vendor-portable RTL (Xilinx + Gowin, zero changes)",
            font_size=11, colour=DIM)

slide_number(slide, 4)


# ═══════════════════════════════════════════════════════════════════
# SLIDE 5 — TRACTION & PROOF
# ═══════════════════════════════════════════════════════════════════
slide = prs.slides.add_slide(prs.slide_layouts[6])
set_bg(slide)

add_textbox(slide, Inches(0.8), Inches(0.4), Inches(10), Inches(0.6),
            "Traction & Engineering Proof", font_size=36, bold=True,
            colour=BLUE)
section_line(slide, Inches(1.05))

# ── KPI Grid (3x3) ──
kpi_data = [
    ("1,056",     "Mops/s",         "Peak throughput (N=16)",     GREEN),
    ("10.6 ns",   "",               "Single-op latency",          BLUE),
    ("92",        "proofs",         "Lean4 verified, 0 sorry",    PURPLE),
    ("80/80",     "",               "Hardware tests passing",     GREEN),
    ("353",       "tests",          "SDK (5 languages)",          TEAL),
    ("95\u2013100%",  "",           "Memory traffic reduction",   YELLOW),
    ("+35.7%",    "",               "XOR vs adder speed (p<.0001)", GREEN),
    ("0%",        "overflow",       "XOR risk (vs 150.7% adder)", GREEN),
    ("~$225",     "total",          "Development cost (AI tokens)", YELLOW),
]

cols_n = 3
kpi_w = Inches(3.6)
kpi_h = Inches(1.15)
kpi_gap = Inches(0.25)
kpi_start_x = Inches(0.8)
kpi_start_y = Inches(1.35)

for idx, (val, unit, desc, clr) in enumerate(kpi_data):
    row = idx // cols_n
    col = idx % cols_n
    kx = kpi_start_x + col * (kpi_w + kpi_gap)
    ky = kpi_start_y + row * (kpi_h + kpi_gap)

    add_rect(slide, kx, ky, kpi_w, kpi_h, BG_CARD, SURFACE, 0.04)
    # Accent bar left
    add_rect(slide, kx, ky, Pt(4), kpi_h, clr)

    display = f"{val} {unit}" if unit else val
    add_textbox(slide, kx + Inches(0.2), ky + Inches(0.1),
                kpi_w - Inches(0.3), Inches(0.5), display,
                font_size=26, bold=True, colour=clr)
    add_textbox(slide, kx + Inches(0.2), ky + Inches(0.65),
                kpi_w - Inches(0.3), Inches(0.4), desc,
                font_size=11, colour=DIM)

# ── Scaling chart (bottom) ──
add_textbox(slide, Inches(0.8), Inches(5.6), Inches(6), Inches(0.4),
            "Parallel Scaling (Verilog-Verified)", font_size=16,
            bold=True, colour=WHITE)

# Simple bar chart using shapes
banks =  [1, 4, 8, 16]
thru  =  [94.5, 324, 540, 1056]
labels = ["N=1\n94.5 MHz", "N=4\n81 MHz", "N=8\n67.5 MHz", "N=16\n66 MHz"]
max_thru = 1056
bar_area_left = Inches(1.0)
bar_area_top = Inches(6.0)
bar_max_h = Inches(1.1)
bar_w = Inches(1.8)

for i, (n, t, lbl) in enumerate(zip(banks, thru, labels)):
    bx = bar_area_left + i * (bar_w + Inches(0.8))
    bh = bar_max_h * (t / max_thru)
    by = bar_area_top + (bar_max_h - bh)

    # Green gradient based on throughput
    g_intensity = int(0xA1 * (t / max_thru)) + 0x30
    g_intensity = min(255, g_intensity)
    add_rect(slide, bx, by, bar_w, bh,
             RGBColor(0x30, g_intensity, 0x50))

    # Value on top
    add_textbox(slide, bx, by - Inches(0.25), bar_w, Inches(0.25),
                f"{int(t)} Mops/s", font_size=10, bold=True,
                colour=GREEN, alignment=PP_ALIGN.CENTER)

    # Label below
    add_textbox(slide, bx, bar_area_top + bar_max_h + Inches(0.02),
                bar_w, Inches(0.35), lbl, font_size=9, colour=DIM,
                alignment=PP_ALIGN.CENTER)

# Linear trendline note
add_textbox(slide, Inches(8.5), Inches(6.2), Inches(4), Inches(0.6),
            [("Linear scaling: N banks = N\u00d7 throughput", GREEN, True),
             ("No diminishing returns", DIM, False)],
            font_size=13)

slide_number(slide, 5)


# ═══════════════════════════════════════════════════════════════════
# SLIDE 6 — MARKET & USE CASES
# ═══════════════════════════════════════════════════════════════════
slide = prs.slides.add_slide(prs.slide_layouts[6])
set_bg(slide)

add_textbox(slide, Inches(0.8), Inches(0.4), Inches(10), Inches(0.6),
            "Market & Use Cases", font_size=36, bold=True, colour=BLUE)
section_line(slide, Inches(1.05))

# ── TAM/SAM/SOM concentric circles (left) ──
# Use nested rectangles as approximation of concentric circles
cx_center = Inches(3.0)
cy_center = Inches(3.8)

# TAM - outermost
tam_r = Inches(2.3)
s = add_circle(slide, cx_center - tam_r, cy_center - tam_r,
               tam_r * 2, RGBColor(0x1E, 0x28, 0x3E))
s.line.color.rgb = BLUE
s.line.width = Pt(2)
add_textbox(slide, cx_center - tam_r, cy_center - tam_r + Inches(0.15),
            tam_r * 2, Inches(0.3), "TAM ~$85B",
            font_size=14, bold=True, colour=BLUE, alignment=PP_ALIGN.CENTER)

# SAM
sam_r = Inches(1.5)
s = add_circle(slide, cx_center - sam_r, cy_center - sam_r,
               sam_r * 2, RGBColor(0x1E, 0x2E, 0x28))
s.line.color.rgb = TEAL
s.line.width = Pt(2)
add_textbox(slide, cx_center - sam_r, cy_center - sam_r + Inches(0.15),
            sam_r * 2, Inches(0.3), "SAM ~$8B",
            font_size=13, bold=True, colour=TEAL, alignment=PP_ALIGN.CENTER)

# SOM
som_r = Inches(0.65)
s = add_circle(slide, cx_center - som_r, cy_center - som_r,
               som_r * 2, RGBColor(0x2A, 0x3A, 0x1E))
s.line.color.rgb = GREEN
s.line.width = Pt(2)
add_textbox(slide, cx_center - som_r, cy_center - som_r + Inches(0.08),
            som_r * 2, Inches(0.55),
            [("SOM", GREEN, True), ("$15\u2013$82.5M (Yr 5)", GREEN, False)],
            font_size=11, alignment=PP_ALIGN.CENTER)

# TAM breakdown labels
breakdown_x = Inches(0.6)
breakdown_y = Inches(6.0)
add_textbox(slide, breakdown_x, breakdown_y, Inches(5.2), Inches(0.8),
            [("FPGA: $10\u201314B  \u2022  Edge: $61B  \u2022  AI HW Accel: $10.2B", DIM, False),
             ("Sources: MarketsandMarkets, Mordor Intelligence, GlobeNewsWire", DIM, False)],
            font_size=9)

# ── Beachhead cards (right) ──
beachheads = [
    ("Edge Sensor\nFusion",
     "1.8 mW, 287 LUT (0.54%)\nLock-free multi-stream merge\nLinear scaling with sensors",
     TEAL,
     "Primary beachhead"),
    ("Industrial /\nDefense RT",
     "\u22642-cycle jitter, no side channels\nFormally verified (92 proofs)\nSafety-critical viable",
     PURPLE,
     "Higher ACV"),
    ("HFT / Low-\nLatency Infra",
     "10.6 ns tick processing\nInstant trade reversal\nLock-free parallel streams",
     RGBColor(0xF3, 0x8B, 0xA8),
     "Highest value"),
]

bh_w = Inches(3.5)
bh_h = Inches(1.55)
bh_x = Inches(6.2)

for i, (title, desc, clr, mkt) in enumerate(beachheads):
    by = Inches(1.35) + i * (bh_h + Inches(0.2))
    add_rect(slide, bh_x, by, bh_w, bh_h, BG_CARD, SURFACE, 0.04)
    add_rect(slide, bh_x, by, Pt(4), bh_h, clr)

    add_textbox(slide, bh_x + Inches(0.2), by + Inches(0.08),
                Inches(2.2), Inches(0.5), title,
                font_size=14, bold=True, colour=clr, line_space=1.1)
    add_textbox(slide, bh_x + Inches(0.2), by + Inches(0.55),
                bh_w - Inches(0.4), Inches(0.9), desc,
                font_size=10, colour=DIM, line_space=1.3)
    # Market badge
    add_textbox(slide, bh_x + Inches(2.3), by + Inches(0.08),
                Inches(1.1), Inches(0.25), mkt,
                font_size=9, colour=clr, alignment=PP_ALIGN.RIGHT)

# SOM derivation
add_textbox(slide, Inches(6.2), Inches(6.15), Inches(6), Inches(0.6),
            [("3 scenarios: Conservative $15M / Base $35.2M / Bull $82.5M (Y5, bottom-up unit economics)",
              DIM, False)],
            font_size=10)

slide_number(slide, 6)


# ═══════════════════════════════════════════════════════════════════
# SLIDE 7 — BUSINESS MODEL
# ═══════════════════════════════════════════════════════════════════
slide = prs.slides.add_slide(prs.slide_layouts[6])
set_bg(slide)

add_textbox(slide, Inches(0.8), Inches(0.4), Inches(10), Inches(0.6),
            "Business Model", font_size=36, bold=True, colour=BLUE)
add_textbox(slide, Inches(0.8), Inches(0.95), Inches(10), Inches(0.4),
            "ARM-style IP licensing at 90%+ gross margin",
            font_size=16, colour=DIM)
section_line(slide, Inches(1.4))

# ── Revenue streams ──
streams = [
    ("RTL IP\nLicensing",
     "Per-design fee +\nper-unit royalty",
     "90%+",
     "Primary revenue.\nARM Cortex-M comparable.",
     BLUE),
    ("Vertical\nAccelerator IP",
     "Pre-built domain\nmodules (sensor, industrial)",
     "85%+",
     "Higher value.\nDomain-specific RTL.",
     PURPLE),
    ("SDK\nPlatform",
     "Annual subscription\nfor codegen + support",
     "90%+",
     "Recurring revenue.\nDeveloper lock-in.",
     GREEN),
    ("Professional\nServices",
     "Custom integration\nengagements",
     "50\u201360%",
     "Strategic land.\nLearning use cases.",
     YELLOW),
]

sw = Inches(2.7)
sh = Inches(2.8)
sx_start = Inches(0.8)
sy = Inches(1.65)

for i, (title, model, margin, note, clr) in enumerate(streams):
    sx = sx_start + i * (sw + Inches(0.25))
    add_rect(slide, sx, sy, sw, sh, BG_CARD, SURFACE, 0.04)
    add_rect(slide, sx, sy, sw, Pt(4), clr)

    add_textbox(slide, sx + Inches(0.15), sy + Inches(0.15),
                sw - Inches(0.3), Inches(0.5), title,
                font_size=15, bold=True, colour=clr,
                alignment=PP_ALIGN.CENTER, line_space=1.1)

    add_textbox(slide, sx + Inches(0.15), sy + Inches(0.75),
                sw - Inches(0.3), Inches(0.5), model,
                font_size=11, colour=WHITE, alignment=PP_ALIGN.CENTER,
                line_space=1.2)

    # Margin badge
    badge_w = Inches(1.2)
    badge_x = sx + (sw - badge_w) / 2
    add_rect(slide, badge_x, sy + Inches(1.4), badge_w, Inches(0.35),
             RGBColor(clr[0] // 4, clr[1] // 4, clr[2] // 4),
             clr, 0.08)
    add_textbox(slide, badge_x, sy + Inches(1.4), badge_w, Inches(0.35),
                f"{margin} GM", font_size=12, bold=True, colour=clr,
                alignment=PP_ALIGN.CENTER)

    add_textbox(slide, sx + Inches(0.15), sy + Inches(1.9),
                sw - Inches(0.3), Inches(0.8), note,
                font_size=10, colour=DIM, alignment=PP_ALIGN.CENTER,
                line_space=1.3)

# ── ARM comparable bar ──
add_rect(slide, Inches(0.8), Inches(4.75), Inches(11.5), Inches(1.3),
         RGBColor(0x1A, 0x1F, 0x2E), SURFACE, 0.03)

add_textbox(slide, Inches(1.1), Inches(4.85), Inches(5), Inches(0.35),
            "Comparable: ARM Holdings (FY2025)", font_size=14,
            bold=True, colour=WHITE)

arm_metrics = [
    ("$4.0B", "Revenue", BLUE),
    ("97%", "Gross Margin", GREEN),
    ("Pure IP", "No Manufacturing", PURPLE),
    ("30+ Years", "Model Proven", YELLOW),
]

am_w = Inches(2.4)
am_x = Inches(1.1)
am_y = Inches(5.3)

for i, (val, lbl, clr) in enumerate(arm_metrics):
    ax = am_x + i * (am_w + Inches(0.3))
    add_textbox(slide, ax, am_y, am_w, Inches(0.35), val,
                font_size=20, bold=True, colour=clr)
    add_textbox(slide, ax, am_y + Inches(0.35), am_w, Inches(0.25), lbl,
                font_size=11, colour=DIM)

# Margin trajectory
add_textbox(slide, Inches(0.8), Inches(6.3), Inches(11.5), Inches(0.5),
            [("Blended margin trajectory:  Year 1: ~75%  \u2192  Year 3: ~85%  \u2192  Year 5: ~88%",
              DIM, False),
             ("Improves as services % declines and SDK recurring revenue grows",
              DIM, False)],
            font_size=11)

slide_number(slide, 7)


# ═══════════════════════════════════════════════════════════════════
# SLIDE 8 — COMPETITIVE LANDSCAPE
# ═══════════════════════════════════════════════════════════════════
slide = prs.slides.add_slide(prs.slide_layouts[6])
set_bg(slide)

add_textbox(slide, Inches(0.8), Inches(0.4), Inches(10), Inches(0.6),
            "Competitive Landscape", font_size=36, bold=True, colour=BLUE)
section_line(slide, Inches(1.05))

# ── Technical comparison table ──
add_textbox(slide, Inches(0.8), Inches(1.2), Inches(6), Inches(0.4),
            "Technical Comparison", font_size=16, bold=True, colour=WHITE)

comp_data = [
    ["Dimension", "ATOMiK", "Event\nSourcing", "CRDTs", "GPU", "Near-\nMemory"],
    ["Reconstruction", "O(1)/cycle", "O(N) replay", "O(N) merge", "Batch", "O(1) local"],
    ["Formal Proofs", "108 Lean4", "None", "Manual", "None", "None"],
    ["Undo/Reversal", "Free", "Log replay", "N/A", "N/A", "N/A"],
    ["Parallelism", "Linear", "Limited", "Eventual", "SIMT", "Ctrl-bound"],
    ["Power", "~20 mW", "50\u2013200W", "50\u2013200W", "300\u2013700W", "10\u201350W"],
    ["Latency", "10.6 ns", "ms-scale", "ms-scale", "\u00b5s-scale", "ns-scale"],
]

add_table(slide, Inches(0.8), Inches(1.55), Inches(11.5), Inches(2.6),
          7, 6, comp_data,
          col_widths=[Inches(1.8), Inches(1.7), Inches(1.8), Inches(1.6),
                      Inches(1.7), Inches(1.6)])

# ── Comparable companies ──
add_textbox(slide, Inches(0.8), Inches(4.35), Inches(6), Inches(0.4),
            "Market Comparables", font_size=16, bold=True, colour=WHITE)

comp_companies = [
    ["Company", "Valuation / Revenue", "Model", "Source"],
    ["ARM Holdings", "$4.0B rev, 97% GM", "IP license + royalty", "SEC filing, FY2025"],
    ["Lattice Semi", "$489M rev, 21.6\u00d7 EV/Rev", "Low-power FPGA", "Earnings, 2025"],
    ["Cerebras", "$8.1B valuation", "AI chip hardware", "Crunchbase, Sep 2025"],
    ["Groq", "$20B (NVIDIA acq.)", "Inference accelerator", "TechCrunch, Dec 2025"],
    ["Tenstorrent", "$2.6B valuation", "RISC-V AI accelerator", "Fortune, Dec 2024"],
    ["Positron AI", "$51.6M Series A", "FPGA inference", "Crunchbase, 2025"],
]

add_table(slide, Inches(0.8), Inches(4.7), Inches(11.5), Inches(2.5),
          7, 4, comp_companies,
          col_widths=[Inches(2.2), Inches(3.5), Inches(3.0), Inches(2.8)])

slide_number(slide, 8)


# ═══════════════════════════════════════════════════════════════════
# SLIDE 9 — ROADMAP
# ═══════════════════════════════════════════════════════════════════
slide = prs.slides.add_slide(prs.slide_layouts[6])
set_bg(slide)

add_textbox(slide, Inches(0.8), Inches(0.4), Inches(10), Inches(0.6),
            "Roadmap", font_size=36, bold=True, colour=BLUE)
add_textbox(slide, Inches(0.8), Inches(0.95), Inches(10), Inches(0.4),
            "From proof-of-concept to production silicon",
            font_size=16, colour=DIM)
section_line(slide, Inches(1.4))

# ── Completed phases (timeline) ──
phases_done = [
    ("Phase 1", "Math\nFormalization", "108 Lean4\nproofs", PURPLE),
    ("Phase 2", "SCORE\nBenchmarks", "95\u2013100%\nmem reduction", BLUE),
    ("Phase 3", "Hardware\nSynthesis", "94.5 MHz\n80/80 tests", TEAL),
    ("Phase 4", "SDK\nGeneration", "5 languages\n353 tests", GREEN),
    ("Phase 5", "3-Node\nDemo", "Multi-device\nmerge", YELLOW),
    ("Phase 6", "Parallel\nScaling", "1,056 Mops/s\n16\u00d7 linear", GREEN),
]

tl_y = Inches(1.7)
tl_h = Inches(2.1)
node_w = Inches(1.65)
node_gap = Inches(0.2)
tl_start = Inches(0.6)

# Timeline line
add_rect(slide, tl_start, tl_y + Inches(0.3),
         Inches(11.5), Pt(3), SURFACE)

for i, (phase, name, result, clr) in enumerate(phases_done):
    nx = tl_start + i * (node_w + node_gap)

    # Node dot on timeline
    dot = add_circle(slide, nx + node_w / 2 - Inches(0.15),
                     tl_y + Inches(0.18), Inches(0.3), clr)

    # Checkmark
    add_textbox(slide, nx + node_w / 2 - Inches(0.15),
                tl_y + Inches(0.16), Inches(0.3), Inches(0.3),
                "\u2713", font_size=12, bold=True, colour=BG,
                alignment=PP_ALIGN.CENTER)

    # Phase label
    add_textbox(slide, nx, tl_y + Inches(0.55), node_w, Inches(0.25),
                phase, font_size=10, bold=True, colour=clr,
                alignment=PP_ALIGN.CENTER)

    # Name
    add_textbox(slide, nx, tl_y + Inches(0.8), node_w, Inches(0.5),
                name, font_size=11, colour=WHITE,
                alignment=PP_ALIGN.CENTER, line_space=1.1)

    # Result
    add_textbox(slide, nx, tl_y + Inches(1.35), node_w, Inches(0.5),
                result, font_size=9, colour=DIM,
                alignment=PP_ALIGN.CENTER, line_space=1.15)

# ── Funding inflection ──
inflection_x = tl_start + 6 * (node_w + node_gap) - Inches(0.15)
# Vertical dashed-line effect
for j in range(8):
    add_rect(slide, inflection_x, tl_y + Inches(j * 0.3),
             Pt(2), Inches(0.15), YELLOW)
add_textbox(slide, inflection_x - Inches(0.4), tl_y - Inches(0.15),
            Inches(1.2), Inches(0.25), "SEED ROUND",
            font_size=10, bold=True, colour=YELLOW,
            alignment=PP_ALIGN.CENTER)

# ── Forward milestones ──
forward = [
    ("Larger FPGA", "N=64+ banks\n>4 Gops/s target", BLUE),
    ("Vertical SDK\nModules", "Sensor fusion,\nindustrial packages", PURPLE),
    ("ASIC Feasibility", "$150K\u2013$300K study\ngo/no-go gates", TEAL),
    ("Paid Pilots", "2\u20133 edge embedded\n$50K\u2013$200K NRE", GREEN),
]

fw_y = Inches(4.3)
fw_w = Inches(2.6)
fw_h = Inches(1.4)
fw_start = Inches(0.8)

# Forward timeline line
add_rect(slide, fw_start, fw_y + Inches(0.2),
         Inches(11.3), Pt(3), SURFACE)

for i, (name, desc, clr) in enumerate(forward):
    fx = fw_start + i * (fw_w + Inches(0.3))

    # Open circle (not filled) for future
    dot = add_circle(slide, fx + fw_w / 2 - Inches(0.15),
                     fw_y + Inches(0.08), Inches(0.3), BG_CARD)
    dot.line.color.rgb = clr
    dot.line.width = Pt(2)

    # Number
    add_textbox(slide, fx + fw_w / 2 - Inches(0.15),
                fw_y + Inches(0.06), Inches(0.3), Inches(0.3),
                str(i + 1), font_size=12, bold=True, colour=clr,
                alignment=PP_ALIGN.CENTER)

    add_textbox(slide, fx, fw_y + Inches(0.45), fw_w, Inches(0.5),
                name, font_size=13, bold=True, colour=clr,
                alignment=PP_ALIGN.CENTER, line_space=1.1)
    add_textbox(slide, fx, fw_y + Inches(0.95), fw_w, Inches(0.5),
                desc, font_size=10, colour=DIM,
                alignment=PP_ALIGN.CENTER, line_space=1.2)

# Risk labels
add_textbox(slide, Inches(0.8), Inches(5.9), Inches(11.5), Inches(0.4),
            [("Risk profile:  ", DIM, False)],
            font_size=11)
add_textbox(slide, Inches(2.1), Inches(5.9), Inches(9), Inches(0.4),
            [("Larger FPGA = Low", GREEN, False)],
            font_size=11)
add_textbox(slide, Inches(4.5), Inches(5.9), Inches(7), Inches(0.4),
            [("  \u2022  SDK Modules = Low", GREEN, False)],
            font_size=11)
add_textbox(slide, Inches(7.3), Inches(5.9), Inches(5), Inches(0.4),
            [("  \u2022  ASIC = Medium", YELLOW, False)],
            font_size=11)
add_textbox(slide, Inches(9.8), Inches(5.9), Inches(3), Inches(0.4),
            [("  \u2022  Pilots = Medium", YELLOW, False)],
            font_size=11)

# Bottom note
add_textbox(slide, Inches(0.8), Inches(6.4), Inches(11.5), Inches(0.5),
            "6 engineering phases complete with validation at every gate. "
            "Seed funds the transition from proof-of-concept to first design wins.",
            font_size=12, colour=DIM)

slide_number(slide, 9)


# ═══════════════════════════════════════════════════════════════════
# SLIDE 10 — THE ASK
# ═══════════════════════════════════════════════════════════════════
slide = prs.slides.add_slide(prs.slide_layouts[6])
set_bg(slide)

add_textbox(slide, Inches(0.8), Inches(0.4), Inches(10), Inches(0.6),
            "The Ask", font_size=36, bold=True, colour=BLUE)
add_textbox(slide, Inches(0.8), Inches(0.95), Inches(10), Inches(0.4),
            "Seed round to bridge from proof-of-concept to first design wins",
            font_size=16, colour=DIM)
section_line(slide, Inches(1.4))

# ── Use of Funds (pie chart as segmented rectangles) ──
add_textbox(slide, Inches(0.8), Inches(1.6), Inches(5), Inches(0.4),
            "Use of Funds", font_size=18, bold=True, colour=WHITE)

funds = [
    ("Hardware R&D", "40%", "Larger FPGA port, ASIC feasibility,\ndev boards",
     BLUE, 0.40),
    ("SDK & Platform", "25%", "Production hardening, vertical\nmodules, docs",
     GREEN, 0.25),
    ("Business Dev", "20%", "Patent prosecution, partnerships,\npilot support",
     PURPLE, 0.20),
    ("Team", "15%", "FPGA engineer,\napplication engineer",
     YELLOW, 0.15),
]

# Horizontal segmented bar
bar_y = Inches(2.05)
bar_h = Inches(0.6)
bar_total_w = Inches(10.5)
bar_x = Inches(0.8)
offset = 0

for name, pct, desc, clr, frac in funds:
    seg_w = int(bar_total_w * frac)
    add_rect(slide, bar_x + int(bar_total_w * offset), bar_y,
             seg_w, bar_h, clr)
    # Percentage label on bar
    if frac >= 0.15:
        add_textbox(slide, bar_x + int(bar_total_w * offset), bar_y + Inches(0.1),
                    seg_w, Inches(0.4), f"{pct}",
                    font_size=14, bold=True, colour=BG,
                    alignment=PP_ALIGN.CENTER)
    offset += frac

# Cards below bar
fund_w = Inches(2.5)
fund_h = Inches(1.3)
fund_y = Inches(2.85)

for i, (name, pct, desc, clr, frac) in enumerate(funds):
    fx = Inches(0.8) + i * (fund_w + Inches(0.3))
    add_rect(slide, fx, fund_y, fund_w, fund_h, BG_CARD, SURFACE, 0.04)
    add_rect(slide, fx, fund_y, fund_w, Pt(3), clr)

    add_textbox(slide, fx + Inches(0.1), fund_y + Inches(0.08),
                fund_w - Inches(0.2), Inches(0.3),
                f"{name} ({pct})", font_size=12, bold=True, colour=clr)
    add_textbox(slide, fx + Inches(0.1), fund_y + Inches(0.4),
                fund_w - Inches(0.2), Inches(0.8), desc,
                font_size=10, colour=DIM, line_space=1.3)

# ── Milestones ──
add_textbox(slide, Inches(0.8), Inches(4.4), Inches(5), Inches(0.4),
            "Key Milestones (Seed Stage)", font_size=18, bold=True,
            colour=WHITE)

milestones = [
    ("Port to mid-range FPGA: N=64+ banks, >4 Gops/s", GREEN),
    ("Land 2 paid evaluations ($50K\u2013$200K NRE) in edge embedded", GREEN),
    ("ASIC feasibility study ($150K\u2013$300K) \u2014 feasibility only, not tapeout", TEAL),
    ("Hire FPGA/ASIC engineer + application engineer (15% of raise)", BLUE),
    ("Seat advisory board within 60 days", PURPLE),
]

for i, (ms, clr) in enumerate(milestones):
    my = Inches(4.85) + i * Inches(0.32)
    add_circle(slide, Inches(1.0), my + Inches(0.03), Inches(0.18), clr)
    add_textbox(slide, Inches(1.0), my + Inches(0.01), Inches(0.18), Inches(0.18),
                str(i + 1), font_size=8, bold=True, colour=BG,
                alignment=PP_ALIGN.CENTER)
    add_textbox(slide, Inches(1.35), my, Inches(5), Inches(0.3),
                ms, font_size=12, colour=WHITE)

# ── Right panel: Why fundable now ──
panel_x = Inches(7)
panel_w = Inches(5.5)
add_rect(slide, panel_x, Inches(4.4), panel_w, Inches(2.6),
         RGBColor(0x1A, 0x2A, 0x1A), GREEN, 0.03)

add_textbox(slide, panel_x + Inches(0.2), Inches(4.5),
            panel_w - Inches(0.4), Inches(0.35),
            "Why This Is Fundable Now", font_size=16, bold=True,
            colour=GREEN)

fundable = [
    ("\u2713  Technical risk retired", "92 proofs + working silicon + 353 tests"),
    ("\u2713  Market timing", "$300B+ hyperscaler AI capex cycle in progress"),
    ("\u2713  Capital efficiency", "~$225 total spend \u2192 1 Gops/s hardware"),
    ("\u2713  Clear IP moat", "Patent pending + formal verification barrier"),
]

for i, (title, desc) in enumerate(fundable):
    fy = Inches(4.95) + i * Inches(0.48)
    add_textbox(slide, panel_x + Inches(0.2), fy,
                panel_w - Inches(0.4), Inches(0.25),
                title, font_size=13, bold=True, colour=GREEN)
    add_textbox(slide, panel_x + Inches(0.45), fy + Inches(0.22),
                panel_w - Inches(0.6), Inches(0.25),
                desc, font_size=10, colour=DIM)

# ── Closing tagline ──
add_rect(slide, Inches(0), SLIDE_H - Inches(0.65), SLIDE_W, Inches(0.65),
         RGBColor(0x10, 0x10, 0x18))
add_textbox(slide, Inches(1), SLIDE_H - Inches(0.6), Inches(6), Inches(0.5),
            "ATOMiK \u2014 Delta-State Computing in Silicon",
            font_size=16, bold=True, colour=BLUE)
add_textbox(slide, Inches(7), SLIDE_H - Inches(0.6), Inches(5.5), Inches(0.5),
            "$225 to 1 Gops/s. Imagine what funded execution looks like.",
            font_size=14, colour=GREEN, alignment=PP_ALIGN.RIGHT)

slide_number(slide, 10)


# ═══════════════════════════════════════════════════════════════════
# SAVE
# ═══════════════════════════════════════════════════════════════════
import os
out_dir = os.path.dirname(os.path.abspath(__file__))
out_path = os.path.join(out_dir, "ATOMiK_Investor_Deck.pptx")
prs.save(out_path)
print(f"Saved: {out_path}")
print(f"Slides: {len(prs.slides)}")
