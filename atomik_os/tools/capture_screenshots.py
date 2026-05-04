#!/usr/bin/env python3
"""Drive ATOMiK OS through a series of states and capture a PNG of each
via the on-board fb2png tool. Pulls each PNG back to the laptop via
base64-over-UART and writes them to atomik_os/docs/screenshots/.

Prereq: latest atomik_os + fb2png deployed to /tmp on the board (see
deploy.py).

Output filenames match docs/SCREENSHOT_PLAN.md.
"""
import argparse, base64, os, sys, time
import serial

PORT, BAUD = "/dev/ttyUSB2", 115200
HERE  = os.path.dirname(os.path.abspath(__file__))
OUT   = os.path.normpath(os.path.join(HERE, "..", "docs", "screenshots"))
os.makedirs(OUT, exist_ok=True)

_n = [0]
def slow(s, line, per=0.0008):
    for c in line:
        s.write(c.encode() if isinstance(c, str) else bytes([c]))
        time.sleep(per)
    s.flush()

def cmd(s, line, t=20, log=False):
    _n[0] += 1
    m = f"_S_{_n[0]:04d}_"
    slow(s, f"{line}; echo {m}\n")
    end = time.time() + t
    buf = bytearray()
    while time.time() < end:
        ch = s.read(8192)
        if ch:
            if log: sys.stdout.buffer.write(ch); sys.stdout.buffer.flush()
            buf.extend(ch)
            if (b"\n" + m.encode()) in buf: return bytes(buf)
    return bytes(buf)

def send_key(s, ch):
    """Send a single keystroke to the running atomik_os process via its
    inherited stdin. Since atomik_os was launched with stdin redirected
    from /dev/null, we need a different injection path: write to its
    /proc/$PID/fd/0 (only works because the kernel + busybox lets us)."""
    # Locate the running atomik_os PID
    pid_buf = cmd(s, "pgrep atomik_os | head -1")
    pid = None
    for line in pid_buf.decode(errors='replace').splitlines():
        line = line.strip()
        if line.isdigit():
            pid = line; break
    if not pid:
        print("[cap] atomik_os not running", file=sys.stderr); return
    # We can't write to /dev/null-redirected stdin. Plan B: drive via the
    # tty the OS is reading by re-launching with a controlling fifo. For
    # now, reset the OS into a fresh state and use a fifo-backed launch.
    raise RuntimeError("send_key path not implemented — see relaunch_with_fifo")

def relaunch_with_fifo(s):
    """Stop atomik_os, mkfifo /tmp/aos_keys, restart it reading from the
    fifo. We then write keystrokes to the fifo to drive the OS."""
    cmd(s, "pkill -9 atomik_os; sleep 1; rm -f /tmp/aos_keys; mkfifo /tmp/aos_keys")
    cmd(s, "echo 0 > /sys/class/vtconsole/vtcon1/bind 2>/dev/null; true")
    # Spawn atomik_os reading from the fifo. The fifo must have a writer
    # before the reader can finish open(); we keep it open via a held cat.
    cmd(s, "( while true; do sleep 60; done ) > /tmp/aos_keys &")
    cmd(s, "nohup /tmp/atomik_os > /tmp/aos.out 2> /tmp/aos.err < /tmp/aos_keys &")
    cmd(s, "sleep 2; pgrep atomik_os && echo OS_RUNNING || echo OS_NOT_RUNNING")

def push_keys(s, text):
    """Append a string of bytes to the OS's stdin fifo."""
    # Use printf so escape sequences pass through. Newlines are literal \n.
    safe = text.replace("'", "'\\''")
    cmd(s, f"printf '{safe}' >> /tmp/aos_keys")
    time.sleep(0.4)   # let the OS process the keys + repaint

def shoot(s, name):
    """Capture /dev/fb0 with fb2png, gzip + base64-stream back, save.

    The on-board PNG is uncompressed (stored deflate) so it's ~6 MB. We
    gzip it on the board first to ~1 MB, then base64 (~1.4 MB), then
    pull in 4 KiB block-aligned chunks via `dd bs=4096 skip=N count=K`
    which is ~4096x faster than the byte-aligned dd we used before. """
    print(f"[cap] shooting {name} ...", flush=True)
    cmd(s, f"rm -f /tmp/{name}.png /tmp/{name}.png.gz /tmp/{name}.b64")
    cmd(s, f"/tmp/fb2png /tmp/{name}.png 2>&1 | head -2", t=120)
    cmd(s, f"gzip -9 /tmp/{name}.png && base64 /tmp/{name}.png.gz > /tmp/{name}.b64",
        t=300)
    sz_buf = cmd(s, f"wc -c /tmp/{name}.b64")
    n = 0
    for tok in sz_buf.decode(errors='replace').split():
        if tok.isdigit():
            n = int(tok); break
    if n == 0:
        print(f"  ! /tmp/{name}.b64 empty"); return
    print(f"  gz+b64 len = {n}", flush=True)

    BLOCK = 4096
    K     = 8                       # bytes per dd call ~ 32 KiB
    pieces = []
    n_blocks = (n + BLOCK - 1) // BLOCK
    pos = 0
    while pos < n_blocks:
        c = cmd(s, f"dd if=/tmp/{name}.b64 bs={BLOCK} skip={pos} count={K} 2>/dev/null",
                t=60)
        text = c.decode(errors='replace')
        valid = ''.join(ch for ch in text if ch in
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=")
        pieces.append(valid)
        pos += K
        if pos % 32 == 0:
            print(f"  pulled {pos*BLOCK}/{n} bytes", flush=True)
    b64 = ''.join(pieces)
    while len(b64) % 4 != 0: b64 += '='
    try:
        gz = base64.b64decode(b64)
    except Exception as e:
        print(f"  ! decode failed: {e}"); return
    import gzip as gzmod
    try:
        png = gzmod.decompress(gz)
    except Exception as e:
        print(f"  ! gunzip failed: {e}"); return
    out_path = os.path.join(OUT, f"{name}.png")
    with open(out_path, "wb") as f: f.write(png)
    print(f"  wrote {out_path} ({len(png)} bytes)", flush=True)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--limit", type=int, default=10,
                    help="Stop after N shots (debugging)")
    args = ap.parse_args()

    s = serial.Serial(PORT, BAUD, timeout=0.5)
    s.reset_input_buffer()
    slow(s, "\n"); time.sleep(0.4); s.read(8192)

    # 1. Restart with fifo-backed stdin so we can inject keys
    print("[cap] restarting atomik_os with fifo-backed stdin", flush=True)
    relaunch_with_fifo(s)
    time.sleep(2)

    shots = [
        # (name, key sequence to send before capture)
        ("01_desktop",            ""),
        ("02_about",              "a"),
        ("03_monitor",            "m"),
        ("04_terminal",           "t"),
        ("05_files",              "f"),
        ("06_notes",              "n"),
        ("07_document",           "d"),
        ("08_doc_calendar",       "load calendar\n"),
        ("09_doc_tasks",          "load tasks\n"),
        ("10_doc_feed",           "load code\n"),
    ]
    n = 0
    for name, keys in shots:
        if n >= args.limit: break
        if keys:
            push_keys(s, keys)
            time.sleep(0.5)
        shoot(s, name)
        n += 1

    print("[cap] done.")

if __name__ == "__main__":
    main()
