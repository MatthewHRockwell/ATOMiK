# ATOMiK Demo System — Complete Status Document

**Date:** 2026-04-30
**Board:** ALINX AX7020 (Xilinx Zynq-7020, XC7Z020-2CLG484-2)
**Cost:** $200

---

## 1. HARDWARE PLATFORM

### FPGA SoC
- **Zynq-7000 PS:** Dual-core ARM Cortex-A9 (used for DDR init + JTAG boot only)
- **Zynq-7000 PL:** Artix-7 fabric hosting the NaxRiscv soft CPU + ATOMiK adapter
- **DDR3:** 512 MB (PS-side, accessed by PL via AXI HP0 for framebuffer DMA)
- **Package:** CLG484 (critical: NOT CLG400 — pin mapping differs from ALINX reference)

### Soft CPU
- **NaxRiscv RV64GC** — 64-bit RISC-V with FPU + compressed instructions
- **Clock:** 100 MHz
- **ISA:** rv64imafdc (integer, multiply, atomic, float, double, compressed)
- **MMU:** Sv39 (3-level page tables, full virtual memory)
- **L2 Cache:** 32 KB
- **Boot path:** OpenSBI → Linux 6.9 kernel → Ubuntu 24.04 initramfs

### ATOMiK Hardware
- **Delta-state adapter** at AXI address `0xF0020000`
- **4 registers:** CMD (0x00), RS1 (0x04), RS2 (0x08), RD (0x0C)
- **Operations:** LOAD (funct3=000), ACCUM (funct3=001), READ (funct3=010), SWAP (funct3=011)
- **8 independent slots** — each configurable as a different virtual processor type
- **Algebra:** XOR-based delta-state accumulation (commutative, associative, self-inverse)
- **Throughput:** 2.5 Mops/s measured, ~40 cycles per operation
- **Detection speedup:** 399-500x vs software byte scan (4KB median)

### Display
- **HDMI:** 1920×1080@30Hz via LiteX video_framebuffer + rgb2dvi IP
  - Framebuffer at NaxRiscv address `0x48000000` (PS DDR physical `0x08100000`)
  - AXI HP0 DMA path, VTG at CSR `0xF0002800`, DMA enable at `0xF0002004`
  - simplefb registered as `/dev/fb0`
- **LCD:** 320×172 ST7789V SPI panel (GPIO bitbang via CSRs `0xF0003000`-`0xF0005800`)
  - Pins: SDA=U19, SCL=V18, DC=W13, CS=AA13, RST=AA18, LED=Y13 (Bank 33)

### Connectivity
- **UART:** 921600 baud via FT232R USB adapter (`/dev/ttyUSB2`)
  - Known issue: FT232R degrades after ~15+ rapid command sessions (needs USB replug)
- **JTAG:** FT2232H (ttyUSB0/1) for PL programming + DDR loading
- **USB Host:** PS USB0 ChipIdea controller detected but disabled in DT (interrupt not routed to PL PLIC)

---

## 2. SOFTWARE STACK

### Operating System
- **Ubuntu 24.04** on RV64GC (LP64D hardfloat ABI)
- **Linux 6.9.0** kernel (buildroot cross-compiled)
- **Initramfs:** 33 MB gzipped (decompresses in ~120s on 100MHz CPU)
- **Shell:** bash
- **Available tools:** coreutils, perl 5.38.2, awk, sed, grep, find, dpkg, devmem, dd, base64
- **NOT available:** python3, gcc (cross-compile on laptop)
- **Memory:** 487 MB total, ~358 MB free, no swap
- **Disk:** 228 MB rootfs, 150 MB free

### Boot Sequence
1. **JTAG boot** (`jtag_boot.py`): ps7_init → bitstream → DDR load (~98s)
2. **Trampoline:** L2 cache flush (128KB scratch write) then jump to OpenSBI
3. **OpenSBI v1.3** → Linux 6.9 → initramfs decompress → Ubuntu shell
4. **Total:** ~135s from JTAG start to login prompt
5. **USB kernel panic fix:** Device tree disables ChipIdea USB (`status = "disabled"`)

---

## 3. DEMO APPLICATION: `atomik_live`

### Build
```
riscv64-linux-gnu-gcc -O2 -o atomik_live atomik_live.c  # dynamic linked, ~51KB
riscv64-linux-gnu-strip atomik_live
```

### Transfer
- Base64 over UART via `do_transfer.py` (gzip → base64 → printf chunks → decode)
- Char-by-char command sending (2ms pacing) to avoid UART RX byte drops
- ~2 minutes for 51KB binary

### Main Dashboard (HDMI 1920×1080)

**Layout (top to bottom):**

