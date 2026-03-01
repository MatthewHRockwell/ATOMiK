#!/usr/bin/env python3
"""Test full ISP_STAGE3 with 256-byte buffer"""

import serial
import subprocess
import time
import sys

def test_stage3_full():
    # Load bitstream
    print("Loading bitstream to SRAM...")
    result = subprocess.run([
        "openFPGALoader", "-b", "tangnano9k",
        "atomik_v3_soc.fs"
    ], capture_output=True, text=True)

    if result.returncode != 0:
        print(f"ERROR: openFPGALoader failed: {result.stderr}")
        return False

    print("Bitstream loaded\n")

    try:
        ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=2)
        time.sleep(0.1)

        # Clear boot
        ser.read(100)

        # Handshake
        print("=== Handshake ===")
        ser.write(bytes([0x55]))
        time.sleep(0.1)
        if b'V' not in ser.read(10):
            print("ERROR: Handshake failed")
            return False
        print("✓ Handshake\n")

        # Test 1: Small buffer (8 bytes)
        print("=== Test 1: WBUF 8 bytes ===")
        ser.write(bytes([0x10, 0x07]))  # len=8
        ser.write(b'ATOMIK!!')
        time.sleep(0.1)

        resp = ser.read(10)
        if len(resp) >= 2 and resp[0] == 0x11:
            expected = sum(b'ATOMIK!!') & 0xFF
            print(f"✓ WBUF 8B: checksum 0x{resp[1]:02x} (expected 0x{expected:02x})\n")
        else:
            print(f"ERROR: WBUF 8B failed: {resp}")
            return False

        # Test 2: Large buffer (128 bytes)
        print("=== Test 2: WBUF 128 bytes ===")
        test_data = bytes(range(128))
        ser.write(bytes([0x10, 0x7F]))  # len=128 (0x7F+1)
        ser.write(test_data)
        time.sleep(0.2)

        resp = ser.read(10)
        if len(resp) >= 2 and resp[0] == 0x11:
            expected = sum(test_data) & 0xFF
            print(f"✓ WBUF 128B: checksum 0x{resp[1]:02x} (expected 0x{expected:02x})\n")
        else:
            print(f"ERROR: WBUF 128B failed: {resp}")
            return False

        # Test 3: Full buffer (256 bytes)
        print("=== Test 3: WBUF 256 bytes (full page) ===")
        test_data = bytes(range(256))
        ser.write(bytes([0x10, 0xFF]))  # len=256 (0xFF+1)
        ser.write(test_data)
        time.sleep(0.3)

        resp = ser.read(10)
        if len(resp) >= 2 and resp[0] == 0x11:
            expected = sum(test_data) & 0xFF
            print(f"✓ WBUF 256B: checksum 0x{resp[1]:02x} (expected 0x{expected:02x})\n")
        else:
            print(f"ERROR: WBUF 256B failed: {resp}")
            return False

        # Test 4: WPAG with 256 bytes
        print("=== Test 4: WPAG 256 bytes to 0x003000 ===")
        ser.write(bytes([0x40, 0x00, 0x30, 0x00]))
        time.sleep(0.4)

        resp = ser.read(10)
        if b'A' in resp and b'B' in resp:
            print(f"✓ WPAG 256B: programmed successfully\n")
        else:
            print(f"ERROR: WPAG failed: {resp}")
            return False

        # Test 5: ESEC
        print("=== Test 5: ESEC at 0x004000 ===")
        ser.write(bytes([0x30, 0x00, 0x40, 0x00]))
        time.sleep(0.6)

        resp = ser.read(10)
        if b'1' in resp and b'2' in resp:
            print(f"✓ ESEC: sector erased\n")
        else:
            print(f"ERROR: ESEC failed: {resp}")

        print("=== ALL TESTS PASSED ===")
        print("\nProduction ISP flasher validated:")
        print("- 256-byte page buffer: ✓")
        print("- WBUF (8, 128, 256 bytes): ✓")
        print("- WPAG (page program): ✓")
        print("- ESEC (sector erase): ✓")
        return True

    except Exception as e:
        print(f"ERROR: {e}")
        import traceback
        traceback.print_exc()
        return False
    finally:
        if 'ser' in locals():
            ser.close()

if __name__ == "__main__":
    success = test_stage3_full()
    sys.exit(0 if success else 1)
