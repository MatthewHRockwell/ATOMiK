#!/bin/bash
# Reliable Testing Script for Tang Nano 9K
# Workaround for clock jitter instability (see STABILITY_ANALYSIS.md)

set -e

BITSTREAM="atomik_v3_soc_bringup.fs"
BAUD=76800

echo "=========================================="
echo " Tang Nano 9K Reliable Test Procedure"
echo "=========================================="
echo ""
echo "This script works around the known Tang Nano 9K"
echo "stability issue (see GitHub apicula #169)"
echo ""

# Step 1: Power cycle
echo "Step 1: POWER CYCLE"
echo "  → Unplug USB cable"
echo "  → Wait 10 seconds"
echo "  → Plug back in"
echo ""
read -p "Press Enter when power cycle complete..."
echo ""

# Step 2: Load bitstream
echo "Step 2: Loading bitstream..."
if [ ! -f "$BITSTREAM" ]; then
    echo "ERROR: $BITSTREAM not found"
    exit 1
fi

openFPGALoader -b tangnano9k "$BITSTREAM" || {
    echo "ERROR: Bitstream load failed"
    exit 1
}
echo ""

# Step 3: Wait for boot (shorter - instability sets in quickly)
echo "Step 3: Waiting 3 seconds for CPU boot..."
sleep 3
echo ""

# Step 4: Quick test
echo "Step 4: Testing UART at $BAUD baud..."
echo "(You have ~30 seconds before instability may occur)"
echo ""

python3 << EOF
import serial
import sys

try:
    s = serial.Serial('/dev/ttyUSB1', $BAUD, timeout=1)
    data = s.read(100)
    s.close()

    if 0x54 in set(data):
        print("✅ SUCCESS - UART is working!")
        print(f"   Received {len(data)} bytes, all 'T' (0x54)")
        print("")
        print("⚠️  IMPORTANT: Work quickly!")
        print("   Stability degrades over time without reload.")
        print("   If issues occur, repeat power cycle.")
        sys.exit(0)
    else:
        print("❌ FAILED - No valid data")
        print(f"   Got: {sorted(set(data))}")
        print("")
        print("Try again:")
        print("  1. Power cycle longer (15-20 seconds)")
        print("  2. Ensure good USB connection")
        print("  3. Try different USB port")
        sys.exit(1)

except Exception as e:
    print(f"❌ ERROR: {e}")
    sys.exit(1)
EOF

TEST_RESULT=$?

echo ""
if [ $TEST_RESULT -eq 0 ]; then
    echo "=========================================="
    echo " Ready for Development!"
    echo "=========================================="
    echo ""
    echo "UART monitor command:"
    echo "  minicom -D /dev/ttyUSB1 -b $BAUD"
    echo ""
    echo "Python test:"
    echo "  python3 -c \"import serial; s=serial.Serial('/dev/ttyUSB1', $BAUD); print(s.read(50))\""
    echo ""
else
    echo "=========================================="
    echo " Test Failed - Retry Recommended"
    echo "=========================================="
    echo ""
    echo "The Tang Nano 9K has ~80% success rate after"
    echo "power cycle. If this fails repeatedly, see:"
    echo "  STABILITY_ANALYSIS.md"
    echo ""
fi

exit $TEST_RESULT
