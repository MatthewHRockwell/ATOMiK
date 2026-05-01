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
# Command response queue: UART reader collects ##RSP: lines here
cmd_response = {"lines": [], "complete": False, "lock": threading.Lock()}


async def ws_handler(websocket):
    """Handle a websocket connection from the browser."""
    clients.add(websocket)
    print(f"[ws] Client connected ({len(clients)} total)")
    try:
        # Send current state immediately
        await websocket.send(json.dumps({"type": "state", **state}))
        # Listen for keyboard commands and remote exec from browser/Claude
        async for message in websocket:
            data = json.loads(message)
            if data.get("type") == "key" and ser:
                key = data.get("key", "")
                if len(key) == 1 and (32 <= ord(key) < 127 or ord(key) == 127 or ord(key) == 8):
                    ser.write(key.encode())
                    ser.flush()
                    print(f"[ws→uart] key: {repr(key)}")
            elif data.get("type") == "exec" and ser:
                # Remote command execution: send ~command to board
                cmd = data.get("cmd", "")
                if cmd:
                    with cmd_response["lock"]:
                        cmd_response["lines"] = []
                        cmd_response["complete"] = False
                    ser.write(f"~{cmd}\n".encode())
                    ser.flush()
                    print(f"[ws→uart] exec: {cmd}")
                    # Wait for response (up to 30s)
                    deadline = time.time() + 30
                    while time.time() < deadline:
                        await asyncio.sleep(0.1)
                        with cmd_response["lock"]:
                            if cmd_response["complete"]:
                                result = "\n".join(cmd_response["lines"])
                                await websocket.send(json.dumps({
                                    "type": "exec_result",
                                    "cmd": cmd,
                                    "output": result,
                                }))
                                break
                    else:
                        await websocket.send(json.dumps({
                            "type": "exec_result",
                            "cmd": cmd,
                            "output": "(timeout)",
                        }))
            elif data.get("type") == "chat":
                # Chat message from Claude → broadcast to all browsers
                msg = json.dumps({
                    "type": "chat",
                    "sender": data.get("sender", "Board Claude"),
                    "text": data.get("text", ""),
                    "ts": time.strftime("%H:%M:%S"),
                })
                await asyncio.gather(*(c.send(msg) for c in clients))
                print(f"[chat] {data.get('sender','?')}: {data.get('text','')[:60]}")
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
                if text.startswith("##RSP:"):
                    tag = text[6:]
                    if tag == "END":
                        with cmd_response["lock"]:
                            cmd_response["complete"] = True
                    elif tag.startswith("CMD:") or tag.startswith("EXIT:"):
                        pass  # metadata, skip
                    elif tag.startswith("ERROR:"):
                        with cmd_response["lock"]:
                            cmd_response["lines"].append(tag)
                            cmd_response["complete"] = True
                    else:
                        with cmd_response["lock"]:
                            cmd_response["lines"].append(tag)
                    print(f"[rsp] {tag}")
                elif text.startswith("##EVENT:"):
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

                        # ── Claude auto-narration ──────────────────────
                        narration = None
                        ch = state["changed"]
                        pct = state["pct_avoided"]
                        cyc = state["cycle"]
                        if cyc == 1:
                            narration = "First detection cycle complete. ATOMiK hardware is live."
                        elif ch == 0:
                            if cyc % 10 == 0:
                                narration = f"Cycle {cyc}: all buffers stable. ATOMiK cost: zero."
                        elif ch == 1:
                            names = ['agent.ctx','model.wt','session.st','config.db',
                                     'cache.hot','replica.0','txn.log','sensor.buf']
                            bufs = state.get("buffers", [])
                            changed_name = "unknown"
                            for bi, bv in enumerate(bufs):
                                if bv and bi < len(names):
                                    changed_name = names[bi]
                                    break
                            narration = f"Single change in {changed_name}. {7} buffers skipped — {pct:.0f}% compute saved."
                        elif ch <= 3:
                            narration = f"{ch} buffers changed. ATOMiK skipped {8-ch}, saving {pct:.0f}% bandwidth."
                        elif ch >= 7:
                            if cyc % 5 == 0:
                                narration = f"Heavy workload: {ch}/8 changed. Even at full load, ATOMiK detection is {state.get('speedup',0):.0f}x faster."
                        if narration and cyc > 1:
                            chat_msg = json.dumps({
                                "type": "chat",
                                "sender": "Board Claude",
                                "text": narration,
                                "ts": time.strftime("%H:%M:%S"),
                            })
                            asyncio.run_coroutine_threadsafe(broadcast(chat_msg), loop)
        except Exception as e:
            print(f"[uart] Error: {e}")
            time.sleep(1)


