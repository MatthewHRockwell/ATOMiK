#!/usr/bin/env python3
"""Test persistent boot from SPI flash"""

import serial
import subprocess
import time
import sys

def test_flash_boot():
    print("=== Testing Persistent Flash Boot ===\n")

    # Load bitstream
    print("Loading bitstream (will trigger ISP timeout → flash boot)...")
    result = subprocess.run([
        "openFPGALoader", "-b", "tangnano9k",
        "atomik_v3_soc.fs"
    ], capture_output=True, text=True)

    if result.returncode != 0:
        print(f"ERROR: Bitstream load failed: {result.stderr}")
        return False

    print("✓ Bitstream loaded\n")

    # Open UART and wait for ISP timeout + flash boot
    print("Opening UART (waiting for ISP timeout → flash execution)...")
    ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=3)

    # ISP timeout is ~200ms, give it some time
    time.sleep(0.5)

    # Read output (should see flash firmware)
    output = ser.read(200)
    ser.close()

    print(f"UART output: {output}\n")

    # Check for flash firmware marker
    if b'F!F!' in output or b'F!' in output:
        print("=== SUCCESS ===")
        print("✓ Flash firmware is executing!")
        print("✓ Persistent boot from SPI flash working")
        print(f"\nOutput: {output.decode('ascii', errors='replace')}")
        return True
    else:
        print("=== FAILURE ===")
        print("✗ Flash firmware not detected")
        print("Expected: 'F!F!' pattern")
        print(f"Got: {output}")
        return False

if __name__ == "__main__":
    success = test_flash_boot()
    sys.exit(0 if success else 1)
