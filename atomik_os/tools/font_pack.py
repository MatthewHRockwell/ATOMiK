#!/usr/bin/env python3
"""font_pack.py — host-side packer for v0.38-K typography atlas.

Renders a TTF/OTF at a target pixel size using PIL, captures each
glyph's 8-bit alpha mask, and packs into a flat .atomik_font binary
the board can mmap + index without FreeType at runtime.

Output format (little-endian; finalized per ChatGPT 2026-05-16):

  Header (32 bytes total)
    magic         4  "AFNT"
    version       2  = 1
    pixel_size    2  nominal em size (px)
    ascender      2  signed; baseline distance from top
    descender     2  signed; baseline distance from bottom (negative)
    line_height   2  recommended baseline-to-baseline pitch
    glyph_count   2
    dir_offset    4  bytes from start of file to glyph directory
    data_offset   4  bytes from start of file to mask blob
    data_size     4  total mask-blob size in bytes (sanity check)
    flags         4  bit 0 = grayscale alpha; bit 1 = premultiplied;
                     bit 2 = monospace; high bits reserved

  Glyph directory (glyph_count * 20 bytes), sorted by codepoint
    codepoint     4  uint32 (room for non-ASCII later)
    width         2
    height        2
    advance       2  signed (advance width)
    offset_x      2  signed; bearing X
    offset_y      2  signed; mask top relative to baseline
                     (negative = above baseline, typical glyph)
    reserved      2  pad to 16 — also room for future kerning index
    data_offset   4  byte offset into the mask blob

  Mask blob (sum of width*height across glyphs).  Each glyph's mask
  start is aligned to 4 bytes — pad with zero alpha as needed.

The board reader builds an index from the directory at boot and
serves draw_text_aa() through it.  No runtime allocation in the
hot path beyond a memcpy from the alpha mask into a blit buffer.

Renderer formula (board side):
    dst_rgb = lerp(dst_rgb, glyph_color, mask_alpha * global_alpha)

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


FLAG_GRAYSCALE_ALPHA = 1 << 0
FLAG_PREMULTIPLIED   = 1 << 1
FLAG_MONOSPACE       = 1 << 2


def pack(font_path: Path, pixel_size: int, codepoints: list[int],
         out_path: Path, monospace: bool = False) -> int:
    font = ImageFont.truetype(str(font_path), pixel_size)
    ascent, descent = font.getmetrics()
    # PIL returns descender as a positive number ("pixels below
    # baseline").  We store it negative per typographic convention so
    # ascender + descender = line_height directly.
    descender = -descent
    line_height = ascent + descent

    flags = FLAG_GRAYSCALE_ALPHA
    if monospace:
        flags |= FLAG_MONOSPACE

    # First pass: render every glyph, collect mask bytes (4-byte aligned).
    entries = []
    blob = bytearray()
    for cp in codepoints:
        ch = chr(cp)
        mask, w, h, adv, ox, oy = render_glyph(font, ch)
        # Align this glyph's mask start to a 4-byte boundary.
        pad = (-len(blob)) & 3
        blob.extend(b"\x00" * pad)
        entries.append((cp, w, h, adv, ox, oy, len(blob)))
        blob.extend(mask)

    HEADER_SIZE = 32
    DIRENT_SIZE = 20
    glyph_count = len(entries)
    dir_offset = HEADER_SIZE
    data_offset = HEADER_SIZE + glyph_count * DIRENT_SIZE
    data_size = len(blob)

    header = struct.pack(
        "<4s H H h h H H I I I I",
        MAGIC, VERSION, pixel_size, ascent, descender, line_height,
        glyph_count, dir_offset, data_offset, data_size, flags,
    )
    assert len(header) == HEADER_SIZE, f"header size {len(header)}"

    dirent = bytearray()
    for cp, w, h, adv, ox, oy, off in entries:
        dirent.extend(struct.pack(
            "<I H H h h h 2x I",
            cp, w, h, adv, ox, oy, off,
        ))
    assert len(dirent) == glyph_count * DIRENT_SIZE

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
    ap.add_argument("--monospace", action="store_true",
                    help="mark this atlas as monospaced (FLAG_MONOSPACE)")
    args = ap.parse_args()

    codepoints = DEFAULT_RANGE
    out = Path(args.output)
    sz = pack(Path(args.font), args.size, codepoints, out,
              monospace=args.monospace)
    print(f"wrote {out}  {sz} bytes  ({len(codepoints)} glyphs at "
          f"{args.size}px)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
