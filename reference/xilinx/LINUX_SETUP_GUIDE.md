# Linux Setup Guide -- PetaLinux, Ubuntu, and UIO for AX7020

**Date:** March 7, 2026
**Status:** Pre-board (from Xilinx documentation and PetaLinux reference)
**Target:** ALINX AX7020 (XC7Z020-2CLG400I)
**Audience:** Developers running Linux on the Zynq PS with ATOMiK in the PL

**Sources:** Xilinx UG1144 (PetaLinux Tools Documentation), UG585 (Zynq-7000 TRM), Linux kernel UIO documentation, Ubuntu ARM rootfs documentation

---

## Table of Contents

1. [Linux Options for Zynq](#1-linux-options-for-zynq)
2. [PetaLinux Project Creation](#2-petalinux-project-creation)
3. [SD Card Boot Preparation](#3-sd-card-boot-preparation)
4. [Device Tree for ATOMiK](#4-device-tree-for-atomik)
5. [UIO Setup](#5-uio-setup)
6. [Ubuntu Rootfs Installation](#6-ubuntu-rootfs-installation)
7. [Cross-Compilation Setup](#7-cross-compilation-setup)
8. [Network Setup](#8-network-setup)
9. [POST: Hardware-Validated Data](#9-post-hardware-validated-data)

---

## 1. Linux Options for Zynq

The Zynq-7020 PS (dual Cortex-A9 @ 667 MHz, 1 GB DDR3) is a full ARM application processor that runs Linux. There are several approaches to building and deploying Linux on the AX7020, each with distinct trade-offs.

### 1.1 Option Comparison

| Option | Pros | Cons | Recommended For |
|--------|------|------|-----------------|
| **PetaLinux** | AMD-supported, BSP integration, auto device tree generation, FSBL/U-Boot built-in | Large install (~50 GB), AMD-specific tooling, Ubuntu LTS host required | Initial bringup, boot chain generation, hardware validation |
| **Ubuntu Rootfs** | Familiar `apt-get`, full desktop possible, large package ecosystem, easy Python/GCC install | Larger rootfs (~500 MB+), higher memory usage, slower boot | Daily development, running ATOMiK tests and benchmarks |
| **Buildroot** | Minimal image (~20 MB), fast boot (<5s), fully customizable, no external dependencies | Manual configuration, smaller package set, more expertise needed | Embedded deployment, production appliances |
| **Mainline Linux** | Upstream kernel, community support, latest features | Manual DTS/defconfig, no AMD BSP integration, requires deep kernel knowledge | Advanced users, upstream contribution |

### 1.2 Recommended Approach

Use **PetaLinux** to generate the boot chain (FSBL, U-Boot, device tree, kernel) and then replace the rootfs with **Ubuntu** for daily development:

```
PetaLinux generates:          Ubuntu provides:
  - FSBL (zynq_fsbl.elf)       - /usr/bin/* (gcc, python3, etc.)
  - U-Boot (u-boot.elf)        - apt-get package manager
  - Device tree (system.dtb)   - SSH server
  - Linux kernel (uImage)      - Full userspace
  - BOOT.BIN                   - Familiar environment
```

This gives the best of both worlds: a correctly configured boot chain that matches the AX7020 hardware, with a full-featured userspace for development.

---

## 2. PetaLinux Project Creation

### 2.1 PetaLinux Installation

PetaLinux is a free download from AMD/Xilinx. It requires a specific Ubuntu LTS host version.

| Requirement | Specification |
|-------------|---------------|
| **Host OS** | Ubuntu 20.04 or 22.04 LTS (64-bit) |
| **Disk Space** | ~50 GB for PetaLinux tools + projects |
| **RAM** | 8 GB minimum, 16 GB recommended |
| **Download** | [AMD/Xilinx PetaLinux Downloads](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/embedded-design-tools.html) |

Install PetaLinux:

```bash
# Install prerequisites
sudo apt install -y gawk python3 python3-pip python3-pexpect \
    xz-utils debianutils iputils-ping libtinfo5 diffstat texinfo \
    gcc-multilib chrpath socat cpio xterm autoconf libtool zlib1g-dev \
    libncurses5-dev libssl-dev lz4 zstd

# Run PetaLinux installer
chmod +x petalinux-v2023.2-10121855-installer.run
./petalinux-v2023.2-10121855-installer.run -d /tools/Xilinx/PetaLinux/2023.2

# Accept license agreements when prompted
```

### 2.2 Create Project

```bash
# Source PetaLinux environment
source /tools/Xilinx/PetaLinux/2023.2/settings.sh

# Create project from Zynq template
petalinux-create --type project --template zynq --name atomik-zynq
cd atomik-zynq

# Import hardware description (XSA from Vivado block design)
# The XSA file contains the PS configuration, address map, and clock frequencies
petalinux-config --get-hw-description=../hardware/zynq/vivado/output/atomik_zynq.xsa
```

The `petalinux-config` command opens a menuconfig interface. Key settings:

| Menu Path | Setting | Value |
|-----------|---------|-------|
| Subsystem AUTO Hardware Settings -> Serial Settings | Primary UART | ps7_uart_0 |
| Subsystem AUTO Hardware Settings -> Ethernet Settings | Primary Ethernet | ps7_ethernet_0 |
| Subsystem AUTO Hardware Settings -> SD/SDIO Settings | Primary SD | ps7_sd_0 |
| Image Packaging Configuration -> Root filesystem type | SD card (ext4) | Select this |
| DTG Settings -> Kernel Bootargs | Auto-generated | Leave default |

### 2.3 Configure Kernel (Enable UIO)

ATOMiK is accessed from Linux userspace via the UIO (Userspace I/O) framework. UIO must be enabled in the kernel configuration.

```bash
# Open kernel configuration menu
petalinux-config -c kernel
```

Navigate to and enable:

```
Device Drivers --->
    Userspace I/O drivers --->
        <*> Userspace I/O platform driver with generic IRQ handling
        <*> Userspace platform driver with generic irq and target memory handling
```

This enables `CONFIG_UIO=y` and `CONFIG_UIO_PDRV_GENIRQ=y` in the kernel config.

### 2.4 Configure Root Filesystem

```bash
# Open rootfs configuration menu
petalinux-config -c rootfs
```

Enable useful packages for ATOMiK development:

```
Filesystem Packages --->
    base --->
        busybox --->
            [*] busybox
    console --->
        utils --->
            [*] bash
    devel --->
        gcc --->
            [*] gcc (if native compilation is desired)
        python3 --->
            [*] python3
    misc --->
        devmem2 --->
            [*] devmem2 (for register access testing)
```

### 2.5 Build

```bash
# Build everything (kernel, rootfs, FSBL, U-Boot, device tree)
petalinux-build

# This takes 30-60 minutes on first build (downloads and compiles the kernel)
# Subsequent builds are incremental and much faster
```

Build outputs are placed in `images/linux/`:

| File | Description |
|------|-------------|
| `zynq_fsbl.elf` | First Stage Boot Loader |
| `u-boot.elf` | U-Boot bootloader |
| `system.dtb` | Compiled device tree blob |
| `uImage` | Linux kernel (uImage format) |
| `image.ub` | FIT image (kernel + DTB + rootfs, single file) |
| `rootfs.tar.gz` | Root filesystem archive |
| `system.bit` | PL bitstream (copied from XSA) |

### 2.6 Package Boot Files

```bash
# Generate BOOT.BIN (FSBL + bitstream + U-Boot)
petalinux-package --boot \
    --fsbl images/linux/zynq_fsbl.elf \
    --fpga images/linux/system.bit \
    --u-boot \
    --force
```

This creates `images/linux/BOOT.BIN` which contains all three components in a single file that the Zynq BootROM loads from the SD card FAT32 partition.

---

## 3. SD Card Boot Preparation

### 3.1 Partition Layout

The SD card requires two partitions: a FAT32 boot partition and an ext4 root filesystem partition.

```
+---------------------------+
| Partition 1: FAT32        |  512 MB, label: BOOT
|   BOOT.BIN                |  FSBL + bitstream + U-Boot
|   image.ub                |  Kernel + DTB + initramfs (FIT image)
|                            |  OR: uImage + system.dtb (separate files)
+---------------------------+
| Partition 2: ext4         |  Remainder, label: rootfs
|   / (root filesystem)     |  Extracted rootfs.tar.gz or Ubuntu rootfs
|   /dev/uio0               |  ATOMiK UIO device (auto-created by kernel)
+---------------------------+
```

### 3.2 Create Partitions

```bash
# Identify the SD card device (CAREFUL: verify this is the SD card, not your system disk!)
lsblk

# Assume the SD card is /dev/sdX (replace X with your actual device letter)
SDCARD=/dev/sdX

# Create partition table
sudo parted $SDCARD mklabel msdos

# Create FAT32 boot partition (512 MB)
sudo parted $SDCARD mkpart primary fat32 1MiB 513MiB
sudo parted $SDCARD set 1 boot on

# Create ext4 rootfs partition (remainder)
sudo parted $SDCARD mkpart primary ext4 513MiB 100%

# Format partitions
sudo mkfs.vfat -F 32 -n BOOT ${SDCARD}1
sudo mkfs.ext4 -L rootfs ${SDCARD}2
```

### 3.3 Populate Boot Partition

```bash
# Mount boot partition
sudo mkdir -p /mnt/boot
sudo mount ${SDCARD}1 /mnt/boot

# Copy boot files (from PetaLinux build)
sudo cp images/linux/BOOT.BIN /mnt/boot/
sudo cp images/linux/image.ub /mnt/boot/

# Alternative: separate kernel and DTB (instead of image.ub)
# sudo cp images/linux/uImage /mnt/boot/
# sudo cp images/linux/system.dtb /mnt/boot/

sudo umount /mnt/boot
```

### 3.4 Populate Root Filesystem

```bash
# Mount rootfs partition
sudo mkdir -p /mnt/rootfs
sudo mount ${SDCARD}2 /mnt/rootfs

# Extract PetaLinux rootfs
sudo tar xzf images/linux/rootfs.tar.gz -C /mnt/rootfs/

# Sync and unmount
sudo sync
sudo umount /mnt/rootfs
```

### 3.5 Boot Mode DIP Switch

The AX7020 boot mode is selected by a DIP switch block that configures MIO[5:2] pin straps. For SD card boot:

| Boot Mode | DIP Switch Setting | MIO[5:2] |
|-----------|-------------------|----------|
| **JTAG** | See ALINX manual | 0x0 |
| **QSPI** | See ALINX manual | 0x1 |
| **SD Card** | See ALINX manual | 0x5 |

```
POST: Exact DIP switch positions will be documented with a photo when the
board arrives. The ALINX user manual contains the definitive settings.
```

### 3.6 First Boot

1. Insert the prepared SD card into the AX7020 MicroSD slot
2. Set the DIP switch to SD boot mode
3. Connect USB-UART cable to the debug console port
4. Open a serial terminal at 115200 baud, 8N1:

   ```bash
   picocom -b 115200 /dev/ttyUSB0
   # Or: screen /dev/ttyUSB0 115200
   # Or: minicom -D /dev/ttyUSB0 -b 115200
   ```

5. Power on the board (5V barrel jack)
6. You should see the FSBL banner, U-Boot messages, and Linux kernel boot log

```
POST: Actual boot log will be captured and included here when the board
arrives.
```

---

## 4. Device Tree for ATOMiK

The device tree tells the Linux kernel about the ATOMiK peripheral in the PL fabric. ATOMiK is mapped as a UIO device so it can be accessed directly from userspace.

### 4.1 Device Tree Overlay

The preferred method is to add the ATOMiK node in PetaLinux's `system-user.dtsi` file, which is merged with the auto-generated device tree.

Edit `project-spec/meta-user/recipes-bsp/device-tree/files/system-user.dtsi`:

```dts
/include/ "system-conf.dtsi"

/ {
    amba {
        atomik@43c00000 {
            compatible = "generic-uio";
            reg = <0x43c00000 0x1000>;
            status = "okay";
        };
    };
};
```

### 4.2 Device Tree Node Breakdown

| Property | Value | Meaning |
|----------|-------|---------|
| `compatible` | `"generic-uio"` | Use the generic UIO platform driver |
| `reg` | `<0x43c00000 0x1000>` | Base address 0x43C0_0000, 4 KB range |
| `status` | `"okay"` | Enable this device |

The address `0x43C00000` matches the M_AXI_GP0 address assignment in the Vivado block design. The 4 KB (0x1000) range covers the full ATOMiK register map with room for multi-bank addressing.

### 4.3 Kernel Command Line (Optional)

If UIO is compiled as a module and the device tree `compatible` property does not auto-bind, add the UIO driver binding on the kernel command line in U-Boot:

```
uio_pdrv_genirq.of_id=generic-uio
```

In PetaLinux, add this via:

```bash
petalinux-config
# Navigate to: DTG Settings -> Kernel Bootargs -> Add extra boot args
# Add: uio_pdrv_genirq.of_id=generic-uio
```

### 4.4 Multiple ATOMiK Instances

If running multiple ATOMiK instances (e.g., separate AXI peripherals for different bank groups), add multiple device tree nodes:

```dts
/ {
    amba {
        atomik0@43c00000 {
            compatible = "generic-uio";
            reg = <0x43c00000 0x1000>;
            status = "okay";
        };

        atomik1@43c10000 {
            compatible = "generic-uio";
            reg = <0x43c10000 0x1000>;
            status = "okay";
        };
    };
};
```

Each instance appears as a separate `/dev/uioN` device.

---

## 5. UIO Setup

### 5.1 Kernel Configuration

UIO support must be enabled in the kernel. If using PetaLinux, follow section 2.3. For manual kernel builds, ensure these options are set:

```
CONFIG_UIO=y
CONFIG_UIO_PDRV_GENIRQ=y
```

### 5.2 Verify UIO Device

After booting Linux with the ATOMiK device tree entry:

```bash
# Check that UIO device exists
ls /dev/uio*
# Expected: /dev/uio0

# View UIO device details
cat /sys/class/uio/uio0/name
# Expected: atomik

cat /sys/class/uio/uio0/maps/map0/addr
# Expected: 0x43c00000

cat /sys/class/uio/uio0/maps/map0/size
# Expected: 0x00001000

# Quick register test with devmem2 (if installed)
devmem2 0x43c0001c
# Reads the STATUS register: should return {acc_zero, n_banks[7:0], version[7:0]}
```

### 5.3 Userspace Access via mmap (C)

UIO allows userspace programs to directly map ATOMiK registers into their address space. No kernel driver is required beyond the generic UIO platform driver.

```c
/* atomik_uio.c -- ATOMiK userspace driver via UIO */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

/* ATOMiK register offsets (from register map) */
#define ATOMIK_LOAD_ADDR    0x00
#define ATOMIK_LOAD_DATA_LO 0x04
#define ATOMIK_LOAD_DATA_HI 0x08
#define ATOMIK_ACCUM_LO     0x0C
#define ATOMIK_ACCUM_HI     0x10
#define ATOMIK_STATE_LO     0x14
#define ATOMIK_STATE_HI     0x18
#define ATOMIK_STATUS       0x1C
#define ATOMIK_SWAP_ADDR    0x20
#define ATOMIK_CONFIG       0x24

/* Register access helpers */
static volatile uint32_t *atomik_base;

static inline void atomik_write(uint32_t offset, uint32_t value) {
    atomik_base[offset / 4] = value;
}

static inline uint32_t atomik_read(uint32_t offset) {
    return atomik_base[offset / 4];
}

int main(int argc, char *argv[])
{
    int fd;
    const size_t map_size = 0x1000;  /* 4 KB */

    /* Open UIO device */
    fd = open("/dev/uio0", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/uio0");
        return 1;
    }

    /* Map ATOMiK registers into userspace */
    atomik_base = (volatile uint32_t *)mmap(
        NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0
    );
    if (atomik_base == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    /* Read STATUS register */
    uint32_t status = atomik_read(ATOMIK_STATUS);
    uint8_t version = status & 0xFF;
    uint8_t n_banks = (status >> 8) & 0xFF;
    uint8_t acc_zero = (status >> 16) & 0x01;
    printf("ATOMiK Status: version=%u, banks=%u, acc_zero=%u\n",
           version, n_banks, acc_zero);

    /* Load initial state: address 0, value 0xDEADBEEFCAFEBABE */
    atomik_write(ATOMIK_LOAD_ADDR, 0);
    atomik_write(ATOMIK_LOAD_DATA_LO, 0xCAFEBABE);
    atomik_write(ATOMIK_LOAD_DATA_HI, 0xDEADBEEF);  /* triggers LOAD */

    /* Accumulate delta: 0x00000001_00000001 */
    atomik_write(ATOMIK_ACCUM_LO, 0x00000001);
    atomik_write(ATOMIK_ACCUM_HI, 0x00000001);  /* triggers ACCUM */

    /* Read reconstructed state */
    uint32_t state_lo = atomik_read(ATOMIK_STATE_LO);
    uint32_t state_hi = atomik_read(ATOMIK_STATE_HI);
    printf("State: 0x%08X_%08X\n", state_hi, state_lo);
    /* Expected: 0xDEADBEEE_CAFEBABF (initial XOR delta) */

    /* Cleanup */
    munmap((void *)atomik_base, map_size);
    close(fd);

    return 0;
}
```

### 5.4 Compile and Run the UIO Test

On the target (if native GCC is installed):

```bash
gcc -o atomik_uio atomik_uio.c -O2
sudo ./atomik_uio
```

Or cross-compile on the host and copy:

```bash
arm-linux-gnueabihf-gcc -o atomik_uio atomik_uio.c -O2
scp atomik_uio root@192.168.1.100:/home/root/
ssh root@192.168.1.100 ./atomik_uio
```

### 5.5 UIO Permissions

By default, `/dev/uio0` requires root access. To allow non-root access, create a udev rule on the target:

```bash
# /etc/udev/rules.d/99-atomik-uio.rules
SUBSYSTEM=="uio", ATTR{name}=="atomik", MODE="0666"
```

Reload udev:

```bash
sudo udevadm control --reload-rules && sudo udevadm trigger
```

### 5.6 UIO vs devmem2 vs Custom Kernel Driver

| Method | Pros | Cons | Use Case |
|--------|------|------|----------|
| **UIO + mmap** | Fast, no kernel code, standard API | Requires device tree entry, no DMA | **Recommended for ATOMiK** |
| **devmem2** | Quick testing, no setup | Slow (one register per invocation), root only | Debug and validation |
| **Custom kernel driver** | Full kernel integration, interrupts, DMA, sysfs | Complex development, kernel dependency | Production with IRQ or DMA |

For ATOMiK, UIO is the recommended approach. The register interface is simple (read/write 32-bit registers), does not require DMA, and UIO provides the lowest-latency userspace access path.

---

## 6. Ubuntu Rootfs Installation

For daily development, an Ubuntu rootfs provides a familiar environment with `apt-get`, Python, GCC, and SSH. This section describes how to replace the PetaLinux-generated rootfs with Ubuntu while keeping the PetaLinux boot chain.

### 6.1 Download Ubuntu ARM Rootfs

```bash
# Download Ubuntu 22.04 ARM (armhf) base rootfs
wget http://cdimage.ubuntu.com/ubuntu-base/releases/22.04/release/ubuntu-base-22.04-base-armhf.tar.gz
```

The base rootfs is approximately 30 MB compressed and 150 MB extracted. It provides a minimal Ubuntu userspace without a desktop environment.

### 6.2 Extract to SD Card

```bash
# Mount the ext4 partition of the SD card
sudo mount ${SDCARD}2 /mnt/rootfs

# Remove existing rootfs (PetaLinux rootfs)
sudo rm -rf /mnt/rootfs/*

# Extract Ubuntu rootfs
sudo tar xzf ubuntu-base-22.04-base-armhf.tar.gz -C /mnt/rootfs/
```

### 6.3 Configure via chroot

Use QEMU user-mode emulation to chroot into the ARM rootfs from the x86 host:

```bash
# Install QEMU static binary (if not already installed)
sudo apt install qemu-user-static

# Copy QEMU ARM static binary into the rootfs
sudo cp /usr/bin/qemu-arm-static /mnt/rootfs/usr/bin/

# Mount pseudo-filesystems for chroot
sudo mount --bind /dev /mnt/rootfs/dev
sudo mount --bind /dev/pts /mnt/rootfs/dev/pts
sudo mount --bind /proc /mnt/rootfs/proc
sudo mount --bind /sys /mnt/rootfs/sys

# Set up DNS resolution
sudo cp /etc/resolv.conf /mnt/rootfs/etc/resolv.conf

# Enter chroot
sudo chroot /mnt/rootfs /bin/bash
```

Inside the chroot:

```bash
# Update package lists
apt-get update

# Install essential packages
apt-get install -y \
    openssh-server \
    network-manager \
    gcc \
    g++ \
    make \
    python3 \
    python3-pip \
    git \
    wget \
    curl \
    htop \
    nano \
    vim \
    bash-completion \
    sudo \
    udev \
    systemd \
    iproute2 \
    iputils-ping \
    net-tools \
    devmem2

# Set root password
passwd root
# Enter password when prompted

# Create a non-root user (optional)
useradd -m -s /bin/bash atomik
passwd atomik
usermod -aG sudo atomik

# Configure hostname
echo "atomik-zynq" > /etc/hostname

# Configure serial console (for UART login)
# Create /etc/init/ttyPS0.conf or systemd service:
cat > /etc/systemd/system/serial-console@ttyPS0.service << 'SVCEOF'
[Unit]
Description=Serial Console on %I
After=systemd-user-sessions.service

[Service]
ExecStart=-/sbin/agetty -L 115200 %I vt100
Type=idle
Restart=always

[Install]
WantedBy=multi-user.target
SVCEOF

systemctl enable serial-console@ttyPS0.service

# Enable SSH
systemctl enable ssh

# Configure fstab
cat > /etc/fstab << 'FSTABEOF'
# <device>       <mount>  <type>  <options>         <dump> <pass>
/dev/mmcblk0p2   /        ext4    errors=remount-ro 0      1
/dev/mmcblk0p1   /boot    vfat    defaults          0      2
FSTABEOF

# Exit chroot
exit
```

### 6.4 Cleanup chroot

```bash
# Unmount pseudo-filesystems
sudo umount /mnt/rootfs/sys
sudo umount /mnt/rootfs/proc
sudo umount /mnt/rootfs/dev/pts
sudo umount /mnt/rootfs/dev

# Remove QEMU binary (optional, saves space)
sudo rm /mnt/rootfs/usr/bin/qemu-arm-static

# Unmount rootfs
sudo sync
sudo umount /mnt/rootfs
```

### 6.5 Boot with Ubuntu Rootfs

The boot chain remains unchanged (PetaLinux BOOT.BIN on FAT32 partition). The kernel boots and mounts the Ubuntu ext4 partition as root. You should see a login prompt on the serial console.

```bash
# Connect via serial
picocom -b 115200 /dev/ttyUSB0

# Login as root or atomik user
# Then verify:
uname -a
# Expected: Linux atomik-zynq 5.15.x-xilinx ... armv7l GNU/Linux

cat /etc/os-release
# Expected: Ubuntu 22.04 LTS
```

---

## 7. Cross-Compilation Setup

### 7.1 Install Cross-Compiler on Host

```bash
# Install the ARM hard-float cross-compiler
sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# Verify
arm-linux-gnueabihf-gcc --version
```

### 7.2 Cross-Compile ATOMiK Test Program

```bash
# Compile the UIO test program (from section 5.3)
arm-linux-gnueabihf-gcc -o atomik_uio atomik_uio.c -O2 -Wall

# Verify the binary is ARM:
file atomik_uio
# Expected: ELF 32-bit LSB executable, ARM, EABI5, ... dynamically linked ...
```

### 7.3 Transfer to Target

```bash
# Via SCP (over Ethernet)
scp atomik_uio root@192.168.1.100:/home/root/

# Via SD card (sneakernet)
sudo mount ${SDCARD}2 /mnt/rootfs
sudo cp atomik_uio /mnt/rootfs/home/root/
sudo umount /mnt/rootfs
```

### 7.4 Cross-Compilation with Libraries

If the ATOMiK userspace library grows into a shared library (`libatomik.so`):

```bash
# Compile with library
arm-linux-gnueabihf-gcc -o atomik_test atomik_test.c -latomik -L./lib -I./include -O2

# Or static linking (no .so needed on target)
arm-linux-gnueabihf-gcc -o atomik_test atomik_test.c -static -latomik -L./lib -I./include -O2
```

### 7.5 Makefile for Cross-Compilation

```makefile
# ==============================================================================
# ATOMiK Zynq Userspace Application Makefile
# ==============================================================================

CROSS_COMPILE ?= arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc
CFLAGS = -O2 -Wall -Wextra -std=c11
LDFLAGS =

# Target device
TARGET_HOST ?= root@192.168.1.100
TARGET_DIR ?= /home/root

SRCS = $(wildcard *.c)
BINS = $(SRCS:.c=)

.PHONY: all deploy clean

all: $(BINS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

deploy: $(BINS)
	scp $(BINS) $(TARGET_HOST):$(TARGET_DIR)/

clean:
	rm -f $(BINS)
```

Usage:

```bash
make                          # Cross-compile all .c files
make deploy                   # SCP binaries to target
make TARGET_HOST=atomik@192.168.1.100 deploy  # Deploy as non-root user
```

---

## 8. Network Setup

The AX7020 has Gigabit Ethernet via the KSZ9031 PHY connected to PS GEM0. This provides high-bandwidth connectivity for SSH, file transfer, and remote ATOMiK testing.

### 8.1 DHCP (Default)

If a DHCP server is available on the network, Ethernet should auto-configure on boot:

```bash
# Check network interface
ip addr show eth0

# If not configured, request DHCP
dhclient eth0
# Or with NetworkManager:
nmcli device connect eth0
```

### 8.2 Static IP

For a direct connection between the development host and the AX7020:

**On the AX7020 (target)**:

```bash
# Set static IP
ip addr add 192.168.1.100/24 dev eth0
ip link set eth0 up

# Or make it persistent (NetworkManager)
nmcli con add type ethernet ifname eth0 con-name atomik-static \
    ip4 192.168.1.100/24 gw4 192.168.1.1
nmcli con up atomik-static

# Or via /etc/network/interfaces (if not using NetworkManager)
cat >> /etc/network/interfaces << 'EOF'
auto eth0
iface eth0 inet static
    address 192.168.1.100
    netmask 255.255.255.0
    gateway 192.168.1.1
EOF
```

**On the development host**:

```bash
# Configure host Ethernet for direct connection
sudo ip addr add 192.168.1.1/24 dev enp3s0  # Replace enp3s0 with your interface
```

### 8.3 SSH Access

```bash
# Enable SSH on the target (if not already done during rootfs setup)
sudo systemctl enable ssh
sudo systemctl start ssh

# From the host, connect via SSH
ssh root@192.168.1.100
# Or: ssh atomik@192.168.1.100

# Set up SSH key for passwordless login
ssh-copy-id root@192.168.1.100
```

### 8.4 File Transfer

```bash
# SCP: copy a single file
scp atomik_test root@192.168.1.100:/home/root/

# SCP: copy a directory
scp -r test_suite/ root@192.168.1.100:/home/root/

# rsync: efficient incremental sync (preferred for repeated transfers)
rsync -avz --progress test_suite/ root@192.168.1.100:/home/root/test_suite/

# rsync: sync back results from target to host
rsync -avz root@192.168.1.100:/home/root/results/ ./results/
```

### 8.5 NFS Mount (Optional, for Development)

For rapid iteration, mount a host directory on the target via NFS so edits on the host are immediately visible on the target:

**On the host**:

```bash
# Install NFS server
sudo apt install nfs-kernel-server

# Export the ATOMiK project directory
echo "/home/mattrock/Projects/ATOMiK 192.168.1.0/24(rw,sync,no_subtree_check,no_root_squash)" \
    | sudo tee -a /etc/exports
sudo exportfs -ra
```

**On the target**:

```bash
# Install NFS client
apt-get install nfs-common

# Mount host directory
mkdir -p /mnt/atomik
mount -t nfs 192.168.1.1:/home/mattrock/Projects/ATOMiK /mnt/atomik

# Or add to /etc/fstab for persistent mount:
echo "192.168.1.1:/home/mattrock/Projects/ATOMiK /mnt/atomik nfs defaults 0 0" >> /etc/fstab
```

### 8.6 Serial Console (UART)

The USB-UART debug console remains available regardless of Ethernet status:

| Parameter | Value |
|-----------|-------|
| **Baud Rate** | 115200 |
| **Data Bits** | 8 |
| **Parity** | None |
| **Stop Bits** | 1 |
| **Flow Control** | None |
| **Host Device** | `/dev/ttyUSB0` (may vary) |
| **Zynq UART** | PS UART0 (MIO 46-47, CP2102) |

```bash
# Connect to serial console
picocom -b 115200 /dev/ttyUSB0
```

The serial console provides access even when Ethernet is unconfigured or the network stack is not yet running (early boot, kernel panics, U-Boot shell).

---

## 9. POST: Hardware-Validated Data

The following sections contain placeholder data that will be updated when the AX7020 board arrives and Linux is booted successfully.

### 9.1 Actual Boot Log

```
POST: Capture the full boot log from FSBL through Linux login prompt.

Expected boot sequence:
  1. Xilinx FSBL banner (zynq_fsbl.elf)
  2. PL configuration (bitstream loaded)
  3. U-Boot banner (u-boot.elf)
  4. Linux kernel boot messages
  5. UIO device creation (/dev/uio0)
  6. Login prompt

Actual boot log:
_______________________________________________________________
_______________________________________________________________
_______________________________________________________________
```

### 9.2 Boot Time Measurement

```
POST: Measure time from power-on to login prompt.

Expected:
  - FSBL + PL config: ~2-3 seconds
  - U-Boot: ~2-3 seconds
  - Kernel + rootfs mount: ~5-10 seconds (PetaLinux) or ~15-30 seconds (Ubuntu)
  - Total: ~10-40 seconds depending on rootfs

Actual:
  - FSBL + PL config: _____ seconds
  - U-Boot: _____ seconds
  - Kernel boot: _____ seconds
  - Total to login prompt: _____ seconds
```

### 9.3 PetaLinux Version-Specific Notes

```
POST: Document any PetaLinux-specific issues or workarounds.

PetaLinux version used: _____
Kernel version: _____
U-Boot version: _____
Any patches required: _____
Any BSP compatibility notes: _____
```

### 9.4 AX7020 DIP Switch Configuration

```
POST: Photograph DIP switch block and document switch positions for each boot mode.

SD Boot:    SW[4:1] = ____
QSPI Boot:  SW[4:1] = ____
JTAG Boot:  SW[4:1] = ____

Photo: (to be added)
```

### 9.5 Serial Console Port Identification

```
POST: Document which USB port / ttyUSB device is the debug UART.

On AX7020:
  - USB-JTAG port: _____ (ttyUSB___)
  - USB-UART port: _____ (ttyUSB___)
  - Default baud rate confirmed: _____

On host (after connecting both USB cables):
  - ls /dev/ttyUSB* output: _____
  - dmesg identification: _____
```

### 9.6 ATOMiK UIO Device Verification

```
POST: Verify UIO device creation and register access on real hardware.

UIO device created: [ ] yes  [ ] no
  /dev/uio0 present: _____
  /sys/class/uio/uio0/name: _____
  /sys/class/uio/uio0/maps/map0/addr: _____
  /sys/class/uio/uio0/maps/map0/size: _____

Register read test:
  devmem2 0x43c0001c result: 0x_____
  Decoded: version=_____, banks=_____, acc_zero=_____

UIO mmap test (atomik_uio.c):
  Status register read: _____
  Load/accumulate/read: PASS / FAIL
```

### 9.7 Ethernet Connectivity

```
POST: Verify GigE link and bandwidth.

Link status: _____
Speed negotiated: _____
IP assignment (DHCP): _____
SSH login: [ ] works  [ ] fails
SCP transfer rate: _____ MB/s
iperf3 bandwidth: _____ Mbps
```

---

## Appendix A: Troubleshooting

### A.1 Common Boot Issues

| Symptom | Possible Cause | Solution |
|---------|---------------|----------|
| No output on serial console | Wrong USB port, wrong baud rate, DIP switch not set to SD | Try other ttyUSB devices, verify 115200 baud, check DIP switch |
| FSBL banner appears, then hangs | Corrupted bitstream or DDR3 config mismatch | Re-build BOOT.BIN, verify DDR3 parameters match AX7020 board |
| U-Boot appears, kernel does not boot | Missing image.ub on FAT32 partition | Verify `image.ub` (or `uImage` + `system.dtb`) on boot partition |
| Kernel boots, no login prompt | Serial console not configured in rootfs | Add `ttyPS0` getty service (see section 6.3) |
| `/dev/uio0` does not appear | Device tree missing ATOMiK node, or UIO not enabled in kernel | Check `system-user.dtsi`, verify `CONFIG_UIO=y` |
| `devmem2` returns bus error | ATOMiK not implemented in PL, or wrong address | Verify PL bitstream is loaded, check address in Vivado address editor |
| Ethernet not working | PHY not initialized, wrong MIO config | Verify PS GEM0 MIO assignments match AX7020 schematic |

### A.2 Useful Debug Commands

```bash
# Check kernel messages (including UIO registration)
dmesg | grep -i uio

# Check if PL is configured
cat /sys/class/fpga_manager/fpga0/state
# Expected: "operating"

# Check device tree entries
ls /proc/device-tree/amba/
# Should show atomik@43c00000 entry

# Check PS clock frequencies
cat /sys/kernel/debug/clk/clk_summary 2>/dev/null || \
cat /proc/device-tree/clocks/*/clock-frequency 2>/dev/null

# Monitor CPU temperature (via XADC)
cat /sys/bus/iio/devices/iio:device0/in_temp0_raw 2>/dev/null
```

---

## Appendix B: Comparison to Tang Nano 9K Software Stack

This table summarizes the software stack differences between the current Tang Nano 9K deployment and the planned AX7020 deployment.

| Layer | Tang Nano 9K | ALINX AX7020 |
|-------|-------------|--------------|
| **OS** | Bare-metal (no OS) | Linux (Ubuntu 22.04) |
| **CPU** | PicoRV32 (soft, 25.2 MHz) | Cortex-A9 (hard, 667 MHz) |
| **ATOMiK Access** | Direct MMIO (`volatile uint32_t *`) | UIO mmap (`/dev/uio0`) |
| **Toolchain** | `riscv64-unknown-elf-gcc` (cross) | `arm-linux-gnueabihf-gcc` (cross) or native GCC |
| **Deployment** | SPI flash via ISP programmer | SD card or QSPI flash |
| **Debugging** | UART printf, LED toggle | UART, SSH, GDB, ILA, printk |
| **Networking** | None | GigE Ethernet (SSH, SCP, NFS) |
| **File System** | None | ext4 on SD card |
| **Package Manager** | None | apt-get |
| **Python** | None | Python 3.x via apt |
| **Boot Time** | Instant (~1 ms) | ~10-40 seconds (Linux) |
| **Memory** | 8 KB SRAM | 1 GB DDR3 |
| **Multi-threading** | None (single-core, no OS) | Linux threads, dual-core SMP |

The core ATOMiK register access pattern is identical on both platforms. The `atomik_write()` / `atomik_read()` functions map directly to the same register offsets. The difference is only in how the base address is obtained (direct pointer on bare-metal vs. mmap on Linux).

---

## Appendix C: Revision History

| Date | Change |
|------|--------|
| 2026-03-07 | Initial version. Pre-board reference from PetaLinux documentation and Zynq Linux guides. |

---

*References: Xilinx UG1144 (PetaLinux Tools Documentation Reference Guide), UG585 (Zynq-7000 SoC Technical Reference Manual), Linux kernel UIO documentation, Ubuntu ARM rootfs documentation, ALINX AX7020 User Manual*
