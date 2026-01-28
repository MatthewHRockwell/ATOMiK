"""ATOMiK 3-Node Demo — CLI entry point.

Usage:
    python -m demo.run_demo                          # auto-discover, TUI
    python -m demo.run_demo --mode simulate          # all simulated
    python -m demo.run_demo --mode simulate --web    # TUI + web dashboard
    python -m demo.run_demo --presentation           # step-by-step narration
    python -m demo.run_demo --act 3                  # run single act
    python -m demo.run_demo --headless               # no TUI, console output
"""

from __future__ import annotations

import argparse
import logging
import threading

from demo.orchestrator import DemoMode, DemoOrchestrator


def _parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        prog="demo",
        description="ATOMiK 3-Node VC Demo",
    )
    p.add_argument(
        "--mode",
        choices=["auto", "simulate", "hardware"],
        default="auto",
        help="Node backend mode (default: auto-discover)",
    )
    p.add_argument("--web", action="store_true", help="Also launch web dashboard on :8000")
    p.add_argument("--presentation", action="store_true", help="Enable narration text for live demos")
    p.add_argument("--act", type=int, choices=[1, 2, 3, 4, 5], help="Run a single act and exit")
    p.add_argument("--headless", action="store_true", help="Console output only (no TUI)")
    p.add_argument("--web-only", action="store_true", help="Web dashboard only (no TUI)")
    p.add_argument("--port", type=int, default=8000, help="Web dashboard port")
    return p.parse_args()


def _run_headless(mode: DemoMode, act_number: int | None) -> None:
    """Run without TUI — console output."""
    def log_event(event: str, data: dict) -> None:
        if event == "act_start":
            print(f"\n{'=' * 60}")
            print(f"Act {data['number']}: {data['title']}")
            print(f"{'=' * 60}")
        elif event == "act_complete":
            status = "PASS" if data["passed"] else "FAIL"
            print(f"  [{status}] {data['summary']}")

    orch = DemoOrchestrator(mode=mode, on_event=log_event)
    orch.setup()

    hw = orch.state.hw_count
    sim = orch.state.sim_count
    print(f"ATOMiK 3-Node Demo — {hw} hardware + {sim} simulated")
    print()

    try:
        if act_number:
            result = orch.run_single_act(act_number)
            if result:
                print()
                for line in result.details:
                    print(f"  {line}")
        else:
            results = orch.run_all()
            print()
            for result in results:
                print(f"\n  --- Act {result.act_number}: {result.title} ---")
                for line in result.details:
                    print(f"  {line}")

            passed = sum(1 for r in results if r.passed)
            print(f"\n{'=' * 60}")
            print(f"  Final: {passed}/{len(results)} acts passed")
            print(f"{'=' * 60}")
    finally:
        orch.teardown()


def _start_web_server(mode: DemoMode, port: int) -> None:
    """Start web dashboard in a background thread."""
    from demo.web.server import run_web_server
    thread = threading.Thread(
        target=run_web_server,
        kwargs={"mode": mode, "port": port},
        daemon=True,
    )
    thread.start()
    print(f"Web dashboard: http://127.0.0.1:{port}")


def main() -> None:
    args = _parse_args()
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(name)s %(levelname)s %(message)s",
    )

    mode = DemoMode(args.mode)

    # Web-only mode
    if args.web_only:
        from demo.web.server import run_web_server
        run_web_server(mode=mode, port=args.port)
        return

    # Headless or single-act mode
    if args.headless or args.act:
        if args.web:
            _start_web_server(mode, args.port)
        _run_headless(mode, args.act)
        return

    # TUI mode (optionally with web)
    if args.web:
        _start_web_server(mode, args.port)

    from demo.tui.app import DemoApp
    app = DemoApp(mode=mode, presentation=args.presentation)
    app.run()


if __name__ == "__main__":
    main()
