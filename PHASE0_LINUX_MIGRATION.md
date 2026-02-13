# ATOMiK v2 — Phase 0: Linux Migration & Environment Setup

**Document Version:** 1.1
**Date:** February 12, 2026
**Author:** Matt Rockwell + Claude (Planning Partner)
**Status:** COMPLETE

**Parent Document:** `ATOMIK_V2_ROADMAP.md`

---

## Pre-Migration Status (COMPLETED)

The following have been completed and verified:

- ✅ ATOMiK monorepo backed up to 32 GB flash drive (full repo including `.git/` history)
- ✅ Other projects, documents, photos backed up
- ✅ `.gitconfig` generated and saved to flash drive
- ✅ Browser bookmarks exported
- ✅ API keys backed up
- ✅ GitHub is up to date — `github.com/MatthewHRockwell/ATOMiK` is current
- ✅ Device encryption is OFF and decryption complete
- ✅ Ubuntu 24.04.3 LTS ISO flashed to 8 GB USB drive
- ✅ Git uses HTTPS (not SSH) — no SSH keys needed

---

## Hardware Profile

| Spec | Value |
|------|-------|
| **Laptop** | Lenovo IdeaPad 3 14ALC6 (System: 82KT) |
| **Processor** | AMD Ryzen 7 5700U, 8 cores / 16 threads, 1.8 GHz |
| **RAM** | 8 GB (DDR4) |
| **GPU** | AMD Radeon integrated (Lucienne/Cezanne APU) |
| **SSD** | WDC PC SN530 512 GB NVMe |
| **BIOS** | LENOVO GLCN68WW (2025-03-19), UEFI mode |
| **Secure Boot** | ON (disable before install, optionally re-enable after) |
| **OS** | Windows 11 Home, Build 26200 — will be ERASED |

**GPU:** The `amdgpu` kernel driver is built into Linux — NO proprietary driver installation needed.

**RAM (8 GB):** Tight but workable. Ubuntu GNOME idles at ~1.5-2 GB vs Windows 11 at ~3-4 GB, so you gain immediate headroom. We will configure a generous swap file (8 GB) to compensate. Avoid running Gowin EDA + VS Code + Chrome simultaneously if possible; use Firefox or a lighter browser if memory pressure becomes an issue.

---

## Step 1: BIOS/UEFI Settings

1. Shut down your laptop completely (not restart — full shutdown)
2. Press the **Novo button** (tiny button/hole next to the power button on the left side of the laptop) with a paperclip or SIM tool while the laptop is OFF
   - This opens the Novo Button Menu
   - Alternatively: Power on and rapidly press **F2** to enter BIOS Setup
   - For one-time boot menu: Power on and rapidly press **F12**
3. In BIOS Setup, make the following changes:

   **Security tab:**
   - **Secure Boot → Disabled** (temporarily — can re-enable after Ubuntu installation)

   **Boot tab:**
   - **Boot Mode: UEFI** (should already be set — do NOT change to Legacy)
   - **Boot Priority / Boot Order:** Move USB device to first position
   - OR: Just use F12 one-time boot menu when ready to install

   **Configuration tab (if present):**
   - **AMD-V: Enabled** (should already be enabled)

4. **DO NOT** disable "Kernel DMA Protection" — fine to leave on
5. **Fast Boot:** If you see a "Fast Boot" or "Quick Boot" option, disable it
6. Save and exit (usually F10)

---

## Step 2: Install Ubuntu 24.04 LTS

1. Insert the Ubuntu installer USB drive
2. Boot from USB (use F12 for one-time boot menu, or rely on boot order you set)
3. Select **"Try or Install Ubuntu"**
   - If you get graphics issues, try **"Ubuntu (safe graphics)"** instead
4. Once the live desktop loads, the installer should auto-launch
5. Follow the installer:
   - **Language:** English (or your preference)
   - **Keyboard:** Your layout
   - **Network:** Connect to WiFi if available (recommended for downloading updates during install)
   - **Installation type:** Select **"Erase disk and install Ubuntu"**
     - If you want full-disk encryption: Click "Advanced features" and select LVM with encryption
     - **WARNING:** This will erase Windows completely. Your backup is on the 32 GB flash drive.
   - **Do NOT select "Install third-party software for graphics and Wi-Fi hardware"** during initial install if you plan to re-enable Secure Boot later (this can conflict). You can install these drivers after.
   - **Timezone:** America/Los_Angeles
   - **User account:** Set your username and password
6. Click **Install** and wait (typically 10-20 minutes)
7. When complete, remove the USB drive and reboot

---

