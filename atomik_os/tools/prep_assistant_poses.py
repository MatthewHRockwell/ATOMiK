#!/usr/bin/env python3
"""prep_assistant_poses.py — turn a piece of Atom pose art into a board-ready
.atomik_asset that the assistant pose engine picks up automatically.

The assistant (src/assistant.c) loads a SET of poses and selects one per frame
by mood + summon gesture + blink.  Only `idle` is required; every other pose is
optional and falls back to idle when absent.  Drop a pose asset in
assets/assistant/ (and ship it to /tmp/atomik_assets/) and it lights up — no
code change.

Recognised pose names (file = assistant_<name>_160.atomik_asset):
    idle   — calm float, the default (REQUIRED)
    happy  — shown in the SUCCESS mood
    alert  — shown in the WARNING mood
    think  — shown in the THINKING mood
    wave   — greeting gesture, played for ~0.9 s right after Atom is summoned
    (blink is procedural — no art needed)

Pipeline (same clean cutout as the canonical idle art):
    source PNG -> rembg/U2-Net matte -> #FE00FE alpha key -> tight crop
    -> longest-edge 160 px (NEAREST, no key bleed) -> make_atomik_asset --auto

Usage:
    python3 tools/prep_assistant_poses.py --pose wave  --source ~/atom_wave.png
    python3 tools/prep_assistant_poses.py --pose happy --source ~/atom_happy.png

Requires: pip install rembg onnxruntime   (model caches in ~/.u2net/)
Keep the art on-model: same silicon-octopus Atom, plain/!busy background, the
character roughly centred so the matte + crop land cleanly.
"""
import argparse
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ASSET_DIR = HERE.parent / "assets" / "assistant"
POSES = {"idle", "happy", "alert", "think", "wave"}
SIZE = 160


def main():
    ap = argparse.ArgumentParser(description="Build an Atom pose .atomik_asset")
    ap.add_argument("--pose", required=True, choices=sorted(POSES),
                    help="pose name the engine recognises")
    ap.add_argument("--source", required=True, help="source PNG of Atom in that pose")
    ap.add_argument("--size", type=int, default=SIZE, help="longest-edge px (default 160)")
    args = ap.parse_args()

    src = Path(args.source).expanduser()
    if not src.exists():
        sys.exit(f"source not found: {src}")

    try:
        import numpy as np
        from PIL import Image
        from rembg import remove
    except ImportError:
        sys.exit("missing deps — run: pip install rembg onnxruntime pillow numpy")

    print(f"[1/3] U2-Net matte of {src.name} ...", flush=True)
    rgba = remove(Image.open(src).convert("RGB"))
    a = np.asarray(rgba)
    alpha = a[:, :, 3]
    if (alpha >= 128).sum() < 100:
        sys.exit("matte found almost no foreground — is the background plain/contrasting?")

    rgb = a[:, :, :3].copy()
    rgb[alpha < 128] = (0xFE, 0x00, 0xFE)            # binary alpha key
    ys, xs = np.where(alpha >= 128)
    rgb = rgb[ys.min():ys.max() + 1, xs.min():xs.max() + 1]   # tight crop
    img = Image.fromarray(rgb)
    scale = args.size / max(img.size)
    img = img.resize((max(1, int(img.width * scale)),
                      max(1, int(img.height * scale))), Image.NEAREST)

    png_out = ASSET_DIR / f"assistant_{args.pose}_{args.size}.png"
    img.save(png_out)
    print(f"[2/3] cut-out PNG -> {png_out.name} ({img.size[0]}x{img.size[1]})", flush=True)

    asset_out = ASSET_DIR / f"assistant_{args.pose}_{args.size}.atomik_asset"
    print(f"[3/3] packing {asset_out.name} ...", flush=True)
    r = subprocess.run(
        [sys.executable, str(HERE / "make_atomik_asset.py"),
         str(png_out), str(asset_out), "--auto", "--alpha-key", "FE00FE"],
        capture_output=True, text=True)
    sys.stdout.write(r.stdout)
    if r.returncode != 0:
        sys.exit(r.stderr)
    print(f"\nDone. Ship it:  scp/deploy {asset_out} -> /tmp/atomik_assets/")
    print(f"The '{args.pose}' pose now shows automatically in the matching mood.")


if __name__ == "__main__":
    main()
