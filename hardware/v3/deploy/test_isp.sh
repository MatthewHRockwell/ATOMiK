#!/bin/bash
# Test ISP Protocol - Phase 3D Task 1
set -e

echo "=========================================="
echo " Phase 3D Task 1: ISP Protocol Test"
echo "=========================================="
echo ""

# Load the CORRECT bitstream
echo "Step 1: Loading ISP bitstream..."
openFPGALoader -b tangnano9k atomik_v3_soc_isp.fs
echo "✓ Bitstream loaded"
echo ""

# Wait for boot
echo "Step 2: Waiting 2 seconds for CPU boot..."
sleep 2
echo ""

# Test ISP handshake
echo "Step 3: Testing ISP handshake (0x55 → 0x56)..."
python3 << 'EOF'
import serial
import time

try:
    ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=0.5)

    # Wait a bit more
    time.sleep(0.5)

    # Send ISP handshake
    for i in range(10):
        ser.write(bytes([0x55, 0x55]))
        time.sleep(0.1)
        resp = ser.read()
        if len(resp) > 0:
            if resp[0] == 0x56:
                print(f"✅ SUCCESS! ISP Boot ROM responded correctly!")
                print(f"   Sent: 0x55, Received: 0x{resp[0]:02X}")
                ser.close()
                exit(0)
            elif resp[0] == ord('T'):
                print(f"❌ FAIL: Received 'T' (0x{resp[0]:02X}) - BRINGUP_MODE is still active!")
                print(f"   This means the wrong bitstream was loaded.")
                ser.close()
                exit(1)
            else:
                print(f"❌ FAIL: Received unexpected byte 0x{resp[0]:02X}")
                ser.close()
                exit(1)

    print("❌ FAIL: No response from Boot ROM")
    print("   Check USB connection and /dev/ttyUSB1")
    ser.close()
    exit(1)

except Exception as e:
    print(f"❌ ERROR: {e}")
    exit(1)
EOF

if [ $? -eq 0 ]; then
    echo ""
    echo "=========================================="
    echo " ✅ Task 1 Hardware Test: PASS"
    echo "=========================================="
    echo ""
    echo "Next step: Flash firmware test"
    echo "Run: cd ../soc/firmware/fw-flash && python3 ../scripts/pico-programmer.py build/fw-flash.v /dev/ttyUSB1"
else
    echo ""
    echo "=========================================="
    echo " ❌ Task 1 Hardware Test: FAIL"
    echo "=========================================="
    echo ""
    echo "Debug: Check that atomik_v3_soc_isp.fs was loaded (not an older bitstream)"
fi
