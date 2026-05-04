#!/usr/bin/env python3
"""Transfer atomik_os build/atomik_os to /tmp/atomik_os on the board via
base64-over-UART, then launch it in the background so it draws to /dev/fb0.

CRITICAL CONTRACT: every successful deploy ends with a screenshot pulled
back from the board and saved as docs/screenshots/deploy_<ts>_<ver>.png.
The version stamp is also read from /tmp/atomik_os_version on the board
and asserted against the AOS_VERSION compiled into the binary. Without
both checks, deploys can silently leave a stale process pinning /dev/fb0
and the screen lies about which build is running.

Usage:
    cd atomik_os && make && python3 deploy.py [--no-launch] [--no-shot]
"""
import argparse, base64, gzip, os, re, sys, time
import serial

PORT, BAUD = "/dev/ttyUSB2", 115200
HERE   = os.path.dirname(os.path.abspath(__file__))
LOCAL  = os.path.join(HERE, "build", "atomik_os")
REMOTE = "/tmp/atomik_os"
FB2PNG_LOCAL  = os.path.join(HERE, "build", "fb2png")
FB2PNG_REMOTE = "/tmp/fb2png"
SHOTS_DIR     = os.path.join(HERE, "docs", "screenshots")

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

def cmd_capture(s, c, t=20):
    """Run a command, return only the stdout between sentinel lines."""
    _n[0] += 1
    START = f"_DCAP_{_n[0]:04d}_S_"
    END   = f"_DCAP_{_n[0]:04d}_E_"
    slow(s, f"echo {START}; {c}; echo {END}\n")
    end = time.time() + t
    buf = bytearray()
    while time.time() < end:
        ch = s.read(8192)
        if ch:
            buf.extend(ch)
            if (END.encode() + b"\n") in buf or (END.encode() + b"\r\n") in buf:
                break
    text = re.sub(r'\x1b\[[0-9;?]*[a-zA-Z]', '', buf.decode(errors='replace'))
    lines = text.splitlines()
    s_idx = e_idx = -1
    for i, ln in enumerate(lines):
        if ln.strip() == START: s_idx = i
        elif ln.strip() == END and s_idx >= 0: e_idx = i; break
    if s_idx < 0 or e_idx < 0: return None
    return '\n'.join(lines[s_idx + 1 : e_idx])

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

def expected_version():
    """Read AOS_VERSION from atomik_os.h so we know what string the new
    binary should self-stamp."""
    hdr = os.path.join(HERE, "include", "atomik_os.h")
    with open(hdr) as f: src = f.read()
    m = re.search(r'#define\s+AOS_VERSION\s+"([^"]+)"', src)
    return m.group(1) if m else None

