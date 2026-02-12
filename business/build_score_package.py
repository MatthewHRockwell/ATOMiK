#!/usr/bin/env python3
"""Build the ATOMiK SCORE Business Package zip file.

Usage:
    python business/build_score_package.py

Produces:
    business/ATOMiK_Business_Package.zip
"""

from __future__ import annotations

import shutil
import sys
from pathlib import Path
from zipfile import ZipFile

# Root of the ATOMiK project
ROOT = Path(__file__).resolve().parent.parent

# Where the new documents live
SCORE_SRC = ROOT / "business" / "score_package"

# Output zip
OUTPUT_ZIP = ROOT / "business" / "ATOMiK_Business_Package.zip"

# Prefix inside the zip
PREFIX = "ATOMiK_Business_Package"


def _copy_map() -> list[tuple[Path, str]]:
    """Return (source_path, zip_dest_path) pairs for all files."""
    entries: list[tuple[Path, str]] = []

    # --- New documents (from score_package/) ---
    new_docs = [
        ("00_START_HERE.md", "00_START_HERE.md"),
        (
            "01_Plain_Language/ATOMiK_for_Idiots.md",
            "01_Plain_Language/ATOMiK_for_Idiots.md",
        ),
        (
            "03_Industry_Applications/Industry_Guide.md",
            "03_Industry_Applications/Industry_Guide.md",
        ),
        ("04_Business_Case/ROI_Pathways.md", "04_Business_Case/ROI_Pathways.md"),
        ("04_Business_Case/Go_To_Market.md", "04_Business_Case/Go_To_Market.md"),
        (
            "05_Live_Demo/Demo_Quickstart.md",
            "05_Live_Demo/Demo_Quickstart.md",
        ),
        (
            "07_Next_Steps/Fabrication_and_Partnerships.md",
            "07_Next_Steps/Fabrication_and_Partnerships.md",
        ),
        ("08_Reference/Glossary.md", "08_Reference/Glossary.md"),
    ]
    for src_rel, dest_rel in new_docs:
        entries.append((SCORE_SRC / src_rel, f"{PREFIX}/{dest_rel}"))

    # --- Existing business documents ---
    existing_docs = [
        (
            "business/one_pager/atomik_one_pager.md",
            "02_Executive_Materials/Executive_One_Pager.md",
        ),
        (
            "business/pitch_deck/slides.md",
            "02_Executive_Materials/Pitch_Deck.md",
        ),
        (
            "business/faq/investor_faq.md",
            "02_Executive_Materials/Investor_FAQ.md",
        ),
        (
            "business/comparisons/competitive_analysis.md",
            "04_Business_Case/Competitive_Analysis.md",
        ),
        (
            "business/demo_recording/recording_script.md",
            "05_Live_Demo/Recording_Script.md",
        ),
    ]
    for src_rel, dest_rel in existing_docs:
        entries.append((ROOT / src_rel, f"{PREFIX}/{dest_rel}"))

    # --- PDFs ---
    pdfs = [
        (
            "math/benchmarks/results/PHASE6_PERFORMANCE.pdf",
            "06_Technical_Evidence/Phase6_Performance.pdf",
        ),
        (
            "papers/paper1-formal-verification/Delta_State_Algebra.pdf",
            "06_Technical_Evidence/Paper1_Formal_Verification.pdf",
        ),
        (
            "papers/paper2-benchmarks/Paper_2_ATOMiK_Benchmarks.pdf",
            "06_Technical_Evidence/Paper2_Benchmarks.pdf",
        ),
    ]
    for src_rel, dest_rel in pdfs:
        entries.append((ROOT / src_rel, f"{PREFIX}/{dest_rel}"))

    # --- Diagrams ---
    diagrams = [
        (
            "docs/diagrams/atomik_core_v2_logic.svg",
            "06_Technical_Evidence/diagrams/atomik_core_v2_logic.svg",
        ),
        (
            "docs/diagrams/sdk_pipeline.svg",
            "06_Technical_Evidence/diagrams/sdk_pipeline.svg",
        ),
        (
            "docs/diagrams/phase6_parallel_banks.svg",
            "06_Technical_Evidence/diagrams/phase6_parallel_banks.svg",
        ),
        (
            "docs/diagrams/phase6_throughput_scaling.svg",
            "06_Technical_Evidence/diagrams/phase6_throughput_scaling.svg",
        ),
        (
            "docs/diagrams/atomik_ecosystem.png",
            "06_Technical_Evidence/diagrams/atomik_ecosystem.png",
        ),
    ]
    for src_rel, dest_rel in diagrams:
        entries.append((ROOT / src_rel, f"{PREFIX}/{dest_rel}"))

    return entries


def build() -> None:
    entries = _copy_map()

    # Check all sources exist
    missing = [str(src) for src, _ in entries if not src.exists()]
    if missing:
        print("ERROR: Missing source files:")
        for m in missing:
            print(f"  {m}")
        sys.exit(1)

    # Build zip
    total_size = 0
    file_count = 0
    with ZipFile(OUTPUT_ZIP, "w") as zf:
        for src, dest in entries:
            zf.write(src, dest)
            total_size += src.stat().st_size
            file_count += 1

    size_mb = total_size / (1024 * 1024)
    zip_size_mb = OUTPUT_ZIP.stat().st_size / (1024 * 1024)

    print(f"ATOMiK Business Package built successfully!")
    print(f"  Files:         {file_count}")
    print(f"  Total size:    {size_mb:.1f} MB (uncompressed)")
    print(f"  Zip size:      {zip_size_mb:.1f} MB")
    print(f"  Output:        {OUTPUT_ZIP}")

    # List contents
    print(f"\nContents:")
    with ZipFile(OUTPUT_ZIP, "r") as zf:
        for info in sorted(zf.infolist(), key=lambda i: i.filename):
            if not info.filename.endswith("/"):
                # Strip prefix for cleaner display
                display = info.filename.replace(f"{PREFIX}/", "", 1)
                size_kb = info.file_size / 1024
                print(f"  {display:<60s} {size_kb:>8.1f} KB")


if __name__ == "__main__":
    build()