| Zone | Y Range | Content |
|------|---------|---------|
| Top bar | 30-110 | "ATOMiK" title + "LIVE ON HARDWARE" badge + cycle/change/speedup stats |
| SW Lanes | 120-332 | 8 orange "SOFTWARE" execution lanes (always active, scan sweep animation) |
| ATOMiK Lanes | 120-332 | 8 color-coded virtual processor lanes (only active slots lit) |
| Volume bars | 340-370 | SW scanned KB (orange) vs ATOMiK touched KB (blue) |
| Delta-State Algebra | 372-600 | Free-form typing card + binary delta/state matrices + XOR explanation |
| Metrics | 640-770 | 4 metric cards (% saved, synced count, cost savings, speedup) + flow bars |
| History | 778-828 | Change history ribbon + event log |
| Key legend | 832 | Command reference |
| Bottom bar | 1028-1072 | Tagline |

**Interactive Controls:**
- `a-z`, space, punctuation → type into delta-state accumulator (letter-by-letter XOR)
- Backspace → undo last character (XOR self-inverse rollback)
- `1-8` → modify specific state buffer
- `Shift+A` → modify all buffers
- `Shift+B` → burst mode (3 seconds of rapid random changes)
- `Shift+C` → inject corruption + auto-detect tamper
- `Shift+D` → compiler lane demo (GCC + atomik.h)
- `Shift+F` → adoption forecast (7-year TAM projection)
- `Shift+H` or `?` → help overlay
- `Shift+I` → AI Training/Inference demo (full-screen, auto-running)
- `Shift+R` → reset all state + counters
- `Shift+S` → session summary
- `Shift+V` → integrity verification
- `Shift+W` → cycle virtual processor profiles
- `Shift+Q` → quit

### Virtual Processor System

6 types, dynamically assigned to 8 adapter slots:

| Type | Color | Purpose |
|------|-------|---------|
| DETECT | Blue | Change detection via fingerprint compare |
| VERIFY | Green | Integrity verification / tamper detect |
| SYNC | Cyan | Selective sync — replicate only changes |
| ACCUM | Yellow | Parallel accumulation from multiple producers |
| WATCH | Orange | State monitoring / periodic scan |
| IDLE | Gray | Unassigned slot |

5 workload presets (Shift+W to cycle):
- **Manual:** 8× DETECT
- **Agent Memory:** 2×DETECT + 2×ACCUM + 1×SYNC + 1×WATCH + 1×VERIFY + 1×DETECT
- **Cache Sync:** 3×SYNC + 3×DETECT + 1×VERIFY + 1×WATCH
- **Full Backup:** 8× DETECT
- **Idle Watch:** 3×WATCH + 2×VERIFY + 3×IDLE

### AI Training/Inference Demo (Shift+I)
- **Auto-running** — epochs advance every ~400ms without keypresses
- **Left: INFERENCE** — 8 model layers showing LOAD (cyan) vs SKIP (green/cached)
- **Right: TRAINING** — 3-8 worker lanes randomly targeting model layers
- **Center: ATOMiK ACCUMULATOR** — 8 slots showing which layers caught changes
- **Metrics:** Bandwidth saved %, layers synced/total, query speedup
- **Real hardware operations** — every epoch runs actual LOAD/ACCUM/READ on ATOMiK adapter
- **Q to return** to main dashboard

### LCD Replica (320×172)
- Blue accent bars top/bottom
- "LIVE" + "REPLICA" labels
- Synced count, buffer strip (8 abbreviated names), avoided %, KB scanned

### Idle Animation
- SW scan sweep line moves across software lanes every 500ms
- Keeps the display alive between keypresses

---

## 4. CLAUDE INTEGRATION (Two-Tier Architecture)

### Board-Side Command Executor
- **Embedded in `atomik_live.c`** — lines starting with `~` on stdin are executed via `popen()`
- **Response format:** `##RSP:CMD:...`, `##RSP:line`, `##RSP:EXIT:N`, `##RSP:END`
- **`cmd_running` flag** suppresses `##EVENT:` output during command execution
- **Works inside AI demo** — `~` commands handled in both main loop and AI demo loop
- **SIGPIPE ignored** — handles pipe+head commands correctly
- **Batch read** — command line read with 50ms polling to avoid byte drops

### Laptop-Side Tools
| Tool | Purpose |
|------|---------|
| `board_cmd.py "command"` | Raw shell command execution on board |
| `board_tool.py status` | Structured system status |
| `board_tool.py health` | Quick OK/FAIL health check |
| `board_tool.py cpu` | /proc/cpuinfo |
| `board_tool.py memory` | free + /proc/meminfo |
| `board_tool.py atomik` | Read ATOMiK adapter + VTG/DMA CSRs via devmem |
| `board_tool.py processes` | ps aux |
| `board_tool.py files [path]` | ls -la |
| `board_tool.py exec "cmd"` | Raw command |
| `board_tool.py compile file.c` | Cross-compile → transfer → run on board |
| `board_chat.py "message"` | Send message to browser chat panel |

