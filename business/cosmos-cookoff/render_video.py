#!/usr/bin/env python3
"""
ATOMiK Cosmos Cookoff Demo Video Generator

Renders a presentation-style demo video using Pillow + ffmpeg.
Output: 1920x1080 @ 30fps, H.264, ~2:45 duration
"""

import os
import subprocess
import math
from PIL import Image, ImageDraw, ImageFont

# --- Config ---
W, H = 1920, 1080
FPS = 30
OUT_DIR = os.path.join(os.path.dirname(__file__), "frames")
VIDEO_OUT = os.path.join(os.path.dirname(__file__), "atomik_cosmos_cookoff_demo.mp4")

# Colors
BG = (15, 15, 25)          # Dark navy
BG2 = (20, 25, 40)         # Slightly lighter
ACCENT = (0, 200, 120)     # ATOMiK green
ACCENT2 = (0, 160, 255)    # Blue
ACCENT3 = (255, 140, 0)    # Orange
WHITE = (255, 255, 255)
GRAY = (160, 160, 170)
DIM = (80, 80, 90)
RED = (255, 70, 70)
COSMOS = (118, 185, 0)     # NVIDIA green

# Fonts
def font(size, bold=False):
    name = "DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf"
    return ImageFont.truetype(f"/usr/share/fonts/truetype/dejavu/{name}", size)

FONT_TITLE = font(72, bold=True)
FONT_SUBTITLE = font(36)
FONT_HEADING = font(48, bold=True)
FONT_BODY = font(32)
FONT_BODY_SM = font(26)
FONT_CODE = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 28)
FONT_CODE_SM = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 22)
FONT_METRIC = font(64, bold=True)
FONT_LABEL = font(28)
FONT_SMALL = font(22)

os.makedirs(OUT_DIR, exist_ok=True)

frame_num = 0

def new_frame(bg=BG):
    return Image.new("RGB", (W, H), bg)

def save_frame(img):
    global frame_num
    img.save(os.path.join(OUT_DIR, f"frame_{frame_num:05d}.png"))
    frame_num += 1

def hold(img, seconds):
    for _ in range(int(seconds * FPS)):
        save_frame(img)

