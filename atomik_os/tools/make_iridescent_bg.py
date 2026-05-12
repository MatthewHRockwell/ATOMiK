#!/usr/bin/env python3
"""make_iridescent_bg.py — generate the v0.38-F Class B background.

Concept images 02-06 share an iridescent flowing-contour background:
dark navy base with bright cyan/violet/green ribbons sweeping across,
soft glow accents at intersections.  This script renders an
approximation host-side as PNG, then `make_atomik_asset.py` packs it
into the .atomik_asset format the board can blit at memcpy speed.

Output: 480x270 PNG (matches the topology_tile dimensions so the
existing tiled-blit path in wallpaper.c keeps working unchanged).

Per ChatGPT 2026-05-09 Class B directive: this is decorative chrome
ONLY.  No numbers, no telemetry, no metric values.  Every live value
in the OS still flows through the metric provider; this asset is the
visual depth that frames the truthful data."""
import argparse, math, sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFilter
except ImportError:
    sys.exit("error: PIL/Pillow not installed — pip install Pillow")


def render(width: int, height: int) -> Image.Image:
    base_top = (0x0A, 0x0E, 0x1A)   # ATOMIK_BG_TOP
    base_bot = (0x12, 0x18, 0x28)   # ATOMIK_BG_BOT
    cyan     = (0x4F, 0xC3, 0xFF)
    violet   = (0x9B, 0x7E, 0xE0)
    green    = (0x46, 0xA7, 0x58)

    # Layer 1: vertical gradient base (deep navy → slightly lighter at bottom).
    img = Image.new("RGB", (width, height), base_top)
    px = img.load()
    for y in range(height):
        t = y / max(1, height - 1)
        r = int(base_top[0] * (1 - t) + base_bot[0] * t)
        g = int(base_top[1] * (1 - t) + base_bot[1] * t)
        b = int(base_top[2] * (1 - t) + base_bot[2] * t)
        for x in range(width):
            px[x, y] = (r, g, b)

    # Layer 2: flowing sine-wave ribbons.  Each ribbon is a thin
    # alpha-blended curve traced across the width.  Different
    # frequencies + amplitudes + colors create the iridescent feel.
    ribbons = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    d = ImageDraw.Draw(ribbons)

    # v0.38-F note: ribbon count tunes screenshot-pull cost.
    # 8 ribbons + 36 accents + blur produced ~700KB compressed FB —
    # ~90 min pull at our UART rate.  If pull cost becomes the
    # bottleneck for iteration speed, drop to 3-4 main ribbons +
    # 12 accents + smaller blur radius and re-ship.
    ribbon_defs = [
        # (y_center, frequency, amplitude, phase, color, alpha, width)
        (height * 0.15, 0.018, 25, 0.0, cyan,   100, 2),
        (height * 0.32, 0.022, 35, 1.3, violet,  85, 2),
        (height * 0.50, 0.015, 50, 2.1, cyan,   120, 3),
        (height * 0.65, 0.028, 30, 0.8, green,   75, 2),
        (height * 0.80, 0.020, 40, 1.7, cyan,    90, 2),
        # A few thinner accent ribbons at higher freq for texture
        (height * 0.25, 0.045, 12, 0.5, violet,  55, 1),
        (height * 0.55, 0.038, 15, 2.4, cyan,    60, 1),
        (height * 0.72, 0.050, 10, 1.9, green,   50, 1),
    ]

    for y_c, freq, amp, phase, color, alpha, w in ribbon_defs:
        prev = None
        for x in range(width):
            # Two-component sine for variety — one slow + one fast.
            sy = math.sin(x * freq + phase) * amp \
               + math.sin(x * freq * 2.7 + phase * 1.5) * amp * 0.25
            y = int(y_c + sy)
            if prev is not None:
                d.line([(x - 1, prev), (x, y)],
                       fill=color + (alpha,), width=w)
            prev = y

    # Blur the ribbon layer so curves have soft glow halos around them.
    ribbons = ribbons.filter(ImageFilter.GaussianBlur(2.0))

    # Layer 3: scattered bright accent dots at "intersection" points.
    # Deterministic positions from i so the asset is reproducible.
    accents = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    d = ImageDraw.Draw(accents)
    for i in range(36):
        ax = (i * 53 + i * i * 7) % width
        ay = ((i * 31) ^ (i * i)) % height
        size = 1 + (i % 4)
        col  = cyan   if i % 3 == 0 else (
               violet if i % 3 == 1 else green)
        alpha = 160 + (i % 4) * 20
        d.ellipse([ax - size, ay - size, ax + size, ay + size],
                  fill=col + (alpha,))
    accents = accents.filter(ImageFilter.GaussianBlur(1.5))

    # Composite: base → ribbons → accents.
    img = img.convert("RGBA")
    img = Image.alpha_composite(img, ribbons)
    img = Image.alpha_composite(img, accents)

    # Layer 4: subtle vignette-free overlay — slight cyan tint across
    # the top third for compositional weight.  Keeps the eye anchored
    # at the hero's actual screen position (above visual center).
    overlay = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    op = overlay.load()
    for y in range(height):
        t = max(0.0, 1.0 - y / (height * 0.5))
        a = int(t * 14)
        for x in range(width):
            op[x, y] = (cyan[0], cyan[1], cyan[2], a)
    img = Image.alpha_composite(img, overlay)

    return img.convert("RGB")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("output")
    ap.add_argument("--width",  type=int, default=480)
    ap.add_argument("--height", type=int, default=270)
    args = ap.parse_args()

    img = render(args.width, args.height)
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    img.save(args.output, "PNG", optimize=True)
    print(f"wrote {args.output}  {args.width}x{args.height}",
          flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
