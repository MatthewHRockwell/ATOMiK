#!/usr/bin/env python3
"""Load ALL SD card files + Linux boot images to DDR via JTAG, then boot.

After Linux boots, use dd+/dev/mem on the board to copy files from DDR
to /tmp, then write them to the SD card via sd_rw.

DDR layout (PS addresses):
  0x00100000  Image69      (8.2MB) — NaxRiscv boots from here
  0x00FF0000  linux69.dtb  (2.6KB)
  0x01000000  fw_jump69.bin (258KB)
  0x02100000  rootfs69.cpio (8.2MB)
  0x0AB00000  trampoline
  ---- SD card files (safe top-of-DDR, loaded for sd_rw) ----
  0x10000000  nax64.bit.bin (3.9MB)
  0x14000000  BOOT.bin      (3.9MB)

After boot, on the board:
  dd if=/dev/mem bs=512 skip=131072 count=7813 of=/tmp/nax64.bit.bin
  dd if=/dev/mem bs=512 skip=163840 count=7770 of=/tmp/BOOT.bin
  (fw_jump69, linux69.dtb, Image69, rootfs are already in DDR at boot addrs)
"""
import os, subprocess, sys, time, serial

ZYNQ_DIR  = "/home/mattrock/Projects/ATOMiK/hardware/zynq"
XSDB      = "/opt/Xilinx/2025.2/Vivado/bin/xsdb"
PS7_INIT  = f"{ZYNQ_DIR}/scripts/ps7_init_rk7020f.tcl"
# v0.40: HDMI demo boot uses the L2-off + video-framebuffer build (ATOMiK CSR
# @0xF0020000 + simple-framebuffer for /dev/fb0).  The old litex/build bitstream
# has no HDMI framebuffer, so atomik_os would have nothing to scan out.
BITSTREAM = f"{ZYNQ_DIR}/litex-build-nax64-l2off-fb/gateware/hamgeek_rk7020f.bit"
PSLD      = f"{ZYNQ_DIR}/ps_loader"
BIOS_PORT = "/dev/ttyUSB2"  # LiteX BIOS UART — varies; probe all ports if this fails

# Linux boot files (NaxRiscv addresses — same as proven jtag_boot.py).
# nax_to_ps() converts to PS DDR offsets for xsdb's dow -data.
LITEX_BUILD = f"{ZYNQ_DIR}/litex-build"
BOOT_FILES = [
    (f"{LITEX_BUILD}/Image_nax64",         0x4000_0000),
    (f"{LITEX_BUILD}/linux_nax64.dtb",     0x40EF_0000),
    (f"{LITEX_BUILD}/fw_jump_nax64.bin",   0x40F0_0000),
    (f"{LITEX_BUILD}/ubuntu_rv64.cpio.gz", 0x4200_0000),
    (f"{PSLD}/build/trampoline.bin",       0x40A0_0000),
]

# SD card files — load to safe top-of-DDR
# SD-card files + sd_rw binary. PS addresses chosen so that NaxRiscv can read
# them via /dev/mem (NaxRiscv 0x40000000-0x5FFFFFFF == PS 0x00100000-0x20100000,
# offset = -0x3FF00000).
#   NaxRiscv 0x56000000 == PS 0x16100000  (sd_rw,        ~580KB)
#   NaxRiscv 0x58000000 == PS 0x18100000  (BOOT.bin,     ~3.9MB)
#   NaxRiscv 0x5C000000 == PS 0x1C100000  (nax64.bit.bin,~3.9MB)
# v0.40: SD-write side-quest files (sd_rw, BOOT.bin, sdboot bitstream) are NOT
# needed to boot Linux for the HDMI demo — they only matter for the separate
# SD-population workflow.  Skipping them removes the /tmp/sd_rw dependency and
# ~8 MB of JTAG transfer, so the demo boot is faster.  Restore from git history
# if you need the SD-write path again.
SD_FILES = []

BOOT_TARGET = 0x40A0_0000   # trampoline address

def nax_to_ps(nax): return nax - 0x3FF0_0000

