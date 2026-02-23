#!/bin/bash
# Phase 3B: Flash XIP Validation - Deployment Script
#
# This script deploys the v3 SoC bitstream and flash firmware to Tang Nano 9K
# for validating Execute-In-Place (XIP) from SPI flash.

set -e  # Exit on error

PROJ_ROOT="/home/mattrock/Projects/ATOMiK/hardware/v3"
BITSTREAM="$PROJ_ROOT/synth/impl/pnr/atomik_v3_soc.fs"
FLASH_FW="$PROJ_ROOT/soc/firmware/fw-flash/build-xip/fw-flash-xip.v"
ISP_PROGRAMMER="/home/mattrock/Projects/TangNano-9K-example/picotiny/pico-programmer.py"
UART_PORT="/dev/ttyUSB1"

echo "========================================="
echo "Phase 3B: Flash XIP Validation Deployment"
echo "========================================="
echo ""

# Step 1: Check prerequisites
echo "[1/3] Checking prerequisites..."
if [ ! -f "$BITSTREAM" ]; then
    echo "ERROR: Bitstream not found at $BITSTREAM"
    echo "Run synthesis first: cd $PROJ_ROOT/synth && gw_sh synth_v3_soc.tcl"
    exit 1
fi

if [ ! -f "$FLASH_FW" ]; then
    echo "ERROR: Flash firmware not found at $FLASH_FW"
    echo "Build firmware first: cd $PROJ_ROOT/soc/firmware/fw-flash && make -f Makefile.xip"
    exit 1
fi

if [ ! -f "$ISP_PROGRAMMER" ]; then
    echo "ERROR: ISP programmer not found at $ISP_PROGRAMMER"
    exit 1
fi

if [ ! -e "$UART_PORT" ]; then
    echo "ERROR: UART port $UART_PORT not found"
    echo "Check USB connection to Tang Nano 9K"
    exit 1
fi

echo "  ✓ Bitstream: $BITSTREAM"
echo "  ✓ Flash FW:  $FLASH_FW"
echo "  ✓ UART:      $UART_PORT"
echo ""

# Step 2: Flash bitstream to persistent flash
echo "[2/3] Flashing bitstream to persistent flash..."
echo "  This will take ~30 seconds..."
openFPGALoader -b tangnano9k -f "$BITSTREAM"
echo "  ✓ Bitstream flashed successfully"
echo ""

# Step 3: Program flash firmware via ISP
echo "[3/3] Programming flash firmware via ISP..."
echo ""
echo "  **ACTION REQUIRED:**"
echo "  Press the RESET button (S1) on Tang Nano 9K NOW"
echo "  (You have 5 seconds before the script continues)"
echo ""
sleep 5

echo "  Starting ISP programmer..."
python3 "$ISP_PROGRAMMER" "$FLASH_FW" "$UART_PORT"

echo ""
echo "========================================="
echo "Deployment Complete!"
echo "========================================="
echo ""
echo "Expected Boot Sequence:"
echo "  1. Boot ROM prints: J<hex>!  (jumping to flash)"
echo "  2. Flash firmware banner appears"
echo "  3. Heartbeat: FLASH-XIP-OK [counter]"
echo ""
echo "Monitor serial output with:"
echo "  picocom -b 115200 $UART_PORT"
echo "or:"
echo "  hexdump -C $UART_PORT"
echo ""
