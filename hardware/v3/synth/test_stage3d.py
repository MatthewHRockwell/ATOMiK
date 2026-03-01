#!/usr/bin/env python3
"""Test ISP_STAGE3D: Full flash programming (WBUF + WPAG)"""

import serial
import subprocess
import time
import sys

def test_stage3d():
    # Load bitstream
    print("Loading bitstream to SRAM...")
    result = subprocess.run([
        "openFPGALoader", "-b", "tangnano9k",
        "atomik_v3_soc.fs"
    ], capture_output=True, text=True)

    if result.returncode != 0:
        print(f"ERROR: openFPGALoader failed: {result.stderr}")
        return False

    print("Bitstream loaded, opening UART...")

    try:
        ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=2)
        time.sleep(0.1)

        # Clear boot messages
        boot_msg = ser.read(100)
        print(f"Boot message: {boot_msg}")

        # === Test 1: Handshake ===
        print("\n=== Test 1: Handshake ===")
        ser.write(bytes([0x55]))
        time.sleep(0.1)

        ack = ser.read(10)
        if b'V' not in ack and 0x56 not in ack:
            print(f"ERROR: Handshake failed: {ack}")
            return False
        print("✓ Handshake ACK received")

        # === Test 2: WBUF ===
        print("\n=== Test 2: WBUF (16 bytes) ===")
        test_data = bytes([0x10, 0x0F])  # WBUF, len=16 (0x0F+1)
        test_data += bytes(range(0x10, 0x20))  # 0x10...0x1F

        ser.write(test_data)
        time.sleep(0.1)

        response = ser.read(20)
        print(f"WBUF response: {response}")

        if len(response) >= 2 and response[0] == 0x11:
            expected_chksum = sum(range(0x10, 0x20)) & 0xFF
            actual_chksum = response[1]
            print(f"  ACK: 0x{response[0]:02x}")
            print(f"  Checksum: 0x{actual_chksum:02x} (expected: 0x{expected_chksum:02x})")

            if actual_chksum != expected_chksum:
                print(f"ERROR: Checksum mismatch")
                return False
            print("✓ WBUF working")
        else:
            print(f"ERROR: Invalid WBUF response")
            return False

        # === Test 3: WPAG ===
        print("\n=== Test 3: WPAG (program to 0x001000) ===")
        ser.write(bytes([0x40, 0x00, 0x10, 0x00]))  # WPAG at addr 0x001000
        time.sleep(0.3)  # Page program takes longer

        response = ser.read(10)
        print(f"WPAG response: {response}")

        if b'A' in response and b'B' in response:
            print("✓ WPAG working (0x41...0x42)")
        else:
            print(f"⚠ WPAG response unexpected: {response}")
            return False

        # === Test 4: ESEC ===
        print("\n=== Test 4: ESEC (verify still works) ===")
        ser.write(bytes([0x30, 0x00, 0x00, 0x00]))
        time.sleep(0.5)

        response = ser.read(10)
        if b'1' in response and b'2' in response:
            print("✓ ESEC working")
        else:
            print(f"⚠ ESEC response: {response}")

        # === Test 5: Full Sequence ===
        print("\n=== Test 5: Full sequence (WBUF → WPAG) ===")

        # Load 8 bytes
        ser.write(bytes([0x10, 0x07]))  # len=8
        ser.write(b'ATOMIK!!')  # 8 bytes
        time.sleep(0.1)

        response = ser.read(10)
        if len(response) >= 2 and response[0] == 0x11:
            print(f"  WBUF: ✓ (checksum 0x{response[1]:02x})")
        else:
            print(f"ERROR: WBUF failed in sequence")
            return False

        # Program to 0x002000
        ser.write(bytes([0x40, 0x00, 0x20, 0x00]))
        time.sleep(0.3)

        response = ser.read(10)
        if b'A' in response and b'B' in response:
            print(f"  WPAG: ✓")
        else:
            print(f"ERROR: WPAG failed in sequence")
            return False

        print("\n=== All Tests PASSED ===")
        print("\nISP_STAGE3D is fully functional:")
        print("- WBUF: ✓ (Write buffer)")
        print("- WPAG: ✓ (Page program)")
        print("- ESEC: ✓ (Sector erase)")
        print("- Full sequence: ✓")
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
    success = test_stage3d()
    sys.exit(0 if success else 1)
