# Phase 9 Proof Note — NaxRiscv RV64 + Dual Display

**Date:** April 24, 2026
**Board:** HamGeek RK-ZYNQ7020-F (XC7Z020-2CLG484I)
**Source commit:** 9f4eff2ea1884403deabb3aa7f3b18aec44fa90b

## Summary

Phase 9 upgraded the Zynq ATOMiK platform from VexRiscv SMP (RV32) to NaxRiscv (RV64GC) running Ubuntu 24.04, added 1920x1080 HDMI output and 320x172 SPI LCD output, and achieved 95-second JTAG boot (28x improvement over SFL).

All six sub-phases are hardware-verified on the physical board.

---

## Build Environment

| Component | Version |
|-----------|---------|
| Vivado | v2025.2 |
| Cross-compiler | riscv64-linux-gnu-gcc 13.3.0 (Ubuntu 24.04) |
| Host OS | Kubuntu 24.04 (Ryzen 7 5700U, 8GB) |
| LiteX | local checkout (litex, litex-boards, pythondata-cpu-naxriscv) |
| NaxRiscv netlist | NaxRiscvLitex_3c064e59e555b5c0eeae3421918289b5 (RV64GC+FPU+RVC) |
| Linux kernel | 6.9.0 (rv64imafdc, buildroot) |
| OpenSBI | fw_jump (litex-hub fork, rv64) |
| Rootfs | Ubuntu 24.04 debootstrap (riscv64, LP64D hardfloat) |

## Artifact Checksums

### Bitstream
| File | MD5 | Size | Built |
|------|-----|------|-------|
| litex-build-nax64/gateware/hamgeek_rk7020f.bit | ea016e0fa510c07009ebfbdab83c755e | 4,045,691 | 2026-04-24 06:09 |

Timing: WNS = +0.196 ns, TNS = 0.000 (zero violations)
CPU: NaxRiscv rv64i2p0_mafdc, CONFIG_CPU_TYPE_NAXRISCV

### Boot Files
| File | MD5 | Size |
|------|-----|------|
| Image_nax64 (Linux 6.9 kernel) | 8b2ee067d9230b4b8707bc7541c8969c | 8,231,096 |
| linux_nax64.dtb | e5f44e2b8e5aa545bb43099aa97ed2f2 | 2,759 |
| fw_jump_nax64.bin (OpenSBI) | 6f3a60d78a03cc82574805e440c1a21d | 133,632 |
| ubuntu_rv64.cpio.gz (rootfs) | d0329cabba7ec64ccfe0dcd184dcb6d6 | 33,932,917 |
| trampoline.bin (L2 flush) | d8ecb4cef45f784942321cd5226b10fa | 36 |

### Source Files
| File | MD5 |
|------|-----|
| litex/soc_nax64_atomik.py | 5e35f9aaba52285a08f22b2575ef8b97 |
| ps_loader/jtag_boot.py | (committed at 58c3bb7) |
| ps_loader/trampoline.S | (committed at 58c3bb7) |
| ps_loader/fb_test.c | 2016f301b8a9780ec8ac763f02ce2547 |
| ps_loader/atomik_splash.c | 8077496788f4ab1c151735630cd6ba61 |
| ps_loader/atomik_hdmi_viz.c | af27c535991d6c418f57866c2cf037c5 |
| ps_loader/lcd_tiny.c | b363efb00aa57633adfd0d9c04d43dd6 |
| ps_loader/lcd_splash.c | 12bee187e180d0e20a3985458d41a52c |

### CSR Memory Map
```
CSR_CTRL_BASE               = 0xF0000000
CSR_IDENTIFIER_MEM_BASE     = 0xF0000800
CSR_UART_BASE               = 0xF0001000
CSR_TIMER0_BASE             = 0xF0001800
CSR_VIDEO_FRAMEBUFFER_BASE  = 0xF0002000
CSR_VIDEO_FRAMEBUFFER_VTG   = 0xF0002800
CSR_ZLCD_CLK_BASE           = 0xF0003000
CSR_ZLCD_CS_BASE            = 0xF0003800
CSR_ZLCD_DC_BASE            = 0xF0004000
CSR_ZLCD_LED_BASE           = 0xF0004800
CSR_ZLCD_MOSI_BASE          = 0xF0005000
CSR_ZLCD_RST_BASE           = 0xF0005800
ATOMIK_ADAPTER_BASE         = 0xF0020000
```

---

## Phase 9.1: JTAG-Direct DDR Boot

**Claim:** 95-second boot from power-on to root shell (28x faster than SFL).

**Evidence:**
- xsdb session output shows `[xsdb] done in 97.6 s` to `101.9 s` across multiple runs
- All runs show zero "Invalid context" errors (JTAG clean)
- BIOS prompt appears, `boot 0x40a00000` triggers Liftoff, kernel boots to login

**Reproduction:**
```bash
cd hardware/zynq/ps_loader
source /opt/Xilinx/2025.2/Vivado/settings64.sh
BITSTREAM=../litex-build-nax64/gateware/hamgeek_rk7020f.bit python3 jtag_boot.py
```