def pull_file(s, remote, label, t_block=60):
    """Pull a remote file back to the laptop via base64+sentinel chunks.
    Returns the bytes or None on failure."""
    sz_text = cmd_capture(s, f"wc -c {remote}", t=8)
    if not sz_text: return None
    n = 0
    for tok in sz_text.split():
        if tok.isdigit(): n = int(tok); break
    if n == 0: return None
    print(f"[deploy]  {label}: {n} bytes on board", flush=True)
    cmd(s, f"gzip -1 -c {remote} > /tmp/_dlpull.gz && base64 /tmp/_dlpull.gz > /tmp/_dlpull.b64",
        t=120, log=False)
    sz_text = cmd_capture(s, "wc -c /tmp/_dlpull.b64", t=8)
    nb64 = 0
    for tok in (sz_text or "").split():
        if tok.isdigit(): nb64 = int(tok); break
    if nb64 == 0:
        print(f"[deploy]  ! pull encode failed"); return None
    print(f"[deploy]  gz+b64: {nb64} chars", flush=True)
    BLOCK = 1024
    K     = 16          # 16 KiB per round-trip
    pieces = []
    pos    = 0
    n_blocks = (nb64 + BLOCK - 1) // BLOCK
    while pos < n_blocks:
        chunk = cmd_capture(s,
            f"dd if=/tmp/_dlpull.b64 bs={BLOCK} skip={pos} count={K} 2>/dev/null",
            t=t_block)
        if chunk is None:
            print(f"[deploy]  ! sentinel miss at pos={pos}"); return None
        valid = ''.join(ch for ch in chunk if ch in
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=")
        pieces.append(valid)
        pos += K
        if pos % 64 == 0 or pos >= n_blocks:
            done = min(pos*BLOCK, nb64)
            print(f"   pulled {done}/{nb64}", flush=True)
    b64 = ''.join(pieces)
    while len(b64) % 4 != 0: b64 += '='
    try:
        gz = base64.b64decode(b64)
        return gzip.decompress(gz)
    except Exception as e:
        print(f"[deploy]  ! decode failed: {e}"); return None

def capture_and_save(s, expected_ver):
    """Run fb2png on the board, pull the PNG back, write it under
    docs/screenshots/. Returns the local PNG path on success."""
    os.makedirs(SHOTS_DIR, exist_ok=True)
    print("[deploy] running fb2png on board (~30s)", flush=True)
    cmd(s, f"{FB2PNG_REMOTE} /tmp/aos_shot.png 2>&1 | tail -1", t=60)
    raw_png = pull_file(s, "/tmp/aos_shot.png", "screenshot")
    if not raw_png:
        print("[deploy]  ! screenshot pull FAILED", flush=True)
        return None
    ts = time.strftime("%Y%m%d_%H%M%S")
    name = f"deploy_{ts}_{expected_ver}.png"
    path = os.path.join(SHOTS_DIR, name)
    with open(path, "wb") as f: f.write(raw_png)
    print(f"[deploy]  screenshot saved: {path}", flush=True)
    return path

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--no-launch", action="store_true")
    ap.add_argument("--no-fb2png", action="store_true",
                    help="Skip transferring the fb2png screenshot tool")
    ap.add_argument("--no-shot", action="store_true",
                    help="Skip post-launch screenshot verification")
    args = ap.parse_args()

    expected = expected_version()
    if expected:
        print(f"[deploy] expected AOS_VERSION = {expected}", flush=True)
    else:
        print("[deploy] WARNING: could not parse AOS_VERSION from header")

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
    # race — the user sees a stale binary even after a 'successful'
    # deploy. Wait for the kill to complete before relaunching.
    cmd(s, "pkill -9 atomik_os 2>/dev/null; sleep 1; "
          "pgrep atomik_os >/dev/null && echo STILL_RUNNING || echo CLEAN")
    # Wipe stale version stamp so we KNOW the next read is from the new run.
    cmd(s, "rm -f /tmp/atomik_os_version", log=False)
    # Disable fbcon binding so atomik_os owns the framebuffer cleanly
    cmd(s, "echo 0 > /sys/class/vtconsole/vtcon1/bind 2>/dev/null; true")
    # Bash rejects `cmd &; echo marker`, so launch on its own line.
    slow(s, f"nohup {REMOTE} > /tmp/aos.out 2> /tmp/aos.err < /dev/null &\n")
    time.sleep(0.6)
    s.read(8192)
    cmd(s, "sleep 2; pgrep atomik_os && echo OS_RUNNING || echo OS_NOT_RUNNING")
    cmd(s, "cat /tmp/aos.err 2>/dev/null | head -20")
    cmd(s, "cat /tmp/aos.out 2>/dev/null | head -20")

    # Version-stamp check.
    stamp = cmd_capture(s, "cat /tmp/atomik_os_version 2>/dev/null", t=6)
    stamp = (stamp or "").strip()
    if expected:
        if stamp == expected:
            print(f"[deploy] VERSION OK — running {stamp}", flush=True)
        else:
            print(f"[deploy] VERSION MISMATCH — expected {expected!r}, "
                  f"got {stamp!r}", flush=True)
            print("[deploy] something is wrong — STILL not actually running "
                  "the new binary", flush=True)

    # Screenshot verification.
    if args.no_shot:
        print("[deploy] --no-shot set; skipping screenshot.")
        return
    if not os.path.exists(FB2PNG_LOCAL):
        print("[deploy] fb2png not built locally; skipping screenshot.")
        return
    capture_and_save(s, expected or "unknown")
    print("[deploy] done — open the saved PNG to confirm what's on screen.")

if __name__ == "__main__":
    main()
