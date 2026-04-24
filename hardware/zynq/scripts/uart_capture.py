#!/usr/bin/env python3
"""Capture UART data from /dev/ttyUSB1, write to stdout and file."""
import sys, time, os

PORT = "/dev/ttyUSB1"
BAUD = 115200
OUTFILE = "/tmp/uart_capture.bin"
TIMEOUT = 15  # seconds total capture time

try:
    import serial
    ser = serial.Serial(PORT, BAUD, timeout=0.1)
except ImportError:
    # Fallback: use termios directly
    import termios, fcntl, struct
    fd = os.open(PORT, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    attrs = termios.tcgetattr(fd)
    # Set 115200 8N1 raw
    attrs[0] = 0  # iflag
    attrs[1] = 0  # oflag
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL  # cflag
    attrs[3] = 0  # lflag
    attrs[4] = termios.B115200  # ispeed
    attrs[5] = termios.B115200  # ospeed
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 1  # 0.1s timeout
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    # Make blocking with timeout
    flags = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, flags & ~os.O_NONBLOCK)
    ser = None

print(f"[CAPTURE] Listening on {PORT} @ {BAUD} baud for {TIMEOUT}s", flush=True)
print(f"[CAPTURE] Output: {OUTFILE}", flush=True)

start = time.time()
total_bytes = 0
buf = bytearray()

with open(OUTFILE, "wb") as f:
    while time.time() - start < TIMEOUT:
        if ser:
            data = ser.read(256)
        else:
            try:
                data = os.read(fd, 256)
            except BlockingIOError:
                data = b""

        if data:
            f.write(data)
            f.flush()
            buf.extend(data)
            total_bytes += len(data)
            # Print hex + ASCII
            for b in data:
                c = chr(b) if 0x20 <= b <= 0x7E else '.'
                print(f"  0x{b:02X} '{c}'", end="", flush=True)
            print(flush=True)

elapsed = time.time() - start
print(f"\n[CAPTURE] Done. {total_bytes} bytes in {elapsed:.1f}s", flush=True)

if total_bytes > 0:
    print(f"[CAPTURE] Hex dump: {buf.hex()}", flush=True)
    try:
        print(f"[CAPTURE] ASCII: {buf.decode('ascii', errors='replace')}", flush=True)
    except:
        pass
else:
    print("[CAPTURE] NO DATA RECEIVED", flush=True)

if ser:
    ser.close()
else:
    os.close(fd)
