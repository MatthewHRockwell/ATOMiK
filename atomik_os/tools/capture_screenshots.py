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
    """Snapshot /dev/fb0 directly. Skip the on-board PNG encoder — it's
    too slow on the 100 MHz soft CPU (5+ min per shot). Instead:

      1. dd /dev/fb0 -> /tmp/fb.raw           (~8 MB BGRX raw, ~5s)
      2. gzip -1 /tmp/fb.raw                  (~3-4 MB, fast vs gzip -9)
      3. base64 -> /tmp/fb.b64
      4. Pull in 32 KiB sentinel-bracketed chunks
      5. Host-side: gunzip + BGRA->RGB + write PNG via PIL
    """
    print(f"[cap] shooting {name} ...", flush=True)
    cmd(s, f"rm -f /tmp/{name}.raw /tmp/{name}.raw.gz /tmp/{name}.b64")
    cmd(s, f"dd if=/dev/fb0 of=/tmp/{name}.raw bs=1M count=8 2>&1 | tail -1",
        t=180)
    cmd(s, f"gzip -1 /tmp/{name}.raw && base64 /tmp/{name}.raw.gz > /tmp/{name}.b64",
        t=240)
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
        # Sentinel-bracket the dd output so we can extract just the
        # payload. We search for the LAST occurrence of START and the
        # LAST occurrence of END before the marker — bash echoes the
        # command line itself, which contains the sentinels too, so a
        # naive find() picks up the wrong instance.
        START = f"<<<P{pos:06d}>>>"
        END   = f"<<<E{pos:06d}>>>"
        c = cmd(s,
                f"echo {START}; dd if=/tmp/{name}.b64 bs={BLOCK} skip={pos} count={K} 2>/dev/null; echo {END}",
                t=60)
        text = c.decode(errors='replace')
        # Strip ANSI escape sequences first
        import re
        text = re.sub(r'\x1b\[[0-9;?]*[a-zA-Z]', '', text)
        # Find sentinel ON ITS OWN LINE — that's the echo output, not
        # the bash command-line echo (which prepends 'echo ' and ';').
        lines = text.splitlines()
        s_idx = e_idx = -1
        for li, ln in enumerate(lines):
            if ln.strip() == START: s_idx = li
            elif ln.strip() == END and s_idx >= 0: e_idx = li; break
        if s_idx < 0 or e_idx < 0:
            print(f"  ! sentinel miss at pos={pos}"); break
        payload_lines = lines[s_idx + 1 : e_idx]
        payload = '\n'.join(payload_lines)
        valid = ''.join(ch for ch in payload if ch in
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
        raw = gzmod.decompress(gz)
    except Exception as e:
        print(f"  ! gunzip failed: {e}"); return
    print(f"  raw fb bytes: {len(raw)}", flush=True)

    W, H   = 1920, 1080
    expect = W * H * 4
    if len(raw) < expect:
        print(f"  ! short raw ({len(raw)} < {expect})"); return
    raw = raw[:expect]
    try:
        from PIL import Image
        img = Image.frombytes('RGBA', (W, H), raw, 'raw', 'BGRA')
        img = img.convert('RGB')
        out_path = os.path.join(OUT, f"{name}.png")
        img.save(out_path, optimize=True)
        print(f"  wrote {out_path}", flush=True)
    except Exception as e:
        print(f"  ! PIL save failed: {e}"); return

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
