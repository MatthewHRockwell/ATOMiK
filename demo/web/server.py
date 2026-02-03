"""FastAPI web server bridging DemoOrchestrator events to WebSocket clients."""

from __future__ import annotations

import asyncio
import json
import logging
from pathlib import Path
from typing import Any

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles

from demo.orchestrator import DemoMode, DemoOrchestrator

log = logging.getLogger(__name__)

STATIC_DIR = Path(__file__).parent / "static"

app = FastAPI(title="ATOMiK Demo Dashboard")
app.mount("/static", StaticFiles(directory=str(STATIC_DIR)), name="static")

# Shared state — set up by run_web_server()
_orchestrator: DemoOrchestrator | None = None
_clients: set[WebSocket] = set()
_event_queue: asyncio.Queue[dict[str, Any]] = asyncio.Queue()
_broadcaster_task: asyncio.Task[None] | None = None


@app.on_event("startup")
async def startup_event() -> None:
    """Start the event broadcaster background task."""
    global _broadcaster_task
    _broadcaster_task = asyncio.create_task(_event_broadcaster())


@app.get("/")
async def index() -> FileResponse:
    return FileResponse(STATIC_DIR / "index.html")


@app.get("/investor")
async def investor_page() -> FileResponse:
    return FileResponse(STATIC_DIR / "investor.html")


@app.get("/engineer")
async def engineer_page() -> FileResponse:
    return FileResponse(STATIC_DIR / "engineer.html")


@app.get("/demos")
async def demos_page() -> FileResponse:
    return FileResponse(STATIC_DIR / "demos.html")


@app.get("/video")
async def video_page() -> FileResponse:
    return FileResponse(STATIC_DIR / "video.html")


@app.get("/api/metrics")
async def api_metrics() -> dict:
    """Return key project metrics for dashboard pages."""
    return {
        "throughput_gops": 1.056,
        "hardware_tests": "80/80",
        "formal_proofs": 92,
        "sdk_tests": 314,
        "sdk_languages": 5,
        "device_cost": 10,
        "dev_cost": 225,
        "lut_utilization": 7,
        "clock_mhz": 94.5,
        "latency_ns": 10.6,
        "parallel_banks": 16,
        "tam_b": 614,
        "sam_b": 12,
        "som_m": 80,
    }


@app.get("/api/benchmark")
async def api_benchmark() -> dict:
    """Return cached benchmark results if available."""
    import json as _json
    results_path = Path(__file__).parent.parent.parent / (
        "software/demos/state_sync_benchmark/results/results.json"
    )
    if results_path.exists():
        return _json.loads(results_path.read_text(encoding="utf-8"))
    return {"error": "No benchmark results. Run: python -m software.demos.state_sync_benchmark"}


@app.websocket("/ws")
async def websocket_endpoint(ws: WebSocket) -> None:
    await ws.accept()
    _clients.add(ws)
    log.info("WebSocket client connected (%d total)", len(_clients))

    # Send initial state
    if _orchestrator:
        await ws.send_json({
            "type": "init",
            "snapshots": _orchestrator.snapshots(),
            "mode": _orchestrator.mode.value,
            "state": {
                "hw_count": _orchestrator.state.hw_count,
                "sim_count": _orchestrator.state.sim_count,
            },
        })

    try:
        while True:
            # Wait for client messages (act triggers)
            data = await ws.receive_text()
            msg = json.loads(data)
            action = msg.get("action")

            if action == "run_all" and _orchestrator:
                _run_in_background(_orchestrator.run_all)
            elif action == "run_act" and _orchestrator:
                act_num = int(msg.get("act", 1))
                _run_in_background(lambda n=act_num: _orchestrator.run_single_act(n))
            elif action == "refresh" and _orchestrator:
                await ws.send_json({
                    "type": "snapshots",
                    "snapshots": _orchestrator.snapshots(),
                })
    except WebSocketDisconnect:
        _clients.discard(ws)
        log.info("WebSocket client disconnected (%d remaining)", len(_clients))


def _run_in_background(fn: Any) -> None:
    """Run a blocking orchestrator function in a thread."""
    import threading
    threading.Thread(target=fn, daemon=True).start()


async def _broadcast(msg: dict[str, Any]) -> None:
    """Send a message to all connected WebSocket clients."""
    dead: set[WebSocket] = set()
    for ws in _clients:
        try:
            await ws.send_json(msg)
        except Exception:
            dead.add(ws)
    _clients.difference_update(dead)


async def _event_broadcaster() -> None:
    """Background task that reads from the event queue and broadcasts."""
    while True:
        msg = await _event_queue.get()
        await _broadcast(msg)


def _on_event(event: str, data: dict[str, Any]) -> None:
    """Callback for orchestrator events — thread-safe enqueue."""
    try:
        _event_queue.put_nowait({"type": event, **data})
    except Exception:
        pass


def run_web_server(
    mode: DemoMode = DemoMode.SIMULATE,
    host: str = "127.0.0.1",
    port: int = 8000,
) -> None:
    """Start the FastAPI server with orchestrator."""
    global _orchestrator

    import uvicorn

    _orchestrator = DemoOrchestrator(mode=mode, on_event=_on_event)
    _orchestrator.setup()

    log.info("Starting web dashboard at http://%s:%d", host, port)

    uvicorn.run(app, host=host, port=port, log_level="info")
