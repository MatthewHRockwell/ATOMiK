# 1080p60 HDMI is Physically Impossible on the HamGeek RK-ZYNQ7020-F

**Date:** 2026-05-30
**Toolchain:** Vivado 2025.2, LiteX (current), NaxRiscv RV64
**Part:** XC7Z020-2CLG484 (-2 speed grade)
**Question settled:** Is the 1080p30 ceiling a hard silicon limit, or an artifact of
earlier (possibly poorly-executed) builds?

## Method

We did **not** trust prior builds. We added a `--video-1080p60` flag to
`hardware/zynq/litex/soc_nax64_atomik.py` that drives the real `VideoS7HDMIPHY`
OSERDES serializer at the full 1080p60 rate and ran a **fresh, complete place-and-route**
on the current toolchain:

```
python3 soc_nax64_atomik.py --build --with-video-phy-only --video-1080p60 \
    --output-dir=../litex-build-1080p60-test --uart-baudrate=921600
```

- Pixel clock requested: 148.5 MHz (MMCM realized 147.5 MHz)
- Serializer clock (5×, DDR): 742.5 MHz (MMCM realized **737.5 MHz** — the nearest
  achievable from the 100 MHz PS reference; the true 742.5 MHz would be *worse*)
- Uses the exact LiteX `VideoS7HDMIPHY` OSERDES cascade that ships at 1080p30 — so this
  is a valid apples-to-apples test, not a hand-rolled serializer.

## Result — post-route timing summary (per-clock)

```
Clock      WNS(ns)   WHS(ns)   WPWS(ns)   TPWS(ns)   TPWS Failing Endpoints
clk_fpga_0  +0.198    +0.082    +3.000      0.000      0  / 42104   (100 MHz sys: MET)
clkout0     +0.835    +0.118    +2.890      0.000      0  /   269   (147.5 MHz pixel: MET)
clkout1       —         —       -0.236     -0.927      7  /     8   (737.5 MHz serializer: FAIL)
```

**Setup timing passes everywhere.** The design routes, DRCs clean, and even writes a
bitstream. The failure is exclusively **minimum pulse width on the 737.5 MHz serializer
clock (clkout1)**: worst pulse-width slack **−0.236 ns**, total **−0.927 ns**, with
**7 of 8 endpoints failing**.

## Why this is a hard limit, not a build-quality issue

A **pulse-width (TPWS) violation** means the clock's high/low time is shorter than the
OSERDESE2 / clock-buffer primitive's *minimum required pulse width* at this speed grade.
That minimum is a fixed electrical property of the silicon cell. It is **independent of**:

- RTL quality or who/what wrote it
- synthesis strategy, placement, or routing effort
- toolchain version (this is Vivado 2025.2, the latest)
- constraint cleverness

The only ways to fix a TPWS violation are a **faster speed-grade part** (the board is
soldered -2) or a **slower clock** (lower resolution/refresh). There is no "build it
better" path past a primitive's minimum pulse width.

737.5 MHz → 1.356 ns period → 0.678 ns half-period; the primitive needs ~0.914 ns, i.e.
a practical ceiling near ~547 MHz for this class — consistent with the prior data point
that 462.5 MHz passed and 742.5 MHz failed.

## Conclusion

**1080p60 is permanently closed on this board.** We are at the OSERDES2 ceiling at
**1080p30 (371.25 MHz serializer)**. True 1080p + 60 Hz would require an external HDMI
transmitter / TMDS redriver chip that this board does not have.

**Achievable 60 Hz path:** **720p60** reuses the *exact* proven 74.25 MHz pixel /
371.25 MHz serializer clocking — guaranteed inside the closed timing envelope. It trades
resolution for true 60 Hz motion (a clean swap of the timing dict + HRES/VRES + one
clkout pair).

Artifacts: build log `/tmp/build_1080p60.log`; timing report
`hardware/zynq/litex-build-1080p60-test/gateware/hamgeek_rk7020f_timing.rpt` (line ~169).
