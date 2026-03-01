#!/usr/bin/env python3
"""Test ISP_STAGE3B on hardware"""

import serial
import subprocess
import time
import sys

def test_stage3b():
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

    # Open UART immediately (0ms delay, not 2-6s)
    try:
        ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=2)
        time.sleep(0.1)  # Small delay for port to settle

        # Read boot message
        boot_msg = ser.read(100)
        print(f"Boot message: {boot_msg}")

        # Send ISP handshake (0x55)
        print("\nSending ISP handshake (0x55)...")
        ser.write(bytes([0x55]))
        time.sleep(0.1)

        # Read ACK (should be 0x56 = 'V')
        ack = ser.read(10)
        print(f"ACK: {ack}")

        if b'V' in ack or 0x56 in ack:
            print("SUCCESS: ISP handshake ACK received!")

            # Test ESEC command (0x30 + 3-byte address)
            print("\nTesting ESEC command (0x30 + addr 0x000000)...")
            ser.write(bytes([0x30, 0x00, 0x00, 0x00]))
            time.sleep(0.5)  # Erase takes time

            response = ser.read(10)
            print(f"ESEC response: {response}")

            if b'1' in response and b'2' in response:
                print("SUCCESS: ESEC command executed (received 0x31...0x32)!")
                return True
            else:
                print("WARNING: ESEC response unexpected")
                return False
        else:
            print(f"ERROR: Expected ACK 'V' (0x56), got: {ack}")
            return False

    except Exception as e:
        print(f"ERROR: {e}")
        return False
    finally:
        if 'ser' in locals():
            ser.close()

if __name__ == "__main__":
    success = test_stage3b()
    sys.exit(0 if success else 1)
