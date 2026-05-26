#!/usr/bin/env python3
"""Generate ATOMiK social preview / Open Graph image (1200x630)."""

from PIL import Image, ImageDraw, ImageFont

WIDTH, HEIGHT = 1200, 630
BG = "#0a0a0f"
WHITE = "#ffffff"
BLUE = "#4f8fff"
GRAY = "#8888a0"
CYAN = "#22d3ee"

img = Image.new("RGB", (WIDTH, HEIGHT), BG)
draw = ImageDraw.Draw(img)

# Fonts
font_bold = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
font_reg = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"

title_font = ImageFont.truetype(font_bold, 120)
subtitle_font = ImageFont.truetype(font_reg, 36)
tagline_font = ImageFont.truetype(font_reg, 22)

# --- Title: "ATOMiK" with blue "i" ---
parts = [("ATOM", WHITE), ("i", BLUE), ("K", WHITE)]
total_w = sum(draw.textlength(t, font=title_font) for t, _ in parts)
x = (WIDTH - total_w) / 2
y_title = 170

for text, color in parts:
    draw.text((x, y_title), text, fill=color, font=title_font)
    x += draw.textlength(text, font=title_font)

# --- Subtitle ---
subtitle = "Delta-State Computing"
sw = draw.textlength(subtitle, font=subtitle_font)
draw.text(((WIDTH - sw) / 2, y_title + 140), subtitle, fill=GRAY, font=subtitle_font)

# --- Tagline ---
tagline = "State-aware compute  |  Evidence-labeled proof  |  Workload-specific evaluation"
tw = draw.textlength(tagline, font=tagline_font)
draw.text(((WIDTH - tw) / 2, y_title + 200), tagline, fill=CYAN, font=tagline_font)

# Save
out = "/home/mattrock/Projects/ATOMiK/docs/landing/og-image.png"
img.save(out, "PNG")
print(f"Saved: {out} ({img.size[0]}x{img.size[1]})")
