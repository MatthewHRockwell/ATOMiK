#!/usr/bin/env python3
"""
ATOMiK Board Chat — send messages to the browser chat panel.

Usage:
    python3 board_chat.py "Analyzing buffer states..."
    python3 board_chat.py --sender "System" "Demo initialized"
    python3 board_chat.py "Detected 3.9x speedup in layer detection"

Messages appear in the Board Claude chat panel in the browser control plane.
"""

import asyncio
import json
import sys

WS_URL = "ws://localhost:8766"


async def send_chat(text, sender="Board Claude"):
    try:
        import websockets
    except ImportError:
        print("pip install websockets", file=sys.stderr)
        return

    try:
        async with websockets.connect(WS_URL) as ws:
            # Skip initial state message
            await asyncio.wait_for(ws.recv(), timeout=2)
            # Send chat message
            await ws.send(json.dumps({
                "type": "chat",
                "sender": sender,
                "text": text,
            }))
            # Small delay to ensure delivery
            await asyncio.sleep(0.1)
    except ConnectionRefusedError:
        print("(bridge not running)", file=sys.stderr)
    except Exception as e:
        print(f"(error: {e})", file=sys.stderr)


def main():
    sender = "Board Claude"
    args = sys.argv[1:]

    if "--sender" in args:
        idx = args.index("--sender")
        if idx + 1 < len(args):
            sender = args[idx + 1]
            args = args[:idx] + args[idx+2:]

    if not args:
        print("Usage: board_chat.py [--sender NAME] MESSAGE")
        sys.exit(1)

    text = " ".join(args)
    asyncio.run(send_chat(text, sender))


if __name__ == "__main__":
    main()
