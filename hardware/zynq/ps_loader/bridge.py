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
                        if len(parts) >= 6:
                            state["speedup"] = float(parts[5])
                        if len(parts) >= 8:
                            state["sw_kb"] = int(parts[6])
                            state["hw_kb"] = int(parts[7])
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


HTML_PAGE = r"""<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>ATOMiK Control Plane</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { background: #08111A; color: #F3F7FB; font-family: 'SF Mono','Fira Code','Courier New',monospace; font-size: 13px; }

.layout { display: grid; grid-template-columns: 280px 1fr 300px; grid-template-rows: 56px 1fr 32px; height: 100vh; gap: 1px; background: #141E2B; }

/* Top bar */
.topbar { grid-column: 1/-1; background: #0B1520; display: flex; align-items: center; padding: 0 24px; gap: 16px; }
.topbar .logo { color: #1EC8FF; font-size: 18px; font-weight: bold; letter-spacing: 1px; }
.topbar .sep { color: #2A3644; }
.topbar .title { color: #9AA8B5; font-size: 14px; }
.topbar .right { margin-left: auto; display: flex; gap: 12px; align-items: center; }
.pill { padding: 4px 14px; border-radius: 3px; font-size: 11px; font-weight: bold; text-transform: uppercase; }
.pill.green { background: #39D98A; color: #08111A; }
.pill.blue { background: #1EC8FF; color: #08111A; }
.pill.red { background: #FF4444; color: #FFF; }
.pill.gray { background: #2A3644; color: #9AA8B5; }

/* Left panel: state map */
.left { background: #0B1520; padding: 16px; overflow-y: auto; }
.panel-title { color: #9AA8B5; font-size: 11px; text-transform: uppercase; letter-spacing: 1.5px; margin-bottom: 12px; }
.buf-row { display: flex; align-items: center; padding: 10px 12px; margin-bottom: 4px; border-radius: 4px; transition: all 0.3s; }
.buf-row.clean { background: #0F1B28; }
.buf-row.changed { background: #0D2A3D; border-left: 3px solid #1EC8FF; }
.buf-row .name { flex: 1; color: #F3F7FB; font-size: 13px; }
.buf-row .size { color: #9AA8B5; font-size: 11px; width: 50px; text-align: right; }
.buf-row .delta { font-size: 11px; width: 70px; text-align: right; margin-left: 8px; }
.buf-row.clean .delta { color: #2A3644; }
.buf-row.changed .delta { color: #1EC8FF; }
.buf-row .dot { width: 8px; height: 8px; border-radius: 50%; margin-right: 10px; }
.buf-row.clean .dot { background: #2A3644; }
.buf-row.changed .dot { background: #1EC8FF; box-shadow: 0 0 8px #1EC8FF40; }

/* Center: delta flow */
.center { background: #0B1520; padding: 24px; display: flex; flex-direction: column; }
.center .hero { display: flex; gap: 24px; margin-bottom: 24px; }
.card { background: #0F1B28; border-radius: 6px; padding: 20px; flex: 1; }
.card .card-label { color: #9AA8B5; font-size: 11px; text-transform: uppercase; letter-spacing: 1px; margin-bottom: 8px; }
.card .card-value { font-size: 36px; font-weight: bold; }
.card .card-unit { color: #9AA8B5; font-size: 13px; margin-top: 4px; }
.green { color: #39D98A; }
.blue { color: #1EC8FF; }
.orange { color: #FF8A3D; }
.dim { color: #9AA8B5; }

.flow-title { color: #9AA8B5; font-size: 11px; text-transform: uppercase; letter-spacing: 1.5px; margin: 16px 0 8px; }
.flow-bar { display: flex; height: 32px; border-radius: 4px; overflow: hidden; margin-bottom: 8px; }
.flow-bar .sw { background: #FF8A3D; transition: width 0.5s; }
.flow-bar .hw { background: #1EC8FF; transition: width 0.5s; }
.flow-bar .avoided { background: #0F1B28; flex: 1; }
.flow-legend { display: flex; gap: 20px; font-size: 11px; color: #9AA8B5; margin-bottom: 16px; }
.flow-legend span::before { content: ''; display: inline-block; width: 10px; height: 10px; border-radius: 2px; margin-right: 6px; vertical-align: middle; }
.flow-legend .l-sw::before { background: #FF8A3D; }
.flow-legend .l-hw::before { background: #1EC8FF; }
.flow-legend .l-av::before { background: #39D98A; }

/* Event log */
.log { flex: 1; overflow-y: auto; margin-top: 12px; }
.log-entry { padding: 4px 0; border-bottom: 1px solid #141E2B; font-size: 12px; color: #9AA8B5; }
.log-entry.recent { color: #F3F7FB; }
.log-entry .ts { color: #2A3644; margin-right: 8px; }

/* Right panel: economics */
.right-panel { background: #0B1520; padding: 16px; }
.econ-card { background: #0F1B28; border-radius: 6px; padding: 16px; margin-bottom: 8px; }
.econ-card .val { font-size: 28px; font-weight: bold; margin: 4px 0; }
.econ-card .lbl { font-size: 11px; color: #9AA8B5; text-transform: uppercase; letter-spacing: 1px; }
.econ-card .sub { font-size: 11px; color: #2A3644; margin-top: 4px; }

.info-row { display: flex; justify-content: space-between; padding: 6px 0; border-bottom: 1px solid #141E2B; }
.info-row .k { color: #9AA8B5; font-size: 11px; }
.info-row .v { color: #F3F7FB; font-size: 11px; }

/* Bottom bar */
.bottombar { grid-column: 1/-1; background: #0B1520; display: flex; align-items: center; padding: 0 24px; gap: 16px; font-size: 11px; color: #2A3644; }
.bottombar kbd { background: #141E2B; padding: 1px 6px; border-radius: 2px; color: #9AA8B5; }

</style>
</head>
<body>
<div class="layout">

<!-- Top bar -->
<div class="topbar">
    <span class="logo">ATOMiK</span>
    <span class="sep">|</span>
    <span class="title">Control Plane</span>
    <div class="right">
        <span class="pill gray" id="status">CONNECTING</span>
        <span class="pill blue">EDGE-01</span>
    </div>
</div>

<!-- Left: State Map -->
<div class="left">
    <div class="panel-title">State Map</div>
    <div id="buffers"></div>
</div>

<!-- Center: Delta Flow -->
<div class="center">
    <div class="hero">
        <div class="card">
            <div class="card-label">Data Avoided</div>
            <div class="card-value green" id="pct">--</div>
            <div class="card-unit">of total state scanned</div>
        </div>
        <div class="card">
            <div class="card-label">Synced This Cycle</div>
            <div class="card-value blue" id="synced">--</div>
            <div class="card-unit">of 8 buffers</div>
        </div>
        <div class="card">
            <div class="card-label">Total Cycles</div>
            <div class="card-value" id="cycles" style="color:#F3F7FB">--</div>
            <div class="card-unit">detection passes</div>
        </div>
    </div>

    <div class="flow-title">Delta Propagation</div>
    <div class="flow-bar">
        <div class="sw" id="sw-bar" style="width:100%"></div>
        <div class="hw" id="hw-bar" style="width:0%"></div>
        <div class="avoided"></div>
    </div>
    <div class="flow-legend">
        <span class="l-sw">Software path</span>
        <span class="l-hw">ATOMiK path</span>
        <span class="l-av">Avoided</span>
    </div>

    <div class="flow-title">Event Log</div>
    <div class="log" id="log"></div>
</div>

<!-- Right: Economics -->
<div class="right-panel">
    <div class="panel-title">Economics</div>
    <div class="econ-card">
        <div class="lbl">Compute Avoided</div>
        <div class="val green" id="compute">--</div>
        <div class="sub">less energy per cycle</div>
    </div>
    <div class="econ-card">
        <div class="lbl">Projected Savings</div>
        <div class="val green" id="savings">--</div>
        <div class="sub">at 1,000 servers / year</div>
    </div>

    <div class="econ-card">
        <div class="lbl">Query Speedup</div>
        <div class="val blue" id="speedup">--</div>
        <div class="sub">vs software byte scan</div>
    </div>

    <div class="panel-title" style="margin-top:8px">Data Volume</div>
    <div class="info-row"><span class="k">SW Scanned</span><span class="v" id="sw-kb">--</span></div>
    <div class="info-row"><span class="k">ATOMiK Touched</span><span class="v" id="hw-kb">--</span></div>
    <div class="info-row"><span class="k">Avoided</span><span class="v green" id="bytes-avoided">--</span></div>

    <div class="panel-title" style="margin-top:8px">System</div>
    <div class="info-row"><span class="k">Node</span><span class="v">edge-01</span></div>
    <div class="info-row"><span class="k">Replica</span><span class="v">control-plane</span></div>
    <div class="info-row"><span class="k">Link</span><span class="v" id="link">UART 921600</span></div>
    <div class="info-row"><span class="k">Board</span><span class="v">Zynq-7020</span></div>
    <div class="info-row"><span class="k">CPU</span><span class="v">NaxRiscv RV64GC</span></div>
    <div class="info-row"><span class="k">Clock</span><span class="v">100 MHz</span></div>
    <div class="info-row"><span class="k">Build</span><span class="v" id="build">e974995</span></div>
</div>

<!-- Bottom bar -->
<div class="bottombar">
    <span>Press <kbd>1</kbd>-<kbd>8</kbd> to modify buffers</span>
    <span><kbd>a</kbd> all</span>
    <span><kbd>r</kbd> reset</span>
    <span style="margin-left:auto">Only changed state crosses the wire</span>
</div>

</div>

<script>
const N = 8;
const names = ['agent.ctx','model.wt','session.st','config.db',
               'cache.hot','replica.0','txn.log','sensor.buf'];
const sizes = ['4 KB','4 KB','4 KB','4 KB','4 KB','4 KB','4 KB','4 KB'];

const grid = document.getElementById('buffers');
for (let i = 0; i < N; i++) {
    const div = document.createElement('div');
    div.className = 'buf-row clean';
    div.id = `buf${i}`;
    div.innerHTML = `<div class="dot"></div>
                     <div class="name">${names[i]}</div>
                     <div class="size">${sizes[i]}</div>
                     <div class="delta">—</div>`;
    grid.appendChild(div);
}

const logEl = document.getElementById('log');
let logEntries = [];
function addLog(msg) {
    const now = new Date();
    const ts = now.toTimeString().slice(0,8);
    logEntries.push({ts, msg});
    if (logEntries.length > 50) logEntries.shift();
    logEl.innerHTML = logEntries.map((e,i) =>
        `<div class="log-entry ${i === logEntries.length-1 ? 'recent' : ''}">` +
        `<span class="ts">${e.ts}</span>${e.msg}</div>`
    ).reverse().join('');
}

addLog('Control plane initialized');

let ws;
function connect() {
    ws = new WebSocket('ws://localhost:8766');
    ws.onopen = () => {
        const s = document.getElementById('status');
        s.className = 'pill green'; s.textContent = 'LIVE';
        addLog('Connected to edge-01');
    };
    ws.onclose = () => {
        const s = document.getElementById('status');
        s.className = 'pill red'; s.textContent = 'DISCONNECTED';
        addLog('Connection lost — reconnecting...');
        setTimeout(connect, 2000);
    };
    ws.onmessage = (e) => {
        const d = JSON.parse(e.data);
        if (d.type === 'event' || d.type === 'state') {
            const pct = Math.round(d.pct_avoided);
            document.getElementById('pct').textContent = pct + '%';
            document.getElementById('synced').textContent = d.changed + ' of 8';
            document.getElementById('cycles').textContent = d.cycle;
            document.getElementById('compute').textContent = pct + '%';
            const sav = Math.round(pct * 50 * 1000 / 100 / 1000);
            document.getElementById('savings').textContent = '$' + (sav < 1 && pct > 0 ? 1 : sav) + 'K';

            // Update bytes counters if available
            if (d.sw_kb !== undefined) {
                const avoided_kb = d.sw_kb - d.hw_kb;
                const byEl = document.getElementById('bytes-avoided');
                if (byEl) byEl.textContent = avoided_kb + ' KB';
                const swEl = document.getElementById('sw-kb');
                if (swEl) swEl.textContent = d.sw_kb + ' KB';
                const hwEl = document.getElementById('hw-kb');
                if (hwEl) hwEl.textContent = d.hw_kb + ' KB';
            }
            if (d.speedup && d.speedup > 1) {
                const spEl = document.getElementById('speedup');
                if (spEl) spEl.textContent = '~' + Math.round(d.speedup) + 'x';
            }

            // Flow bars
            const hwPct = 100 - pct;
            document.getElementById('sw-bar').style.width = '100%';
            document.getElementById('hw-bar').style.width = hwPct + '%';

            // Buffer states
            let changedNames = [];
            for (let i = 0; i < N; i++) {
                const el = document.getElementById(`buf${i}`);
                if (d.buffers && d.buffers[i]) {
                    el.className = 'buf-row changed';
                    el.querySelector('.delta').textContent = 'SYNCED';
                    el.querySelector('.delta').style.color = '#1EC8FF';
                    changedNames.push(names[i]);
                } else {
                    el.className = 'buf-row clean';
                    el.querySelector('.delta').textContent = '—';
                    el.querySelector('.delta').style.color = '#2A3644';
                }
            }

            if (d.type === 'event' && d.changed > 0) {
                addLog(`Delta applied: ${changedNames.join(', ')} (${pct}% avoided)`);
            }
        }
    };
}

document.addEventListener('keydown', (e) => {
    if ('12345678arq'.includes(e.key) && ws && ws.readyState === 1) {
        ws.send(JSON.stringify({type: 'key', key: e.key}));
        if (e.key >= '1' && e.key <= '8') addLog(`Injected change: ${names[parseInt(e.key)-1]}`);
        else if (e.key === 'a') addLog('Injected change: ALL buffers');
        else if (e.key === 'r') addLog('System reset');
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
