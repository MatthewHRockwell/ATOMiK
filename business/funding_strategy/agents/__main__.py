"""CLI entry point: python -m business.funding_strategy.agents [command]"""

from __future__ import annotations

import asyncio
import sys

from .runner import FundingPipeline

USAGE = """\
Usage: python -m business.funding_strategy.agents <command>

Commands:
  plan      Show execution plan with readiness status
  begin     Run the submission pipeline
  status    Show progress dashboard
  debug     Show the structured issue/debug log
  dataroom  Regenerate the investor data room
  sync      Force docs + context sync (no commit)
"""


def main() -> None:
    cmd = sys.argv[1] if len(sys.argv) > 1 else "plan"

    if cmd in ("--help", "-h"):
        print(USAGE)
        return

    pipeline = FundingPipeline()

    if cmd == "plan":
        pipeline.plan()
    elif cmd == "begin":
        phase = int(sys.argv[2]) if len(sys.argv) > 2 else None
        asyncio.run(pipeline.begin(phase))
    elif cmd == "status":
        pipeline.status()
    elif cmd == "debug":
        pipeline.debug_log.show()
    elif cmd == "dataroom":
        _regenerate_dataroom()
    elif cmd == "sync":
        written = pipeline.docs_sync.sync(pipeline.tracker, pipeline.config)
        if written:
            print(f"Updated: {', '.join(written)}")
        else:
            print("Nothing to sync.")
    else:
        print(USAGE)
        sys.exit(1)


def _regenerate_dataroom() -> None:
    """Run the data room generation script if available."""
    from pathlib import Path

    script = Path("business/data_room/_generate.py")
    if not script.exists():
        print("Data room generator not found: business/data_room/_generate.py")
        sys.exit(1)

    import runpy

    runpy.run_path(str(script), run_name="__main__")


if __name__ == "__main__":
    main()
