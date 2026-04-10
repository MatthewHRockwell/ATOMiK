#!/usr/bin/env python3
"""
SFL (Serial Flash Loader) boot tool for LiteX BIOS.

Uploads boot images (OpenSBI, DTB, kernel, rootfs) to DDR via the LiteX
BIOS serial boot protocol, then jumps to the boot address.

Implements the SFL protocol from litex/tools/litex_term.py:
- CRC16-CCITT framing
- Real-time magic detection (byte-by-byte sliding window)
- Automatic serialboot command if BIOS is at prompt

Usage:
    python3 sfl_boot.py --port /dev/ttyUSB2 --images boot.json
    python3 sfl_boot.py --port /dev/ttyUSB2 \\
        --opensbi fw_jump.bin@0x40f00000 \\
        --dtb nax64.dtb@0x40ef0000 \\
        --kernel Image@0x40400000 \\
        --rootfs rootfs.cpio@0x42000000
"""

import argparse
import json
import os
import serial
import struct
import sys
import time

# SFL protocol constants (from litex/tools/litex_term.py)
SFL_MAGIC_REQ = b"sL5DdSMmkekro\n"
SFL_MAGIC_ACK = b"z6IHG7cYDID6o\n"
SFL_CMD_LOAD  = b"\x01"
SFL_CMD_JUMP  = b"\x02"
SFL_PAYLOAD_MAX = 255

# CRC16-CCITT table
CRC16_TABLE = [
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0,
]


def crc16(data):
    crc = 0
    for b in data:
        crc = CRC16_TABLE[((crc >> 8) ^ b) & 0xFF] ^ ((crc << 8) & 0xFFFF)
    return crc


def sfl_frame(cmd, payload):
    """Encode an SFL frame: [length][crc16_hi][crc16_lo][cmd][payload]"""
    crc = crc16(cmd + payload)
    return bytes([len(payload)]) + crc.to_bytes(2, "big") + cmd + payload


def sfl_handshake(ser, timeout=10):
    """Trigger serialboot and complete the SFL magic handshake."""
    ser.reset_input_buffer()
    ser.write(b"serialboot\r\n")

    detect_buf = bytes(len(SFL_MAGIC_REQ))
    start = time.time()
    while time.time() - start < timeout:
        c = ser.read(1)
        if c:
            detect_buf = detect_buf[1:] + c
            if detect_buf == SFL_MAGIC_REQ:
                ser.write(SFL_MAGIC_ACK)
                time.sleep(0.1)
                ser.read(4096)  # drain echo
                return True
    return False


def sfl_upload(ser, name, addr, data):
    """Upload data to target memory via SFL protocol."""
    chunk_size = SFL_PAYLOAD_MAX - 4  # 251 bytes (255 - 4 addr bytes)
    total = len(data)
    offset = 0
    retries = 0

    while offset < total:
        chunk = data[offset:offset + chunk_size]
        payload = (addr + offset).to_bytes(4, "big") + chunk
        frame = sfl_frame(SFL_CMD_LOAD, payload)
        ser.write(frame)
        ack = ser.read(1)

        if ack == b"K":
            offset += len(chunk)
            if offset % (chunk_size * 200) == 0 or offset >= total:
                pct = offset * 100 // total
                rate_kb = offset / 1024 / (time.time() - sfl_upload._start + 0.01)
                print(f"\r  [{name}] {offset // 1024}KB/{total // 1024}KB "
                      f"({pct}%) {rate_kb:.0f}KB/s", end="", flush=True)
        elif ack == b"C":
            retries += 1
            if retries > 100:
                print(f"\n  ERROR: too many CRC retries ({retries})")
                return False
        else:
            print(f"\n  ERROR at offset 0x{offset:x}: ack={ack!r}")
            return False

    print(f"\r  [{name}] {total // 1024}KB/{total // 1024}KB "
          f"(100%) [{retries} retries]")
    return True


sfl_upload._start = 0  # timing reference


def sfl_jump(ser, addr):
    """Send SFL jump command."""
    frame = sfl_frame(SFL_CMD_JUMP, addr.to_bytes(4, "big"))
    ser.write(frame)


