#!/usr/bin/env python3
"""Pull a file FROM the Zynq board over raw serial. Reverse of zynq_serial_push.py."""
import argparse, hashlib, os, re, select, sys, termios, time

PROMPT_RE = re.compile(rb"root@image_7020:[^\r\n]*# ")

def configure(fd):
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0; attrs[1] = 0
    attrs[2] = termios.CLOCAL | termios.CREAD | termios.CS8
    attrs[3] = 0
    attrs[4] = termios.B115200; attrs[5] = termios.B115200
    attrs[6][termios.VMIN] = 0; attrs[6][termios.VTIME] = 1
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    termios.tcflush(fd, termios.TCIOFLUSH)

def read_until(fd, pattern, timeout):
    end = time.time() + timeout
    buf = bytearray()
    regex = re.compile(pattern) if isinstance(pattern, bytes) else pattern
    while time.time() < end:
        remaining = max(0.0, min(0.2, end - time.time()))
        ready, _, _ = select.select([fd], [], [], remaining)
        if ready:
            chunk = os.read(fd, 65536)
            if chunk:
                buf.extend(chunk)
                if regex.search(buf):
                    return bytes(buf)
    raise TimeoutError(buf.decode("utf-8", "replace"))

def read_exact(fd, count, timeout):
    end = time.time() + timeout
    buf = bytearray()
    while len(buf) < count and time.time() < end:
        remaining = max(0.0, min(0.2, end - time.time()))
        ready, _, _ = select.select([fd], [], [], remaining)
        if ready:
            chunk = os.read(fd, min(65536, count - len(buf)))
            if chunk:
                buf.extend(chunk)
                if len(buf) % (256 * 1024) == 0 or len(buf) == count:
                    print(f"\r{len(buf)}/{count} bytes", end="", file=sys.stderr)
    print(file=sys.stderr)
    return bytes(buf)

def send_cmd(fd, command, timeout):
    os.write(fd, b"\n")
    read_until(fd, PROMPT_RE, 5)
    marker = "__CODEX_DONE__"
    payload = f"{command}; printf '\\n{marker}:%s\\n' \"$?\"\n".encode()
    os.write(fd, payload)
    out = read_until(fd, rb"__CODEX_DONE__:\d+\r?\n", timeout)
    return out.decode("utf-8", "replace")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("remote_path", help="File path on the board")
    parser.add_argument("local_path", help="Local destination path")
    parser.add_argument("--device", default="/dev/ttyUSB1")
    args = parser.parse_args()

    fd = os.open(args.device, os.O_RDWR | os.O_NOCTTY | os.O_SYNC)
    try:
        configure(fd)

        # Get remote file size
        out = send_cmd(fd, f"wc -c < {args.remote_path}", 10)
        m = re.search(r"(\d+)", out)
        if not m:
            raise RuntimeError(f"Cannot get file size: {out}")
        size = int(m.group(1))
        print(f"Remote file: {size} bytes", file=sys.stderr)

        # Deploy sender script on board
        sender = (
            "cat >/tmp/codex_txraw.py <<'PY'\n"
            "import os, sys, termios, tty, hashlib\n"
            "src = sys.argv[1]\n"
            "fd = sys.stdout.fileno()\n"
            "old = termios.tcgetattr(fd)\n"
            "data = open(src, 'rb').read()\n"
            "h = hashlib.sha256(data).hexdigest()\n"
            "sys.stderr.write(f'SENDING {len(data)} {h}\\n')\n"
            "sys.stderr.flush()\n"
            "try:\n"
            "    tty.setraw(fd)\n"
            "    os.write(fd, data)\n"
            "finally:\n"
            "    termios.tcsetattr(fd, termios.TCSANOW, old)\n"
            "sys.stderr.write('TX_DONE\\n')\n"
            "sys.stderr.flush()\n"
            "PY\n"
            "printf '__TX_READY__\\n'\n"
        )

        os.write(fd, b"\n")
        read_until(fd, PROMPT_RE, 5)
        os.write(fd, sender.encode())
        read_until(fd, rb"__TX_READY__\r?\n", 10)

        # Start sender — redirect stderr to serial, stdout is raw data
        cmd = f"python3 /tmp/codex_txraw.py {args.remote_path} 2>/dev/null\n"
        os.write(fd, cmd.encode())

        # Small delay for Python to start and set raw mode
        time.sleep(1)

        # Read exactly 'size' bytes of raw data
        timeout = max(120.0, size / 6000.0)
        data = read_exact(fd, size, timeout)

        if len(data) != size:
            raise RuntimeError(f"Short read: got {len(data)}, expected {size}")

        # Write to local file
        with open(args.local_path, "wb") as f:
            f.write(data)

        local_sha = hashlib.sha256(data).hexdigest()
        print(f"Received {len(data)} bytes", file=sys.stderr)
        print(f"local_sha256={local_sha}")

        # Wait for board to restore terminal
        time.sleep(2)
        termios.tcflush(fd, termios.TCIFLUSH)

    finally:
        os.close(fd)

if __name__ == "__main__":
    main()
