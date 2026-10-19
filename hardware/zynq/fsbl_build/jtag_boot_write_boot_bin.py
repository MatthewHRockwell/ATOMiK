#!/usr/bin/env python3
"""Boot Linux via JTAG, preload new BOOT.bin into DDR, write it to SD FAT from Linux.

Key fix: serial port is opened BEFORE XSDB starts so we capture the LiteX BIOS
banner as it prints. XSDB runs as a background Popen. 10 s after XSDB finishes,
we blindly send the 'boot' command (BIOS is at its prompt by then). Then we
monitor for the Linux shell and run the SD-write commands.

DDR preload layout:
  NaxRiscv 0x5C000000 = PS 0x1C100000  <- new BOOT.bin (~4 MB)
  (top quarter of the 512 MB main_ram window; kernel+initramfs stay below 0x50000000)
"""
import os, subprocess, sys, time, serial

ZYNQ_DIR    = "/home/mattrock/Projects/ATOMiK/hardware/zynq"
XSDB        = "/opt/Xilinx/2025.2/Vivado/bin/xsdb"
PS7_INIT    = f"{ZYNQ_DIR}/scripts/ps7_init_rk7020f.tcl"
BITSTREAM   = f"{ZYNQ_DIR}/litex-build-nax64-sdboot/gateware/hamgeek_rk7020f.bit"
LITEX_BUILD = f"{ZYNQ_DIR}/litex-build"
PSLD        = f"{ZYNQ_DIR}/ps_loader"
BIOS_PORT   = "/dev/ttyUSB2"

BOOT_FILES = [
    (f"{LITEX_BUILD}/Image_nax64",          0x4000_0000),
    (f"{LITEX_BUILD}/linux_nax64.dtb",      0x40EF_0000),
    (f"{LITEX_BUILD}/fw_jump_nax64.bin",    0x40F0_0000),
    (f"{LITEX_BUILD}/ubuntu_rv64.cpio.gz",  0x4200_0000),
    (f"{PSLD}/build/trampoline.bin",        0x40A0_0000),
]

NEW_BOOT_BIN = f"{ZYNQ_DIR}/fsbl_build/BOOT_l2off.bin"
NEW_BOOT_NAX = 0x5C00_0000
NEW_BOOT_PS  = 0x1C10_0000
BOOT_TARGET  = 0x40A0_0000

def nax_to_ps(nax):
    return nax - 0x3FF0_0000

def build_xsdb_script():
    lines = [
        "connect", "after 500",
        'targets -set -filter {name =~ "APU*"}',
        "rst -system", "after 500",
        'puts "-- ps7_init --"',
        f"source {PS7_INIT}", "ps7_init",
        'puts "-- fpga --"',
        'targets -set -filter {name =~ "xc7z*"}',
        f"fpga -file {BITSTREAM}",
        'targets -set -filter {name =~ "APU*"}',
        "ps7_post_config",
        'targets -set -filter {name =~ "ARM*Cortex-A9 MPCore #0"}',
        "stop", "after 5000",
    ]
    for path, nax_addr in BOOT_FILES:
        ps_addr = nax_to_ps(nax_addr)
        sz = os.path.getsize(path)
        lines.append(f'puts "-- {os.path.basename(path)} {sz//1024}KB --"')
        lines.append(f"dow -data {path} 0x{ps_addr:08x}")
    sz = os.path.getsize(NEW_BOOT_BIN)
    lines.append(f'puts "-- BOOT_l2off.bin {sz//1024}KB --"')
    lines.append(f"dow -data {NEW_BOOT_BIN} 0x{NEW_BOOT_PS:08x}")
    lines += ["con", 'puts "-- ALL LOADED; CPU running --"', "disconnect", "exit"]
    return "\n".join(lines)


