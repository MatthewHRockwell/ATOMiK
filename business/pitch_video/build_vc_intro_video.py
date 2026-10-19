#!/usr/bin/env python3
"""Build the 60-second ATOMiK VC intro video from approved repo assets."""

from __future__ import annotations

import argparse
import math
import shutil
import subprocess
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageOps


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "business" / "pitch_video"
SLIDES_DIR = OUT / "slides"
PUBLIC = ROOT / "website" / "public"
W, H, FPS = 1920, 1080, 30

VOICEOVER = """AI systems, databases, and edge devices spend enormous energy asking one question: what changed?

Atomic makes change a hardware signal.

Instead of repeatedly scanning memory, Atomic tracks delta state directly and exposes it to software as a simple primitive.

The result is a new layer beneath runtimes, agents, and operating systems: faster awareness, lower waste, and cleaner orchestration.

We've already validated the core path on FPGA hardware, including Linux userspace talking through the OS stack into the accelerator, and a Zynq-backed desktop UI showing the product direction.

Now we are turning a validated hardware primitive into a platform for real workloads.

Atomic is building the state-change layer for the next generation of adaptive computing."""

SLIDES = [
    ("09-current-live-atomik-desk-v039k.png", "HARDWARE_VALIDATED prototype UI", "ATOMiK", "State change as a hardware signal", "Live Zynq prototype screenshot", 8.0),
    ("02-atomik-desk-hero-concept.png", "The software tax", "Everything keeps asking: what changed?", "AI agents, databases, caches, and operating systems all repeat the same state-checking work.", "Concept visual", 8.0),
    ("03-adaptive-mode-mixed-workload-concept.png", "The ATOMiK primitive", "Track deltas directly", "Move change detection below the runtime so software can react to state instead of rescanning it.", "Roadmap concept visual", 8.0),
    ("01-current-live-atomik-desk.jpg", "Hardware evidence", "Validated through real FPGA paths", "Linux userspace has exercised the OS-to-accelerator path; the Desk UI shows the product direction on Zynq hardware.", "Hardware-validated prototype evidence", 9.0),
    ("04-build-lane-gcc-concept.png", "Developer wedge", "A platform layer, not another dashboard", "The goal is a developer experience where state change becomes a native system signal.", "Roadmap concept visual", 8.0),
    ("05-replica-flow-sync-concept.png", "Investment thesis", "The state-change layer for adaptive computing", "ATOMiK turns repeated state checking into a hardware-backed primitive for real workloads.", "Roadmap concept visual", 8.0),
]


def load_font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont:
    name = "DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf"
    path = Path("/usr/share/fonts/truetype/dejavu") / name
    if path.exists():
        return ImageFont.truetype(str(path), size)
    return ImageFont.load_default()


FONT_KICKER = load_font(32, True)
FONT_TITLE = load_font(76, True)
FONT_BODY = load_font(36)
FONT_LABEL = load_font(26)
FONT_MARK = load_font(34, True)


def wrap(draw: ImageDraw.ImageDraw, text: str, font: ImageFont.FreeTypeFont, width: int) -> list[str]:
    lines: list[str] = []
    current = ""
    for word in text.split():
        candidate = f"{current} {word}".strip()
        if draw.textbbox((0, 0), candidate, font=font)[2] <= width:
            current = candidate
            continue
        if current:
            lines.append(current)
        current = word
    if current:
        lines.append(current)
    return lines


def draw_lines(draw: ImageDraw.ImageDraw, x: int, y: int, lines: list[str], font: ImageFont.FreeTypeFont, fill: tuple[int, int, int, int], gap: int) -> int:
    for line in lines:
        draw.text((x, y), line, font=font, fill=fill)
        bbox = draw.textbbox((x, y), line, font=font)
        y += bbox[3] - bbox[1] + gap
    return y


def background(path: Path, index: int) -> Image.Image:
    src = Image.open(path).convert("RGB")
    scale = 1.065 + (0.01 if index % 2 else 0)
    fit = ImageOps.fit(src, (math.ceil(W * scale), math.ceil(H * scale)), Image.Resampling.LANCZOS)
    ox = int((fit.width - W) * (0.35 if index % 2 else 0.65))
    oy = int((fit.height - H) * (0.45 if index % 3 else 0.55))
    base = fit.crop((ox, oy, ox + W, oy + H)).filter(ImageFilter.GaussianBlur(0.25)).convert("RGBA")

    overlay = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    px = overlay.load()
    for y in range(H):
        v = int(85 + 115 * (y / H))
        for x in range(W):
            left = int(150 * max(0.0, 1.0 - x / (W * 0.72)))
            px[x, y] = (0, 0, 0, min(225, v + left))
    return Image.alpha_composite(base, overlay)


