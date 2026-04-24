#!/usr/bin/env python3
"""Sweep baud rates on /dev/ttyUSB* and report which gives clean ASCII."""
import serial, time, sys, os

PORT = None
for p in ['/dev/ttyUSB0', '/dev/ttyUSB1']:
    if os.path.exists(p):
        PORT = p
        break
if not PORT:
    print("No ttyUSB found"); sys.exit(1)

BAUDS = [9600, 19200, 38400, 57600, 115200, 125000, 128000, 130000,
         133333, 150000, 153600, 166666, 172800, 200000, 230400]

print(f"Port: {PORT}")
print(f"Sweeping {len(BAUDS)} baud rates, 2s each...")
print()

for baud in BAUDS:
    try:
        s = serial.Serial(PORT, baud, timeout=0.5)
        s.reset_input_buffer()
        time.sleep(0.1)

        data = b''
        start = time.time()
        while time.time() - start < 2.0:
            chunk = s.read(512)
            if chunk:
                data += chunk

        s.close()

        if not data:
            print(f"  {baud:>7}: 0 bytes")
            continue

        # Count printable ASCII
        printable = sum(1 for b in data if 0x20 <= b <= 0x7E or b in (0x0D, 0x0A))
        pct = 100.0 * printable / len(data) if data else 0

        # Try to find "ATOMIK" or "OK" substring
        has_atomik = b'ATOMIK' in data
        has_ok = b'OK' in data
        has_55 = data.count(0x55)

        # Show first 40 bytes as both hex and ASCII
        hex_preview = data[:40].hex()
        ascii_preview = ''.join(chr(b) if 0x20 <= b <= 0x7E else '.' for b in data[:40])

        marker = ""
        if has_atomik: marker = " <<< ATOMIK FOUND!"
        elif pct > 80: marker = " <<< HIGH ASCII"
        elif has_55 > 20: marker = " <<< U-PATTERN"

        print(f"  {baud:>7}: {len(data):>5} bytes, {pct:5.1f}% printable, 0x55 count={has_55}{marker}")
        print(f"           hex: {hex_preview}")
        print(f"           asc: {ascii_preview}")
        print()

    except Exception as e:
        print(f"  {baud:>7}: ERROR: {e}")
