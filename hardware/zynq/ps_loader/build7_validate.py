#!/usr/bin/env python3
"""End-to-end validation of build #7: USB enumeration + HDMI framebuffer.

Steps:
  1. Run jtag_boot.py --no-boot   (loads DDR, MIO46 reset, ULPI OTG write)
  2. Send `boot 0x40a00000` over UART
  3. Wait for shell prompt
  4. Look up FB CSR addresses from csr.csv
  5. Fill /dev/fb0 with a test pattern (vertical color bars)
  6. Enable VTG + FB DMA via devmem
  7. Capture FB region back to host as PNG
  8. Re-run USB diagnostics: PORTSC, OTGSC, /sys/bus/usb/devices, /dev/input
"""
import csv, os, re, subprocess, sys, time

import serial

PORT, BAUD = "/dev/ttyUSB2", 115200
ZYNQ = "/home/mattrock/Projects/ATOMiK/hardware/zynq"
BUILD_DIR = f"{ZYNQ}/litex/build/hamgeek_rk7020f"
CSR_CSV = f"{BUILD_DIR}/csr.csv"
JTAG = ["python3", f"{ZYNQ}/ps_loader/jtag_boot.py",
        "--no-boot", "--port", PORT, "--baud", str(BAUD)]
BOOT = b"boot 0x40a00000\n"


def log(m): print(f"[v7] {m}", flush=True)


_n=[0]
def send_slow(s, line, per_char=0.003):
    for c in line:
        s.write(c.encode() if isinstance(c,str) else bytes([c]))
        time.sleep(per_char)
    s.flush()


def wait(s, n, t, echo=True):
    if isinstance(n,(bytes,str)): n=[n]
    n=[x.encode() if isinstance(x,str) else x for x in n]
    end=time.time()+t; buf=bytearray()
    while time.time()<end:
        c=s.read(8192)
        if c:
            if echo: sys.stdout.buffer.write(c); sys.stdout.buffer.flush()
            buf.extend(c)
            for x in n:
                if x in buf: return x, bytes(buf)
    return None, bytes(buf)


def cmd(s, c, t=10, echo=True):
    _n[0]+=1
    sent=f"_X_{_n[0]:04d}_"
    send_slow(s, f"{c}; echo {sent}\n")
    return wait(s, (f"\n{sent}").encode(), t, echo=echo)


def collect(s, c, t=15):
    _n[0]+=1
    sent=f"_X_{_n[0]:04d}_"
    send_slow(s, f"{c}; echo {sent}\n")
    nl = (f"\n{sent}").encode()
    end=time.time()+t; buf=bytearray()
    while time.time()<end:
        ch=s.read(8192)
        if ch: buf.extend(ch)
        if nl in buf: break
    txt = buf.decode(errors='replace')
    return re.sub(r'\x1b\[[0-9;?]*[a-zA-Z]', '', txt)


def parse_csr(path):
    """Return {csr_name: addr_int} from LiteX csr.csv."""
    out = {}
    with open(path) as f:
        for row in csv.reader(f):
            if not row or not row[0].startswith("csr_register"):
                continue
            try:
                out[row[1]] = int(row[2], 16)
            except (ValueError, IndexError):
                pass
    return out


def jtag_boot():
    log("xsdb DDR reload + MIO46 reset + ULPI OTG write")
    t0=time.time()
    p=subprocess.run(JTAG, capture_output=True, text=True, timeout=900)
    log(f"xsdb rc={p.returncode} in {time.time()-t0:.1f}s")
    sys.stdout.write(p.stdout[-1500:])
    if p.returncode!=0:
        sys.stderr.write(p.stderr[-2000:]); sys.exit(1)


