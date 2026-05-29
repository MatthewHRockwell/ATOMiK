#!/usr/bin/env python3
"""Disabled legacy data-room generator.

This script previously emitted claim-heavy financial and technical drafts that
are no longer aligned with the May 2026 Aggie Angel pitch packet. The current
data-room files are hand-reviewed and source-backed. Do not regenerate them from
this legacy script.
"""

from __future__ import annotations

import sys


def main() -> int:
    print(
        "ERROR: business/data_room/_generate.py is disabled. "
        "Use the reviewed files in business/data_room/ directly.",
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