def render_slide(item: tuple[str, str, str, str, str, float], index: int) -> Path:
    image, kicker, title, subtitle, label, _seconds = item
    canvas = background(PUBLIC / image, index)
    draw = ImageDraw.Draw(canvas)
    x, y = 106, 118

    draw.text((x, y), kicker.upper(), font=FONT_KICKER, fill=(118, 235, 207, 255))
    y += 58
    y = draw_lines(draw, x, y, wrap(draw, title, FONT_TITLE, 1210), FONT_TITLE, (248, 250, 252, 255), 10)
    y += 18
    draw_lines(draw, x, y, wrap(draw, subtitle, FONT_BODY, 1120), FONT_BODY, (221, 228, 236, 255), 12)

    lb = draw.textbbox((0, 0), label, font=FONT_LABEL)
    pill = (x, H - 118, x + lb[2] - lb[0] + 40, H - 118 + lb[3] - lb[1] + 24)
    draw.rounded_rectangle(pill, radius=14, fill=(4, 9, 18, 205), outline=(90, 108, 128, 180), width=2)
    draw.text((x + 20, H - 108), label, font=FONT_LABEL, fill=(226, 232, 240, 255))

    mark = "ATOMiK"
    mb = draw.textbbox((0, 0), mark, font=FONT_MARK)
    draw.text((W - mb[2] - 92, H - 105), mark, font=FONT_MARK, fill=(248, 250, 252, 230))

    out = SLIDES_DIR / f"slide_{index:02d}.png"
    canvas.convert("RGB").save(out, quality=95)
    return out


def run(cmd: list[str]) -> None:
    print("+ " + " ".join(cmd))
    subprocess.run(cmd, check=True)


def media_duration(path: Path) -> float:
    result = subprocess.run(
        ["ffprobe", "-v", "error", "-show_entries", "format=duration", "-of", "default=noprint_wrappers=1:nokey=1", str(path)],
        check=True,
        capture_output=True,
        text=True,
    )
    return float(result.stdout.strip())


def build_tts() -> Path:
    audio = OUT / "voiceover.mp3"
    run([
        "/tmp/atomik-tts-venv/bin/edge-tts",
        "--voice",
        "en-US-GuyNeural",
        "--rate=-2%",
        "--file",
        str(OUT / "voiceover.txt"),
        "--write-media",
        str(audio),
        "--write-subtitles",
        str(OUT / "voiceover.srt"),
    ])
    return audio


def write_concat(slides: list[Path], audio: Path | None) -> Path:
    scale = 1.0
    if audio:
        scale = max(1.0, media_duration(audio) / sum(slide[-1] for slide in SLIDES))
    concat = OUT / "slides.ffconcat"
    lines = ["ffconcat version 1.0\n"]
    for slide, item in zip(slides, SLIDES):
        lines.append(f"file '{slide.resolve()}'\n")
        lines.append(f"duration {item[-1] * scale:.3f}\n")
    lines.append(f"file '{slides[-1].resolve()}'\n")
    concat.write_text("".join(lines), encoding="utf-8")
    return concat


def build_video(audio: Path | None) -> Path:
    slides = [render_slide(item, i + 1) for i, item in enumerate(SLIDES)]
    concat = write_concat(slides, audio)
    mp4 = OUT / "ATOMiK_VC_Intro_60s.mp4"
    cmd = ["ffmpeg", "-y", "-f", "concat", "-safe", "0", "-i", str(concat)]
    if audio:
        cmd += ["-i", str(audio)]
    cmd += ["-vf", f"fps={FPS},format=yuv420p", "-c:v", "libx264", "-preset", "medium", "-crf", "18"]
    if audio:
        cmd += ["-c:a", "aac", "-b:a", "192k", "-shortest"]
    cmd += ["-movflags", "+faststart", str(mp4)]
    run(cmd)
    return mp4


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--skip-tts", action="store_true")
    args = parser.parse_args()

    OUT.mkdir(parents=True, exist_ok=True)
    if SLIDES_DIR.exists():
        shutil.rmtree(SLIDES_DIR)
    SLIDES_DIR.mkdir(parents=True)
    (OUT / "voiceover.txt").write_text(VOICEOVER + "\n", encoding="utf-8")

    audio = None if args.skip_tts else build_tts()
    video = build_video(audio)
    print(f"Wrote {video}")
    print(f"Duration: {media_duration(video):.2f}s")


if __name__ == "__main__":
    main()
