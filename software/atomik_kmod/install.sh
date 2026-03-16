#!/bin/bash
# ATOMiK Delta-State Accelerator — Installer
# Usage: sudo ./install.sh [--license ATOMIK-XXXX-XXXX-XXXX-XXXX-XXXX]
set -e

VERSION="0.4.0"
DKMS_SRC="/usr/src/atomik-${VERSION}"
MIN_KERNEL="5.15"

if [ "$(id -u)" -ne 0 ]; then
    echo "Error: must run as root (sudo ./install.sh)"
    exit 1
fi

LICENSE_KEY=""
while [ $# -gt 0 ]; do
    case "$1" in
        --license) LICENSE_KEY="$2"; shift 2 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

echo "ATOMiK v${VERSION} Installer"
echo "========================="
echo

# ─── Pre-flight checks ───────────────────────────────────────────────

echo "[pre-flight] Checking system compatibility..."

# Kernel version
KVER=$(uname -r | grep -oE '^[0-9]+\.[0-9]+')
KVER_MAJ=$(echo "$KVER" | cut -d. -f1)
KVER_MIN=$(echo "$KVER" | cut -d. -f2)
MIN_MAJ=$(echo "$MIN_KERNEL" | cut -d. -f1)
MIN_MIN=$(echo "$MIN_KERNEL" | cut -d. -f2)

if [ "$KVER_MAJ" -lt "$MIN_MAJ" ] || { [ "$KVER_MAJ" -eq "$MIN_MAJ" ] && [ "$KVER_MIN" -lt "$MIN_MIN" ]; }; then
    echo "  ERROR: Kernel $(uname -r) is too old. ATOMiK requires >= ${MIN_KERNEL}."
    exit 1
fi
echo "  Kernel $(uname -r) ... OK"

# Kernel headers
KDIR="/lib/modules/$(uname -r)/build"
if [ ! -d "$KDIR" ]; then
    echo "  ERROR: Kernel headers not found at ${KDIR}"
    echo "  Install with: sudo apt install linux-headers-$(uname -r)"
    exit 1
fi
echo "  Kernel headers ... OK"

# Required tools
MISSING=""
for cmd in make gcc dkms modprobe depmod; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        MISSING="$MISSING $cmd"
    fi
done
if [ -n "$MISSING" ]; then
    echo "  ERROR: Missing required tools:${MISSING}"
    echo "  Install with: sudo apt install build-essential dkms kmod"
    exit 1
fi
echo "  Build tools ... OK"

echo

# ─── Step 1: Build tools ─────────────────────────────────────────────

echo "[1/7] Building userspace tools..."
make -C tools clean all

# ─── Step 2: Install tools ───────────────────────────────────────────

echo "[2/7] Installing tools to /usr/local/bin/..."
install -m 755 tools/atomik-status /usr/local/bin/
install -m 755 tools/atomik-test /usr/local/bin/
install -m 755 tools/atomik-bench /usr/local/bin/
install -m 755 tools/atomik-full-test.sh /usr/local/bin/atomik-full-test
install -m 755 uninstall.sh /usr/local/bin/atomik-uninstall

# ─── Step 3: Install DKMS source ─────────────────────────────────────

echo "[3/7] Setting up DKMS..."
if dkms status atomik/${VERSION} 2>/dev/null | grep -q installed; then
    echo "  Removing previous DKMS install..."
    dkms remove atomik/${VERSION} --all 2>/dev/null || true
fi
rm -rf "${DKMS_SRC}"
mkdir -p "${DKMS_SRC}"

# Copy only kernel module source — NO keygen, NO internal tools
cp Kbuild Makefile dkms.conf \
   atomik_main.c atomik_chardev.c atomik_fingerprint.c \
   atomik_sysfs.c atomik_hw.c atomik_license.c \
   atomik_core_kern.h atomik_stats.h \
   "${DKMS_SRC}/"
cp -r include/ "${DKMS_SRC}/"

dkms add atomik/${VERSION}
dkms build atomik/${VERSION}
dkms install atomik/${VERSION}

# ─── Step 4: Install modprobe config ─────────────────────────────────

echo "[4/7] Installing modprobe config..."
mkdir -p /etc/modprobe.d
if [ -n "${LICENSE_KEY}" ]; then
    printf 'options atomik license_key=%s\n' "${LICENSE_KEY}" > /etc/modprobe.d/atomik.conf
    chmod 600 /etc/modprobe.d/atomik.conf
else
    cp systemd/atomik.conf /etc/modprobe.d/atomik.conf
fi

# ─── Step 5: Create group and udev rule ──────────────────────────────

echo "[5/7] Setting up device access..."
if ! getent group atomik >/dev/null 2>&1; then
    groupadd atomik
fi
if [ -n "${SUDO_USER}" ]; then
    usermod -aG atomik "${SUDO_USER}"
    echo "  Added user '${SUDO_USER}' to 'atomik' group (re-login to take effect)"
fi

install -m 644 systemd/99-atomik.rules /etc/udev/rules.d/
udevadm control --reload-rules

# ─── Step 6: Install systemd service ─────────────────────────────────

echo "[6/7] Installing systemd service..."
cp systemd/atomik.service /etc/systemd/system/
systemctl daemon-reload
systemctl enable atomik.service

# Create trial state directory
mkdir -p /var/lib/atomik
chmod 700 /var/lib/atomik

# ─── Step 7: Load and verify ─────────────────────────────────────────

echo "[7/7] Loading ATOMiK module..."
modprobe atomik
echo
atomik-status
echo
echo "Installation complete. ATOMiK will start automatically on boot."
echo "Run 'atomik-test' to verify the installation."
echo "Run 'atomik-bench' to benchmark your system."
echo "To uninstall: sudo atomik-uninstall"
