# Running the ATOMiK Desk demo on the AX7020

The validated, standalone, self-driving demo: ATOMiK Desk on real Zynq HDMI,
cycling the three customer Workloads scenarios with LIVE_MEASURED numbers, plus
the animated Atom assistant. Runs with **no laptop tethered** once launched.

## 0. Hardware
- ALINX/HamGeek AX7020 (XC7Z020-2CLG484), HDMI to a 1080p monitor.
- Two USB cables to the laptop: **console** (FT232R, `0403:6001`) and **JTAG**
  (FT2232H, `0403:6010`). Confirm both with `lsusb | grep 0403`.

## 1. Boot (the board can't auto-boot Linux today)
After a cold power-cycle the FPGA is blank. JTAG-load the bitstream + Linux:
```
source /opt/Xilinx/2025.2/Vivado/settings64.sh
ATOMIK_BITSTREAM=hardware/zynq/litex-build-nax64-mbus-irq/gateware/hamgeek_rk7020f.bit \
  python3 hardware/zynq/fsbl_build/jtag_load_all_then_boot.py
```
~96s to load + `Liftoff`. The NaxRiscv Linux console is on **/dev/ttyUSB2**
(probe for the `root@atomik-rv64` shell; the port varies per enumeration).

## 2. Deploy (verified — fonts/assets/binary are size-checked + retried)
```
cd atomik_os && make all
ATOMIK_PORT=/dev/ttyUSB2 python3 deploy.py --no-shot --no-launch --mode INVESTOR
```
Watch for **`all 4 AA fonts verified on board`** at the end. If you see
`*** FONTS DID NOT LAND ***`, re-run — a pixel-font UI (blocky/cramped) means a
font atlas is missing. Also ship the demo tools (small):
`deploy.transfer` `tools/fbcrop`, `hardware/zynq/test/aworkload`,
`hardware/zynq/test/atomik_bench_sweep_tiny`→`/tmp/abench`,
`tools/atomik_bench_daemon.sh`, `tools/board_demo_launch.sh`→`/tmp/wl.sh`.

## 3. Launch the standalone demo
```
# optional: pin Atom's mood/pose
printf success > /tmp/atomik_assist_force      # success|thinking|warning|explain
sh /tmp/wl.sh                                   # -> WL_DEMO_UP
```
`wl.sh` runs the live `aworkload` (adapter-verified savings) + the bench daemon
(parallel-bank throughput), sets demo mode (auto-cycles the 3 scenarios every
12s), and opens Workloads via the `/tmp/atomik_open_workloads` startup hook.
It launches with `</dev/null` stdin (idles correctly — the EOF-spin fix).

## 4. Capture a screenshot (optional)
```
/tmp/fb2png /tmp/shot.png                        # full 1080p frame, OR
/tmp/fbcrop /tmp/win.png 560 230 800 620         # a window region (small)
```
Then SIGSTOP atomik_os (verify `State: T (stopped)` in `/proc/<pid>/status`)
**before** pulling, so the frame is coherent. On a degraded UART link, pull
small crops (~95 KB) — full 6 MB frames stall.

## Known gotchas (learned the hard way)
- **Verify fonts landed.** "VERSION OK" does NOT mean the fonts/assets shipped;
  byte-doubling can corrupt those steps silently → pixel-font fallback.
- **The FTDI link degrades** under sustained transfer over a long session
  (pulls crawl). Simple commands stay clean. Unplug/replug the cable at the
  laptop to refresh; never `USBDEVFS_RESET` it (knocks JTAG off the bus).
- **Never `pkill -f` a string that's in your own wrapper's cmdline** (self-kill).
- Console shell is PID 1 init — **never send Ctrl-D** (kernel panic).
