#!/usr/bin/env python3
"""
ATOMiK UART-to-Browser Bridge

Reads ##EVENT lines from the board's UART, forwards state change events
to a browser via websocket. The browser shows a live replica dashboard.

Also forwards keyboard input FROM the browser TO the board's stdin,
so the presenter can control the demo from the laptop keyboard while
the board runs standalone on the HDMI.

Usage:
    python3 bridge.py [--port /dev/ttyUSB2] [--baud 921600]

Then open http://localhost:8765 in a browser.
"""

import argparse
import asyncio
import json
import os
import signal
import sys
import threading
import time

try:
    import serial
except ImportError:
    print("pip install pyserial")
    sys.exit(1)

try:
    import websockets
    from websockets.server import serve
except ImportError:
    print("pip install websockets")
    sys.exit(1)

# Global state shared between UART reader and websocket server
state = {
    "cycle": 0,
    "changed": 0,
    "pct_avoided": 0.0,
    "buffers": [0] * 8,
    "total_changes": 0,
    "connected": False,
    "last_event": "",
}
clients = set()


async def ws_handler(websocket):
    """Handle a websocket connection from the browser."""
    clients.add(websocket)
    print(f"[ws] Client connected ({len(clients)} total)")
    try:
        # Send current state immediately
        await websocket.send(json.dumps({"type": "state", **state}))
        # Listen for keyboard commands from browser
        async for message in websocket:
            data = json.loads(message)
            if data.get("type") == "key" and ser:
                key = data.get("key", "")
                if key in "12345678arq":
                    ser.write(key.encode())
                    ser.flush()
                    print(f"[ws→uart] key: {key}")
    except websockets.exceptions.ConnectionClosed:
        pass
    finally:
        clients.discard(websocket)
        print(f"[ws] Client disconnected ({len(clients)} total)")


async def broadcast(msg):
    """Send a message to all connected browser clients."""
    if clients:
        await asyncio.gather(*(c.send(msg) for c in clients))


def uart_reader(port, baud, loop):
    """Read UART in a background thread, parse ##EVENT lines."""
    global ser
    ser = serial.Serial(port, baud, timeout=0.5)
    ser.reset_input_buffer()
    print(f"[uart] Listening on {port} @ {baud}")
    state["connected"] = True

    buf = b""
    while True:
        try:
            data = ser.read(1024)
            if not data:
                continue
            buf += data
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                text = line.decode(errors="replace").strip()
                if text.startswith("##EVENT:"):
                    parts = text.split(":")
                    if len(parts) >= 4:
                        state["cycle"] = int(parts[1])
                        state["changed"] = int(parts[2])
                        state["pct_avoided"] = float(parts[3])
                        if len(parts) >= 5:
                            mask = int(parts[4], 16)
                            state["buffers"] = [(mask >> i) & 1 for i in range(8)]
                        state["total_changes"] += state["changed"]
                        state["last_event"] = text
                        # Broadcast to browsers
                        msg = json.dumps({"type": "event", **state})
                        asyncio.run_coroutine_threadsafe(broadcast(msg), loop)
                        print(f"[event] cycle={state['cycle']} "
                              f"changed={state['changed']} "
                              f"avoided={state['pct_avoided']:.0f}%")
        except Exception as e:
            print(f"[uart] Error: {e}")
            time.sleep(1)


