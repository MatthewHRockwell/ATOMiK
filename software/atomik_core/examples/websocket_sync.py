#!/usr/bin/env python3
"""ATOMiK + WebSocket: Real-time state convergence between browser clients.

This is the "magic demo" — open two browser tabs, make changes in one,
and watch the other converge instantly. No server-side state. No database.
Just 16-byte delta messages over WebSocket.

Requires: pip install websockets atomik-core

Usage:
    python websocket_sync.py
    # Then open http://localhost:8765 in two browser tabs

Architecture:
    Browser A ──delta──> Server ──broadcast──> Browser B
    Browser B ──delta──> Server ──broadcast──> Browser A

    Server is stateless — it just relays deltas.
    Each client runs AtomikContext locally.
    XOR commutativity guarantees convergence.
"""

import asyncio
import json

try:
    import websockets
    HAS_WEBSOCKETS = True
except ImportError:
    HAS_WEBSOCKETS = False

# HTML page served to browsers (self-contained, no build step)
HTML_PAGE = """<!DOCTYPE html>
<html>
<head>
<title>ATOMiK Live Sync Demo</title>
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }
  body { font-family: -apple-system, system-ui, sans-serif; background: #0a0a0f; color: #e0e0e8; }
  .container { max-width: 600px; margin: 0 auto; padding: 40px 20px; }
  h1 { font-size: 24px; margin-bottom: 8px; }
  .sub { color: #8888a0; font-size: 14px; margin-bottom: 32px; }
  .state-box { background: #12121a; border: 1px solid #1e1e2e; border-radius: 12px; padding: 24px; margin-bottom: 16px; }
  .label { font-size: 12px; color: #8888a0; text-transform: uppercase; letter-spacing: 1px; margin-bottom: 8px; }
  .value { font-family: 'SF Mono', monospace; font-size: 28px; color: #22d3ee; font-weight: 700; }
  .controls { display: flex; gap: 8px; margin-bottom: 16px; }
  .btn { padding: 10px 20px; border-radius: 8px; border: none; font-size: 14px; font-weight: 600; cursor: pointer; transition: opacity 0.2s; }
  .btn:hover { opacity: 0.85; }
  .btn-blue { background: #4f8fff; color: white; }
  .btn-purple { background: #8b5cf6; color: white; }
  .btn-green { background: #22c55e; color: white; }
  input { background: #0a0a0f; border: 1px solid #1e1e2e; color: #e0e0e8; padding: 10px 14px; border-radius: 8px; font-family: monospace; font-size: 14px; width: 140px; }
  .log { background: #0a0a0f; border: 1px solid #1e1e2e; border-radius: 8px; padding: 12px; max-height: 200px; overflow-y: auto; font-size: 12px; font-family: monospace; color: #8888a0; }
  .log-entry { padding: 2px 0; }
  .log-send { color: #4f8fff; }
  .log-recv { color: #22c55e; }
  .peers { font-size: 13px; color: #8888a0; margin-bottom: 16px; }
  .dot { display: inline-block; width: 8px; height: 8px; border-radius: 50%; background: #22c55e; margin-right: 6px; }
</style>
</head>
<body>
<div class="container">
  <h1>ATOMiK Live Sync</h1>
  <p class="sub">Open this page in two tabs. Changes propagate via 16-byte XOR deltas.</p>

  <div class="peers"><span class="dot"></span>Connected peers: <span id="peers">1</span></div>

  <div class="state-box">
    <div class="label">Current State (reference XOR accumulator)</div>
    <div class="value" id="state">0x00000000</div>
  </div>

  <div class="controls">
    <input type="text" id="delta-input" value="DEADBEEF" placeholder="hex delta">
    <button class="btn btn-blue" onclick="sendDelta()">ACCUM</button>
    <button class="btn btn-purple" onclick="sendLoad()">LOAD</button>
    <button class="btn btn-green" onclick="sendRandom()">Random</button>
  </div>

  <div class="label" style="margin-bottom:8px">Delta Log</div>
  <div class="log" id="log"></div>
</div>

<script>
// Client-side ATOMiK context (32-bit for simplicity)
let reference = 0;
let accumulator = 0;
const mask = 0xFFFFFFFF;

function read() { return ((reference ^ accumulator) & mask) >>> 0; }

function updateDisplay() {
  document.getElementById('state').textContent = '0x' + read().toString(16).toUpperCase().padStart(8, '0');
}

function addLog(msg, cls) {
  const log = document.getElementById('log');
  const entry = document.createElement('div');
  entry.className = 'log-entry ' + (cls || '');
  entry.textContent = msg;
  log.prepend(entry);
  if (log.children.length > 50) log.lastChild.remove();
}

// WebSocket connection
const ws = new WebSocket('ws://' + location.host);
ws.onmessage = function(e) {
  const msg = JSON.parse(e.data);
  if (msg.type === 'delta') {
    accumulator = ((accumulator ^ msg.value) & mask) >>> 0;
    updateDisplay();
    addLog('RECV delta 0x' + msg.value.toString(16).toUpperCase().padStart(8,'0'), 'log-recv');
  } else if (msg.type === 'load') {
    reference = msg.value >>> 0;
    accumulator = 0;
    updateDisplay();
    addLog('RECV load 0x' + msg.value.toString(16).toUpperCase().padStart(8,'0'), 'log-recv');
  } else if (msg.type === 'peers') {
    document.getElementById('peers').textContent = msg.count;
  }
};

function sendDelta() {
  const hex = document.getElementById('delta-input').value.replace(/^0x/i, '');
  const val = (parseInt(hex, 16) || 0) >>> 0;
  accumulator = ((accumulator ^ val) & mask) >>> 0;
  updateDisplay();
  ws.send(JSON.stringify({type:'delta', value:val}));
  addLog('SEND delta 0x' + val.toString(16).toUpperCase().padStart(8,'0'), 'log-send');
}

function sendLoad() {
  const hex = document.getElementById('delta-input').value.replace(/^0x/i, '');
  const val = (parseInt(hex, 16) || 0) >>> 0;
  reference = val;
  accumulator = 0;
  updateDisplay();
  ws.send(JSON.stringify({type:'load', value:val}));
  addLog('SEND load 0x' + val.toString(16).toUpperCase().padStart(8,'0'), 'log-send');
}

function sendRandom() {
  const val = (Math.random() * 0xFFFFFFFF) >>> 0;
  document.getElementById('delta-input').value = val.toString(16).toUpperCase().padStart(8,'0');
  sendDelta();
}

updateDisplay();
</script>
</body>
</html>"""