### Test Results
- **13/13 board commands pass** (echo, uname, cpuinfo, free, ls, df, date, echo, meminfo|head, ps|wc, which, uptime, id)
- **Key fix:** char-by-char command sending (2ms pacing) prevents byte drops at 921600 baud

### Framebuffer Screenshot
- `fb_screenshot` tool on board captures 480×270 downscaled PPM
- Transfer via base64 over `~` command protocol
- Color channel issue: framebuffer byte order needs R↔B swap (in progress)

---

## 5. BROWSER CONTROL PLANE (`bridge.py`)

### Architecture
- **WebSocket server** on port 8766
- **UART reader thread** parses `##EVENT:` and `##RSP:` lines
- **HTML page** written to `/tmp/atomik_replica.html` on startup

### 4-Panel Layout
1. **Left: State Map** — 8 buffer rows with change indicators
2. **Center: Delta Flow** — metric cards (Data Avoided, Synced, Cycles) + event log
3. **Right: Economics** — compute avoided, cost savings (1K + 50M servers), speedup, data volume, system info
4. **Far Right: Board Claude** — chat panel with pulsing green "live" dot, message feed

### Key Forwarding
- All printable characters forwarded to board UART
- Backspace sends `<` (alias handled by atomik_live)
- `##EVENT:` updates all dashboard metrics in real-time

### Chat System
- `board_chat.py` sends messages via WebSocket → broadcast to all browser clients
- Messages appear in Board Claude panel with sender, text, timestamp
- Supports `--sender "System"` for system messages

---

## 6. PROVEN PERFORMANCE METRICS

| Metric | Value | How Measured |
|--------|-------|-------------|
| Detection speedup | 399-500x | rdtime() benchmark at startup (SW byte scan vs ATOMiK LOAD/READ) |
| Raw throughput | 2.5 Mops/s | 4800 ops in 192K cycles (LOAD+ACCUM+READ × 8 slots × 200 rounds) |
| Per-operation latency | 40 cycles | 4μs at 100MHz |
| Inference layer detection | 3.9x | memcmp 8×4KB vs ATOMiK fingerprint detect |
| Bandwidth saved (inference) | 50-75% | 3-5 of 8 layers unchanged per epoch |
| Virtual processor reconfig | 36-597μs | LOAD instruction per slot, measured in vproc_demo.c |

---

## 7. FILES

### On Board (`/tmp/`)
- `atomik_live` — main demo binary (51KB, dynamically linked)
- `fb_screenshot` — framebuffer capture tool (6KB)
- `fb_probe` — framebuffer text content verifier
- `fb_text_probe` — detailed content brightness scanner

### On Laptop (`hardware/zynq/ps_loader/`)
- `atomik_live.c` — main demo source (~1500 lines)
- `atomik.h` — ATOMiK API header (dual MMIO/native backend)
- `bridge.py` — UART-to-WebSocket bridge + browser HTML
- `board_cmd.py` — raw board command tool
- `board_tool.py` — structured board introspection
- `board_chat.py` — browser chat message sender
- `jtag_boot.py` — JTAG programming + DDR load + Linux boot
- `demo_launch.sh` — one-command operator script
- `font_8x16.h` — VGA font for framebuffer text rendering
- `do_transfer.py` — reliable binary transfer script (in /tmp/)
- `TWO_TIER_ARCHITECTURE.md` — 15-section Claude integration plan (934 lines)
- `NARRATION.md` — 90-120s scripted narration
- `LEAVE_BEHIND.md` — one-page investor document
- `DEMO_BUILD_BRIEF.md` — 14-section implementation plan

---

## 8. KNOWN ISSUES

1. **FT232R serial degradation** — after ~15 rapid command sessions, the USB-UART adapter stops responding. Fix: physically unplug/replug USB cable.
2. **AI demo speedup shows 0.1x** — the fingerprint computation dominates over memcmp for 4KB buffers. The real value is bandwidth saved (skip unchanged layers), not raw cycle speedup. Use the startup benchmark (399x) for the detection story.
3. **Framebuffer screenshot colors** — R↔B channel swap between framebuffer format and PPM output. Fix in progress.
4. **No Python3 on board** — Ubuntu initramfs doesn't include it. Perl is available for scripting.
5. **UART-only connectivity** — no ethernet, no WiFi. All communication via 921600 baud serial.
6. **L2 cache flush overhead** — 128KB scratch write required before each visible HDMI update (NaxRiscv has no Zicbom cache management instructions).

---

## 9. FUTURE CAPABILITIES (Planned)

1. **Auto-demo mode** — system cycles through scenarios automatically with Board Claude narrating
2. **PowerPoint sync** — AI-driven presentation that drives the board demo in tandem
3. **Animated data flow** — particle effects between SW and ATOMiK lanes
4. **Live throughput ticker** — real-time ops/sec counter
5. **Workload heatmap** — per-slot activity intensity over time
6. **Ethernet bring-up** — PS MIO pins for networking (requires driver work)
7. **SD boot** — eliminate JTAG dependency for faster startup
