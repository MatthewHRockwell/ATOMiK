"""ATOMiK Core — run benchmarks or show version info.

Usage:
    python -m atomik_core              # Show version and run quick demo
    python -m atomik_core benchmark    # Run full benchmark suite
    python -m atomik_core benchmark --json  # JSON output for CI
    python -m atomik_core version      # Show version info
"""

import sys
import platform


def _version_info():
    """Print version, Python, and platform details."""
    import atomik_core

    print(f"atomik-core {atomik_core.__version__}")
    print(f"Python {sys.version}")
    print(f"Platform: {platform.platform()}")


def _quick_demo():
    """Run a 4-operation demo showing LOAD, ACCUM, READ, SWAP."""
    from atomik_core import AtomikContext

    print()
    print("Quick demo — the 4 ATOMiK operations:")
    print()

    ctx = AtomikContext(width=64)

    # LOAD
    ctx.load(0xDEADBEEFCAFEBABE)
    print(f"  LOAD  0xDEADBEEFCAFEBABE")
    print(f"    state = {hex(ctx.read())}")

    # ACCUM
    ctx.accum(0x00000000000000FF)
    print(f"  ACCUM 0x00000000000000FF")
    print(f"    state = {hex(ctx.read())}")

    # ACCUM (self-inverse)
    ctx.accum(0x00000000000000FF)
    print(f"  ACCUM 0x00000000000000FF  (self-inverse: undo previous)")
    print(f"    state = {hex(ctx.read())}")

    # SWAP
    snapshot = ctx.swap(0x0000000000000000)
    print(f"  SWAP  0x0000000000000000  (atomic snapshot + new epoch)")
    print(f"    snapshot = {hex(snapshot)}")
    print(f"    state    = {hex(ctx.read())}")

    print()
    print("Run 'python -m atomik_core benchmark' for full benchmarks")


def main():
    args = sys.argv[1:]

    if not args:
        _version_info()
        _quick_demo()
        return

    cmd = args[0]

    if cmd == "version":
        _version_info()
    elif cmd == "benchmark":
        from atomik_core.benchmark import main as bench_main

        # Pass through remaining args (e.g. --json)
        sys.argv = [sys.argv[0]] + args[1:]
        bench_main()
    else:
        print(f"Unknown command: {cmd}", file=sys.stderr)
        print(__doc__, file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
