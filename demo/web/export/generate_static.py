"""Export the demo site as static HTML for GitHub Pages.

Copies HTML/CSS/JS files and bakes in pre-computed benchmark data
so the site works without a backend server.
"""

from __future__ import annotations

import json
import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent.parent
WEB_DIR = ROOT / "demo" / "web"
STATIC_DIR = WEB_DIR / "static"
OUTPUT_DIR = ROOT / "docs" / "site"
RESULTS_PATH = ROOT / "software" / "demos" / "state_sync_benchmark" / "results" / "results.json"


def export_static() -> None:
    """Generate a static site suitable for GitHub Pages."""
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    # Copy all static files
    for src in STATIC_DIR.rglob("*"):
        if src.is_file():
            dest = OUTPUT_DIR / src.relative_to(STATIC_DIR)
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dest)

    # Fix paths in HTML (from /static/ to relative ./)
    for html_file in OUTPUT_DIR.glob("*.html"):
        text = html_file.read_text(encoding="utf-8")
        text = text.replace('href="/static/', 'href="')
        text = text.replace('src="/static/', 'src="')
        text = text.replace('href="/"', 'href="index.html"')
        text = text.replace('href="/investor"', 'href="investor.html"')
        text = text.replace('href="/engineer"', 'href="engineer.html"')
        text = text.replace('href="/demos"', 'href="demos.html"')
        text = text.replace('href="/video"', 'href="video.html"')
        html_file.write_text(text, encoding="utf-8")

    # Bake benchmark results into JS if available
    if RESULTS_PATH.exists():
        results = json.loads(RESULTS_PATH.read_text(encoding="utf-8"))
        inject_js = f"window.__BENCHMARK_DATA = {json.dumps(results)};\n"

        for js_file in ["engineer.js", "demos.js"]:
            js_path = OUTPUT_DIR / js_file
            if js_path.exists():
                text = js_path.read_text(encoding="utf-8")
                text = inject_js + text
                # Replace fetch calls with local data
                text = text.replace(
                    "await fetch('/api/benchmark')",
                    "await Promise.resolve({ok:true,json:()=>window.__BENCHMARK_DATA})"
                )
                js_path.write_text(text, encoding="utf-8")

        # Also bake metrics into investor.js
        investor_js = OUTPUT_DIR / "investor.js"
        if investor_js.exists():
            text = investor_js.read_text(encoding="utf-8")
            text = text.replace(
                "await fetch('/api/metrics')",
                "await Promise.resolve({ok:false})"
            )
            investor_js.write_text(text, encoding="utf-8")

    print(f"Static site exported to {OUTPUT_DIR}/")
    print(f"  HTML files: {len(list(OUTPUT_DIR.glob('*.html')))}")
    print(f"  JS files: {len(list(OUTPUT_DIR.glob('*.js')))}")
    print(f"  CSS files: {len(list(OUTPUT_DIR.glob('*.css')))}")


if __name__ == "__main__":
    export_static()