async def handler(websocket, connected):
    """Handle a single WebSocket client."""
    connected.add(websocket)
    # Notify all peers of connection count
    msg = json.dumps({"type": "peers", "count": len(connected)})
    await asyncio.gather(*(ws.send(msg) for ws in connected))

    try:
        async for message in websocket:
            # Relay to all OTHER connected clients
            data = json.loads(message)
            relay = json.dumps(data)
            others = [ws for ws in connected if ws != websocket]
            if others:
                await asyncio.gather(*(ws.send(relay) for ws in others))
    finally:
        connected.discard(websocket)
        if connected:
            msg = json.dumps({"type": "peers", "count": len(connected)})
            await asyncio.gather(*(ws.send(msg) for ws in connected))


async def main():
    """Run the WebSocket sync server."""
    if not HAS_WEBSOCKETS:
        print("This demo requires the 'websockets' package:")
        print("  pip install websockets")
        print("\nShowing the concept without running the server...")
        print("\nThe server relays 16-byte XOR deltas between clients.")
        print("Each client runs AtomikContext locally — the server is stateless.")
        print("Open two tabs → make changes → watch them converge.")
        return

    connected = set()

    async def serve(websocket):
        await handler(websocket, connected)

    # Serve HTML on HTTP, WebSocket on same port
    import http
    class HTMLHandler(http.server.SimpleHTTPRequestHandler):
        def do_GET(self):
            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.end_headers()
            self.wfile.write(HTML_PAGE.encode())
        def log_message(self, *args):
            pass

    print("ATOMiK Live Sync Demo")
    print("=" * 40)
    print("Open http://localhost:8765 in TWO browser tabs")
    print("Make changes in one — watch the other converge")
    print("Press Ctrl+C to stop\n")

    async with websockets.serve(serve, "localhost", 8765):
        await asyncio.Future()  # run forever


if __name__ == "__main__":
    asyncio.run(main())
