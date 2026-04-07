#!/bin/bash
#
# prepare_boot_images.sh — Prepare buildroot images for Zynq ATOMiK boot
#
# Takes buildroot output/images/ and produces ready-to-boot files in litex-build/:
#   1. Copies kernel, opensbi, rootfs
#   2. Injects atomik_test into rootfs via cpio concatenation
#   3. Updates DTB with correct initrd-start/end addresses
#   4. Generates boot_linux.json for litex_term SFL upload
#
# Usage:
#   ./prepare_boot_images.sh /path/to/buildroot/output/images
#
# The script uses these fixed VexRiscv DDR addresses:
#   0x40000000  kernel Image
#   0x40ef0000  DTB
#   0x40f00000  OpenSBI fw_jump.bin
#   0x42000000  rootfs.cpio (with atomik_test injected)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ZYNQ_DIR="$(dirname "$SCRIPT_DIR")"
LITEX_BUILD="$ZYNQ_DIR/litex-build"
TEST_BIN="$ZYNQ_DIR/test/atomik_test_linux"
DTS_SRC="$LITEX_BUILD/linux32.dts"

# Addresses (VexRiscv view)
KERNEL_ADDR="0x40000000"
DTB_ADDR="0x40ef0000"
OPENSBI_ADDR="0x40f00000"
INITRD_ADDR="0x42000000"

usage() {
    echo "Usage: $0 <buildroot-images-dir>"
    echo "  e.g. $0 /home/mattrock/buildroot-litex/output/images"
    exit 1
}

[ $# -eq 1 ] || usage
IMAGES_DIR="$1"

# Verify inputs exist
for f in "$IMAGES_DIR/Image" "$IMAGES_DIR/fw_jump.bin" "$IMAGES_DIR/rootfs.cpio"; do
    [ -f "$f" ] || { echo "ERROR: $f not found"; exit 1; }
done
[ -f "$TEST_BIN" ] || { echo "ERROR: $TEST_BIN not found (run: make -C $ZYNQ_DIR test)"; exit 1; }
[ -f "$DTS_SRC" ] || { echo "ERROR: $DTS_SRC not found"; exit 1; }

echo "=== ATOMiK Zynq Boot Image Preparation ==="
echo ""

# Step 1: Copy base images
echo "[1/5] Copying base images..."
cp "$IMAGES_DIR/Image" "$LITEX_BUILD/Image69"
cp "$IMAGES_DIR/fw_jump.bin" "$LITEX_BUILD/fw_jump69.bin"
cp "$IMAGES_DIR/rootfs.cpio" "$LITEX_BUILD/rootfs69_base.cpio"

KERNEL_SIZE=$(stat -c%s "$LITEX_BUILD/Image69")
OPENSBI_SIZE=$(stat -c%s "$LITEX_BUILD/fw_jump69.bin")
printf "  Kernel:  %d bytes (%.1f MB)\n" "$KERNEL_SIZE" "$(echo "$KERNEL_SIZE / 1048576" | bc -l)"
printf "  OpenSBI: %d bytes\n" "$OPENSBI_SIZE"

# Step 2: Create supplementary cpio with atomik_test
echo "[2/5] Injecting atomik_test into rootfs..."
TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

mkdir -p "$TMPDIR/root"
cp "$TEST_BIN" "$TMPDIR/root/atomik_test"
chmod 755 "$TMPDIR/root/atomik_test"

# Create cpio archive (newc format, matching buildroot)
(cd "$TMPDIR" && find . | cpio -o -H newc --quiet) > "$TMPDIR/overlay.cpio"

# Concatenate: base rootfs + overlay
cat "$LITEX_BUILD/rootfs69_base.cpio" "$TMPDIR/overlay.cpio" > "$LITEX_BUILD/rootfs69.cpio"

ROOTFS_SIZE=$(stat -c%s "$LITEX_BUILD/rootfs69.cpio")
printf "  rootfs:  %d bytes (%.1f MB, includes atomik_test)\n" "$ROOTFS_SIZE" "$(echo "$ROOTFS_SIZE / 1048576" | bc -l)"

# Step 3: Compute initrd-end address
INITRD_START=$(printf "%d" "$INITRD_ADDR")
INITRD_END=$((INITRD_START + ROOTFS_SIZE))
INITRD_END_HEX=$(printf "0x%08X" "$INITRD_END")

echo "[3/5] Updating DTB initrd addresses..."
echo "  initrd-start: $INITRD_ADDR"
echo "  initrd-end:   $INITRD_END_HEX"

# Patch DTS: update initrd-start and initrd-end, rdinit → /sbin/init for buildroot
# (buildroot rootfs has /sbin/init, not just /bin/sh)
sed -e "s|linux,initrd-start = <0x[0-9A-Fa-f]*>;|linux,initrd-start = <$INITRD_ADDR>;|" \
    -e "s|linux,initrd-end = <0x[0-9A-Fa-f]*>;|linux,initrd-end = <$INITRD_END_HEX>;|" \
    -e "s|rdinit=/bin/sh|rdinit=/sbin/init|" \
    "$DTS_SRC" > "$LITEX_BUILD/linux69.dts"

# Step 4: Compile DTB
echo "[4/5] Compiling DTB..."
dtc -I dts -O dtb -o "$LITEX_BUILD/linux69.dtb" "$LITEX_BUILD/linux69.dts" 2>/dev/null
DTB_SIZE=$(stat -c%s "$LITEX_BUILD/linux69.dtb")
printf "  DTB:     %d bytes\n" "$DTB_SIZE"

# Step 5: Generate boot_linux.json
echo "[5/5] Generating boot_linux.json..."
cat > "$LITEX_BUILD/boot_linux69.json" << EOF
{
    "Image69":        "$KERNEL_ADDR",
    "rootfs69.cpio":  "$INITRD_ADDR",
    "linux69.dtb":    "$DTB_ADDR",
    "fw_jump69.bin":  "$OPENSBI_ADDR"
}
EOF

echo ""
echo "=== Ready ==="
echo ""
echo "Files in $LITEX_BUILD/:"
echo "  Image69           — Linux 6.9 kernel (rv32ima)"
echo "  rootfs69.cpio     — BusyBox rootfs + atomik_test"
echo "  linux69.dtb       — DTB (updated initrd range)"
echo "  fw_jump69.bin     — OpenSBI 1.3.1"
echo "  boot_linux69.json — SFL upload addresses"
echo ""

# Total upload size for SFL
TOTAL=$((KERNEL_SIZE + ROOTFS_SIZE + DTB_SIZE + OPENSBI_SIZE))
printf "Total upload: %d bytes (%.1f MB)\n" "$TOTAL" "$(echo "$TOTAL / 1048576" | bc -l)"
echo "Estimated SFL upload time: ~$((TOTAL / 11520)) seconds at 115200 baud"
echo ""
echo "To boot:"
echo "  cd $ZYNQ_DIR"
echo "  python3 scripts/auto_boot_linux.py --images boot_linux69.json"
echo ""
echo "After login, run:  /root/atomik_test"
