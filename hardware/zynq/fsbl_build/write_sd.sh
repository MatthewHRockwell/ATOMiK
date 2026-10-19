#!/bin/bash
# write_sd.sh — Format SD card and copy ATOMiK standalone boot files.
# Usage: sudo bash write_sd.sh /dev/sdX
# WARNING: This WIPES the target device.

set -euo pipefail

DEV="${1:-}"
if [ -z "$DEV" ]; then
    echo "Usage: sudo $0 /dev/sdX"
    lsblk -o NAME,SIZE,TYPE,FSTYPE | grep -v loop
    exit 1
fi

ZYNQ_DIR="$(dirname "$(realpath "$0")")/.."

echo "=== ATOMiK SD boot card writer ==="
echo "Target: $DEV"
echo "Press Ctrl-C to abort, Enter to continue..."
read

# Unmount any existing partitions
umount "${DEV}"* 2>/dev/null || true

# Wipe partition table and create new GPT/MBR with one FAT32 partition
echo "Partitioning $DEV..."
parted -s "$DEV" mklabel msdos
parted -s "$DEV" mkpart primary fat32 4MiB 100%
parted -s "$DEV" set 1 boot on

# Format FAT32
PART="${DEV}1"
sleep 1
mkfs.vfat -F 32 -n "ATOMIK_BOOT" "$PART"
echo "Formatted $PART as FAT32"

# Mount
MNT=$(mktemp -d /tmp/atomik_sd_XXXXXX)
mount "$PART" "$MNT"
echo "Mounted at $MNT"

# Copy files
echo "Copying boot files..."
cp -v "$ZYNQ_DIR/fsbl_build/BOOT.bin"                                          "$MNT/BOOT.bin"
cp -v "$ZYNQ_DIR/litex-build-nax64-sdboot/gateware/hamgeek_rk7020f.bit.bin"   "$MNT/nax64.bit.bin"
cp -v "$ZYNQ_DIR/litex-build/fw_jump69.bin"                                     "$MNT/fw_jump69.bin"
cp -v "$ZYNQ_DIR/litex-build/linux69.dtb"                                       "$MNT/linux69.dtb"
cp -v "$ZYNQ_DIR/litex-build/rootfs69.cpio"                                     "$MNT/rootfs69.cpio"
cp -v "$ZYNQ_DIR/litex-build/Image69"                                           "$MNT/Image69"

echo ""
echo "SD card contents:"
ls -lh "$MNT/"

sync
umount "$MNT"
rmdir "$MNT"
echo ""
echo "=== Done. SD card is ready. ==="
echo ""
echo "Next steps:"
echo "  1. Set AX7020 boot mode jumpers to SD (check board manual for JP1/JP2)"
echo "  2. Insert SD card into board slot"
echo "  3. Power on — board boots autonomously (no JTAG needed)"
echo "  4. Monitor UART at /dev/ttyUSB1 @ 921600 to watch LiteX BIOS boot"
