#!/usr/bin/env python3
import argparse
import os
import re
import select
import sys
import termios
import time


def configure(fd: int) -> None:
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = termios.CLOCAL | termios.CREAD | termios.CS8
    attrs[3] = 0
    attrs[4] = termios.B115200
    attrs[5] = termios.B115200
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 1
    termios.tcsetattr(fd, termios.TCSANOW, attrs)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("command")
    parser.add_argument("--device", default="/dev/ttyUSB1")
    parser.add_argument("--timeout", type=float, default=15.0)
    args = parser.parse_args()

    fd = os.open(args.device, os.O_RDWR | os.O_NOCTTY | os.O_SYNC)
    try:
        configure(fd)
        marker = "__CODEX_BLIND__"
        cmd = f"\n{args.command}; printf '\\n{marker}:%s\\n' \"$?\"\n".encode()
        os.write(fd, cmd)
        end = time.time() + args.timeout
        buf = bytearray()
        pat = re.compile(rb"__CODEX_BLIND__:\d+\r?\n")
        while time.time() < end:
            ready, _, _ = select.select([fd], [], [], 0.2)
            if not ready:
                continue
            chunk = os.read(fd, 65536)
            if not chunk:
                continue
            buf.extend(chunk)
            if pat.search(buf):
                break
        sys.stdout.write(buf.decode("utf-8", "replace"))
        return 0 if pat.search(buf) else 1
    finally:
        os.close(fd)


if __name__ == "__main__":
    raise SystemExit(main())