## Step 3: First Boot — System Update

```bash
sudo apt update && sudo apt upgrade -y
sudo reboot
```

---

## Step 4: Install Essential System Packages

```bash
# Build essentials
sudo apt install -y build-essential git curl wget cmake pkg-config \
  gcc g++ make autoconf automake libtool

# Python
sudo apt install -y python3 python3-pip python3-venv python3-dev

# Node.js (for various tooling)
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt install -y nodejs

# Serial port tools
sudo apt install -y screen minicom picocom

# USB/FTDI libraries (critical for FPGA programming)
sudo apt install -y libftdi1-2 libftdi1-dev libhidapi-hidraw0 \
  libhidapi-dev libudev-dev zlib1g-dev libusb-1.0-0-dev

# Misc utilities
sudo apt install -y tree htop net-tools unzip p7zip-full jq
```

---

## Step 5: Verify GPU Drivers

**No action required.** The `amdgpu` driver is built into the Linux kernel and works out of the box with your Ryzen 7 5700U's integrated Radeon graphics.

```bash
# Verify GPU is detected and using the correct driver:
lspci | grep -i vga
# Expected: "Advanced Micro Devices, Inc. [AMD/ATI] Lucienne"

# Check driver in use:
lspci -k | grep -A 3 -i vga
# Should show: "Kernel driver in use: amdgpu"

# Optional: hardware video acceleration for browser performance
sudo apt install -y mesa-va-drivers mesa-vdpau-drivers
```

---

## Step 6: Configure Swap File (Important for 8 GB RAM)

```bash
# Check if Ubuntu installer already created swap:
swapon --show

# If no swap exists, or it's less than 8 GB, create one:
sudo fallocate -l 8G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile

# Make it permanent:
echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab

# Set swappiness lower (prioritize RAM, use swap as safety net):
echo 'vm.swappiness=10' | sudo tee -a /etc/sysctl.conf
sudo sysctl -p

# Verify:
free -h
# Should show ~8 GB swap
```

---

## Step 7: Restore Your Data

```bash
# Create your project directory
mkdir -p ~/Projects

# Mount your thumb drive (usually auto-mounted, but if not):
lsblk
# It will typically auto-mount to /media/your-username/DRIVE_LABEL

# Copy ATOMiK repo
cp -r /media/$USER/YOUR_DRIVE_LABEL/ATOMiK ~/Projects/ATOMiK

# Verify Git history survived
cd ~/Projects/ATOMiK
git log --oneline -5
git status

# Restore Git config
cp /media/$USER/YOUR_DRIVE_LABEL/.gitconfig ~/.gitconfig

# Verify Git remote (should be HTTPS)
cd ~/Projects/ATOMiK
git remote -v
# Should show: origin https://github.com/MatthewHRockwell/ATOMiK.git
```

---

## Step 8: Install Gowin EDA IDE

### 8.1 Download

1. Go to: https://www.gowinsemi.com/en/support/home/
2. Register/login for a Gowin account
3. Download: **Gowin EDA** for Linux (x86_64)
   - **Recommendation:** Use the Educational Edition immediately (it works for Tang Nano 9K / GW1NR-9 development). Apply for a Standard license simultaneously.
4. The download will be a `.tar.gz` file

### 8.2 Install

```bash
# Create installation directory
sudo mkdir -p /opt/gowin

# Extract (replace filename with actual downloaded file)
cd ~/Downloads
sudo tar -xzf Gowin_V1.9.x.x_linux.tar.gz -C /opt/gowin

# Make the IDE executable
sudo chmod +x /opt/gowin/IDE/bin/gw_ide
sudo chmod +x /opt/gowin/Programmer/bin/programmer

# Test launch
/opt/gowin/IDE/bin/gw_ide
```

### 8.3 Configure License

**Current status:** Educational Edition available now. Standard Linux EDA license application submitted, awaiting response.

```bash
# Option A: Educational Edition — no license file needed, works immediately

# Option B: When Standard license arrives
# Find your MAC address:
ip link show | grep ether

# Place the license file:
sudo cp ~/Downloads/gowin.lic /opt/gowin/IDE/bin/gowin.lic

# NOTE: Gowin licenses are tied to MAC address. Same laptop = same MAC = license works.
```

### 8.4 Troubleshooting (Known Linux Issues)

