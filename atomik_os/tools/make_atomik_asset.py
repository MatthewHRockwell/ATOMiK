#!/usr/bin/env python3
"""make_atomik_asset.py — convert PNG (or any PIL-readable image) into a
.atomik_asset (ATKA v1) file the board can blit at memcpy speed.

Per ChatGPT 2026-05-09 directive (Lane 2: Class A + Class B): keep PNG
decoding off the soft CPU.  This script is the host-side half of the
v0.36 asset pipeline.  Format spec is documented inline and in
atomik_os/include/atomik_os.h next to the loader.

Usage:
    tools/make_atomik_asset.py input.png output.atomik_asset
        [--rle | --raw | --auto]
        [--alpha-key TRANSPARENT_HEX]   (e.g. --alpha-key 000000)

By default --auto: encode with both RLE and raw, pick the smaller
result.  Use --rle or --raw to force one mode for benchmarking.

Output is little-endian, header always 24 bytes.
"""
import argparse, struct, sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("error: PIL/Pillow not installed — pip install Pillow")

MAGIC   = b"ATKA"
VERSION = 1
FLAG_RLE = 0x0001
HEADER_FMT = "<4sHHIIII"          # magic, ver, flags, w, h, payload_bytes, reserved


def to_xrgb_pixels(img: Image.Image) -> list[int]:
    """Convert any PIL image into a flat list of XRGB8888 ints (LE
    same-as-fb encoding: 0x00RRGGBB).  Alpha channel is ignored on the
    way out — atomik_asset_blit_alpha() applies a uniform alpha at blit
    time.  This matches our XRGB framebuffer exactly so the loader
    becomes a memcpy."""
    if img.mode != "RGBA":
        img = img.convert("RGBA")
    w, h = img.size
    px = img.load()
    out: list[int] = []
    for y in range(h):
        for x in range(w):
            r, g, b, _ = px[x, y]
            out.append((r << 16) | (g << 8) | b)
    return out


def encode_raw(pixels: list[int]) -> bytes:
    return b"".join(struct.pack("<I", p) for p in pixels)


def encode_rle(pixels: list[int]) -> bytes:
    """Stream of (uint16 count, uint32 pixel) records.  Runs are split
    at 65535 to keep count in uint16 range.  Worst case (no two
    consecutive identical pixels) is 6 bytes per pixel — 1.5x raw —
    which is why the converter usually picks raw for noisy art."""
    out = bytearray()
    n = len(pixels)
    i = 0
    while i < n:
        v = pixels[i]
        j = i + 1
        while j < n and pixels[j] == v and (j - i) < 0xFFFF:
            j += 1
        run_len = j - i
        out += struct.pack("<HI", run_len, v)
        i = j
    return bytes(out)


def write_asset(path: Path, w: int, h: int, flags: int, payload: bytes) -> None:
    header = struct.pack(HEADER_FMT,
                         MAGIC, VERSION, flags, w, h, len(payload), 0)
    assert len(header) == 24
    path.write_bytes(header + payload)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("input")
    ap.add_argument("output")
    mode = ap.add_mutually_exclusive_group()
    mode.add_argument("--rle",  action="store_true")
    mode.add_argument("--raw",  action="store_true")
    mode.add_argument("--auto", action="store_true",
                      help="default: pick whichever is smaller")
    args = ap.parse_args()

    img = Image.open(args.input)
    w, h = img.size
    pixels = to_xrgb_pixels(img)
    n_pix = len(pixels)
    if n_pix != w * h:
        sys.exit(f"error: pixel count mismatch (got {n_pix}, expected {w*h})")

    raw = encode_raw(pixels)
    rle = encode_rle(pixels)
    out = Path(args.output)

    if args.rle:
        flags, payload, mode_name = FLAG_RLE, rle, "rle"
    elif args.raw:
        flags, payload, mode_name = 0, raw, "raw"
    else:   # auto
        if len(rle) <= len(raw):
            flags, payload, mode_name = FLAG_RLE, rle, "rle (auto)"
        else:
            flags, payload, mode_name = 0, raw, "raw (auto)"

    write_asset(out, w, h, flags, payload)
    print(f"wrote {out}  {w}x{h}  {mode_name}  "
          f"raw={len(raw)}  rle={len(rle)}  chosen={len(payload)}  "
          f"ratio={len(payload)/len(raw):.3f}",
          flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
