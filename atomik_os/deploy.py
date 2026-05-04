#!/usr/bin/env python3
"""Transfer atomik_os build/atomik_os to /tmp/atomik_os on the board via
base64-over-UART, then launch it in the background so it draws to /dev/fb0.

Usage:
    cd atomik_os && make && python3 deploy.py [--no-launch]
"""
import argparse, base64, gzip, os, sys, time
import serial

PORT, BAUD = "/dev/ttyUSB2", 115200
HERE   = os.path.dirname(os.path.abspath(__file__))
LOCAL  = os.path.join(HERE, "build", "atomik_os")
REMOTE = "/tmp/atomik_os"
FB2PNG_LOCAL  = os.path.join(HERE, "build", "fb2png")
FB2PNG_REMOTE = "/tmp/fb2png"

_n = [0]
def slow(s, line, per=0.0008):
    """Per-char throttled write. The soft-CPU LiteX UART RX FIFO is shallow
    so bursts of >~32 chars get dropped; throttling per char avoids that.
    Empirically 0.8 ms/char is reliable at 115200 baud on this board."""
    for c in line:
        s.write(c.encode() if isinstance(c, str) else bytes([c]))
        time.sleep(per)
    s.flush()

def cmd(s, c, t=20, log=True):
    _n[0] += 1
    m = f"_DOS_{_n[0]:04d}_"
    slow(s, f"{c}; echo {m}\n")
    end = time.time() + t
    buf = bytearray()
    while time.time() < end:
        ch = s.read(8192)
        if ch:
            if log: sys.stdout.buffer.write(ch); sys.stdout.buffer.flush()
            buf.extend(ch)
            if (b"\n" + m.encode()) in buf: return bytes(buf)
    return bytes(buf)

def transfer(s, local_path, remote_path, label):
    sz = os.path.getsize(local_path)
    print(f"[deploy] local {label}: {sz} bytes", flush=True)
    with open(local_path, "rb") as f: raw = f.read()
    gz  = gzip.compress(raw, compresslevel=9)
    b64 = base64.b64encode(gz).decode()
    print(f"[deploy]  gzipped+b64: {len(b64)} chars (gz={len(gz)} bytes)",
          flush=True)
    cmd(s, f"rm -f /tmp/{label}.b64 /tmp/{label}.gz {remote_path}", log=False)
    CHUNK  = 1024
    chunks = [b64[i:i+CHUNK] for i in range(0, len(b64), CHUNK)]
    print(f"[deploy]  sending {len(chunks)} chunks of {CHUNK} chars",
          flush=True)
    t0 = time.time()
    for i, ch in enumerate(chunks):
        cmd(s, f"printf '%s' '{ch}' >> /tmp/{label}.b64", t=30, log=False)
        if i % 5 == 0 or i == len(chunks)-1:
            elapsed = time.time() - t0
            print(f"   chunk {i+1}/{len(chunks)} ({elapsed:.0f}s)",
                  flush=True)
    cmd(s, f"base64 -d /tmp/{label}.b64 > /tmp/{label}.gz")
    cmd(s, f"gunzip -c /tmp/{label}.gz > {remote_path} && chmod +x {remote_path}")
    cmd(s, f"stat -c%s {remote_path}")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--no-launch", action="store_true")
    ap.add_argument("--no-fb2png", action="store_true",
                    help="Skip transferring the fb2png screenshot tool")
    args = ap.parse_args()

    s = serial.Serial(PORT, BAUD, timeout=0.5)
    s.reset_input_buffer()
    slow(s, "\n"); time.sleep(0.4); s.read(8192)

    transfer(s, LOCAL, REMOTE, "aos")
    if (not args.no_fb2png) and os.path.exists(FB2PNG_LOCAL):
        transfer(s, FB2PNG_LOCAL, FB2PNG_REMOTE, "fb2png")

    if args.no_launch:
        print("[deploy] --no-launch set; not starting.")
        return
    print("[deploy] launching atomik_os in background")
    # CRITICAL: kill any existing atomik_os instances before launch.
    # Without this, multiple processes end up writing to /dev/fb0
    # concurrently and the OLDEST one keeps winning the compositor
    # race — so the user sees a stale binary even after a 'successful'
    # deploy. Wait for the kill to complete before relaunching.
    cmd(s, "pkill -9 atomik_os 2>/dev/null; sleep 1; "
          "pgrep atomik_os >/dev/null && echo STILL_RUNNING || echo CLEAN")
    # Disable fbcon binding so atomik_os owns the framebuffer cleanly
    cmd(s, "echo 0 > /sys/class/vtconsole/vtcon1/bind 2>/dev/null; true")
    # Run detached. atomik_os reads stdin (UART) for keys, writes /dev/fb0.
    # Bash rejects `cmd &; echo marker`, so send the background launch
    # on its own line, then a separate marker round-trip.
    slow(s, f"nohup {REMOTE} > /tmp/aos.out 2> /tmp/aos.err < /dev/null &\n")
    time.sleep(0.6)
    s.read(8192)
    cmd(s, "sleep 2; pgrep atomik_os && echo OS_RUNNING || echo OS_NOT_RUNNING")
    cmd(s, "cat /tmp/aos.err 2>/dev/null | head -20")
    cmd(s, "cat /tmp/aos.out 2>/dev/null | head -20")
    print("[deploy] check the HDMI monitor.")

if __name__ == "__main__":
    main()