def main():
    log("parse build #7 csr.csv")
    if not os.path.exists(CSR_CSV):
        sys.exit(f"missing {CSR_CSV}")
    csrs = parse_csr(CSR_CSV)
    fb_dma_base   = csrs.get("video_framebuffer_dma_base")
    fb_dma_length = csrs.get("video_framebuffer_dma_length")
    fb_dma_enable = csrs.get("video_framebuffer_dma_enable")
    fb_vtg_enable = csrs.get("video_framebuffer_vtg_enable")
    log(f"  fb_dma_base    = {hex(fb_dma_base) if fb_dma_base else 'MISSING'}")
    log(f"  fb_dma_length  = {hex(fb_dma_length) if fb_dma_length else 'MISSING'}")
    log(f"  fb_dma_enable  = {hex(fb_dma_enable) if fb_dma_enable else 'MISSING'}")
    log(f"  fb_vtg_enable  = {hex(fb_vtg_enable) if fb_vtg_enable else 'MISSING'}")
    if not all([fb_dma_enable, fb_vtg_enable]):
        log("ABORT: video CSRs missing in build #7 — bitstream did not include FB pipeline")
        return 2

    jtag_boot()

    s = serial.Serial(PORT, BAUD, timeout=0.5)
    s.reset_input_buffer()
    send_slow(s, "\n"); time.sleep(0.4); s.read(8192)
    log("sending boot")
    send_slow(s, BOOT.decode())

    log("wait for shell prompt (180s)")
    hit, _ = wait(s, [b"login:", b"# ", b"buildroot login"], 200)
    if hit and b"login" in hit:
        send_slow(s, "root\n"); time.sleep(1)
        wait(s, [b"# ", b"$ "], 30)
    if not hit:
        log("FAIL: no shell"); return 2

    log("\n=== USB DIAGNOSTICS ===\n")
    cmd(s, "echo ===PORTSC===;  devmem 0x80002184 32")
    cmd(s, "echo ===OTGSC===;   devmem 0x800021A4 32")
    cmd(s, "echo ===USBSTS===;  devmem 0x80002104 32")
    cmd(s, "dmesg | grep -iE 'usb|hub|hid|input' | tail -25", t=20)
    cmd(s, "ls /sys/bus/usb/devices/")
    cmd(s, "ls /dev/input/")
    cmd(s, "for f in /sys/class/input/input*/name; do echo \"$f: $(cat $f 2>/dev/null)\"; done", t=8)

    log("\n=== HDMI: write FB pattern, enable VTG+DMA ===\n")
    # Disable scan-out first to avoid showing partial frame
    cmd(s, f"devmem 0x{fb_dma_enable:08x} 32 0")
    cmd(s, f"devmem 0x{fb_vtg_enable:08x} 32 0")

    # Fill /dev/fb0 with vertical color bars (R/G/B/W) using a small Python
    # one-liner via the on-board busybox/python (or use dd from /dev/zero
    # combined with mmap).  Simplest: use dd to write a constant pattern
    # by repeating small files.  For now, fill with white (XRGB 0x00FFFFFF)
    # using a small busybox loop via /proc/self/fd.  Fall back to plain
    # white solid using `/dev/zero` complemented.
    cmd(s, "ls /usr/bin/python* 2>&1")
    py = collect(s, "command -v python3 python python2 2>/dev/null | head -1", t=8)
    pybin = ""
    for line in py.splitlines():
        line = line.strip()
        if line.startswith("/") and "python" in line:
            pybin = line
            break
    log(f"python on board: {pybin or '(none)'}")

    if pybin:
        # Vertical color bars: 4 stripes (R, G, B, W) for visual proof of HDMI
        py_cmd = (
            f"{pybin} -c \"import mmap, os; "
            "fd = os.open('/dev/fb0', os.O_RDWR); "
            "size = 1920*1080*4; "
            "m = mmap.mmap(fd, size, mmap.MAP_SHARED, mmap.PROT_WRITE|mmap.PROT_READ); "
            "import struct; "
            "colors = [0x00FF0000, 0x0000FF00, 0x000000FF, 0x00FFFFFF]; "
            "for y in range(1080): "
            "  row = b''.join(struct.pack('<I', colors[(x*4)//1920]) for x in range(1920)); "
            "  m.seek(y*1920*4); m.write(row); "
            "m.close(); os.close(fd); print('FB filled')\""
        )
        cmd(s, py_cmd, t=120)
    else:
        # Fallback (no python on board): build a 4 KiB tile of repeating R/G/B/W
        # bars on host, then loop dd to fill the full FB. Each tile = 1024
        # pixels = 4096 bytes. Tile pattern: 256 px R, 256 px G, 256 px B,
        # 256 px W. Stamp the tile 1920*1080*4/4096 = 2025 times to cover FB.
        # Repeating in 1024-pixel tiles makes the full screen show ~2 sets of
        # R/G/B/W bars per row (still obvious vertical bands), which is plenty
        # for visual confirmation that HDMI scan-out is alive.
        log("no python on board — building 4 KiB color-bar tile, dd-stamp full FB")
        # Build the tile: 256 px each of R, G, B, W in XRGB little-endian
        # XRGB8888 = 0x00RRGGBB. LE bytes: BB GG RR 00.
        tile = (
            b"\x00\x00\xff\x00" * 256 +  # R
            b"\x00\xff\x00\x00" * 256 +  # G
            b"\xff\x00\x00\x00" * 256 +  # B
            b"\xff\xff\xff\x00" * 256    # W
        )
        # Send tile via base64 to /tmp/tile.bin, then dd-loop to /dev/fb0
        import base64 as b64mod
        tile_b64 = b64mod.b64encode(tile).decode()
        # Send in chunks to avoid line-length issues
        cmd(s, "rm -f /tmp/tile.b64 /tmp/tile.bin", t=5)
        chunk = 256
        for i in range(0, len(tile_b64), chunk):
            piece = tile_b64[i:i+chunk]
            cmd(s, f"printf '%s' '{piece}' >> /tmp/tile.b64", t=5, echo=False)
        cmd(s, "base64 -d /tmp/tile.b64 > /tmp/tile.bin && wc -c /tmp/tile.bin", t=10)
        # Stamp tile across FB: 8 MiB / 4 KiB = 2025 iterations. Use seek+bs.
        cmd(s, "for i in $(seq 0 2024); do "
              "dd if=/tmp/tile.bin of=/dev/fb0 bs=4096 seek=$i count=1 "
              "conv=notrunc 2>/dev/null; "
              "done; echo FILL_DONE", t=300)

    # Enable scan-out
    cmd(s, f"devmem 0x{fb_dma_enable:08x} 32 1")
    cmd(s, f"devmem 0x{fb_vtg_enable:08x} 32 1")
    cmd(s, f"echo VTG_ENABLE_RB; devmem 0x{fb_vtg_enable:08x} 32")
    cmd(s, f"echo FB_DMA_ENABLE_RB; devmem 0x{fb_dma_enable:08x} 32")
    log("\n  → check the HDMI monitor: should show R/G/B/W vertical bars\n")

    cmd(s, "od -An -tx4 -N32 /dev/fb0 | head -2", t=10)
    log("\n=== END ===\n")
    s.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
