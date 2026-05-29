#!/usr/bin/env python3
"""Legacy deck updater disabled.

This script belonged to an older investor-deck iteration and may contain stale
metrics or market-comparable assumptions in git history. Do not use it for the
Aggie Angel Network deck.

Use:
    /tmp/atomik-pptx-venv/bin/python business/pitch_deck/generate_deck.py

or install python-pptx in your own environment and run:
    python3 business/pitch_deck/generate_deck.py
"""

from pathlib import Path
import sys

CURRENT = Path(__file__).resolve().with_name("generate_deck.py")
print(f"ERROR: {Path(__file__).name} is a disabled legacy updater.", file=sys.stderr)
print(f"Use {CURRENT} instead.", file=sys.stderr)
raise SystemExit(2)