HTML_PAGE = """<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>ATOMiK Replica</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body {
    background: #08111A; color: #F3F7FB;
    font-family: 'Courier New', monospace;
    display: flex; flex-direction: column;
    align-items: center; justify-content: center;
    min-height: 100vh;
}
.header {
    display: flex; align-items: center; gap: 20px;
    margin-bottom: 40px;
}
.header h1 { color: #1EC8FF; font-size: 36px; }
.badge {
    background: #1EC8FF; color: #08111A;
    padding: 8px 20px; border-radius: 4px;
    font-weight: bold; font-size: 14px;
}
.title { color: #9AA8B5; font-size: 18px; margin-bottom: 30px; }
.buffers {
    display: grid; grid-template-columns: repeat(4, 1fr);
    gap: 12px; margin-bottom: 40px;
}
.buf {
    width: 140px; height: 80px;
    display: flex; flex-direction: column;
    align-items: center; justify-content: center;
    border-radius: 6px; transition: background 0.3s;
}
.buf.clean { background: #2A3644; }
.buf.changed { background: #1EC8FF; }
.buf .label { font-size: 12px; color: #9AA8B5; }
.buf.changed .label { color: #08111A; }
.buf .status { font-size: 16px; font-weight: bold; margin-top: 4px; }
.buf.changed .status { color: #08111A; }
.metrics {
    display: flex; gap: 40px; margin-bottom: 40px;
}
.metric {
    text-align: center;
}
.metric .value {
    font-size: 48px; font-weight: bold;
    color: #39D98A;
}
.metric .label {
    font-size: 14px; color: #9AA8B5;
    margin-top: 8px;
}
.controls {
    color: #9AA8B5; font-size: 14px;
    margin-top: 20px;
}
.controls kbd {
    background: #2A3644; padding: 2px 8px;
    border-radius: 3px; color: #F3F7FB;
}
.footer {
    margin-top: 40px; color: #2A3644;
    font-size: 12px;
}
</style>
</head>
<body>
<div class="header">
    <h1>ATOMiK</h1>
    <div class="badge">REPLICA</div>
    <div class="badge" id="live" style="background:#2A3644;color:#9AA8B5">CONNECTING...</div>
</div>
<div class="title">Remote endpoint — receiving only changed state</div>
<div class="buffers" id="buffers"></div>
<div class="metrics">
    <div class="metric">
        <div class="value" id="pct">--</div>
        <div class="label">data avoided</div>
    </div>
    <div class="metric">
        <div class="value" id="changed">--</div>
        <div class="label">synced this cycle</div>
    </div>
    <div class="metric">
        <div class="value" id="cycle">--</div>
        <div class="label">total cycles</div>
    </div>
</div>
<div class="controls">
    Press <kbd>1</kbd>-<kbd>8</kbd> to modify buffers &nbsp;
    <kbd>a</kbd> all &nbsp;
    <kbd>r</kbd> reset
</div>
<div class="footer">
    Only changed state crosses the wire. Unchanged buffers stay dark.
</div>

<script>
const N = 8;
const names = ['agent.ctx','model.wt','session.st','config.db',
               'cache.hot','replica.0','txn.log','sensor.buf'];

// Create buffer elements
const grid = document.getElementById('buffers');
for (let i = 0; i < N; i++) {
    const div = document.createElement('div');
    div.className = 'buf clean';
    div.id = `buf${i}`;
    div.innerHTML = `<div class="label">${names[i]}</div>
                     <div class="status">SKIP</div>`;
    grid.appendChild(div);
}

let ws;
function connect() {
    ws = new WebSocket('ws://localhost:8766');
    ws.onopen = () => {
        document.getElementById('live').style.background = '#39D98A';
        document.getElementById('live').style.color = '#08111A';
        document.getElementById('live').textContent = 'LIVE';
    };
    ws.onclose = () => {
        document.getElementById('live').style.background = '#FF4444';
        document.getElementById('live').textContent = 'DISCONNECTED';
        setTimeout(connect, 2000);
    };
    ws.onmessage = (e) => {
        const d = JSON.parse(e.data);
        if (d.type === 'event' || d.type === 'state') {
            document.getElementById('pct').textContent = Math.round(d.pct_avoided) + '%';
            document.getElementById('changed').textContent = d.changed + ' of 8';
            document.getElementById('cycle').textContent = d.cycle;

            // Update each buffer based on the change mask
            for (let i = 0; i < N; i++) {
                const el = document.getElementById(`buf${i}`);
                if (d.buffers && d.buffers[i]) {
                    el.className = 'buf changed';
                    el.querySelector('.status').textContent = 'SYNCED';
                } else {
                    el.className = 'buf clean';
                    el.querySelector('.status').textContent = 'SKIP';
                }
            }
        }
    };
}

// Forward keyboard to board
document.addEventListener('keydown', (e) => {
    if ('12345678arq'.includes(e.key) && ws && ws.readyState === 1) {
        ws.send(JSON.stringify({type: 'key', key: e.key}));
    }
});

connect();
</script>
</body>
</html>
"""

ser = None


async def http_handler(path, request_headers):
    """Serve the HTML page on the websocket port."""
    pass


async def main(port, baud):
    loop = asyncio.get_event_loop()

    # Start UART reader thread
    uart_thread = threading.Thread(
        target=uart_reader, args=(port, baud, loop), daemon=True
    )
    uart_thread.start()

    # Write HTML file
    html_path = "/tmp/atomik_replica.html"
    with open(html_path, "w") as f:
        f.write(HTML_PAGE)
    print(f"[http] Replica page: file://{html_path}")
    print(f"[http] Or open http://localhost:8765 after starting")

    # Start websocket server
    async with serve(ws_handler, "0.0.0.0", 8766):
        print("[ws] Websocket server on ws://0.0.0.0:8766")
        print("[bridge] Ready. Open the HTML file in a browser.")
        print("[bridge] Press Ctrl-C to stop.")
        await asyncio.Future()  # run forever


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="ATOMiK UART-to-Browser Bridge")
    ap.add_argument("--port", default="/dev/ttyUSB2")
    ap.add_argument("--baud", type=int, default=921600)
    args = ap.parse_args()

    try:
        asyncio.run(main(args.port, args.baud))
    except KeyboardInterrupt:
        print("\n[bridge] Stopped.")
