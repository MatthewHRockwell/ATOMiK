#!/usr/bin/env python3
"""aos_diag.py — read-only board diagnostic for atomik_os.

Run this BEFORE deploying anything when the board is in an unknown
state.  It answers four questions in ~10 seconds:

  1. Is Linux shell responsive at all?
  2. Does /dev/fb0 exist (and what major:minor)?
  3. Is fbcon unbound from the framebuffer?
  4. If atomik_os is running, what's its state + last stderr line?

This script does NOT modify the board.  It only reads.  Use it before
any deploy decision so you stop wasting cycles on "let's redeploy and
see" — read the actual state first.

Failure modes it catches that have burned hours in prior sessions:
  - /dev/fb0 missing (CONFIG_DEVTMPFS gap, the 2026-05-10 bug)
  - fbcon still bound (atomik_os draws but fbcon overpaints)
  - atomik_os silently exited (VERSION stamp present, no process)
  - UART byte-loss / kernel-oops degraded state
"""
import argparse, re, sys, time

try:
    import serial
except ImportError:
    sys.exit("error: pyserial not installed — apt install python3-serial")

PORT = "/dev/ttyUSB2"
BAUD = 115200


def slow(s, data, per=0.01):
    for c in data:
        s.write(bytes([c]) if isinstance(c, int) else c.encode())
        time.sleep(per)
    s.flush()


def cmd(s, c, t=30):
    """Send command, wait up to t seconds for a shell prompt to return.
    Returns whatever came back, or "" on full timeout."""
    slow(s, b"\n" + c.encode() + b"\n")
    end = time.time() + t
    buf = bytearray()
    while time.time() < end:
        ch = s.read(8192)
        if ch:
            buf.extend(ch)
            if buf.endswith(b"# ") or buf.endswith(b"#"):
                time.sleep(0.3)
                buf.extend(s.read(8192))
                break
    return re.sub(r"\x1b\[[0-9;?]*[a-zA-Z]", "",
                  buf.decode(errors="replace"))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--port", default=PORT)
    args = ap.parse_args()

    try:
        s = serial.Serial(args.port, BAUD, timeout=10.0)
    except Exception as e:
        print(f"FAIL  serial open: {e}", flush=True)
        return 2
    time.sleep(1)
    s.reset_input_buffer()

    # Q1: shell responsive
    out = cmd(s, "echo AOS-DIAG-READY")
    if "AOS-DIAG-READY" not in out:
        print(f"FAIL  shell unresponsive — no echo reply within timeout", flush=True)
        print(f"      (board may be hung or UART degraded; consider power cycle + JTAG)",
              flush=True)
        return 3
    print("OK    shell responds", flush=True)

    # Q2: /dev/fb0 existence
    out = cmd(s, "ls -la /dev/fb0 2>&1")
    if "No such file" in out:
        print("FAIL  /dev/fb0 missing", flush=True)
        print("      fix:    mknod /dev/fb0 c 29 0", flush=True)
        return 4
    m = re.search(r"crw[\-rwx]+\s+\d+\s+\w+\s+\w+\s+(\d+),\s*(\d+)", out)
    if m:
        print(f"OK    /dev/fb0 exists (c {m.group(1)},{m.group(2)})", flush=True)
    else:
        print(f"WARN  /dev/fb0 exists but unable to parse major:minor", flush=True)

    # Q3: fbcon bind state
    out = cmd(s,
              'for v in /sys/class/vtconsole/vtcon*; do '
              'echo "$(basename $v): bind=$(cat $v/bind 2>/dev/null) '
              'name=$(cat $v/name 2>/dev/null)"; done')
    bound_fb = False
    for line in out.splitlines():
        if "frame buffer device" in line and "bind=1" in line:
            bound_fb = True
            print(f"FAIL  {line.strip()}", flush=True)
            print(f"      fix:    echo 0 > /sys/class/vtconsole/<that>/bind",
                  flush=True)
    if not bound_fb:
        print("OK    fbcon unbound from framebuffer", flush=True)

    # Q4: atomik_os process + stderr
    out = cmd(s, "ps -ef | grep atomik_os | grep -v grep | head -3")
    procs = [l for l in out.splitlines() if "/tmp/atomik_os" in l]
    if not procs:
        print("INFO  atomik_os is not running", flush=True)
    else:
        print(f"OK    atomik_os running: {procs[0].strip()[:120]}", flush=True)

    # Always show the last stderr lines — this is the first thing to
    # read after any unexpected behavior, NOT a redeploy.
    out = cmd(s, "head -10 /tmp/aos.err 2>/dev/null | tail -10")
    err_lines = [l for l in out.splitlines() if l.strip() and "head -10" not in l
                                              and not l.startswith("[?2004")]
    err_lines = [l for l in err_lines if not l.startswith("root@")]
    if err_lines:
        print("AOS_ERR:", flush=True)
        for l in err_lines[:6]:
            print(f"  | {l.strip()}", flush=True)
    else:
        print("OK    /tmp/aos.err is empty (no startup error)", flush=True)

    return 0 if not bound_fb else 1


if __name__ == "__main__":
    sys.exit(main())