HTML_PAGE = r"""<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ATOMiK Control Plane</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { background: #08111A; color: #F3F7FB; font-family: 'SF Mono','Fira Code','Courier New',monospace; font-size: 13px; }

.layout { display: grid; grid-template-columns: 240px 1fr 280px 280px; grid-template-rows: 56px 1fr 32px; height: 100vh; gap: 1px; background: #141E2B; }

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
.right-panel { background: #0B1520; padding: 16px; overflow-y: auto; }
.econ-card { background: #0F1B28; border-radius: 6px; padding: 12px; margin-bottom: 6px; }
.econ-card .val { font-size: 24px; font-weight: bold; margin: 2px 0; }
.econ-card .lbl { font-size: 11px; color: #9AA8B5; text-transform: uppercase; letter-spacing: 1px; }
.econ-card .sub { font-size: 11px; color: #2A3644; margin-top: 2px; }

/* Chat panel: Board Claude */
.chat-panel { background: #0B1520; padding: 12px; display: flex; flex-direction: column; }
.chat-header { color: #1EC8FF; font-size: 12px; font-weight: bold; text-transform: uppercase; letter-spacing: 1.5px; margin-bottom: 8px; display: flex; align-items: center; gap: 8px; }
.chat-header .dot { width: 8px; height: 8px; border-radius: 50%; background: #39D98A; box-shadow: 0 0 6px #39D98A80; animation: pulse 2s infinite; }
@keyframes pulse { 0%,100% { opacity: 1; } 50% { opacity: 0.4; } }
.chat-messages { flex: 1; overflow-y: auto; display: flex; flex-direction: column; gap: 6px; }
.chat-msg { background: #0F1B28; border-radius: 6px; padding: 8px 10px; font-size: 12px; line-height: 1.4; border-left: 2px solid #1EC8FF; }
.chat-msg .sender { color: #1EC8FF; font-size: 10px; font-weight: bold; text-transform: uppercase; margin-bottom: 2px; }
.chat-msg .text { color: #D0D8E0; }
.chat-msg .time { color: #2A3644; font-size: 10px; float: right; }
.chat-msg.system { border-left-color: #39D98A; }
.chat-msg.system .sender { color: #39D98A; }
.chat-input { margin-top: 8px; display: flex; gap: 4px; }
.chat-input input { flex: 1; background: #141E2B; border: 1px solid #2A3644; border-radius: 4px; padding: 6px 8px; color: #F3F7FB; font-family: inherit; font-size: 12px; outline: none; }
.chat-input input:focus { border-color: #1EC8FF; }

.info-row { display: flex; justify-content: space-between; padding: 6px 0; border-bottom: 1px solid #141E2B; }
.info-row .k { color: #9AA8B5; font-size: 11px; }
.info-row .v { color: #F3F7FB; font-size: 11px; }

/* Bottom bar */
.bottombar { grid-column: 1/-1; background: #0B1520; display: flex; align-items: center; padding: 0 24px; gap: 16px; font-size: 11px; color: #2A3644; }
.bottombar kbd { background: #141E2B; padding: 1px 6px; border-radius: 2px; color: #9AA8B5; }

/* Mobile responsive */
@media (max-width: 800px) {
    .layout { grid-template-columns: 1fr; grid-template-rows: auto; }
    .left { display: none; }
    .right-panel { display: none; }
    .center { width: 100%; }
    .chat-panel { width: 100%; }
    .topbar { flex-wrap: wrap; }
}

/* Touch panel */
.touch-panel { margin-top: 16px; }
.touch-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 8px; margin-bottom: 12px; }
.touch-cell { background: #0F1B28; border: 2px solid #2A3644; border-radius: 8px; padding: 16px 8px; text-align: center; cursor: pointer; transition: all 0.2s; user-select: none; -webkit-user-select: none; }
.touch-cell:active { transform: scale(0.95); background: #1EC8FF; border-color: #1EC8FF; }
.touch-cell .tname { color: #F3F7FB; font-size: 13px; font-weight: bold; }
.touch-cell .tstatus { color: #2A3644; font-size: 11px; margin-top: 4px; }
.touch-cell.changed { border-color: #1EC8FF; background: #0D2A3D; }
.touch-cell.changed .tstatus { color: #1EC8FF; }
.touch-actions { display: flex; gap: 8px; }
.touch-btn { flex: 1; padding: 14px; border: none; border-radius: 6px; font-family: inherit; font-size: 14px; font-weight: bold; cursor: pointer; text-transform: uppercase; }
.touch-btn.burst { background: #FF8A3D; color: #08111A; }
.touch-btn.reset { background: #FF4444; color: #FFF; }

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

    <div class="touch-panel">
        <div class="panel-title">Touch Control</div>
        <div class="touch-grid" id="touch-grid"></div>
        <div class="touch-actions">
            <button class="touch-btn burst" onclick="sendKey('B')">BURST</button>
            <button class="touch-btn reset" onclick="sendKey('R')">RESET</button>
        </div>
    </div>
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
        <div class="lbl">At 1,000 Servers</div>
        <div class="val green" id="savings">--</div>
        <div class="sub">saved per year</div>
    </div>
    <div class="econ-card">
        <div class="lbl">Global TAM (50M Servers)</div>
        <div class="val green" id="savings-global">--</div>
        <div class="sub">total addressable market</div>
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

<!-- Chat: Board Claude -->
<div class="chat-panel">
    <div class="chat-header"><div class="dot"></div>Board Claude</div>
    <div class="chat-messages" id="chat"></div>
</div>

<!-- Bottom bar -->
<div class="bottombar" style="grid-column: 1/-1;">
    <span>Type letters for delta viz</span>
    <span><kbd>Shift+W</kbd> vproc profile</span>
    <span><kbd>Shift+R</kbd> reset</span>
    <span style="margin-left:auto">ATOMiK: Only meaningful changes cross the wire</span>
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

// Touch grid
const touchGrid = document.getElementById('touch-grid');
if (touchGrid) {
    for (let i = 0; i < 8; i++) {
        const cell = document.createElement('div');
        cell.className = 'touch-cell';
        cell.id = 'tcell' + i;
        cell.innerHTML = '<div class="tname">' + names[i] + '</div><div class="tstatus">clean</div>';
        cell.addEventListener('click', () => sendKey(String(i + 1)));
        cell.addEventListener('touchstart', (e) => { e.preventDefault(); sendKey(String(i + 1)); }, {passive: false});
        touchGrid.appendChild(cell);
    }
}
function sendKey(k) {
    if (ws && ws.readyState === 1) {
        ws.send(JSON.stringify({type: 'key', key: k}));
        addLog('Touched: ' + k);
    }
}

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
            document.getElementById('savings').textContent = '$' + (sav < 1 && pct > 0 ? 1 : sav) + 'K/yr';
            const savGlobal = (pct * 50 * 50e6 / 100 / 1e9).toFixed(1);
            document.getElementById('savings-global').textContent = '$' + savGlobal + 'B/yr';

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

            // Update touch cells
            for (let i = 0; i < 8; i++) {
                const tc = document.getElementById('tcell' + i);
                if (tc) {
                    if (d.buffers && d.buffers[i]) {
                        tc.className = 'touch-cell changed';
                        tc.querySelector('.tstatus').textContent = 'SYNCED';
                    } else {
                        tc.className = 'touch-cell';
                        tc.querySelector('.tstatus').textContent = 'clean';
                    }
                }
            }

            if (d.type === 'event' && d.changed > 0) {
                addLog(`Delta applied: ${changedNames.join(', ')} (${pct}% avoided)`);
            }
        }
        /* Chat messages from Board Claude */
        if (d.type === 'chat') {
            addChat(d.sender || 'Board Claude', d.text || '', d.ts || '');
        }
    };
}

/* Chat system */
const chatEl = document.getElementById('chat');
function addChat(sender, text, ts) {
    const cls = sender.includes('System') ? 'system' : '';
    const div = document.createElement('div');
    div.className = 'chat-msg ' + cls;
    div.innerHTML = `<span class="time">${ts}</span><div class="sender">${sender}</div><div class="text">${text}</div>`;
    chatEl.appendChild(div);
    chatEl.scrollTop = chatEl.scrollHeight;
    /* Keep max 50 messages */
    while (chatEl.children.length > 50) chatEl.removeChild(chatEl.firstChild);
}

document.addEventListener('keydown', (e) => {
    if (!ws || ws.readyState !== 1) return;
    if (e.key === 'Backspace') {
        e.preventDefault();
        ws.send(JSON.stringify({type: 'key', key: '<'}));
        addLog('Backspace: delta rolled back');
    } else if (e.key.length === 1) {
        ws.send(JSON.stringify({type: 'key', key: e.key}));
        if (e.key >= '1' && e.key <= '8') addLog(`Injected change: ${names[parseInt(e.key)-1]}`);
        else if (e.key === 'a') addLog('Injected change: ALL buffers');
        else if (e.key === 'r') addLog('System reset');
        else addLog(`Typed: '${e.key}'`);
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