```bash
# Issue: "symbol lookup error: libfontconfig.so.1: undefined symbol: FT_Done_MM_Var"
# Fix: Remove or rename the bundled freetype library
sudo mv /opt/gowin/IDE/lib/libfreetype.so.6 /opt/gowin/IDE/lib/libfreetype.so.6.bak

# Issue: Qt platform plugin errors
# Fix: Set environment variables
echo 'export QT_QPA_PLATFORM_PLUGIN_PATH=/opt/gowin/IDE/lib/plugins' >> ~/.bashrc

# Issue: "Cable open failed" when programming
# Fix: See FTDI/udev rules in Step 9.2

# Create convenience aliases
echo 'alias gowin="/opt/gowin/IDE/bin/gw_ide"' >> ~/.bashrc
echo 'alias gowin-pgm="/opt/gowin/Programmer/bin/programmer"' >> ~/.bashrc
source ~/.bashrc
```

---

## Step 9: Install openFPGALoader

openFPGALoader is the recommended way to program Tang Nano 9K from Linux — more reliable than Gowin Programmer on Linux.

### 9.1 Build and Install

```bash
# Install build dependencies
sudo apt install -y libftdi1-2 libftdi1-dev libhidapi-hidraw0 \
  libhidapi-dev libudev-dev zlib1g-dev cmake pkg-config make g++

# Create a tools directory
mkdir -p ~/Tools

# Clone and build from source (apt version is outdated)
cd ~/Tools
git clone https://github.com/trabucayre/openFPGALoader.git
cd openFPGALoader
mkdir build && cd build
cmake ../
cmake --build . -j$(nproc)
sudo make install

# Set up udev rules (CRITICAL — without this, you need sudo for every flash)
sudo cp ../99-openfpgaloader.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger

# Add your user to the plugdev group
sudo usermod -aG plugdev $USER

# IMPORTANT: Log out and log back in for the group change to take effect
# Or reboot: sudo reboot
```

### 9.2 Verify FPGA Connectivity

```bash
# Plug in your Tang Nano 9K via USB-C

# Check USB device is detected
lsusb | grep -i "Future Technology\|FTDI\|0403"
# Expected: "Future Technology Devices International, Ltd FT2232C/D/H Dual UART/FIFO IC"

# Check serial ports
ls /dev/ttyUSB*
# Expected: /dev/ttyUSB0 and /dev/ttyUSB1

# Detect the FPGA
openFPGALoader --detect
# Expected output:
# Jtag frequency : requested 6.00MHz -> real 6.00MHz
# index 0: idcode 0x100481b manufacturer Gowin family GW1N model GW1N(R)-9C irlength 8

# Test flash programming with an existing bitstream (if you have one from backup)
# To SRAM (temporary — lost on power cycle):
openFPGALoader -b tangnano9k your_bitstream.fs

# To Flash (persistent):
openFPGALoader -b tangnano9k -f your_bitstream.fs
```

### 9.3 FTDI Driver Conflict Resolution

```bash
# If openFPGALoader reports "unable to open ftdi device":
# The ftdi_sio kernel module may be claiming the device

# Temporarily unload:
sudo modprobe -r ftdi_sio

# Permanently blacklist:
echo "blacklist ftdi_sio" | sudo tee /etc/modprobe.d/ftdi_sio.conf

# BETTER APPROACH: Use udev rules to selectively unbind only the JTAG interface,
# keeping the UART interface for serial communication (needed for PicoRV32):
cat << 'EOF' | sudo tee /etc/udev/rules.d/99-tangnano.rules
# Tang Nano 9K - JTAG interface (interface 0) -> no ftdi_sio
SUBSYSTEM=="usb", ATTR{idVendor}=="0403", ATTR{idProduct}=="6010", \
  ATTR{bInterfaceNumber}=="00", RUN+="/bin/sh -c 'echo $kernel > /sys/bus/usb/drivers/ftdi_sio/unbind'"

# Tang Nano 9K - UART interface (interface 1) -> keep ftdi_sio for serial
# (no action needed, it will auto-bind)
EOF

sudo udevadm control --reload-rules && sudo udevadm trigger
```

---

## Step 10: Install Claude Code CLI

```bash
# Install Claude Code
curl -fsSL https://cli.claude.com/install.sh | sh

# Verify
claude --version

# Login
claude login

# Health check
claude doctor

# If login fails:
# 1. Try: claude logout && claude login
# 2. Pro/Max plan: OAuth flow should work in browser
# 3. API key: verify at console.anthropic.com
```

---

## Step 11: Install VS Code

