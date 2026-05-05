#!/usr/bin/env python3
"""ATOMiK keyboard bridge — laptop terminal -> board UART -> atomik_os.

Why this exists: the AX7020's USB host stack hits a -110 enumeration
timeout on every HID device, so /dev/input/eventN never gets created
on the board (parked thread; see feedback_usb_pause.md). Until that
hardware path is fixed, this script lets a laptop keyboard drive the
desktop in real time.

How it works:
- Puts the laptop terminal into raw, no-echo mode so each keystroke
  is delivered immediately (no waiting for Enter, no double-echo).
- Forwards every byte verbatim to /tmp/aos_keys on the board via
  UART. atomik_os's input_poll() reads stdin, which is the FIFO,
  which we now write to.
- Ctrl-C exits the bridge cleanly without killing the OS.

Usage (laptop terminal):
    python3 atomik_os/tools/atomik_kbd_bridge.py

Then just type. 'S' opens Stocks, 'D' opens Document, etc. The OS
reacts as if a real keyboard were attached.
"""
import os, sys, termios, tty, time, select
import serial

PORT, BAUD = "/dev/ttyUSB2", 115200
FIFO       = "/tmp/aos_keys"

def slow_write(s, data, per=0.002):
    for c in data:
        s.write(c.encode() if isinstance(c, str) else bytes([c]))
        time.sleep(per)
    s.flush()

def push_byte(s, ch):
    """Write ONE keystroke to the board's keystroke FIFO. We escape
    quotes by using printf %s with the single byte as octal so any
    character (including quotes, backslashes, control codes) survives
    the bash interpolation."""
    octal = "\\" + format(ord(ch), '03o')
    slow_write(s, f"printf '%b' '{octal}' >> {FIFO}\n")

def main():
    if not os.path.exists(PORT):
        sys.exit(f"no UART at {PORT}")
    s = serial.Serial(PORT, BAUD, timeout=0.5)
    s.reset_input_buffer()
    slow_write(s, "\n"); time.sleep(0.5); s.read(8192)
    # Sanity: ensure the FIFO exists. atomik_os won't read keys if
    # deploy.py didn't create it during launch.
    slow_write(s, f"test -p {FIFO} && echo FIFO_OK || echo FIFO_MISSING\n")
    time.sleep(0.6)
    out = s.read(4096).decode(errors='replace')
    if "FIFO_OK" not in out:
        print(f"[bridge] WARNING: {FIFO} doesn't exist — re-run "
              f"deploy.py or check that v0.22+ launched the OS with "
              f"fifo-backed stdin.", file=sys.stderr)
    print(f"[bridge] forwarding laptop keystrokes to {FIFO}")
    print(f"[bridge] press keys (a-z, 0-9, etc); Ctrl-] to exit")
    fd = sys.stdin.fileno()
    saved = termios.tcgetattr(fd)
    try:
        tty.setraw(fd)
        while True:
            r, _, _ = select.select([fd], [], [], 1.0)
            if not r: continue
            ch = os.read(fd, 1)
            if not ch: continue
            b = ch[0]
            if b == 0x1D:  # Ctrl-]
                break
            try:
                push_byte(s, chr(b))
            except Exception as e:
                # Don't kill the bridge on a single hiccup — just note.
                sys.stderr.write(f"\r\n[bridge] push failed: {e}\r\n")
    finally:
        termios.tcsetattr(fd, termios.TCSANOW, saved)
        s.close()
        print("\n[bridge] exit")

if __name__ == "__main__":
    main()
