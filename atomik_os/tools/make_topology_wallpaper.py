#!/usr/bin/env python3
"""make_topology_wallpaper.py — generate a subtle topology-grid PNG
for use as the v0.36+ ATOMiK Desk wallpaper.

Per ChatGPT 2026-05-09 directive: Class B assets exist to add cinematic
depth UNDER real telemetry — never on top.  This wallpaper is meant to
look like an iridescent compute fabric without resembling any of the
Class C "fake metric" elements (no charts, no numbers, no fake gauges).

Output: 1920×1080 PNG with a deep navy base, faint cyan grid lines,
brighter intersection nodes, and a soft radial gradient toward the
center to keep the foreground UI legible.

After running, feed the PNG through tools/make_atomik_asset.py to
produce a ready-to-blit .atomik_asset.
"""
import argparse, math, sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:
    sys.exit("error: PIL/Pillow not installed — pip install Pillow")


def render(width: int, height: int, grid: int) -> Image.Image:
    base = (0x0A, 0x0E, 0x1A)         # ATOMIK_BG_TOP
    bot  = (0x12, 0x18, 0x28)         # ATOMIK_BG_BOT
    accent = (0x4F, 0xC3, 0xFF)       # cyan, low alpha

    img = Image.new("RGB", (width, height), base)
    px = img.load()

    # Vertical gradient base (matches our procedural wallpaper tone).
    for y in range(height):
        t = y / max(1, height - 1)
        r = int(base[0] * (1 - t) + bot[0] * t)
        g = int(base[1] * (1 - t) + bot[1] * t)
        b = int(base[2] * (1 - t) + bot[2] * t)
        for x in range(width):
            px[x, y] = (r, g, b)

    # Faint grid lines.  Use ImageDraw on a separate alpha mask so we
    # can blend at low opacity.
    draw_layer = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    d = ImageDraw.Draw(draw_layer)
    grid_alpha = 18              # very faint
    node_alpha = 70              # slightly stronger at intersections
    for x in range(0, width, grid):
        d.line([(x, 0), (x, height)], fill=accent + (grid_alpha,), width=1)
    for y in range(0, height, grid):
        d.line([(0, y), (width, y)], fill=accent + (grid_alpha,), width=1)
    # Brighter intersection nodes — small dots at each grid crossing.
    for x in range(0, width, grid):
        for y in range(0, height, grid):
            d.ellipse([x - 1, y - 1, x + 1, y + 1],
                      fill=accent + (node_alpha,))

    # Radial darkening toward the edges (vignette) so foreground UI
    # reads cleanly.  Computed as a soft falloff from the center.
    cx, cy = width / 2, height / 2 - 40
    max_r = math.hypot(cx, cy)
    vignette = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    vp = vignette.load()
    for y in range(height):
        for x in range(width):
            dx = (x - cx) / max_r
            dy = (y - cy) / max_r
            r = math.hypot(dx, dy)
            # 0 at center, ~1 at corner — invert so center is brighter.
            a = int(min(80, max(0, (r - 0.55) * 200)))
            vp[x, y] = (0, 0, 0, a)

    img = img.convert("RGBA")
    img = Image.alpha_composite(img, draw_layer)
    img = Image.alpha_composite(img, vignette)

    # A faint accent glow near where the ATOMiK wordmark will sit.
    glow = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    gp = glow.load()
    gx, gy = width / 2, height / 2 - 40
    for y in range(height):
        for x in range(width):
            dx = (x - gx) / 320
            dy = (y - gy) / 200
            r = math.hypot(dx, dy)
            if r < 1:
                a = int((1 - r) * (1 - r) * 32)
                gp[x, y] = (accent[0], accent[1], accent[2], a)
    img = Image.alpha_composite(img, glow)

    return img.convert("RGB")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("output")
    ap.add_argument("--width",  type=int, default=1920)
    ap.add_argument("--height", type=int, default=1080)
    ap.add_argument("--grid",   type=int, default=64,
                    help="grid spacing in pixels (default 64)")
    args = ap.parse_args()

    img = render(args.width, args.height, args.grid)
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    img.save(args.output, "PNG", optimize=True)
    print(f"wrote {args.output}  {args.width}x{args.height}  "
          f"grid={args.grid}px",
          flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