def main():
    boot_size   = os.path.getsize(NEW_BOOT_BIN)
    skip_pages  = NEW_BOOT_NAX // 4096
    count_pages = (boot_size + 4095) // 4096

    sd_cmds = [
        "mknod /dev/mem c 1 1 2>/dev/null; echo mknod_ok",
        "ls /dev/mmcblk* 2>/dev/null | head -5",
        f"dd if=/dev/mem bs=4096 skip={skip_pages} count={count_pages} 2>/dev/null | head -c {boot_size} > /tmp/BOOT_l2off.bin && ls -lh /tmp/BOOT_l2off.bin",
        "mount /dev/mmcblk0p1 /mnt 2>/dev/null || mount /dev/mmcblk1p1 /mnt 2>/dev/null; df /mnt 2>/dev/null || echo mount_failed",
        "ls -lh /mnt/BOOT.BIN /mnt/BOOT.bin 2>/dev/null; ls /mnt/ | head -6",
        "cp /tmp/BOOT_l2off.bin /mnt/BOOT.BIN && sync && ls -lh /mnt/BOOT.BIN",
        "umount /mnt && echo UNMOUNT_OK",
    ]

    total_mb = (sum(os.path.getsize(p) for p, _ in BOOT_FILES) + boot_size) // 1024 // 1024
    print(f"=== JTAG boot + SD BOOT.bin update ({total_mb}MB to load) ===")
    print(f"  Bitstream:  {os.path.basename(BITSTREAM)}")
    print(f"  New BOOT:   {NEW_BOOT_BIN} ({boot_size//1024}KB)")
    print(f"  Preload:    NaxRiscv 0x{NEW_BOOT_NAX:08X} = PS 0x{NEW_BOOT_PS:08X}")

    script = build_xsdb_script()

    # Open serial BEFORE XSDB so BIOS banner is captured from the start
    ser = serial.Serial(BIOS_PORT, 115200, timeout=0.3)
    print(f"\nSerial open on {BIOS_PORT}. Launching XSDB…")

    xp = subprocess.Popen(
        [XSDB], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, text=True,
    )
    xp.stdin.write(script)
    xp.stdin.close()
    t0 = time.time()

    buf = b""
    xsdb_done = False
    xsdb_done_t = None
    boot_sent = False
    linux_ready = False
    deadline = time.time() + 900

    while time.time() < deadline:
        # Drain serial
        chunk = ser.read(512)
        if chunk:
            buf += chunk[-4096:]   # keep last 4KB for trigger detection
            sys.stdout.buffer.write(chunk); sys.stdout.buffer.flush()

        # Poll XSDB completion
        if not xsdb_done and xp.poll() is not None:
            xsdb_done = True
            xsdb_done_t = time.time()
            remaining = xp.stdout.read()
            for ln in remaining.splitlines():
                s = ln.strip()
                if s.startswith("--") or "error" in s.lower():
                    sys.stdout.write(f"\n[xsdb] {s}\n"); sys.stdout.flush()
            sys.stdout.write(f"\n[xsdb] done in {xsdb_done_t-t0:.0f}s (rc={xp.returncode})\n")
            sys.stdout.flush()
            if xp.returncode != 0:
                ser.close(); return

        # 10 s after XSDB, send boot command
        if xsdb_done and not boot_sent and (time.time() - xsdb_done_t) >= 10:
            ser.write(f"boot 0x{BOOT_TARGET:08X}\n".encode()); ser.flush()
            sys.stdout.write(f"\n[→] boot 0x{BOOT_TARGET:08X}\n"); sys.stdout.flush()
            boot_sent = True

        # Detect Linux shell
        if boot_sent and not linux_ready:
            tail = buf[-300:]
            if any(t in tail for t in [b"login:", b"$ ", b"# ", b"root@", b"ubuntu@"]):
                sys.stdout.write("\n[Linux shell detected]\n"); sys.stdout.flush()
                linux_ready = True
            # Timeout: force proceed 8 min after boot command sent
            if (time.time() - xsdb_done_t) > 500:
                sys.stdout.write("\n[Linux wait timeout — proceeding]\n"); sys.stdout.flush()
                linux_ready = True

        if linux_ready:
            break

        time.sleep(0.05)

    # Login if needed
    ser.timeout = 2
    if b"login:" in buf[-500:]:
        ser.write(b"root\n"); time.sleep(2)
        ser.read(2048)

    # Run SD write commands
    sys.stdout.write("\n\n=== Running SD write commands ===\n"); sys.stdout.flush()
    for cmd in sd_cmds:
        sys.stdout.write(f"\n$ {cmd}\n"); sys.stdout.flush()
        ser.write(f"{cmd}\n".encode()); ser.flush()
        wait = 20 if "dd " in cmd else 4
        time.sleep(wait)
        out = ser.read(8192)
        if out:
            sys.stdout.buffer.write(out); sys.stdout.buffer.flush()
        else:
            sys.stdout.write("  (no output)\n"); sys.stdout.flush()

    ser.close()
    print("\n\n=== Done. Power-cycle board + set switches to SD mode (11) to test. ===")


if __name__ == "__main__":
    main()
