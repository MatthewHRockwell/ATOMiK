#!/bin/bash
#
# bake_overlay.sh — assemble the atomik_os initramfs overlay and concatenate it
# onto the pristine Ubuntu rootfs, producing the bootable ubuntu_rv64.cpio.gz.
#
# This is the CONCAT workflow (fast, no fakeroot repack): a gzipped newc cpio
# "overlay" is appended after the base rootfs's gzip member.  The NaxRiscv
# kernel unpacks both members, with the overlay's files winning.  Used so a
# rebuild needs only a reboot, not a 36-minute UART redeploy.
#
# WHY THIS SCRIPT EXISTS: the overlay must carry EVERYTHING atomik_os reads at
# runtime that isn't in the base rootfs — the binary, the helper tools, the AA
# font atlases, AND the Atom assistant pose assets.  An earlier hand-rolled
# /tmp recipe forgot the assets, so the assistant rendered as an empty bubble
# with no character (idle pose is the only mandatory one).  Keeping the file
# list here, sourced from the repo, prevents that regression from recurring.
#
# Usage:  bash bake_overlay.sh
# Inputs (all in the repo):
#   atomik_os/build/{atomik_os,fb2png}           atomik_os/tools/fbcrop
#   atomik_os/assets/fonts/*.atomik_font         atomik_os/assets/assistant/*.atomik_asset
#   hardware/zynq/litex-build/overlay/aos_launch.sh
#   hardware/zynq/litex-build/overlay/abench     (prebuilt parallel-bank daemon)
#   hardware/zynq/litex-build/ubuntu_rv64.cpio.gz.preaos  (pristine base)
# Output:
#   hardware/zynq/litex-build/ubuntu_rv64.cpio.gz
set -euo pipefail

REPO=/home/mattrock/Projects/ATOMiK
LB="$REPO/hardware/zynq/litex-build"
OVL="$LB/overlay"
PREAOS="$LB/ubuntu_rv64.cpio.gz.preaos"
OUT="$LB/ubuntu_rv64.cpio.gz"

[ -f "$PREAOS" ] || { echo "ERROR: $PREAOS missing"; exit 1; }

STAGE=$(mktemp -d)
trap "rm -rf $STAGE" EXIT

# --- collect overlay files into the stage root ---
cp "$REPO/atomik_os/build/atomik_os"  "$STAGE/atomik_os"
cp "$REPO/atomik_os/build/fb2png"     "$STAGE/fb2png"
cp "$REPO/atomik_os/tools/fbcrop"     "$STAGE/fbcrop"
cp "$OVL/abench"                      "$STAGE/abench"
cp "$OVL/aos_launch.sh"               "$STAGE/aos_launch.sh"
chmod 755 "$STAGE"/atomik_os "$STAGE"/fb2png "$STAGE"/fbcrop "$STAGE"/abench "$STAGE"/aos_launch.sh

mkdir -p "$STAGE/atomik_fonts"
cp "$REPO/atomik_os/assets/fonts/"*.atomik_font "$STAGE/atomik_fonts/"

# Atom assistant poses — the piece the old recipe dropped.  idle is mandatory;
# the rest (happy/alert/think/explain) are optional moods.
mkdir -p "$STAGE/atomik_assets"
cp "$REPO/atomik_os/assets/assistant/"*.atomik_asset "$STAGE/atomik_assets/"

# --- pack the overlay as a single newc cpio, gzip, concat onto the base ---
( cd "$STAGE" && find . -mindepth 1 | sed 's|^\./||' | sort \
    | cpio -o -H newc 2>/dev/null | gzip -9 ) > "$STAGE/overlay.cpio.gz"

cat "$PREAOS" "$STAGE/overlay.cpio.gz" > "$OUT"

echo "baked $OUT"
echo "  base    : $(stat -c%s "$PREAOS") bytes"
echo "  overlay : $(stat -c%s "$STAGE/overlay.cpio.gz") bytes"
echo "  total   : $(stat -c%s "$OUT") bytes"
echo "  fonts   : $(ls "$STAGE"/atomik_fonts/*.atomik_font  | wc -l)"
echo "  assets  : $(ls "$STAGE"/atomik_assets/*.atomik_asset | wc -l)   <- assistant poses"