def build_xsdb_script():
    lines = [
        "connect", "after 500",
        'targets -set -filter {name =~ "APU*"}',
        "rst -system", "after 500",
        'puts "-- ps7_init --"',
        f"source {PS7_INIT}", "ps7_init",
        f'puts "-- fpga --"',
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
        lines.append(f'puts "-- dow {os.path.basename(path)} → PS 0x{ps_addr:08X} ({sz:,}B) --"')
        lines.append(f"dow -data {path} 0x{ps_addr:08x}")

    for path, ps_addr in SD_FILES:
        sz = os.path.getsize(path)
        lines.append(f'puts "-- dow {os.path.basename(path)} → PS 0x{ps_addr:08X} ({sz:,}B) --"')
        lines.append(f"dow -data {path} 0x{ps_addr:08x}")

    lines += ["con", 'puts "-- ALL LOADED; CPU running --"', "disconnect", "exit"]
    return "\n".join(lines)

def main():
    print("=== JTAG: Load all files + boot ===")
    for path, _ in BOOT_FILES + SD_FILES:
        sz = os.path.getsize(path)
        print(f"  {os.path.basename(path)}: {sz//1024}KB")

    script = build_xsdb_script()
    print(f"\nRunning xsdb (loading ~{sum(os.path.getsize(p) for p,_ in BOOT_FILES+SD_FILES)//1024//1024}MB)...")
    t0 = time.time()
    r = subprocess.run([XSDB], input=script, text=True, capture_output=True, timeout=600)
    elapsed = time.time() - t0
    for ln in r.stdout.splitlines():
        if ln.strip().startswith("--") or "error" in ln.lower():
            print(f"  {ln.strip()}")
    print(f"\nXSDB done in {elapsed:.0f}s (return code {r.returncode})")
    if r.returncode != 0:
        print("XSDB ERROR:", r.stderr[:500]); sys.exit(1)

    # Send boot command
    print(f"\nSending boot command on {BIOS_PORT} @ 115200...")
    time.sleep(2)
    with serial.Serial(BIOS_PORT, 115200, timeout=2) as s:
        s.write(b"\n\n"); time.sleep(0.5)
        d = s.read(2048)
        if b"litex" in d.lower() or b"> " in d:
            s.write(f"boot 0x{BOOT_TARGET:08X}\n".encode()); s.flush()
            print(f"  Sent: boot 0x{BOOT_TARGET:08X}")
        else:
            # Try anyway
            s.write(f"boot 0x{BOOT_TARGET:08X}\n".encode()); s.flush()
        time.sleep(2); d2 = s.read(4096)
        if b"Liftoff" in d2:
            print("  Liftoff confirmed!")
        print(f"  Response: {d2[:80]!r}")

    if SD_FILES:
        # NaxRiscv addresses for /dev/mem dd (PS - 0x3FF00000 = NaxRiscv).
        NAX_SD_RW  = 0x5600_0000
        NAX_BOOT   = 0x5800_0000
        NAX_BITSTR = 0x5C00_0000
        sz_sd_rw   = os.path.getsize(SD_FILES[0][0])
        sz_boot    = os.path.getsize(SD_FILES[1][0])
        sz_bitstr  = os.path.getsize(SD_FILES[2][0])
        print(f"""
=== Wait ~100s for Linux to boot, then on the BOARD (NaxRiscv addresses): ===

mknod /dev/mem c 1 1 2>/dev/null
dd if=/dev/mem bs=512 skip={NAX_SD_RW//512}  count={sz_sd_rw//512+1}  2>/dev/null | head -c {sz_sd_rw}  > /tmp/sd_rw         && chmod +x /tmp/sd_rw
dd if=/dev/mem bs=512 skip={NAX_BOOT//512}   count={sz_boot//512+1}   2>/dev/null | head -c {sz_boot}   > /tmp/BOOT.bin
dd if=/dev/mem bs=512 skip={NAX_BITSTR//512} count={sz_bitstr//512+1} 2>/dev/null | head -c {sz_bitstr} > /tmp/nax64.bit.bin

ls -lh /tmp/sd_rw /tmp/BOOT.bin /tmp/nax64.bit.bin
/tmp/sd_rw detect
""")
    else:
        print("""
=== HDMI demo boot: wait ~2-4 min for systemd, then deploy atomik_os ===

  cd atomik_os && python3 deploy.py --mode INVESTOR

deploy.py base64-ships build/atomik_os + fb2png to /tmp over ttyUSB2, launches
it on /dev/fb0 (investor mode), and pulls back a real fb2png screenshot.
For premium AA fonts, ship the .atomik_font set to /tmp/atomik_fonts first
(see ship_fonts step in the session notes).
""")

if __name__ == "__main__":
    main()
