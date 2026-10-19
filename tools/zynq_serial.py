#!/usr/bin/env python3
import argparse
import os
import re
import select
import sys
import termios
import time


PROMPT_RE = re.compile(rb"root@image_7020:[^\r\n]*# ")
LOGIN_RE = re.compile(rb"(?:^|\r?\n)image_7020 login:\s*$", re.M)


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
    termios.tcflush(fd, termios.TCIOFLUSH)


def read_until(fd: int, pattern: bytes | re.Pattern[bytes], timeout: float) -> bytes:
    end = time.time() + timeout
    buf = bytearray()
    regex = re.compile(pattern) if isinstance(pattern, bytes) else pattern
    while time.time() < end:
        remaining = max(0.0, min(0.2, end - time.time()))
        ready, _, _ = select.select([fd], [], [], remaining)
        if not ready:
            continue
        chunk = os.read(fd, 65536)
        if not chunk:
            continue
        buf.extend(chunk)
        if regex.search(buf):
            return bytes(buf)
    raise TimeoutError(buf.decode("utf-8", "replace"))


def wait_for_shell(fd: int, timeout: float) -> bytes:
    end = time.time() + timeout
    buf = bytearray()
    while time.time() < end:
        remaining = max(0.0, min(0.2, end - time.time()))
        ready, _, _ = select.select([fd], [], [], remaining)
        if not ready:
            continue
        chunk = os.read(fd, 65536)
        if not chunk:
            continue
        buf.extend(chunk)
        if PROMPT_RE.search(buf):
            return bytes(buf)
        if LOGIN_RE.search(buf):
            os.write(fd, b"root\n")
            buf.clear()
    raise TimeoutError(buf.decode("utf-8", "replace"))


def run_command(device: str, command: str, timeout: float) -> str:
    fd = os.open(device, os.O_RDWR | os.O_NOCTTY | os.O_SYNC)
    try:
        configure(fd)
        os.write(fd, b"\n")
        wait_for_shell(fd, 8)
        marker = "__CODEX_DONE__"
        payload = f"{command}; printf '\\n{marker}:%s\\n' \"$?\"\n".encode()
        os.write(fd, payload)
        out = read_until(fd, rb"__CODEX_DONE__:\d+\r?\n", timeout)
        return out.decode("utf-8", "replace")
    finally:
        os.close(fd)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("command")
    parser.add_argument("--device", default="/dev/ttyUSB1")
    parser.add_argument("--timeout", type=float, default=15.0)
    args = parser.parse_args()
    sys.stdout.write(run_command(args.device, args.command, args.timeout))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
