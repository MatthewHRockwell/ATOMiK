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
# v0.38-D recovery hardening — refined 2026-05-10 evening:
# Tested 0.5 ms/char with atomik_os killed; kernel UART RX still drops
# bytes (`/tmp/aoos_keys` got a doubled `o` in a launch command, breaking
# the deploy).  The 2 ms throttle is required regardless of atomik_os
# state — it's a kernel UART driver constraint, not a soft-CPU
# saturation issue.  Win on this iteration is the chunk size bump
# (1024 → 2048 in transfer()) which still halves sentinel round-trips.
SLOW_RATE_S = 0.002       # 2 ms/char — safe for the kernel UART RX path
FAST_RATE_S = 0.002       # kept-equal for now; do NOT lower without testing
_active_rate = [SLOW_RATE_S]

def set_rate(per_s):
    _active_rate[0] = per_s

def slow(s, line, per=None):
    """Per-char throttled write.  Default rate is whatever set_rate()
    last installed.  During pre-launch upload (atomik_os killed), we
    flip to FAST_RATE_S (~4× faster) since the soft CPU isn't busy
    compositing 1080p frames.  Once atomik_os is launched we restore
    SLOW_RATE_S to avoid the byte-loss / character-duplication failure
    mode (`2>&1` arriving as `2>&&1`)."""
    rate = per if per is not None else _active_rate[0]
    for c in line:
        s.write(c.encode() if isinstance(c, str) else bytes([c]))
        time.sleep(rate)
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
    """Run a command, return only the stdout between sentinel lines.
    Uses regex to find the LAST START..END pair, which skips the bash
    command-line echo (which contains the sentinels as typed text)."""
    _n[0] += 1
    START = f"_DCAP_{_n[0]:04d}_S_"
    END   = f"_DCAP_{_n[0]:04d}_E_"
    slow(s, f"echo {START}; {c}; echo {END}\n")
    end = time.time() + t
    buf = bytearray()
    # Require a leading newline before END so we don't fire on the
    # command-line echo of `; echo END\n`.
    while time.time() < end:
        ch = s.read(8192)
        if ch:
            buf.extend(ch)
            if (b"\n" + END.encode()) in buf:
                # Wait a beat for any trailing payload still in flight.
                time.sleep(0.1)
                buf.extend(s.read(8192))
                break
    text = re.sub(r'\x1b\[[0-9;?]*[a-zA-Z]', '',
                  buf.decode(errors='replace'))
    pattern = re.compile(re.escape(START) + r'(.*?)' + re.escape(END),
                         re.DOTALL)
    matches = pattern.findall(text)
    if not matches: return None
    return matches[-1].strip("\r\n")

