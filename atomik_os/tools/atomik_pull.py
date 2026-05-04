#!/usr/bin/env python3
"""ATOMiK deltas-puller bridge.

Periodically copies /tmp/atomik_os_document_<id>.deltas from the
running board over UART to a local file, where atomik_view.py --watch
re-renders it. Two scripts running in tandem:

    Terminal A:
        atomik_pull.py --doc 1                # writes /tmp/board_doc_1.deltas
    Terminal B:
        atomik_view.py --watch /tmp/board_doc_1.deltas

End state: open the on-board Document, type chat commands, watch the
laptop window morph in lockstep — same wire format, same primitive
renderer logic, two displays.

Bandwidth note: typical Document state is a few hundred bytes; the
pull cycle is therefore much smaller than the framebuffer-streaming
alternative. This is the v1.0 cross-device sync pitch made tangible —
deltas in, deltas out, no pixels.
"""
import argparse, base64, os, re, sys, time

PORT, BAUD = "/dev/ttyUSB2", 115200


def slow(s, line, per=0.0008):
    for c in line:
        s.write(c.encode() if isinstance(c, str) else bytes([c]))
        time.sleep(per)
    s.flush()


def remote_cmd(s, sh, t=8):
    """Run a shell command on the board, capture its stdout between
    sentinel lines so we can extract the payload cleanly."""
    START = "<<<APUL_BEGIN>>>"
    END   = "<<<APUL_END>>>"
    slow(s, f"echo {START}; {sh}; echo {END}\n")
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
    if s_idx < 0 or e_idx < 0:
        return None
    return '\n'.join(lines[s_idx + 1 : e_idx])


def pull_once(s, doc_id, out_path):
    remote = f"/tmp/atomik_os_document_{doc_id}.deltas"
    # 1. Check if file exists; bail quietly if not.
    exists = remote_cmd(s, f"test -f {remote} && echo Y || echo N", t=4)
    if not exists or "Y" not in exists:
        return False
    # 2. base64 -w 0 collapses to a single line — easy to parse.
    body = remote_cmd(s, f"base64 -w 0 {remote}", t=8)
    if body is None:
        return False
    valid = ''.join(ch for ch in body if ch in
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=")
    if not valid:
        return False
    while len(valid) % 4 != 0: valid += '='
    try:
        raw = base64.b64decode(valid)
    except Exception:
        return False
    # Write atomically — rename so atomik_view --watch sees a clean swap.
    tmp = out_path + ".tmp"
    with open(tmp, "wb") as f: f.write(raw)
    os.replace(tmp, out_path)
    return True


def main():
    import serial
    ap = argparse.ArgumentParser()
    ap.add_argument("--doc", type=int, default=1)
    ap.add_argument("--out", default=None,
                    help="Local file (default /tmp/board_doc_<doc>.deltas)")
    ap.add_argument("--interval", type=float, default=2.0,
                    help="Seconds between pulls (default 2.0)")
    args = ap.parse_args()
    out = args.out or f"/tmp/board_doc_{args.doc}.deltas"

    s = serial.Serial(PORT, BAUD, timeout=0.5)
    s.reset_input_buffer()
    slow(s, "\n"); time.sleep(0.4); s.read(8192)

    print(f"[pull] doc={args.doc}  out={out}  interval={args.interval}s",
          flush=True)
    last_size = -1
    n = 0
    while True:
        ok = pull_once(s, args.doc, out)
        if ok:
            sz = os.path.getsize(out)
            if sz != last_size:
                print(f"[pull] {time.strftime('%H:%M:%S')}  {sz} bytes",
                      flush=True)
                last_size = sz
        else:
            if n == 0:
                print(f"[pull] doc {args.doc} not present yet — open it on the "
                      f"board with 'D'", flush=True)
        n += 1
        time.sleep(args.interval)


if __name__ == "__main__":
    main()