**Critical details:**
- 5-second wait after ps7_post_config (prevents BIOS memtest collision)
- Trampoline at 0x40A00000 flushes L2 cache before jumping to OpenSBI
- Without trampoline, kernel reads stale L2 memtest data and hangs

---

## Phase 9.2: HDMI Framebuffer

**Claim:** 1920x1080@30Hz framebuffer via AXI HP0 on Dell 3440x1440 ultrawide.

**Evidence:**
- Kernel log: `simple-framebuffer 48000000.framebuffer: format=x8r8g8b8, mode=1920x1080x32`
- Solid color fills (red, green, blue, white) all render correctly
- fb_test program confirmed on hardware

**Build:**
```bash
riscv64-linux-gnu-gcc -O2 -static fb_test.c -o fb_test
```

**Key architecture:**
- Framebuffer at 0x48000000 (NaxRiscv) = 0x08100000 (PS DDR)
- AXI HP0 DMA reads from PS DDR (dedicated port, no bus contention)
- L2 eviction flush: write 128 KB scratch to force dirty lines to DDR

---

## Phase 9.3: HDMI Console Text

**Claim:** Readable 240x67 character console on HDMI via simplefb/fbcon.

**Evidence:**
- Kernel log: `Console: switching to colour frame buffer device 240x67`
- Kernel messages, login prompt, and shell commands visible on HDMI monitor
- DTS `simple-framebuffer` node at 0x48000000 with `x8r8g8b8` format

---

## Phase 9.4: ATOMiK Live Visualization

**Claim:** Delta-state operations rendered in real-time on HDMI framebuffer.

**Evidence:**
- atomik_hdmi_viz.c maps ATOMiK adapter at 0xF0020000 and framebuffer at 0x48000000
- Performs load/accumulate/read/swap operations and renders state as colored blocks
- Confirmed on hardware with visual output on Dell monitor

**Build:**
```bash
riscv64-linux-gnu-gcc -O2 -static atomik_hdmi_viz.c -o atomik_hdmi_viz
```

---

## Phase 9.5: ATOMiK Boot Splash

**Claim:** ATOMiK branded splash screen on 1080p HDMI.

**Evidence:**
- Dark blue background with ATOMiK blue accent bars renders on monitor
- Runs from userspace after Linux boot

**Build:**
```bash
riscv64-linux-gnu-gcc -O2 -static atomik_splash.c -o atomik_splash
```

---

## Phase 9.6: SPI LCD Splash

**Claim:** ST7789V 320x172 LCD displaying ATOMiK splash, all 6 pins confirmed.

**Evidence:**
- devmem SPI bitbang: Sleep Out (0x11) + Display ON (0x29) → fuzzy pixels visible (confirms connectivity)
- lcd_tiny program: full init + color fill → ATOMiK splash displayed
- UART log shows: `LED on / Reset / Init / Fill / Bars / DONE`
- Zero JTAG errors across 4 boot cycles with LCD pins active

**Pin mapping (from RK-ZYNQ7020-F Schematics.pdf page 5, Bank 33):**

| Signal | CLG484 Pin | Confirmed |
|--------|-----------|-----------|
| LCD_SDA | U19 | Yes — SPI data received by ST7789V |
| LCD_SCL | V18 | Yes — SPI clock drives display |
| LCD_DC | W13 | Yes — command/data select works |
| LCD_CS | AA13 | Yes — chip select enables display |
| LCD_RST | AA18 | Yes — reset sequence initializes controller |
| LCD_LED | Y13 | Yes — backlight illuminates display |

**V19 root cause:** V19 = PHY2_RXCTL (Ethernet PHY). Driving it as output crashed JTAG. The contaminated English manual listed V19=LCD_DC; correct DC pin is W13.

**Build:**
```bash
riscv64-linux-gnu-gcc -Os -nostdlib -static -fno-builtin -o lcd_tiny lcd_tiny.c
riscv64-linux-gnu-strip lcd_tiny
# Result: 2,400 bytes
```

**Transfer to board:**
```bash
# Base64 encode, send via UART in 128-char printf chunks, decode on target
base64 lcd_tiny | split into chunks | printf >> /tmp/lcd.b64
base64 -d /tmp/lcd.b64 > /tmp/lcd_tiny && chmod +x /tmp/lcd_tiny
/tmp/lcd_tiny
```

---

## SoC Build Command

```bash
cd hardware/zynq/litex
source /opt/Xilinx/2025.2/Vivado/settings64.sh
source /home/mattrock/Projects/ATOMiK/.venv/bin/activate
python3 soc_nax64_atomik.py --build \
  --uart-baudrate=921600 \
  --output-dir=../litex-build-nax64 \
  --with-video-framebuffer-hp
```

**NaxRiscv configuration** (set in main() after parse_args):
- `kwargs["cpu_type"] = "naxriscv"` (force, not setdefault)
- `NaxRiscv.no_netlist_cache = True` (FPU/RVC not in LiteX cache hash)
- `NaxRiscv.update_repo = "no"`
- `NaxRiscv.with_fpu = True`, `NaxRiscv.with_rvc = True`
- `NaxRiscv.l2_bytes = 32768`, `NaxRiscv.l2_ways = 4`
- LCD GPIO names prefixed "z" to preserve baseline UART CSR address (0xF0001000)
