#!/usr/bin/env python3
"""Use JTAG to write Image69 and rootfs69.cpio to safe DDR addresses.
The board is running Linux. We halt the ARM, write via dow -data to
PS DDR addresses that Linux isn't using (top of 512MB), then resume.
The board-side sd_rw tool then copies those DDR regions to the SD card.

The NaxRiscv cache for those addresses will be cold (never accessed),
so the writes arrive cleanly in DDR. Linux will be paused for ~2 min.

PS DDR 0x18000000 (NaxRiscv 0x58000000) = Image69
PS DDR 0x1C000000 (NaxRiscv 0x5C000000) = rootfs69.cpio

Memory check: Linux kernel loaded at 0x40000000 (NaxRiscv), uses ~194MB
= up to NaxRiscv 0x4C200000. We write at 0x58000000 and 0x5C000000,
which are in the unused top 128MB of DDR. Cache pages are cold there.
"""
import os, subprocess, sys, time

ZYNQ_DIR = "/home/mattrock/Projects/ATOMiK/hardware/zynq"
XSDB     = "/opt/Xilinx/2025.2/Vivado/bin/xsdb"
PS7_INIT = f"{ZYNQ_DIR}/scripts/ps7_init_rk7020f.tcl"

IMAGE    = f"{ZYNQ_DIR}/litex-build/Image69"
ROOTFS   = f"{ZYNQ_DIR}/litex-build/rootfs69.cpio"
FW_JUMP  = f"{ZYNQ_DIR}/litex-build/fw_jump69.bin"
DTB      = f"{ZYNQ_DIR}/litex-build/linux69.dtb"

# PS DDR target addresses (safe top-of-DDR region, Linux doesn't use these)
PS_IMAGE    = 0x18000000   # 384MB into 512MB DDR
PS_ROOTFS   = 0x1C000000   # 448MB into 512MB DDR
# These are already at the right PS DDR addresses from jtag_boot.py:
PS_FW_JUMP  = 0x01000000   # OpenSBI (NaxRiscv 0x40F00000)
PS_DTB      = 0x00EF0000   # DTB     (NaxRiscv 0x40EF0000)

# SECTORS to write on the SD card (from make_fat32_image.py output)
LBA_IMAGE   = 41600
LBA_ROOTFS  = 24896
LBA_FW_JUMP = 23952
LBA_DTB     = 24832
N_IMAGE     = 16686
N_ROOTFS    = 16679
N_FW_JUMP   = 515
N_DTB       = 6

def check_exists():
    for f in [IMAGE, ROOTFS, FW_JUMP, DTB]:
        if not os.path.exists(f):
            print(f"ERROR: {f} not found"); sys.exit(1)
        print(f"  {os.path.basename(f)}: {os.path.getsize(f)//1024}KB")

def build_jtag_script():
    """Write Image69 and rootfs to safe DDR addresses without rebooting."""
    # We don't need ps7_init or fpga load — Linux is already running.
    # Just connect, halt ARM, write, resume.
    lines = [
        "connect",
        "after 500",
        # Target the ARM CPU (not NaxRiscv)
        'targets -set -filter {name =~ "ARM*Cortex-A9 MPCore #0"}',
        "stop",
        "after 1000",
        f'puts "Writing Image69 ({os.path.getsize(IMAGE)//1024}KB) to PS 0x{PS_IMAGE:08X}..."',
        f"dow -data {IMAGE} 0x{PS_IMAGE:08x}",
        f'puts "Writing rootfs69.cpio ({os.path.getsize(ROOTFS)//1024}KB) to PS 0x{PS_ROOTFS:08X}..."',
        f"dow -data {ROOTFS} 0x{PS_ROOTFS:08x}",
        f'puts "Writing fw_jump69.bin to PS 0x{PS_FW_JUMP:08X}..."',
        f"dow -data {FW_JUMP} 0x{PS_FW_JUMP:08x}",
        f'puts "Writing linux69.dtb to PS 0x{PS_DTB:08X}..."',
        f"dow -data {DTB} 0x{PS_DTB:08x}",
        "con",   # resume ARM
        'puts "DDR writes complete. ARM resumed."',
        "disconnect",
        "exit",
    ]
    return "\n".join(lines)

def main():
    print("=== JTAG DDR write for Image69 + rootfs ===")
    check_exists()

    script = build_jtag_script()
    print("\nRunning xsdb...")
    r = subprocess.run([XSDB], input=script, text=True,
                       capture_output=True, timeout=300)
    for ln in r.stdout.splitlines():
        print(f"  {ln}")
    if r.returncode != 0:
        print("STDERR:", r.stderr[:500])
        print("FAILED")
        sys.exit(1)
    print("DDR writes done.")

    # Print instructions for board-side copy
    print(f"""
=== Now run this on the BOARD (root@atomik-rv64) ===

# Copy Image69 from DDR to SD card:
dd if=/dev/mem bs=512 skip={PS_IMAGE//512} count={N_IMAGE} 2>/dev/null \\
  | /tmp/sd_rw write {LBA_IMAGE} {N_IMAGE}

# Copy rootfs69.cpio from DDR to SD card:
dd if=/dev/mem bs=512 skip={PS_ROOTFS//512} count={N_ROOTFS} 2>/dev/null \\
  | /tmp/sd_rw write {LBA_ROOTFS} {N_ROOTFS}

# Copy fw_jump69.bin from DDR to SD card:
dd if=/dev/mem bs=512 skip={PS_FW_JUMP//512} count={N_FW_JUMP} 2>/dev/null \\
  | /tmp/sd_rw write {LBA_FW_JUMP} {N_FW_JUMP}

# Copy linux69.dtb from DDR to SD card:
dd if=/dev/mem bs=512 skip={PS_DTB//512} count={N_DTB} 2>/dev/null \\
  | /tmp/sd_rw write {LBA_DTB} {N_DTB}
""")

if __name__ == "__main__":
    main()
