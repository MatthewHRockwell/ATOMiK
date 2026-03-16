"""Smoke tests for ATOMiK example scripts.

Each example uses mock/simulated backends so no external services are needed.
"""

import io
import sys
import contextlib
import importlib.util
import os
import asyncio

import pytest


EXAMPLES_DIR = os.path.join(
    os.path.dirname(__file__), os.pardir, "examples"
)


def _import_example(name: str):
    """Import an example module by filename (without .py)."""
    path = os.path.normpath(os.path.join(EXAMPLES_DIR, f"{name}.py"))
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


# =========================================================================
# Redis change detection example
# =========================================================================

def test_redis_example():
    """redis_change_detection.main() runs with MockRedis, no real Redis."""
    mod = _import_example("redis_change_detection")
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        mod.main()

    output = buf.getvalue()
    assert "ATOMiK + Redis" in output
    assert "Writes skipped" in output
    assert "Redis writes" in output


# =========================================================================
# PostgreSQL change tracker example
# =========================================================================

def test_postgres_example():
    """postgres_change_tracker.main() runs with simulated data, no real DB."""
    mod = _import_example("postgres_change_tracker")
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        mod.main()

    output = buf.getvalue()
    assert "ATOMiK + PostgreSQL" in output
    assert "Changed" in output
    assert "Added" in output
    assert "Snapshot updated" in output


# =========================================================================
# WebSocket sync example
# =========================================================================

def test_websocket_example_no_deps():
    """websocket_sync fallback path when websockets is absent."""
    # Temporarily hide the websockets package so the example takes
    # the HAS_WEBSOCKETS=False branch.
    saved = sys.modules.get("websockets")
    sys.modules["websockets"] = None  # force ImportError on import

    try:
        # Import fresh so the try/except in the module sees the blocked import
        mod = _import_example("websocket_sync")
        assert mod.HAS_WEBSOCKETS is False

        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            asyncio.run(mod.main())

        output = buf.getvalue()
        assert "concept" in output.lower() or "server is stateless" in output.lower()
        assert "16-byte" in output or "XOR deltas" in output
    finally:
        # Restore original state
        if saved is not None:
            sys.modules["websockets"] = saved
        else:
            sys.modules.pop("websockets", None)
