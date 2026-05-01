#!/usr/bin/env python3
"""Capture a screenshot from the ATOMiK board and save as PNG."""
import base64
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from board_cmd import send_command

def capture_selfie():
    print("Capturing board selfie...")

    # Run the BMP capture on board
    out, rc = send_command("/tmp/fb_to_bmp")
    print(f"  {out}")

    # Transfer via base64 chunks
    send_command("base64 /tmp/board_selfie.bmp > /tmp/selfie.b64")
    sz_out, _ = send_command("wc -c /tmp/selfie.b64")
    total = int(sz_out.split()[0]) if sz_out.strip() else 0
    print(f"  B64 size: {total}")

    all_data = []
    pos = 0
    chunk = 500
    while pos < total:
        result, _ = send_command(f"dd if=/tmp/selfie.b64 bs=1 skip={pos} count={chunk} 2>/dev/null")
        if result:
            all_data.append(result.replace('\n', '').replace('\r', ''))
        pos += chunk
        if pos % 50000 < chunk:
            print(f"  {int(100*pos/total)}%")

    b64_str = ''.join(all_data)
    b64_clean = ''.join(c for c in b64_str if c in 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=')
    while len(b64_clean) % 4 != 0:
        b64_clean += '='

    raw = base64.b64decode(b64_clean)

    # Save as BMP first
    ts = time.strftime("%Y%m%d_%H%M%S")
    bmp_path = f"/tmp/atomik_selfie_{ts}.bmp"
    with open(bmp_path, 'wb') as f:
        f.write(raw)

    # Convert to PNG
    try:
        from PIL import Image
        img = Image.open(bmp_path)
        png_path = f"/tmp/atomik_selfie_{ts}.png"
        img.save(png_path)
        print(f"  Saved: {png_path} ({img.size[0]}x{img.size[1]})")
        os.remove(bmp_path)
        return png_path
    except ImportError:
        print(f"  Saved: {bmp_path} (install PIL for PNG)")
        return bmp_path

if __name__ == "__main__":
    capture_selfie()
