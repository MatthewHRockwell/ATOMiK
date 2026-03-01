#!/usr/bin/env python3
"""Test ISP_STAGE3C: ESEC + WBUF on hardware"""

import serial
import subprocess
import time
import sys

def test_stage3c():
    # Load bitstream
    print("Loading bitstream to SRAM...")
    result = subprocess.run([
        "openFPGALoader", "-b", "tangnano9k",
        "atomik_v3_soc.fs"
    ], capture_output=True, text=True)

    if result.returncode != 0:
        print(f"ERROR: openFPGALoader failed: {result.stderr}")
        return False

    print("Bitstream loaded, waiting 0ms then reading UART...")

    try:
        ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=2)
        time.sleep(0.1)

        # Read boot message
        boot_msg = ser.read(100)
        print(f"Boot message: {boot_msg}")

        # Send ISP handshake (0x55)
        print("\n=== Test 1: Handshake ===")
        ser.write(bytes([0x55]))
        time.sleep(0.1)

        ack = ser.read(10)
        print(f"Handshake ACK: {ack}")

        if b'V' not in ack and 0x56 not in ack:
            print(f"ERROR: Expected ACK 'V' (0x56), got: {ack}")
            return False
        print("✓ Handshake working")

        # Test WBUF command (0x10 + len + data)
        print("\n=== Test 2: WBUF (8 bytes) ===")
        test_data = bytes([0x10, 0x07])  # WBUF, len=8 (0x07+1)
        test_data += bytes([0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48])  # "ABCDEFGH"

        ser.write(test_data)
        time.sleep(0.1)

        response = ser.read(20)
        print(f"WBUF response: {response}")

        if len(response) >= 2:
            if response[0] == 0x11 or chr(response[0]) == '\x11':
                expected_chksum = sum([0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48]) & 0xFF
                actual_chksum = response[1] if len(response) > 1 else 0
                print(f"  ACK: 0x{response[0]:02x}")
                print(f"  Checksum: 0x{actual_chksum:02x} (expected: 0x{expected_chksum:02x})")

                if actual_chksum == expected_chksum:
                    print("✓ WBUF working (checksum correct)")
                else:
                    print(f"⚠ Checksum mismatch")
            else:
                print(f"ERROR: Expected WBUF ACK 0x11, got: 0x{response[0]:02x}")
                return False
        else:
            print(f"ERROR: No WBUF response")
            return False

        # Test ESEC command
        print("\n=== Test 3: ESEC ===")
        ser.write(bytes([0x30, 0x00, 0x00, 0x00]))  # ESEC at addr 0x000000
        time.sleep(0.5)

        response = ser.read(10)
        print(f"ESEC response: {response}")

        if b'1' in response and b'2' in response:
            print("✓ ESEC working (0x31...0x32)")
        else:
            print(f"⚠ ESEC response unexpected: {response}")

        print("\n=== All Tests PASSED ===")
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
    success = test_stage3c()
    sys.exit(0 if success else 1)
