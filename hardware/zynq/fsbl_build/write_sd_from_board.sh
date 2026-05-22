#!/bin/bash
# write_sd_from_board.sh — Populate the SD card directly from board Linux.
#
# Run this on the BOARD (root shell on atomik-rv64).
# Prerequisites:
#   /tmp/BOOT.bin     — transferred from laptop via fast_send.py
#   /tmp/nax64.bit.bin — transferred from laptop via fast_send.py
#   /tmp/sd_rw        — compiled SD raw sector writer (sdhc via /dev/mem)
#
# Files from DDR (already loaded there by jtag_boot.py):
#   Image69     at NaxRiscv phys 0x40000000 (PS 0x00100000)
#   linux69.dtb at NaxRiscv phys 0x40EF0000 (PS 0x00EF0000)
#   fw_jump69.bin at NaxRiscv phys 0x40F00000 (PS 0x00F00000)
#   rootfs69.cpio at NaxRiscv phys 0x42100000 (PS 0x02100000)
#
# SD card layout (FAT32):
#   LBA 0        MBR
#   LBA 8192     VBR (FAT32 partition start)
#   LBA 8224     FAT1 (32 sectors)
#   LBA 8256     FAT2 (32 sectors)
#   LBA 8288     Root directory
#   LBA 8352     BOOT.bin        (~7800 sectors, 3.99MB)
#   LBA 16152    nax64.bit.bin   (~7800 sectors, 3.99MB)
#   LBA 23952    fw_jump69.bin   (~512 sectors, 262KB)
#   LBA 24464    linux69.dtb     (~6 sectors, 3KB)
#   LBA 24470    rootfs69.cpio   (~16384 sectors, 8.4MB)
#   LBA 40854    Image69         (~16384 sectors, 8.4MB)
#
# Cluster 2 = root dir
# Cluster 3 = BOOT.bin    first cluster LBA = 8288+64=8352
# Cluster 125 = nax64.bit.bin ...

set -e
SD_RW=/tmp/sd_rw

check_file() {
    if [ ! -f "$1" ]; then
        echo "ERROR: $1 not found"
        exit 1
    fi
    echo "  OK: $1 ($(ls -lh $1 | awk '{print $5}'))"
}

echo "=== Checking prerequisites ==="
check_file /tmp/BOOT.bin
check_file /tmp/nax64.bit.bin
check_file $SD_RW

# Detect SD card
echo ""
echo "=== Detecting SD card ==="
$SD_RW detect

# Create FAT32 structure using Python (simpler than shell arithmetic)
echo ""
echo "=== Writing MBR + FAT32 structure ==="
python3 /tmp/make_fat32_from_board.py

echo ""
echo "=== Writing BOOT.bin to SD ==="
cat /tmp/BOOT.bin | $SD_RW write 8352 7800

echo ""
echo "=== Writing nax64.bit.bin to SD ==="
cat /tmp/nax64.bit.bin | $SD_RW write 16152 7800

echo ""
echo "=== Dumping fw_jump69.bin from DDR (0x40F00000) ==="
dd if=/dev/mem bs=512 skip=$((0x40F00000/512)) count=512 2>/dev/null | $SD_RW write 23952 512

echo ""
echo "=== Dumping linux69.dtb from DDR (0x40EF0000) ==="
dd if=/dev/mem bs=512 skip=$((0x40EF0000/512)) count=8 2>/dev/null | $SD_RW write 24464 8

echo ""
echo "=== Dumping rootfs69.cpio from DDR (0x42100000) ==="
echo "NOTE: This reads 8MB from DDR — kernel may have freed this memory."
echo "      If garbage, you must transfer rootfs69.cpio separately."
dd if=/dev/mem bs=512 skip=$((0x42100000/512)) count=16384 2>/dev/null | $SD_RW write 24472 16384

echo ""
echo "=== Dumping Image69 from DDR (0x40000000) ==="
echo "NOTE: This is the running kernel — reading its own text is safe (read-only)."
dd if=/dev/mem bs=512 skip=$((0x40000000/512)) count=16384 2>/dev/null | $SD_RW write 40856 16384

echo ""
echo "=== SD card written ==="
echo "Set AX7020 boot jumpers to SD mode and power cycle."
