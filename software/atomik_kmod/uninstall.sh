#!/bin/bash
# ATOMiK Delta-State Accelerator — Uninstaller
# Usage: sudo ./uninstall.sh [--purge]
set -e

VERSION="0.5.0"
DKMS_SRC="/usr/src/atomik-${VERSION}"
PURGE=0

if [ "$(id -u)" -ne 0 ]; then
    echo "Error: must run as root (sudo ./uninstall.sh)"
    exit 1
fi

if [ "$1" = "--purge" ]; then
    PURGE=1
fi

echo "ATOMiK v${VERSION} Uninstaller"
echo "==========================="
echo

# 1. Unload the module if loaded
if lsmod | grep -q "^atomik "; then
    echo "[1/6] Unloading atomik module..."
    rmmod atomik
else
    echo "[1/6] Module not loaded, skipping"
fi

# 2. Disable and remove systemd service
echo "[2/6] Removing systemd service..."
if systemctl is-enabled atomik.service 2>/dev/null | grep -q enabled; then
    systemctl disable atomik.service
fi
rm -f /etc/systemd/system/atomik.service
systemctl daemon-reload

# 3. Remove DKMS
echo "[3/6] Removing DKMS module..."
if dkms status atomik/${VERSION} 2>/dev/null | grep -q .; then
    dkms remove atomik/${VERSION} --all
fi
rm -rf "${DKMS_SRC}"
depmod -a

# 4. Remove installed files and udev rule
echo "[4/6] Removing installed files..."
rm -f /usr/local/bin/atomik-status
rm -f /usr/local/bin/atomik-test
rm -f /usr/local/bin/atomik-bench
rm -f /usr/local/bin/atomik-full-test
rm -f /usr/local/bin/atomik-uninstall
rm -f /etc/modprobe.d/atomik.conf
rm -f /etc/udev/rules.d/99-atomik.rules
udevadm control --reload-rules 2>/dev/null || true

# 5. Remove atomik group
echo "[5/6] Removing atomik group..."
if getent group atomik >/dev/null 2>&1; then
    groupdel atomik 2>/dev/null || echo "  Warning: could not remove atomik group (users may still be members)"
fi

# 6. Purge state (optional)
if [ ${PURGE} -eq 1 ]; then
    echo "[6/6] Purging trial state (/var/lib/atomik/)..."
    rm -rf /var/lib/atomik
else
    echo "[6/6] Keeping trial state in /var/lib/atomik/ (use --purge to remove)"
fi

echo
echo "ATOMiK has been uninstalled."
