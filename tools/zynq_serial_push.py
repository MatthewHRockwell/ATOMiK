#!/usr/bin/env python3
import argparse
import hashlib
import os
import re
import select
import sys
import termios
import time


PROMPT_RE = re.compile(rb"root@image_7020:[^\r\n]*# ")


def configure(fd: int, flush: bool = True) -> None:
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
    if flush:
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


def send_cmd(fd: int, command: str, timeout: float) -> str:
    os.write(fd, b"\n")
    read_until(fd, PROMPT_RE, 5)
    marker = "__CODEX_DONE__"
    payload = f"{command}; printf '\\n{marker}:%s\\n' \"$?\"\n".encode()
    os.write(fd, payload)
    out = read_until(fd, rb"__CODEX_DONE__:\d+\r?\n", timeout)
    return out.decode("utf-8", "replace")


def push_file(fd: int, src: str, dst: str) -> tuple[str, str]:
    size = os.path.getsize(src)
    local_sha = hashlib.sha256()

    receiver = (
        "cat >/tmp/codex_rxraw.py <<'PY'\n"
        "import hashlib, os, sys, termios, tty\n"
        "dst = sys.argv[1]\n"
        "size = int(sys.argv[2])\n"
        "fd = sys.stdin.fileno()\n"
        "old = termios.tcgetattr(fd)\n"
        "h = hashlib.sha256()\n"
        "try:\n"
        "    tty.setraw(fd)\n"
        "    print('READY', flush=True)\n"
        "    with open(dst, 'wb', buffering=0) as f:\n"
        "        rem = size\n"
        "        while rem:\n"
        "            chunk = os.read(fd, min(65536, rem))\n"
        "            if not chunk:\n"
        "                raise SystemExit('short read')\n"
        "            f.write(chunk)\n"
        "            h.update(chunk)\n"
        "            rem -= len(chunk)\n"
        "        os.fsync(f.fileno())\n"
        "finally:\n"
        "    termios.tcsetattr(fd, termios.TCSANOW, old)\n"
        "print('DONE %d %s' % (size, h.hexdigest()), flush=True)\n"
        "PY\n"
        "printf '__RX_READY__\\n'\n"
    )

    os.write(fd, b"\n")
    read_until(fd, PROMPT_RE, 5)
    os.write(fd, receiver.encode())
    read_until(fd, rb"__RX_READY__\r?\n", 10)

    cmd = f"python3 /tmp/codex_rxraw.py {dst} {size}\n"
    os.write(fd, cmd.encode())
    read_until(fd, rb"READY\r?\n", 10)

    with open(src, "rb") as f:
        sent = 0
        while True:
            chunk = f.read(16384)
            if not chunk:
                break
            os.write(fd, chunk)
            local_sha.update(chunk)
            sent += len(chunk)
            if sent % (256 * 1024) == 0 or sent == size:
                print(f"\r{sent}/{size} bytes", end="", file=sys.stderr)
    print(file=sys.stderr)

    done = read_until(fd, rb"DONE (\d+) ([0-9a-f]{64})\r?\n", max(60.0, size / 6000.0))
    match = re.search(rb"DONE (\d+) ([0-9a-f]{64})\r?\n", done)
    if not match:
        raise RuntimeError("missing transfer completion marker")
    remote_size = int(match.group(1))
    remote_sha = match.group(2).decode()
    local_hex = local_sha.hexdigest()
    if remote_size != size or remote_sha != local_hex:
        raise RuntimeError(
            f"verification failed local={local_hex} remote={remote_sha} size={remote_size}/{size}"
        )

    return local_hex, remote_sha


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("src")
    parser.add_argument("dst")
    parser.add_argument("--device", default="/dev/ttyUSB1")
    args = parser.parse_args()

    fd = os.open(args.device, os.O_RDWR | os.O_NOCTTY | os.O_SYNC)
    try:
        configure(fd)
        local_sha, remote_sha = push_file(fd, args.src, args.dst)
    finally:
        os.close(fd)

    print(f"local_sha256={local_sha}")
    print(f"remote_sha256={remote_sha}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