def transfer(s, local_path, remote_path, label):
    sz = os.path.getsize(local_path)
    print(f"[deploy] local {label}: {sz} bytes", flush=True)
    with open(local_path, "rb") as f: raw = f.read()
    gz  = gzip.compress(raw, compresslevel=9)
    b64 = base64.b64encode(gz).decode()
    print(f"[deploy]  gzipped+b64: {len(b64)} chars (gz={len(gz)} bytes)",
          flush=True)
    cmd(s, f"rm -f /tmp/{label}.b64 /tmp/{label}.gz {remote_path}", log=False)
    # v0.38-D recovery: bump chunk size 1024→2048 — halves sentinel round
    # trips, takes ~half the time at the same per-char rate.  Safe because
    # bash's default input line buffer is 8192+ on this kernel.
    CHUNK  = 2048
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
    # 4 KiB per round-trip. Larger chunks (8-16 KiB) are sometimes
    # missed: the soft-CPU UART writer can't sustain the burst without
    # gaps that confuse our sentinel matcher. 4K is reliable.
    BLOCK = 1024
    K     = 4
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
    docs/screenshots/. Returns the local PNG path on success.

    SIGSTOPs atomik_os during the pull: the 100MHz NaxRiscv saturates
    its UART driver while compositing 1080p frames, causing serial
    corruption that breaks chunked transfers. With atomik_os paused,
    the same chunked pull is reliable. SIGCONT restores it afterwards."""
    os.makedirs(SHOTS_DIR, exist_ok=True)
    print("[deploy] running fb2png on board (~30s)", flush=True)
    cmd(s, f"{FB2PNG_REMOTE} /tmp/aos_shot.png 2>&1 | tail -1", t=60)
    print("[deploy] pausing atomik_os (SIGSTOP) for clean UART pull",
          flush=True)
    cmd(s, "pkill -STOP atomik_os && echo PAUSED || echo NOPROC",
        t=8, log=False)
    raw_png = pull_file(s, "/tmp/aos_shot.png", "screenshot")
    print("[deploy] resuming atomik_os (SIGCONT)", flush=True)
    cmd(s, "pkill -CONT atomik_os && echo RESUMED || echo NOPROC",
        t=8, log=False)
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
    ap.add_argument("--no-assets", action="store_true",
                    help="Skip shipping atomik_os/assets/*.atomik_asset")
    ap.add_argument("--no-shot", action="store_true",
                    help="Skip post-launch screenshot verification")
    ap.add_argument("--mode", choices=["DEV", "DEMO", "INVESTOR"],
                    default="DEV",
                    help="Initial metric_mode (writes /tmp/atomik_mode "
                         "before launch). Default DEV.")
    args = ap.parse_args()

    expected = expected_version()
    if expected:
        print(f"[deploy] expected AOS_VERSION = {expected}", flush=True)
    else:
        print("[deploy] WARNING: could not parse AOS_VERSION from header")

    s = serial.Serial(PORT, BAUD, timeout=0.5)
    s.reset_input_buffer()
    slow(s, "\n"); time.sleep(0.4); s.read(8192)

    # v0.38-D recovery hardening: kill atomik_os FIRST so the upload
    # phase runs against an idle soft CPU.  Then switch to FAST_RATE_S
    # for the chunk uploads (atomik_os isn't compositing, so the
    # historical "byte loss at faster rates" risk doesn't apply).
    cmd(s, "pkill -9 atomik_os 2>/dev/null; sleep 1; "
          "pgrep atomik_os >/dev/null && echo STILL_RUNNING || echo CLEAN")
    set_rate(FAST_RATE_S)
    print(f"[deploy] upload @ {FAST_RATE_S*1000:.1f} ms/char "
          f"(atomik_os killed; safe to push fast)", flush=True)

    transfer(s, LOCAL, REMOTE, "aos")
    if (not args.no_fb2png) and os.path.exists(FB2PNG_LOCAL):
        transfer(s, FB2PNG_LOCAL, FB2PNG_REMOTE, "fb2png")

    # v0.36: ship .atomik_asset files so the board can blit pre-rendered
    # backgrounds (Class B per ChatGPT 2026-05-09).  Each asset goes to
    # /tmp/atomik_assets/<name> — wallpaper.c probes that directory.
    if not args.no_assets:
        assets_dir = os.path.join(HERE, "assets")
        if os.path.isdir(assets_dir):
            asset_files = sorted([f for f in os.listdir(assets_dir)
                                  if f.endswith(".atomik_asset")])
            if asset_files:
                cmd(s, "mkdir -p /tmp/atomik_assets", log=False)
                for name in asset_files:
                    local = os.path.join(assets_dir, name)
                    remote = f"/tmp/atomik_assets/{name}"
                    label = f"asset_{name.split('.')[0]}"
                    print(f"[deploy] shipping asset: {name}", flush=True)
                    transfer(s, local, remote, label)

    if args.no_launch:
        print("[deploy] --no-launch set; not starting.")
        return
    print("[deploy] launching atomik_os in background")
    # v0.38-D recovery: restore SLOW_RATE_S now — once atomik_os is
    # running, the soft CPU is busy compositing 1080p and the kernel
    # UART RX path will drop bytes at faster rates.
    set_rate(SLOW_RATE_S)
    # Tear down any stale fifo writer from a previous --autostart run.
    cmd(s, "kill -9 $(cat /tmp/aos_fifo_writer.pid 2>/dev/null) 2>/dev/null; "
          "rm -f /tmp/aos_fifo_writer.pid /tmp/aos_keys", log=False)
    # Wipe stale version stamp so we KNOW the next read is from the new run.
    cmd(s, "rm -f /tmp/atomik_os_version /tmp/aos.err /tmp/aos.out", log=False)
    # v0.38-J++ mode hint: write /tmp/atomik_mode so atomik_os reads it
    # at startup and skips the DEV default.
    print(f"[deploy] initial mode = {args.mode}", flush=True)
    cmd(s, f"echo {args.mode} > /tmp/atomik_mode", log=False)
    # Disable fbcon binding so atomik_os owns the framebuffer cleanly.
    cmd(s, "echo 0 > /sys/class/vtconsole/vtcon1/bind 2>/dev/null; true")
    # v0.38-D recovery FIX: create /dev/fb0 + /dev/mem if missing.
    # CONFIG_DEVTMPFS is not yet enabled in this kernel (task #15);
    # on freshly-booted images these nodes don't exist and atomik_os
    # bails with "open /dev/fb0: No such file or directory" — which
    # is exactly the failure mode that burned 2 h on 2026-05-10.
    # Skip if already present (kernel boot may have set them up).
    cmd(s, "[ -e /dev/fb0 ] || mknod /dev/fb0 c 29 0; "
          "[ -e /dev/mem ] || mknod /dev/mem c 1 1; "
          "ls -la /dev/fb0 /dev/mem 2>&1 | head -3",
          log=False)
    # v0.22 autostart: launch with stdin redirected from a FIFO instead
    # of /dev/null. atomik_ai_daemon.py and other relays can then inject
    # keystrokes via `printf ... >> /tmp/aos_keys` without restarting.
    cmd(s, "mkfifo /tmp/aos_keys 2>/dev/null; "
          "( while true; do sleep 3600; done ) > /tmp/aos_keys &"
          " echo $! > /tmp/aos_fifo_writer.pid")
    # Bash rejects `cmd &; echo marker`, so launch on its own line.
    slow(s, f"nohup {REMOTE} > /tmp/aos.out 2> /tmp/aos.err "
            f"< /tmp/aos_keys &\n")
    time.sleep(0.6)
    s.read(8192)
    cmd(s, "sleep 2; pgrep atomik_os && echo OS_RUNNING || echo OS_NOT_RUNNING")
    cmd(s, "cat /tmp/aos.err 2>/dev/null | head -20")
    cmd(s, "cat /tmp/aos.out 2>/dev/null | head -20")

    # Version-stamp check. atomik_os writes /tmp/atomik_os_version inside
    # main(), but on a 100MHz NaxRiscv the path through fb_open() +
    # fopen() can take a couple of seconds. Retry for up to 10s.
    stamp = ""
    for attempt in range(10):
        stamp = (cmd_capture(s, "cat /tmp/atomik_os_version 2>/dev/null",
                             t=6) or "").strip()
        if stamp:
            break
        time.sleep(1)
    if expected:
        if stamp == expected:
            print(f"[deploy] VERSION OK — running {stamp}", flush=True)
        else:
            print(f"[deploy] VERSION MISMATCH — expected {expected!r}, "
                  f"got {stamp!r} (after {attempt+1}s of retries)",
                  flush=True)
            print("[deploy] something is wrong — STILL not actually running "
                  "the new binary", flush=True)

    # v0.38-D recovery hardening: VERSION OK only proves atomik_os
    # reached main() far enough to write the version stamp — which
    # happens BEFORE fb_open().  Verify the process is still alive
    # 10 s later AND /tmp/aos.err is empty before declaring success.
    # This is the gate that would have surfaced the /dev/fb0 missing
    # bug on 2026-05-10 immediately instead of after another 2 h.
    print("[deploy] post-launch verify (10s settle + alive + err clean) …",
          flush=True)
    time.sleep(10)
    alive = (cmd_capture(s,
                          "pgrep atomik_os >/dev/null && echo ALIVE || echo DEAD",
                          t=8) or "").strip()
    err  = (cmd_capture(s, "head -3 /tmp/aos.err 2>/dev/null", t=8) or "").strip()
    if alive == "ALIVE" and not err:
        print("[deploy]  alive after 10s, aos.err empty — atomik_os is healthy",
              flush=True)
    else:
        print(f"[deploy]  HEALTH CHECK FAILED: alive={alive!r} "
              f"aos.err={err!r}", flush=True)
        print("[deploy]  do NOT trust VERSION OK alone — atomik_os crashed "
              "after init.  Read /tmp/aos.err on the board BEFORE redeploying.",
              flush=True)

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