def centered_text(draw, y, text, fnt, fill=WHITE):
    bbox = draw.textbbox((0, 0), text, font=fnt)
    tw = bbox[2] - bbox[0]
    draw.text(((W - tw) // 2, y), text, font=fnt, fill=fill)

def draw_box(draw, x, y, w, h, fill=None, outline=ACCENT, width=2, radius=12):
    if fill:
        draw.rounded_rectangle([x, y, x+w, y+h], radius=radius, fill=fill, outline=outline, width=width)
    else:
        draw.rounded_rectangle([x, y, x+w, y+h], radius=radius, outline=outline, width=width)

def draw_arrow(draw, x1, y1, x2, y2, fill=GRAY, width=3):
    draw.line([(x1, y1), (x2, y2)], fill=fill, width=width)
    # Arrowhead
    angle = math.atan2(y2-y1, x2-x1)
    arrow_len = 12
    for da in [2.5, -2.5]:
        ax = x2 - arrow_len * math.cos(angle + da)
        ay = y2 - arrow_len * math.sin(angle + da)
        draw.line([(x2, y2), (int(ax), int(ay))], fill=fill, width=width)

def fade_in(img, frames=15):
    """Fade from black to img"""
    black = Image.new("RGB", (W, H), (0, 0, 0))
    for i in range(frames):
        alpha = (i + 1) / frames
        blended = Image.blend(black, img, alpha)
        save_frame(blended)

def fade_out(img, frames=15):
    """Fade from img to black"""
    black = Image.new("RGB", (W, H), (0, 0, 0))
    for i in range(frames):
        alpha = 1.0 - (i + 1) / frames
        blended = Image.blend(black, img, alpha)
        save_frame(blended)

def cross_fade(img1, img2, frames=20):
    """Cross-fade between two images"""
    for i in range(frames):
        alpha = (i + 1) / frames
        blended = Image.blend(img1, img2, alpha)
        save_frame(blended)


# =========================================================================
# SCENE 1: Title Card (0:00 - 0:08)
# =========================================================================
def scene_title():
    img = new_frame()
    draw = ImageDraw.Draw(img)

    # ATOMiK logo text
    centered_text(draw, 280, "ATOMiK", FONT_TITLE, ACCENT)
    centered_text(draw, 380, "Hardware-Verified State Reasoning", FONT_SUBTITLE, WHITE)
    centered_text(draw, 430, "for Physical AI", FONT_SUBTITLE, WHITE)

    # Separator line
    draw.line([(W//2 - 200, 500), (W//2 + 200, 500)], fill=ACCENT, width=2)

    # Subtitle
    centered_text(draw, 540, "NVIDIA Cosmos Cookoff 2026", FONT_BODY, GRAY)
    centered_text(draw, 590, "Matthew Rockwell", FONT_BODY_SM, DIM)

    # Bottom: board price
    centered_text(draw, 750, "$13.50 FPGA  |  108 Lean4 Proofs  |  RV64I + ATOMiK ISA", FONT_BODY_SM, DIM)

    fade_in(img, 20)
    hold(img, 6)
    return img


# =========================================================================
# SCENE 2: The Problem (0:08 - 0:25)
# =========================================================================
def scene_problem():
    img = new_frame()
    draw = ImageDraw.Draw(img)

    centered_text(draw, 40, "The Problem", FONT_HEADING, WHITE)

    # Left: Traditional approach
    draw.text((100, 140), "Traditional State Verification", font=FONT_BODY, fill=RED)

    problems = [
        "Full memory buffer copies",
        "Software checksums (variable latency)",
        "Timing side channels",
        "O(n) per verification",
        "Cache coherency attacks possible",
    ]
    for i, p in enumerate(problems):
        y = 210 + i * 48
        draw.text((120, y), f"\u2717  {p}", font=FONT_BODY_SM, fill=(200, 100, 100))

    # Right: ATOMiK approach
    draw.text((1000, 140), "ATOMiK Delta-State Algebra", font=FONT_BODY, fill=ACCENT)

    solutions = [
        "State reconstructed, never stored",
        "XOR accumulator (deterministic latency)",
        "No timing side channels",
        "O(1) change detection",
        "Mathematically proven (108 theorems)",
    ]
    for i, s in enumerate(solutions):
        y = 210 + i * 48
        draw.text((1020, y), f"\u2713  {s}", font=FONT_BODY_SM, fill=(100, 220, 140))

    # Center divider
    draw.line([(W//2, 140), (W//2, 470)], fill=DIM, width=1)

    # Core equation
    draw_box(draw, W//2 - 400, 510, 800, 90, fill=(30, 35, 50), outline=ACCENT)
    centered_text(draw, 525, "current_state = initial_state \u2295 accumulator", FONT_CODE, ACCENT)

    # Properties
    props = [
        ("Commutative", "Order doesn't matter"),
        ("Associative", "Grouping doesn't matter"),
        ("Self-Inverse", "Undo = same operation"),
        ("Identity", "XOR 0 = no change"),
    ]
    for i, (name, desc) in enumerate(props):
        x = 200 + i * 400
        y = 650
        draw_box(draw, x - 20, y, 360, 80, fill=(25, 30, 45), outline=DIM)
        draw.text((x, y + 8), name, font=FONT_BODY_SM, fill=ACCENT)
        draw.text((x, y + 42), desc, font=FONT_SMALL, fill=GRAY)

    # Bottom: Lean4
    centered_text(draw, 790, "All properties formally verified: 108 theorems in Lean4", FONT_BODY_SM, DIM)

    return img


# =========================================================================
# SCENE 3: Architecture Diagram (0:25 - 0:45)
# =========================================================================
def scene_architecture():
    img = new_frame()
    draw = ImageDraw.Draw(img)

    centered_text(draw, 30, "Dual-PLL SoC Architecture", FONT_HEADING, WHITE)
    centered_text(draw, 90, "Tang Nano 9K  \u2014  Gowin GW1NR-9  \u2014  $13.50", FONT_BODY_SM, DIM)

    # CPU Domain box
    draw_box(draw, 80, 180, 800, 500, fill=(25, 30, 50), outline=ACCENT2)
    draw.text((100, 190), "CPU Domain (21.6 MHz)", font=FONT_BODY, fill=ACCENT2)

    # CPU block
    draw_box(draw, 120, 260, 350, 70, fill=(35, 45, 65), outline=ACCENT2)
    draw.text((150, 275), "RV64I CPU + ATOMiK ISA", font=FONT_BODY_SM, fill=WHITE)

    # SRAM block
    draw_box(draw, 120, 360, 350, 60, fill=(35, 45, 65), outline=ACCENT2)
    draw.text((180, 372), "SRAM (16 KB)", font=FONT_BODY_SM, fill=WHITE)

    # UART block
    draw_box(draw, 120, 450, 160, 60, fill=(35, 45, 65), outline=ACCENT2)
    draw.text((155, 462), "UART", font=FONT_BODY_SM, fill=WHITE)

    # SPI Flash block
    draw_box(draw, 310, 450, 160, 60, fill=(35, 45, 65), outline=ACCENT2)
    draw.text((330, 462), "SPI Flash", font=FONT_BODY_SM, fill=WHITE)

    # ATOMiK block inside CPU
    draw_box(draw, 520, 260, 330, 70, fill=(20, 50, 35), outline=ACCENT)
    draw.text((540, 275), "ATOMiK Core (XOR)", font=FONT_BODY_SM, fill=ACCENT)

    # Display MMIO
    draw_box(draw, 520, 360, 330, 60, fill=(35, 45, 65), outline=ACCENT2)
    draw.text((545, 372), "Display MMIO (S3)", font=FONT_BODY_SM, fill=WHITE)

    # CDC Bridge
    draw_box(draw, 900, 350, 120, 80, fill=(50, 40, 20), outline=ACCENT3)
    draw.text((910, 365), "  CDC", font=FONT_BODY_SM, fill=ACCENT3)
    draw.text((910, 395), "Bridge", font=FONT_BODY_SM, fill=ACCENT3)

    # Arrow from Display MMIO to CDC
    draw_arrow(draw, 850, 390, 900, 390, fill=ACCENT3)

    # Pixel Domain box
    draw_box(draw, 1040, 180, 800, 500, fill=(25, 20, 40), outline=(180, 0, 200))
    draw.text((1060, 190), "Pixel Domain (25.2 MHz)", font=FONT_BODY, fill=(200, 100, 255))

    # Arrow from CDC to pixel domain
    draw_arrow(draw, 1020, 390, 1040, 390, fill=ACCENT3)

    # Video pipeline
    pipeline = [
        ("svo_tcard", "Test Card Gen", 1080, 270),
        ("svo_overlay", "Text Overlay", 1080, 360),
        ("delta_display", "ATOMiK Delta", 1080, 450),
        ("svo_enc", "Blanking+Sync", 1080, 540),
    ]
    for name, desc, x, y in pipeline:
        color = ACCENT if "ATOMiK" in desc else (200, 100, 255)
        draw_box(draw, x, y, 300, 55, fill=(40, 30, 55), outline=color)
        draw.text((x + 10, y + 5), name, font=FONT_CODE_SM, fill=color)
        draw.text((x + 10, y + 30), desc, font=FONT_SMALL, fill=GRAY)

    # Arrows between pipeline stages
    for i in range(len(pipeline) - 1):
        _, _, x, y1 = pipeline[i]
        _, _, _, y2 = pipeline[i+1]
        draw_arrow(draw, x + 150, y1 + 55, x + 150, y2, fill=DIM)

    # TMDS/OSER10 block
    draw_box(draw, 1450, 360, 200, 70, fill=(40, 30, 55), outline=(200, 100, 255))
    draw.text((1470, 370), "TMDS + OSER10", font=FONT_BODY_SM, fill=(200, 100, 255))
    draw.text((1470, 400), "5x serializer", font=FONT_SMALL, fill=GRAY)

    # Arrow from enc to TMDS
    draw_arrow(draw, 1380, 567, 1550, 430, fill=DIM)

    # HDMI output
    draw_box(draw, 1680, 360, 130, 70, fill=(60, 30, 30), outline=RED)
    draw.text((1700, 375), "HDMI", font=FONT_BODY, fill=RED)
    draw_arrow(draw, 1650, 395, 1680, 395, fill=RED)

    # PLL labels
    draw_box(draw, 100, 700, 220, 50, fill=(25, 30, 50), outline=ACCENT2)
    draw.text((115, 710), "PLL1: 108 MHz", font=FONT_BODY_SM, fill=ACCENT2)

    draw_box(draw, 380, 700, 220, 50, fill=(25, 20, 40), outline=(180, 0, 200))
    draw.text((395, 710), "PLL2: 126 MHz", font=FONT_BODY_SM, fill=(200, 100, 255))

    # Resource usage
    resources = "6,174 LUT (71%)  |  3,809 CLS (89%)  |  19 BSRAM (74%)  |  2/2 rPLL"
    centered_text(draw, 800, resources, FONT_BODY_SM, DIM)

    # Timing
    timing = "CPU Fmax: 21.99 MHz (+1.8%)  |  Pixel Fmax: 33.96 MHz (+34.8%)  |  0 TNS"
    centered_text(draw, 840, timing, FONT_BODY_SM, ACCENT)

    return img


# =========================================================================
# SCENE 4: Performance Metrics (0:45 - 1:15)
# =========================================================================
def scene_performance():
    img = new_frame()
    draw = ImageDraw.Draw(img)

    centered_text(draw, 30, "Hardware-Validated Performance", FONT_HEADING, WHITE)
    centered_text(draw, 90, "Measured on Tang Nano 9K  \u2014  21.6 MHz CPU  \u2014  Single ATOMiK Bank", FONT_BODY_SM, DIM)

    # Metric cards
    metrics = [
        ("76-80%", "Faster Change Detection", "vs software memcmp", ACCENT),
        ("916,000\u00d7", "Memory Traffic Reduction", "max across workloads", ACCENT2),
        ("\u22640.5 cy", "Latency Std Dev", "deterministic timing", ACCENT3),
        ("1,056", "Mops/s (16-bank)", "hardware-validated", (200, 100, 255)),
    ]

    for i, (value, label, sublabel, color) in enumerate(metrics):
        x = 110 + i * 440
        y = 180
        draw_box(draw, x, y, 400, 200, fill=(25, 30, 50), outline=color)
        # Value
        bbox = draw.textbbox((0, 0), value, font=FONT_METRIC)
        vw = bbox[2] - bbox[0]
        draw.text((x + (400 - vw) // 2, y + 20), value, font=FONT_METRIC, fill=color)
        # Label
        bbox = draw.textbbox((0, 0), label, font=FONT_LABEL)
        lw = bbox[2] - bbox[0]
        draw.text((x + (400 - lw) // 2, y + 110), label, font=FONT_LABEL, fill=WHITE)
        # Sublabel
        bbox = draw.textbbox((0, 0), sublabel, font=FONT_SMALL)
        sw = bbox[2] - bbox[0]
        draw.text((x + (400 - sw) // 2, y + 150), sublabel, font=FONT_SMALL, fill=DIM)

    # Bar chart: Change detection comparison
    chart_y = 450
    draw.text((100, chart_y), "Change Detection Latency (cycles)", font=FONT_BODY, fill=WHITE)

    bars = [
        ("sw_memcmp (32B)", 245, RED),
        ("sw_memcmp (64B)", 410, RED),
        ("ATOMiK hw (32B)", 58, ACCENT),
        ("ATOMiK hw (64B)", 99, ACCENT),
    ]

    max_val = 450
    bar_w = 1400
    for i, (label, val, color) in enumerate(bars):
        y = chart_y + 60 + i * 65
        bw = int(val / max_val * bar_w)
        draw.text((100, y + 5), label, font=FONT_BODY_SM, fill=GRAY)
        draw.rounded_rectangle([420, y, 420 + bw, y + 40], radius=6, fill=color)
        draw.text((430 + bw, y + 5), f"{val} cy", font=FONT_BODY_SM, fill=WHITE)

    # Bottom: deterministic note
    draw_box(draw, 100, 750, 1720, 70, fill=(30, 35, 20), outline=ACCENT)
    centered_text(draw, 760, "All operations complete in exactly the same number of cycles \u2014 no timing side channels", FONT_BODY_SM, ACCENT)

    # Memory reduction table
    draw.text((100, 860), "Memory Reduction:", font=FONT_BODY_SM, fill=WHITE)
    mem_data = [("32B state", "7,670\u00d7"), ("64B state", "30,000\u00d7"), ("512B state", "916,000\u00d7")]
    for i, (sz, red) in enumerate(mem_data):
        x = 400 + i * 400
        draw.text((x, 860), f"{sz}: ", font=FONT_BODY_SM, fill=GRAY)
        draw.text((x + 120, 860), red, font=FONT_BODY_SM, fill=ACCENT)

    return img


# =========================================================================
# SCENE 5: Cosmos Reason 2 Integration (1:15 - 1:55)
# =========================================================================
def scene_cosmos():
    img = new_frame()
    draw = ImageDraw.Draw(img)

    centered_text(draw, 30, "Cosmos Reason 2 + ATOMiK", FONT_HEADING, WHITE)
    centered_text(draw, 90, "Hardware-Verified State Reasoning for Physical AI", FONT_BODY_SM, DIM)

    # Architecture flow
    # Sensors/Cameras box
    draw_box(draw, 150, 200, 300, 80, fill=(40, 40, 50), outline=GRAY)
    centered_text(draw, 220, "Sensors / Cameras", FONT_BODY_SM, WHITE)

    draw_arrow(draw, 300, 280, 300, 340, fill=GRAY)

    # Cosmos Reason 2
    draw_box(draw, 100, 340, 400, 120, fill=(30, 50, 20), outline=COSMOS)
    draw.text((130, 355), "Cosmos Reason 2", font=FONT_BODY, fill=COSMOS)
    draw.text((130, 400), "Observes + reasons about", font=FONT_BODY_SM, fill=GRAY)
    draw.text((130, 430), "physical world state", font=FONT_BODY_SM, fill=GRAY)

    draw_arrow(draw, 300, 460, 300, 520, fill=COSMOS)
    draw.text((320, 480), '"State X changed"', font=FONT_CODE_SM, fill=COSMOS)

    # ATOMiK Hardware
    draw_box(draw, 100, 520, 400, 120, fill=(20, 40, 35), outline=ACCENT)
    draw.text((130, 535), "ATOMiK Hardware", font=FONT_BODY, fill=ACCENT)
    draw.text((130, 580), "Verifies state transition", font=FONT_BODY_SM, fill=GRAY)
    draw.text((130, 610), "via delta-state algebra", font=FONT_BODY_SM, fill=GRAY)

    draw_arrow(draw, 300, 640, 300, 700, fill=ACCENT)
    draw.text((320, 660), '"Verified: consistent"', font=FONT_CODE_SM, fill=ACCENT)

    # Actuator
    draw_box(draw, 150, 700, 300, 80, fill=(40, 40, 50), outline=GRAY)
    centered_text(draw, 720, "Safe to Act", FONT_BODY_SM, WHITE)

    # Right side: Integration details
    details_x = 650
    draw.text((details_x, 200), "Integration Points", font=FONT_BODY, fill=WHITE)

    points = [
        ("1. State Fingerprinting",
         "ATOMiK accumulates state deltas into a\nhardware register. One read = full state hash."),
        ("2. O(1) Change Detection",
         "Single register read tells Cosmos whether\nANY state changed. No buffer comparison."),
        ("3. Delta Extraction",
         "XOR of old \u2295 new state reveals exactly\nWHICH bits changed. Focused reasoning."),
        ("4. Order Independence",
         "Multi-sensor data arrives asynchronously.\nXOR commutativity guarantees correctness."),
    ]

    for i, (title, desc) in enumerate(points):
        y = 270 + i * 145
        draw_box(draw, details_x, y, 650, 120, fill=(25, 30, 50), outline=DIM)
        draw.text((details_x + 15, y + 10), title, font=FONT_BODY_SM, fill=ACCENT)
        draw.text((details_x + 15, y + 45), desc, font=FONT_SMALL, fill=GRAY)

    # Bottom
    centered_text(draw, 870, "Cosmos is the brain. ATOMiK is the verification layer.", FONT_BODY, ACCENT)

    return img


# =========================================================================
# SCENE 6: Mathematical Foundation (1:55 - 2:15)
# =========================================================================
def scene_math():
    img = new_frame()
    draw = ImageDraw.Draw(img)

    centered_text(draw, 30, "Mathematical Foundation", FONT_HEADING, WHITE)
    centered_text(draw, 90, "108 Lean4 Theorems  \u2014  Delta-State Algebra", FONT_BODY_SM, DIM)

    # Abelian Group properties
    draw.text((100, 180), "XOR forms an Abelian Group over bit-vectors:", font=FONT_BODY, fill=WHITE)

    proofs = [
        ("Closure",       "a \u2295 b \u2208 B\u207f",           "XOR of n-bit vectors is n-bit"),
        ("Associativity", "(a \u2295 b) \u2295 c = a \u2295 (b \u2295 c)", "Grouping doesn't affect result"),
        ("Identity",      "a \u2295 0 = a",                "XOR with zero is identity"),
        ("Self-Inverse",  "a \u2295 a = 0",                "XOR with self cancels"),
        ("Commutativity", "a \u2295 b = b \u2295 a",           "Order doesn't matter"),
    ]

    for i, (name, formula, desc) in enumerate(proofs):
        y = 260 + i * 90
        draw_box(draw, 100, y, 1720, 75, fill=(25, 30, 50), outline=DIM)
        draw.text((130, y + 10), name, font=FONT_BODY, fill=ACCENT)
        draw.text((500, y + 10), formula, font=FONT_CODE, fill=WHITE)
        draw.text((900, y + 15), desc, font=FONT_BODY_SM, fill=GRAY)
        # Checkmark
        draw.text((1750, y + 10), "\u2713", font=FONT_BODY, fill=ACCENT)

    # State reconstruction equation
    draw_box(draw, 200, 730, 1520, 100, fill=(20, 40, 35), outline=ACCENT)
    centered_text(draw, 740, "State Reconstruction Theorem", FONT_BODY_SM, ACCENT)
    centered_text(draw, 780, "S(t) = S\u2080 \u2295 \u0394\u2081 \u2295 \u0394\u2082 \u2295 ... \u2295 \u0394\u2099  =  S\u2080 \u2295 A(t)", FONT_CODE, WHITE)

    # Security properties
    centered_text(draw, 880, "Security: deterministic latency \u2192 no timing side channels  |  no speculative execution  |  no cache coherency attacks", FONT_SMALL, DIM)

    return img


# =========================================================================
# SCENE 7: Hardware Demo Simulation (2:15 - 2:35)
# =========================================================================
def scene_hw_demo():
    img = new_frame()
    draw = ImageDraw.Draw(img)

    centered_text(draw, 30, "Live Hardware Results", FONT_HEADING, WHITE)

    # Simulated terminal
    term_x, term_y = 80, 100
    term_w, term_h = 900, 600
    draw.rounded_rectangle([term_x, term_y, term_x+term_w, term_y+term_h],
                          radius=10, fill=(10, 10, 15), outline=DIM)
    # Terminal title bar
    draw.rounded_rectangle([term_x, term_y, term_x+term_w, term_y+30],
                          radius=10, fill=(40, 40, 50))
    draw.text((term_x + 15, term_y + 5), "UART @ 115200 baud  \u2014  /dev/ttyUSB1", font=FONT_SMALL, fill=GRAY)

    # Terminal content
    lines = [
        ("> X  (ATOMiK Self-Test)", DIM),
        ("[X] atomik_load .......... PASS", ACCENT),
        ("[X] atomik_accum ......... PASS", ACCENT),
        ("[X] atomik_read .......... PASS", ACCENT),
        ("[X] atomik_roundtrip ..... PASS", ACCENT),
        ("[X] atomik_change_det .... PASS", ACCENT),
        ("[X] atomik_memcpy ........ PASS", ACCENT),
        ("[X] atomik_burst ......... PASS", ACCENT),
        ("[X] atomik_reset ......... PASS", ACCENT),
        ("[X] atomik_multi ......... PASS", ACCENT),
        ("", WHITE),
        ("> V  (Display Pipeline)", DIM),
        ("[V] disp_ctrl_write ...... PASS", (200, 100, 255)),
        ("[V] disp_lut_write ....... PASS", (200, 100, 255)),
        ("[V] disp_scan_write ...... PASS", (200, 100, 255)),
        ("[V] disp_status_read ..... PASS", (200, 100, 255)),
        ("[V] disp_frame_count ..... PASS  (82 fps)", (200, 100, 255)),
        ("[V] disp_enable_toggle ... PASS", (200, 100, 255)),
    ]

    for i, (text, color) in enumerate(lines):
        draw.text((term_x + 15, term_y + 40 + i * 30), text, font=FONT_CODE_SM, fill=color)

    # Right side: HDMI output representation
    hdmi_x, hdmi_y = 1050, 100
    draw_box(draw, hdmi_x, hdmi_y, 790, 500, fill=(5, 5, 10), outline=DIM)
    draw.text((hdmi_x + 10, hdmi_y + 5), "HDMI Output  \u2014  640\u00d7480 @ 60 Hz", font=FONT_SMALL, fill=DIM)

    # Draw simplified test card
    card_x, card_y = hdmi_x + 50, hdmi_y + 40
    card_w, card_h = 690, 420
    # Color bars
    colors_bar = [
        (255, 255, 255), (255, 255, 0), (0, 255, 255), (0, 255, 0),
        (255, 0, 255), (255, 0, 0), (0, 0, 255), (0, 0, 0)
    ]
    bar_width = card_w // 8
    for i, c in enumerate(colors_bar):
        x = card_x + i * bar_width
        draw.rectangle([x, card_y, x + bar_width, card_y + card_h * 2 // 3], fill=c)

    # Bottom gradient bar
    bot_y = card_y + card_h * 2 // 3
    for x in range(card_w):
        gray = int(255 * x / card_w)
        draw.line([(card_x + x, bot_y), (card_x + x, card_y + card_h)], fill=(gray, gray, gray))

    # Overlay text on test card
    draw.text((card_x + 10, card_y + 10), "ATOMiK v3 SoC", font=FONT_BODY_SM, fill=(0, 0, 0))
    draw.text((card_x + 10, card_y + 40), "RV64I @ 21.6 MHz", font=FONT_SMALL, fill=(0, 0, 0))

    # Bottom summary
    draw_box(draw, 80, 730, 1760, 90, fill=(25, 30, 50), outline=ACCENT)
    draw.text((120, 745), "Hardware Test Results:", font=FONT_BODY, fill=WHITE)
    draw.text((120, 785), "[X] 9/9 ATOMiK PASS    [V] 6/6 Display PASS    [P] 10/10 Phase2 PASS", font=FONT_CODE_SM, fill=ACCENT)
    draw.text((1100, 785), "Zero timing violations  |  0 TNS", font=FONT_CODE_SM, fill=ACCENT)

    # Board info
    centered_text(draw, 870, "Tang Nano 9K  |  GW1NR-LV9QN88PC6/I5  |  $13.50 retail", FONT_BODY_SM, DIM)

    return img


# =========================================================================
# SCENE 8: Closing (2:35 - 2:50)
# =========================================================================
def scene_closing():
    img = new_frame()
    draw = ImageDraw.Draw(img)

    centered_text(draw, 150, "ATOMiK", FONT_TITLE, ACCENT)
    centered_text(draw, 250, "Hardware-Verified State Reasoning for Physical AI", FONT_SUBTITLE, WHITE)

    draw.line([(W//2 - 200, 320), (W//2 + 200, 320)], fill=ACCENT, width=2)

    # Key points
    points = [
        "$13.50 FPGA with mathematically-proven state verification",
        "76-80% faster change detection than software",
        "Deterministic latency — no timing side channels",
        "108 Lean4 theorems — verified delta-state algebra",
        "Custom RV64I CPU with ATOMiK fused instructions",
        "Open source: hardware, firmware, SDK, proofs",
    ]
    for i, p in enumerate(points):
        y = 370 + i * 50
        centered_text(draw, y, p, FONT_BODY_SM, GRAY)

    # Repo
    draw_box(draw, W//2 - 350, 700, 700, 60, fill=(25, 30, 50), outline=ACCENT)
    centered_text(draw, 710, "github.com/MatthewHRockwell/ATOMiK", FONT_CODE, ACCENT)

    centered_text(draw, 820, "NVIDIA Cosmos Cookoff 2026", FONT_BODY_SM, DIM)
    centered_text(draw, 860, "Matthew Rockwell", FONT_BODY_SM, DIM)

    return img


# =========================================================================
# RENDER ALL SCENES
# =========================================================================
print("Rendering video frames...")

# Scene 1: Title (8s)
s1 = scene_title()  # includes fade_in + hold

# Scene 2: Problem (17s)
s2 = scene_problem()
cross_fade(s1, s2, 20)
hold(s2, 15)

# Scene 3: Architecture (20s)
s3 = scene_architecture()
cross_fade(s2, s3, 20)
hold(s3, 18)

# Scene 4: Performance (30s)
s4 = scene_performance()
cross_fade(s3, s4, 20)
hold(s4, 28)

# Scene 5: Cosmos (40s)
s5 = scene_cosmos()
cross_fade(s4, s5, 20)
hold(s5, 38)

# Scene 6: Math (20s)
s6 = scene_math()
cross_fade(s5, s6, 20)
hold(s6, 18)

# Scene 7: HW Demo (20s)
s7 = scene_hw_demo()
cross_fade(s6, s7, 20)
hold(s7, 18)

# Scene 8: Closing (15s)
s8 = scene_closing()
cross_fade(s7, s8, 20)
hold(s8, 10)
fade_out(s8, 30)

print(f"Rendered {frame_num} frames")

# Encode with ffmpeg
print("Encoding video with ffmpeg...")
cmd = [
    "ffmpeg", "-y",
    "-framerate", str(FPS),
    "-i", os.path.join(OUT_DIR, "frame_%05d.png"),
    "-c:v", "libx264",
    "-preset", "medium",
    "-crf", "23",
    "-pix_fmt", "yuv420p",
    "-movflags", "+faststart",
    VIDEO_OUT
]
subprocess.run(cmd, check=True)

# Cleanup frames
import shutil
shutil.rmtree(OUT_DIR)

print(f"\nVideo saved to: {VIDEO_OUT}")

# Print duration
duration = frame_num / FPS
print(f"Duration: {int(duration // 60)}:{int(duration % 60):02d} ({frame_num} frames @ {FPS} fps)")
