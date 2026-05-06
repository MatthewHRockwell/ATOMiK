#!/bin/bash
#
# inject_atomik_os.sh — rebuild the ubuntu_rv64.cpio.gz initramfs with the
# current atomik_os binary preinstalled at /root/atomik_os.
#
# History note: an earlier "concatenate" version of this script
# (gunzip base | cat - overlay.cpio | gzip) produced an archive whose
# kernel-side unpack consistently logged "Initramfs unpacking failed:
# read error" and silently truncated /root/atomik_os, breaking ld-linux
# at runtime.  Root cause: cpio newc archives must be padded to a 4-byte
# boundary AT EVERY ENTRY, and the concatenation point between the base
# archive's TRAILER!!! entry and the overlay's first header isn't always
# aligned — some kernels stop reading at the trailer, others read past
# but mis-parse the overlay.
#
# This version is the reliable approach: extract the base archive to a
# scratch directory, drop atomik_os + its support files into the right
# places, then re-pack the whole tree as a single newc cpio.  Slower
# (extra fakeroot+repack) but produces a clean archive every time.
#
# Usage:
#   bash inject_atomik_os.sh
#
# Inputs:
#   $LITEX_BUILD/ubuntu_rv64.cpio.gz.preinject  (pristine rootfs)
#   $ATOMIK_OS_BIN                              (atomik_os build artifact)
# Output:
#   $LITEX_BUILD/ubuntu_rv64.cpio.gz            (replaces previous, w/ overlay)

set -euo pipefail

LITEX_BUILD=/home/mattrock/Projects/ATOMiK/hardware/zynq/litex-build
ATOMIK_OS_BIN=/home/mattrock/Projects/ATOMiK/atomik_os/build/atomik_os
PREINJECT="$LITEX_BUILD/ubuntu_rv64.cpio.gz.preinject"
OUT="$LITEX_BUILD/ubuntu_rv64.cpio.gz"

[ -f "$PREINJECT"     ] || { echo "ERROR: $PREINJECT not found"; exit 1; }
[ -x "$ATOMIK_OS_BIN" ] || { echo "ERROR: $ATOMIK_OS_BIN not found or not executable"; exit 1; }

TMP=$(mktemp -d)
trap "sudo rm -rf $TMP 2>/dev/null || rm -rf $TMP" EXIT

ROOT="$TMP/root"
mkdir -p "$ROOT"

echo "[1/4] extracting pristine $PREINJECT → $ROOT"
# Disable pipefail for the extract: cpio may exit with a non-zero status
# when it skips files it can't fully recreate as a non-root user (device
# nodes, suid binaries) but the regular files we care about land fine.
set +o pipefail
gunzip -c "$PREINJECT" | (cd "$ROOT" && cpio -idm --no-absolute-filenames 2>&1 | tail -1)
set -o pipefail
echo "  extracted size: $(du -sh "$ROOT" | cut -f1)"

echo "[2/4] dropping overlay files into the tree"
# atomik_os binary
mkdir -p "$ROOT/root"
cp "$ATOMIK_OS_BIN" "$ROOT/root/atomik_os"
chmod 755 "$ROOT/root/atomik_os"
ATOMIK_MD5=$(md5sum "$ATOMIK_OS_BIN" | cut -d' ' -f1)
echo "  /root/atomik_os ($(stat -c%s "$ATOMIK_OS_BIN") bytes, md5 $ATOMIK_MD5)"

# /sbin/atomik_boot.sh — userspace VBUS write + atomik_os launcher
mkdir -p "$ROOT/sbin"
cat > "$ROOT/sbin/atomik_boot.sh" <<'BOOT'
#!/bin/sh
# atomik_boot.sh — wake USB VBUS + launch atomik_os.
# This rootfs has no devtmpfs (task #15 will enable it), so we mknod
# the device files we need.

sleep 6
[ -e /dev/mem ] || mknod /dev/mem c 1 1
chmod 600 /dev/mem
mkdir -p /dev/input
for f in /sys/class/input/event*/dev; do
    n=$(basename $(dirname $f))
    maj=$(cat $f | cut -d: -f1)
    min=$(cat $f | cut -d: -f2)
    [ -e /dev/input/$n ] || mknod /dev/input/$n c $maj $min
    chmod 600 /dev/input/$n
done
[ -e /dev/fb0 ] || mknod /dev/fb0 c 29 0
chmod 600 /dev/fb0

# DrvVbusExternal=1 (ULPI OTG_CTRL bit 6) so the on-board TPS2051
# 5V switch turns on. See project_usb_host_WORKING.md.
devmem 0x80002170 32 0x600A0040 2>/dev/null || true
sleep 2

exec /root/atomik_os
BOOT
chmod 755 "$ROOT/sbin/atomik_boot.sh"

# /etc/init.d/S99atomik for sysvinit, harmless on systemd
mkdir -p "$ROOT/etc/init.d"
cat > "$ROOT/etc/init.d/S99atomik" <<'SYSV'
#!/bin/sh
case "$1" in
  start) /sbin/atomik_boot.sh > /dev/tty1 2>&1 < /dev/tty1 & ;;
esac
SYSV
chmod 755 "$ROOT/etc/init.d/S99atomik"

echo "[3/4] re-packing as a single newc cpio (no concatenation)"
# `find . -depth -print0 | cpio -o -H newc` ensures every entry is
# present exactly once and the archive ends with a single TRAILER!!!.
# `--reproducible` keeps deterministic timestamps so re-runs produce
# byte-identical output when nothing has changed.
(cd "$ROOT" && find . -depth -print0 | LANG=C sort -z | \
    cpio -o -H newc --reproducible --quiet --null) | gzip -9 > "$OUT"

NEW_SIZE=$(stat -c%s "$OUT")
echo "[4/4] done"
echo "  $OUT — $NEW_SIZE bytes ($(echo "scale=1; $NEW_SIZE / 1048576" | bc) MB)"
echo
echo "To deploy:"
echo "  cd /home/mattrock/Projects/ATOMiK/hardware/zynq"
echo "  python3 ps_loader/jtag_boot.py --no-boot"
echo "  python3 /tmp/boot_v4.py    # boot trigger over UART"