```bash
# Install via snap (simplest)
sudo snap install code --classic

# Or via .deb package:
wget -qO- https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > packages.microsoft.gpg
sudo install -D -o root -g root -m 644 packages.microsoft.gpg /etc/apt/keyrings/packages.microsoft.gpg
echo "deb [arch=amd64 signed-by=/etc/apt/keyrings/packages.microsoft.gpg] https://packages.microsoft.com/repos/code stable main" | sudo tee /etc/apt/sources.list.d/vscode.list
sudo apt update && sudo apt install -y code

# Recommended extensions for ATOMiK development:
code --install-extension mshr-h.VerilogHDL
code --install-extension ms-python.python
code --install-extension rust-lang.rust-analyzer
code --install-extension lushay-labs.lushay-code  # FPGA toolchain extension

# Set as Git editor:
git config --global core.editor "code --wait"
```

---

## Step 12: Install OSS CAD Suite

Yosys (synthesis), nextpnr (place & route), and Apicula (Gowin bitstream generation) — an alternative/complement to Gowin EDA for scriptable synthesis pipelines.

```bash
cd ~/Tools
# Check https://github.com/YosysHQ/oss-cad-suite-build/releases for latest URL
wget https://github.com/YosysHQ/oss-cad-suite-build/releases/download/2024-02-14/oss-cad-suite-linux-x64-20240214.tgz

tar -xzf oss-cad-suite-linux-x64-*.tgz

# Add to PATH
echo 'export PATH="$HOME/Tools/oss-cad-suite/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc

# Verify
yosys --version
nextpnr-gowin --version
```

---

## Step 13: Install RISC-V GCC Toolchain

For PicoRV32 bare-metal C development.

```bash
# Try the pre-built package first
sudo apt install -y gcc-riscv64-unknown-elf

# If not available or you need RV32 specifically, build from source:
sudo apt install -y autoconf automake autotools-dev curl python3 python3-pip \
  libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo \
  gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build git cmake \
  libglib2.0-dev libslirp-dev

cd ~/Tools
git clone https://github.com/riscv-collab/riscv-gnu-toolchain
cd riscv-gnu-toolchain
./configure --prefix=/opt/riscv --with-arch=rv32imc --with-abi=ilp32
sudo make -j$(nproc)

# Add to PATH
echo 'export PATH="/opt/riscv/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc

# Verify
riscv32-unknown-elf-gcc --version
```

---

## Step 14: Install Lean4

For maintaining and extending the 92 formal proofs.

```bash
# Install elan (Lean version manager)
curl https://raw.githubusercontent.com/leanprover/elan/master/elan-init.sh -sSf | sh

# Verify
lean --version
lake --version

# Test with existing proofs
cd ~/Projects/ATOMiK/math/proofs
lake build
# Expected: All 92 theorems verify with 0 sorry statements
```

---

## Step 15: Set Up Python Environment

```bash
cd ~/Projects/ATOMiK
python3 -m venv .venv
source .venv/bin/activate
pip install -e "./software[dev,demo]"

# Run existing tests
cd software
python -m pytest
# Expected: 353 tests passing
```

---

## Phase 0 Validation Checklist

Before proceeding to Phase 1 (see `ATOMIK_V2_ROADMAP.md`), verify ALL of the following:

- [x] Ubuntu 24.04 LTS boots and runs stably (Kubuntu 24.04.4 LTS)
- [x] GPU drivers working (amdgpu/Lucienne, direct rendering: Yes)
- [x] WiFi/Ethernet working (wlp1s0)
- [x] ATOMiK repo restored and `git log` shows full history
- [x] `git push` to GitHub works over HTTPS (gh auth, MatthewHRockwell)
- [x] Gowin EDA IDE launches without errors (via LD_PRELOAD fix for libstdc++/Qt)
- [x] Gowin license configured (Educational Edition, gowin_E_E00AF6865C8B.lic)
- [x] openFPGALoader installed and `openFPGALoader --detect` finds Tang Nano 9K (GW1N(R)-9C)
- [x] Can flash a bitstream: `openFPGALoader -b tangnano9k -f <bitstream>.fs`
- [x] Serial port works: /dev/ttyUSB0 and /dev/ttyUSB1 present
- [x] Claude Code CLI: `claude --version` works (v2.1.39), authenticated
- [x] VS Code installed with Verilog extension (+ Python, Rust, Lushay)
- [x] RISC-V GCC: `riscv64-unknown-elf-gcc --version` works (13.2.0)
- [x] Python venv works and ATOMiK SDK tests pass (353 passed)
- [x] Lean4: `lean --version` works (v4.27.0)
- [x] OSS CAD Suite: `yosys --version` works (0.38)

---

*Once all items are checked, Phase 0 is complete. Proceed to Phase 1 in `ATOMIK_V2_ROADMAP.md`.*
*Last updated: February 12, 2026*
