#!/usr/bin/env python3
"""prep_assistant_assets.py — turn the canonical Atom character art
into board-friendly .atomik_asset variants for v0.39-A.

Source:  docs/design/assistant/atomik_assistant_final.png
Output:  assets/assistant/assistant_idle_<size>.atomik_asset
         assets/assistant/assistant_explain_<size>.atomik_asset
         (idle + explain share the same art for the MVP)

Sizes:   96, 160, 256 px (longest edge)

Steps per variant:
  1. Load source RGB PNG.
  2. Detect background by sampling the four corners and flood-fill
     all pixels within Euclidean distance THRESHOLD of the average
     corner color.  Replace them with a single magic key color
     (#FE00FE magenta) so the asset format's alpha-key feature can
     treat them as transparent at blit time.
  3. Crop to the character's tight bounding box (non-magenta pixels).
  4. Resize so the longest edge == requested size; preserve aspect.
  5. Save intermediate PNG.
  6. Invoke make_atomik_asset.py with --alpha-key FE00FE and --auto.

Class B discipline: the assistant art is decorative chrome only.
Speech / claims still come from the metric provider + event bus.
"""
import argparse, math, subprocess, sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:
    sys.exit("error: PIL/Pillow not installed - pip install Pillow")


HERE         = Path(__file__).resolve().parent
ATOMIK_OS    = HERE.parent
DEFAULT_SRC  = ATOMIK_OS / "docs/design/assistant/atomik_assistant_final.png"
OUT_DIR      = ATOMIK_OS / "assets/assistant"
MAGIC_KEY    = (0xFE, 0x00, 0xFE)  # #FE00FE — alpha-key (v0.39-B)
THRESHOLD    = 38                  # background-color tolerance
SIZES        = [96, 160, 256]


def avg_corner_color(img: Image.Image) -> tuple[int, int, int]:
    """Sample the four corners and return the mean RGB."""
    px = img.load()
    w, h = img.size
    samples = [
        px[0, 0], px[w - 1, 0], px[0, h - 1], px[w - 1, h - 1],
        px[2, 2], px[w - 3, 2], px[2, h - 3], px[w - 3, h - 3],
    ]
    r = sum(p[0] for p in samples) / len(samples)
    g = sum(p[1] for p in samples) / len(samples)
    b = sum(p[2] for p in samples) / len(samples)
    return (int(r), int(g), int(b))


def background_to_magic(img: Image.Image) -> Image.Image:
    """Replace background-colored pixels with #FE00FE."""
    img = img.convert("RGB")
    bg = avg_corner_color(img)
    thresh2 = THRESHOLD * THRESHOLD
    px = img.load()
    w, h = img.size
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            dr = r - bg[0]; dg = g - bg[1]; db = b - bg[2]
            if dr*dr + dg*dg + db*db < thresh2:
                px[x, y] = MAGIC_KEY
    return img


def tight_crop(img: Image.Image) -> Image.Image:
    """Crop to the bounding box of non-magenta pixels."""
    px = img.load()
    w, h = img.size
    min_x, min_y, max_x, max_y = w, h, -1, -1
    for y in range(h):
        for x in range(w):
            if px[x, y] != MAGIC_KEY:
                if x < min_x: min_x = x
                if y < min_y: min_y = y
                if x > max_x: max_x = x
                if y > max_y: max_y = y
    if max_x < 0:
        return img
    pad = 8
    box = (max(0, min_x - pad), max(0, min_y - pad),
           min(w, max_x + 1 + pad), min(h, max_y + 1 + pad))
    return img.crop(box)


def resize_longest(img: Image.Image, target: int) -> Image.Image:
    w, h = img.size
    scale = target / max(w, h)
    new_w = max(1, int(round(w * scale)))
    new_h = max(1, int(round(h * scale)))
    out = img.resize((new_w, new_h), Image.LANCZOS)
    # Re-snap any pixels close to magenta back to exact magenta so the
    # alpha-key still matches after the bilinear/Lanczos pass.
    px = out.load()
    for y in range(new_h):
        for x in range(new_w):
            r, g, b = px[x, y]
            if r > 240 and g < 30 and b > 240:
                px[x, y] = MAGIC_KEY
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--source", default=str(DEFAULT_SRC),
                    help="canonical character PNG path")
    ap.add_argument("--out-dir", default=str(OUT_DIR),
                    help="output asset directory")
    ap.add_argument("--sizes", nargs="+", type=int, default=SIZES,
                    help="output sizes (longest edge)")
    args = ap.parse_args()

    src = Path(args.source)
    if not src.is_file():
        sys.exit(f"error: source not found: {src}")
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    raw = Image.open(src).convert("RGB")
    # v0.39-A.5: real alpha cutout via chromakey.  Paint background
    # pixels with a magic magenta key (#FE00FE) and ship the asset with
    # --alpha-key FE00FE so the board-side blit skips matching pixels.
    KEY = (0xFE, 0x00, 0xFE)
    bg = avg_corner_color(raw)
    thresh2 = THRESHOLD * THRESHOLD
    px = raw.load()
    w_src, h_src = raw.size
    for y in range(h_src):
        for x in range(w_src):
            r, g, b = px[x, y]
            dr = r - bg[0]; dg = g - bg[1]; db = b - bg[2]
            if dr*dr + dg*dg + db*db < thresh2:
                px[x, y] = KEY
    # Tight crop based on non-key pixels.
    min_x, min_y = w_src, h_src
    max_x, max_y = -1, -1
    for y in range(h_src):
        for x in range(w_src):
            if px[x, y] != KEY:
                if x < min_x: min_x = x
                if y < min_y: min_y = y
                if x > max_x: max_x = x
                if y > max_y: max_y = y
    pad = 12
    cropped = raw.crop((
        max(0, min_x - pad), max(0, min_y - pad),
        min(w_src, max_x + 1 + pad), min(h_src, max_y + 1 + pad),
    ))
    print(f"cropped: {cropped.size}  (from {raw.size})", flush=True)

    for sz in args.sizes:
        w, h = cropped.size
        scale = sz / max(w, h)
        new_w = max(1, int(round(w * scale)))
        new_h = max(1, int(round(h * scale)))
        scaled = cropped.resize((new_w, new_h), Image.LANCZOS)
        # Re-snap any pixels close to magenta back to exact magenta so
        # the chromakey still matches after Lanczos resampling.
        sp = scaled.load()
        for yy in range(new_h):
            for xx in range(new_w):
                rr, gg, bb = sp[xx, yy]
                if rr > 220 and gg < 40 and bb > 220:
                    sp[xx, yy] = KEY
        png_path = out_dir / f"assistant_idle_{sz}.png"
        scaled.save(png_path)
        asset_path = out_dir / f"assistant_idle_{sz}.atomik_asset"
        cmd = [sys.executable, str(HERE / "make_atomik_asset.py"),
               str(png_path), str(asset_path), "--auto",
               "--alpha-key", "FE00FE"]
        print("  " + " ".join(cmd), flush=True)
        subprocess.check_call(cmd)
        explain_path = out_dir / f"assistant_explain_{sz}.atomik_asset"
        explain_path.write_bytes(asset_path.read_bytes())
        print(f"  wrote {asset_path}  +  {explain_path}", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
