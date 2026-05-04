#!/usr/bin/env python3
"""Transfer atomik_os build/atomik_os to /tmp/atomik_os on the board via
base64-over-UART, then launch it in the background so it draws to /dev/fb0.

Usage:
    cd atomik_os && make && python3 deploy.py [--no-launch]
"""
import argparse, base64, gzip, os, sys, time
import serial

PORT, BAUD = "/dev/ttyUSB2", 115200
LOCAL  = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                      "build", "atomik_os")
REMOTE = "/tmp/atomik_os"

_n = [0]
def slow(s, line, per=0.003):
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

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--no-launch", action="store_true")
    args = ap.parse_args()

    sz = os.path.getsize(LOCAL)
    print(f"[deploy] local atomik_os: {sz} bytes")
    with open(LOCAL, "rb") as f: raw = f.read()
    gz = gzip.compress(raw, compresslevel=9)
    b64 = base64.b64encode(gz).decode()
    print(f"[deploy] gzipped+b64: {len(b64)} chars (gz={len(gz)} bytes)")

    s = serial.Serial(PORT, BAUD, timeout=0.5)
    s.reset_input_buffer()
    slow(s, "\n"); time.sleep(0.4); s.read(8192)

    cmd(s, f"rm -f /tmp/aos.b64 /tmp/aos.gz {REMOTE}", log=False)

    CHUNK = 256
    chunks = [b64[i:i+CHUNK] for i in range(0, len(b64), CHUNK)]
    print(f"[deploy] sending {len(chunks)} chunks of {CHUNK} chars")
    t0 = time.time()
    for i, ch in enumerate(chunks):
        cmd(s, f"printf '%s' '{ch}' >> /tmp/aos.b64", log=False)
        if i % 10 == 0 or i == len(chunks)-1:
            elapsed = time.time() - t0
            print(f"  chunk {i+1}/{len(chunks)} ({elapsed:.0f}s)")

    cmd(s, "wc -c /tmp/aos.b64")
    cmd(s, "base64 -d /tmp/aos.b64 > /tmp/aos.gz")
    cmd(s, f"gunzip -c /tmp/aos.gz > {REMOTE} && chmod +x {REMOTE}")
    cmd(s, f"stat -c%s {REMOTE}")

    if args.no_launch:
        print("[deploy] --no-launch set; not starting.")
        return
    print("[deploy] launching atomik_os in background")
    # Disable fbcon binding so atomik_os owns the framebuffer cleanly
    cmd(s, "echo 0 > /sys/class/vtconsole/vtcon1/bind 2>/dev/null; true")
    # Run detached. atomik_os reads stdin (UART) for keys, writes /dev/fb0.
    cmd(s, f"nohup {REMOTE} > /tmp/aos.out 2> /tmp/aos.err < /dev/null &")
    cmd(s, "sleep 2; pgrep atomik_os")
    cmd(s, "cat /tmp/aos.err 2>/dev/null | head -20")
    cmd(s, "cat /tmp/aos.out 2>/dev/null | head -20")
    print("[deploy] check the HDMI monitor.")

if __name__ == "__main__":
    main()
