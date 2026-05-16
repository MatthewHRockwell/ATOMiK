#!/usr/bin/env python3
"""font_pack.py — host-side packer for v0.38-K typography atlas.

Renders a TTF/OTF at a target pixel size using PIL, captures each
glyph's 8-bit alpha mask, and packs into a flat .atomik_font binary
the board can mmap + index without FreeType at runtime.

Output format (little-endian):

  Header (16 bytes)
    magic        4   "AFNT"
    version      1   = 1
    reserved     3   0
    pixel_size   2   line height (px)
    ascender     2   distance from top-of-cell to baseline
    glyph_count  2   number of glyphs
    reserved2    2   0

  Glyph directory (glyph_count * 16 bytes), sorted by codepoint
    codepoint    2
    width        2
    height       2
    advance      2
    offset_x     2  signed; bearing X (where mask is placed
                    relative to pen origin)
    offset_y     2  signed; offset from baseline DOWN to top of mask.
                    Negative values mean the mask top is above
                    baseline (most glyphs).
    data_offset  4  byte offset into the mask blob

  Mask blob (sum of width*height across glyphs), 8-bit alpha each.

The board reader builds an index from the directory at boot and
serves draw_text_aa() through it.  No runtime allocation in the
hot path beyond a memcpy from the alpha mask into a blit buffer.

This is v0.38-K Class B at one level (pre-rendered chrome) and Class
A friendly because nothing about the glyph data is a "metric".  It is
just baked typography. """
import argparse, struct, sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    sys.exit("error: PIL/Pillow not installed - pip install Pillow")


MAGIC = b"AFNT"
VERSION = 1
# Codepoints we always include.  Pixel font today covers ASCII 32-126;
# atlas matches that for plug-and-play substitution.
DEFAULT_RANGE = list(range(32, 127))


def render_glyph(font: ImageFont.FreeTypeFont, ch: str):
    """Return (mask_bytes, width, height, advance, offset_x, offset_y).

    PIL.getmask returns an L-mode image with per-pixel grayscale that
    we read as 8-bit alpha.  PIL.getbbox gives the tight bounding box
    of inked pixels; we use it to crop to the minimum mask, recording
    the offset relative to pen origin so the board can place it
    correctly on the baseline. """
    # Render onto a generous canvas so descenders / accents don't clip.
    px = font.size
    canvas = Image.new("L", (px * 3, px * 3), 0)
    d = ImageDraw.Draw(canvas)
    # Draw at (px, px) so we have margin on every side.
    d.text((px, px), ch, fill=255, font=font)
    bbox = canvas.getbbox()
    if bbox is None:
        # Whitespace glyph (space) - no inked pixels.  Mask is 0x0,
        # but advance still matters.
        try:
            adv = int(font.getlength(ch))
        except AttributeError:
            adv, _ = font.getsize(ch)
        return b"", 0, 0, adv, 0, 0
    x0, y0, x1, y1 = bbox
    mask = canvas.crop(bbox)
    w, h = mask.size
    # Pen origin we drew at was (px, px).  Bearing X = how far the
    # mask's left edge is from the pen.  offset_y is measured from
    # the baseline; we don't yet know the baseline because PIL uses
    # font.getmetrics() for ascent.
    ascent, _ = font.getmetrics()
    pen_x, pen_y = px, px
    baseline_y = pen_y + ascent
    offset_x = x0 - pen_x
    # offset_y = top of mask measured DOWN from baseline.
    # If mask top is above baseline, offset_y is negative.
    offset_y = y0 - baseline_y
    try:
        advance = int(font.getlength(ch))
    except AttributeError:
        advance, _ = font.getsize(ch)
    return mask.tobytes(), w, h, advance, offset_x, offset_y


def pack(font_path: Path, pixel_size: int, codepoints: list[int],
         out_path: Path) -> int:
    font = ImageFont.truetype(str(font_path), pixel_size)
    ascent, _descent = font.getmetrics()

    entries = []
    blob = bytearray()
    for cp in codepoints:
        ch = chr(cp)
        mask, w, h, adv, ox, oy = render_glyph(font, ch)
        entries.append((cp, w, h, adv, ox, oy, len(blob)))
        blob.extend(mask)

    header = struct.pack(
        "<4s B 3x H H H 2x",
        MAGIC, VERSION, pixel_size, ascent, len(entries),
    )
    dirent = bytearray()
    for cp, w, h, adv, ox, oy, off in entries:
        dirent.extend(struct.pack(
            "<H H H H h h I",
            cp, w, h, adv, ox, oy, off,
        ))

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("wb") as f:
        f.write(header)
        f.write(dirent)
        f.write(blob)
    return out_path.stat().st_size


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("font", help="path to TTF/OTF")
    ap.add_argument("output", help="output .atomik_font")
    ap.add_argument("--size", type=int, required=True,
                    help="pixel size (e.g. 14, 18, 36)")
    ap.add_argument("--ascii-only", action="store_true",
                    help="restrict to printable ASCII 32-126 (default)")
    args = ap.parse_args()

    codepoints = DEFAULT_RANGE
    out = Path(args.output)
    sz = pack(Path(args.font), args.size, codepoints, out)
    print(f"wrote {out}  {sz} bytes  ({len(codepoints)} glyphs at "
          f"{args.size}px)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
