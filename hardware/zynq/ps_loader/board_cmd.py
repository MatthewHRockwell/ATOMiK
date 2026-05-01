#!/usr/bin/env python3
"""
ATOMiK Board Command Tool — execute commands on the Zynq board.

Commands execute via atomik_live's embedded popen() executor while the
demo keeps running on HDMI/LCD. Results tagged as ##RSP: lines on UART.

Usage:
    python3 board_cmd.py "uname -a"
    python3 board_cmd.py "cat /proc/cpuinfo"
    python3 board_cmd.py "ls -la /tmp/"
    python3 board_cmd.py --shell

For Claude Code: this lets you run commands directly on the Zynq hardware.
Board is NaxRiscv RV64GC @ 100MHz, Ubuntu 24.04, 487MB RAM.
"""

import argparse
import json
import sys
import time

PORT = "/dev/ttyUSB2"
BAUD = 921600
TIMEOUT = 30


def send_command(command, port=PORT, baud=BAUD, timeout=TIMEOUT):
    """Send ~command to board, collect ##RSP: lines until ##RSP:END."""
    import serial

    ser = serial.Serial(port, baud, timeout=1)
    time.sleep(0.1)

    # Drain pending data
    if ser.in_waiting:
        ser.read(ser.in_waiting)

    # Send command char-by-char (UART RX on 100MHz soft CPU drops bytes at full speed)
    ser.write(b"~")
    time.sleep(0.005)
    for c in command:
        ser.write(c.encode())
        time.sleep(0.002)
    ser.write(b"\n")
    ser.flush()

    # Collect response lines
    lines = []
    exit_code = None
    deadline = time.time() + timeout

    while time.time() < deadline:
        raw = ser.readline()
        if not raw:
            continue
        text = raw.decode(errors='replace').strip()

        if text == "##RSP:END":
            break
        elif text.startswith("##RSP:EXIT:"):
            try:
                exit_code = int(text.split(":")[-1])
            except ValueError:
                exit_code = -1
        elif text.startswith("##RSP:CMD:"):
            pass  # command echo, skip
        elif text.startswith("##RSP:ERROR:"):
            lines.append(text[12:])
            exit_code = 1
        elif text.startswith("##RSP:"):
            lines.append(text[6:])
        # Ignore ##EVENT: and other lines

    ser.close()
    return "\n".join(lines), exit_code if exit_code is not None else 0


def interactive_shell(port, baud):
    """Interactive shell — type commands, see results."""
    print(f"ATOMiK Board Shell ({port} @ {baud})")
    print("Commands execute on Zynq. Ctrl-D to exit.\n")

    while True:
        try:
            cmd = input("board> ")
        except (EOFError, KeyboardInterrupt):
            print("\nExiting.")
            break

        if not cmd.strip():
            continue

        output, rc = send_command(cmd, port, baud)
        if output:
            print(output)
        if rc != 0:
            print(f"[exit code: {rc}]")


def main():
    ap = argparse.ArgumentParser(description="Execute commands on ATOMiK Zynq board")
    ap.add_argument("command", nargs="?", help="Command to execute on board")
    ap.add_argument("--port", default=PORT)
    ap.add_argument("--baud", type=int, default=BAUD)
    ap.add_argument("--timeout", type=int, default=TIMEOUT)
    ap.add_argument("--shell", action="store_true", help="Interactive shell")
    ap.add_argument("--json", action="store_true", help="JSON output")
    args = ap.parse_args()

    if args.shell:
        interactive_shell(args.port, args.baud)
        return

    if not args.command:
        ap.print_help()
        sys.exit(1)

    output, rc = send_command(args.command, args.port, args.baud, args.timeout)

    if args.json:
        print(json.dumps({"command": args.command, "output": output, "exit_code": rc}))
    else:
        if output:
            print(output)
        sys.exit(rc)


if __name__ == "__main__":
    main()
