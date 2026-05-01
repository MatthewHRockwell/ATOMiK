#!/usr/bin/env python3
"""
slide_sync.py -- PowerPoint slide sync controller for ATOMiK live demo.

Monitors /tmp/atomik_slide.txt for slide number changes and triggers
board actions via serial + narration via websocket.

Usage:
    python3 slide_sync.py              # monitor mode
    python3 slide_sync.py --test       # test all slides (5s between each)
    echo 7 > /tmp/atomik_slide.txt     # manual slide trigger
"""

import argparse
import asyncio
import os
import signal
import sys
import time

SLIDE_FILE = "/tmp/atomik_slide.txt"
SERIAL_PORT = "/dev/ttyUSB2"
SERIAL_BAUD = 921600
WS_URL = "ws://localhost:8766"
POLL_INTERVAL = 0.2  # 200ms

SLIDE_MAP = {
    1:  {"action": None,   "chat": "ATOMiK pitch deck started. Board standing by."},
    2:  {"action": None,   "chat": "The problem: wasteful state rediscovery."},
    3:  {"action": None,   "chat": "The solution: delta-state algebra."},
    4:  {"action": None,   "chat": "How it works: XOR accumulation."},
    5:  {"action": None,   "chat": "Hardware architecture overview."},
    6:  {"action": "R",    "chat": "LIVE DEMO: Resetting board for clean start..."},
    7:  {"action": "1",    "chat": "LIVE DEMO: Modifying agent.ctx -- watch ATOMiK detect the change."},
    8:  {"action": "135",  "chat": "LIVE DEMO: Multiple buffer changes -- ATOMiK skips unchanged regions."},
    9:  {"action": "C",    "chat": "LIVE DEMO: Break It challenge -- corrupting one byte..."},
    10: {"action": "B",    "chat": "LIVE DEMO: Benchmark race -- memcmp vs ATOMiK."},
    11: {"action": "I",    "chat": "LIVE DEMO: AI training/inference workload."},
    12: {"action": "W",    "chat": "LIVE DEMO: Virtual processor reconfiguration."},
    13: {"action": "T",    "chat": "LIVE DEMO: TCO calculator -- your savings at scale."},
    14: {"action": "F",    "chat": "Adoption forecast: $1.7B TAM."},
    15: {"action": "S",    "chat": "Session summary: proof of live execution."},
}

# --- Globals for graceful shutdown ---
_running = True


def _sigint_handler(signum, frame):
    global _running
    print("\n[slide_sync] Caught SIGINT, shutting down...")
    _running = False


def read_slide_number():
    """Read the current slide number from the slide file. Returns 0 if missing/invalid."""
    try:
        with open(SLIDE_FILE, "r") as f:
            return int(f.read().strip())
    except (FileNotFoundError, ValueError):
        return 0


def send_serial_action(action_str):
    """Send each character of action_str to the board via serial with appropriate delays."""
    import serial

    try:
        ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=1)
    except serial.SerialException as e:
        print(f"  [serial] ERROR: {e}")
        return

    try:
        for i, ch in enumerate(action_str):
            ser.write(ch.encode("ascii"))
            if len(action_str) > 1 and i < len(action_str) - 1:
                # Multi-char actions: 300ms between chars
                time.sleep(0.3)
            else:
                # Single-char or last char: 3ms settle
                time.sleep(0.003)
        print(f"  [serial] Sent: {action_str!r}")
    except serial.SerialException as e:
        print(f"  [serial] Write error: {e}")
    finally:
        ser.close()


async def send_chat_message(message):
    """Send a narration message to the browser chat via websocket."""
    try:
        import websockets
    except ImportError:
        print("  [chat] websockets not installed, skipping chat message")
        return

    try:
        async with websockets.connect(WS_URL) as ws:
            await ws.send(message)
            print(f"  [chat] Sent: {message!r}")
    except Exception as e:
        print(f"  [chat] ERROR: {e}")


def handle_slide(slide_num):
    """Process a slide transition."""
    entry = SLIDE_MAP.get(slide_num)
    if entry is None:
        print(f"[slide {slide_num}] No mapping defined, ignoring.")
        return

    action = entry["action"]
    chat = entry["chat"]
    action_display = action if action else "(none)"
    print(f'[slide {slide_num}] action: {action_display}, chat: "{chat}"')

    # Send serial action if defined
    if action:
        send_serial_action(action)

    # Send chat narration
    asyncio.get_event_loop().run_until_complete(send_chat_message(chat))


def monitor_loop():
    """Main polling loop: watch slide file for changes."""
    global _running

    print(f"[slide_sync] Monitoring {SLIDE_FILE} (poll every {int(POLL_INTERVAL * 1000)}ms)")
    print(f"[slide_sync] Serial: {SERIAL_PORT} @ {SERIAL_BAUD}")
    print(f"[slide_sync] WebSocket: {WS_URL}")
    print("[slide_sync] Press Ctrl+C to stop.\n")

    last_slide = read_slide_number()
    if last_slide > 0:
        print(f"[slide_sync] Current slide file reads: {last_slide}")

    while _running:
        current = read_slide_number()
        if current != last_slide and current > 0:
            handle_slide(current)
            last_slide = current
        time.sleep(POLL_INTERVAL)

    print("[slide_sync] Stopped.")


def test_mode():
    """Write slide numbers 1-15 to the slide file with 5s delays."""
    global _running

    print("[slide_sync] TEST MODE: cycling through slides 1-15 (5s each)\n")

    for slide_num in range(1, 16):
        if not _running:
            break
        with open(SLIDE_FILE, "w") as f:
            f.write(str(slide_num))
        print(f"--- Wrote slide {slide_num} to {SLIDE_FILE} ---")
        handle_slide(slide_num)
        if slide_num < 15 and _running:
            print(f"    Waiting 5s before next slide...\n")
            # Sleep in small increments so SIGINT is responsive
            for _ in range(50):
                if not _running:
                    break
                time.sleep(0.1)

    print("\n[slide_sync] Test complete.")


def main():
    parser = argparse.ArgumentParser(
        description="ATOMiK slide sync controller -- bridges pitch deck to live hardware demo"
    )
    parser.add_argument(
        "--test", action="store_true",
        help="Test mode: write slides 1-15 to slide file with 5s delays"
    )
    args = parser.parse_args()

    signal.signal(signal.SIGINT, _sigint_handler)

    if args.test:
        test_mode()
    else:
        monitor_loop()


if __name__ == "__main__":
    main()