def main():
    parser = argparse.ArgumentParser(
        description="SFL serial boot tool for LiteX BIOS")
    parser.add_argument("--port", default="/dev/ttyUSB2",
                        help="Serial port (default: /dev/ttyUSB2)")
    parser.add_argument("--baud", default=115200, type=int,
                        help="Baud rate (default: 115200)")
    parser.add_argument("--images", default=None,
                        help="JSON file mapping filenames to hex addresses")
    parser.add_argument("--boot-addr", default=None,
                        help="Jump address (default: last image address)")
    parser.add_argument("--timeout", default=120, type=int,
                        help="Boot output capture timeout in seconds")
    # Individual image arguments (alternative to --images)
    parser.add_argument("--opensbi", default=None, help="OpenSBI: file@addr")
    parser.add_argument("--dtb", default=None, help="DTB: file@addr")
    parser.add_argument("--kernel", default=None, help="Kernel: file@addr")
    parser.add_argument("--rootfs", default=None, help="Rootfs: file@addr")
    args = parser.parse_args()

    # Build image list
    images = []
    if args.images:
        with open(args.images) as f:
            img_map = json.load(f)
        base_dir = os.path.dirname(os.path.abspath(args.images))
        for filename, addr_str in img_map.items():
            path = os.path.join(base_dir, filename)
            addr = int(addr_str, 16) if isinstance(addr_str, str) else addr_str
            images.append((filename, addr, path))
    else:
        for arg_name in ["dtb", "kernel", "rootfs", "opensbi"]:
            val = getattr(args, arg_name)
            if val:
                parts = val.split("@")
                path = parts[0]
                addr = int(parts[1], 16)
                images.append((os.path.basename(path), addr, path))

    if not images:
        parser.error("No images specified. Use --images or --opensbi/--dtb/--kernel/--rootfs")

    # Determine boot address
    boot_addr = int(args.boot_addr, 16) if args.boot_addr else None
    # Default: jump to the last image loaded (usually OpenSBI)
    if boot_addr is None:
        boot_addr = images[-1][1]

    print(f"=== SFL Boot Tool ===")
    print(f"Port: {args.port} @ {args.baud}")
    print(f"Images: {len(images)}")
    for name, addr, path in images:
        size = os.path.getsize(path)
        print(f"  {name}: 0x{addr:08x} ({size // 1024}KB)")
    print(f"Boot addr: 0x{boot_addr:08x}")
    print()

    # Open serial
    ser = serial.Serial(args.port, args.baud, timeout=0.01)
    time.sleep(3)  # wait for BIOS to init

    # SFL handshake
    print("Triggering serialboot...")
    if not sfl_handshake(ser):
        print("ERROR: SFL handshake failed (no magic from BIOS)")
        ser.close()
        sys.exit(1)
    print("SFL handshake OK")

    # Upload all images
    ser.timeout = 1
    sfl_upload._start = time.time()
    for name, addr, path in images:
        with open(path, "rb") as f:
            data = f.read()
        print(f"Loading {name} ({len(data) // 1024}KB) → 0x{addr:08x}")
        if not sfl_upload(ser, name, addr, data):
            print(f"FAILED loading {name}")
            ser.close()
            sys.exit(1)

    elapsed = time.time() - sfl_upload._start
    total_kb = sum(os.path.getsize(p) for _, _, p in images) // 1024
    print(f"\nAll images loaded ({total_kb}KB in {elapsed:.0f}s, "
          f"{total_kb / elapsed:.0f}KB/s)")

    # Jump to boot address
    print(f"Jumping to 0x{boot_addr:08x}...")
    sfl_jump(ser, boot_addr)

    # Capture boot output
    print("\n=== Boot Output ===")
    ser.timeout = 1
    start = time.time()
    total_bytes = 0
    try:
        while time.time() - start < args.timeout:
            data = ser.read(1024)
            if data:
                total_bytes += len(data)
                sys.stdout.write(data.decode("utf-8", errors="replace"))
                sys.stdout.flush()
    except KeyboardInterrupt:
        print("\n[Interrupted]")

    print(f"\n=== {total_bytes} bytes in {time.time() - start:.0f}s ===")
    ser.close()


if __name__ == "__main__":
    main()
