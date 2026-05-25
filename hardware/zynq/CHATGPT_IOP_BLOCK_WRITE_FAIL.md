# ChatGPT prompt — Zynq-7000 IOP block (0xE0001000+) writes silently fail after PCAP

Paste everything below this line.

---

I've spent the day debugging standalone SD boot on an ALINX AX7020 (Zynq-7000 XC7Z020-2CLG484-2). Everything in the boot chain now works EXCEPT: I cannot write to the PS UART0 controller registers at `0xE0001000+`. The writes silently fail, even from JTAG/xsdb. SLCR writes in the same session work fine. I need help determining what's gating the IOP block.

## What works

After SD boot completes my FSBL + LiteX BIOS chain:

- BootROM accepts the strict-FAT32 partition layout (cluster_count > 65525).
- FSBL runs to its final marker (0xC0, DONE_MAGIC written to PS DDR 0x10100000).
- PCAP programs the PL bitstream — `DEVCFG_STATUS = 0x50000F30`, `INT_STS` has `D_P_DONE + DMA_DONE + PCFG_DONE_INT` all set.
- `FPGA_RST_CTRL @0xF8000240 = 0x00000000` (all PL resets cleared).
- `LVL_SHFTR_EN @0xF8000900 = 0x0000000F` (level shifters on).
- `FPGA0_CLK_CTRL @0xF8000170 = 0x00300600` (current LiteX-generated value).
- `APER_CLK_CTRL @0xF800012C = 0x015C0C0D` (matches LiteX-generated ps7_init).
- NaxRiscv runs the LiteX BIOS to completion — I patched BIOS `main()` with two marker writes:
  - `Nax 0x50000030 = 0xA11A11A1` (pre-uart_init) — fires
  - `Nax 0x50000034 = 0xB22B22B2` (post-uart_init) — fires

## What doesn't work

LiteX `serial` UART pin is on PL pin V12 (40-pin expansion header), not wired to the FT2232H on this board. In the JTAG-boot path, `xsdb` virtualizes the LiteX UART CSR over JTAG and presents it as a `/dev/ttyUSB`. SD boot has no xsdb, so V12 dumps to nowhere.

I tried to bypass this by writing to PS UART0 (MIO10/11, which IS wired to FT2232H on this board per AX7020 schematic + prior bare-metal validation 54 days ago). The FSBL does:

```c
/* MIO10 = UART0_TX, MIO11 = UART0_RX */
w32(0xF8000728u, 0x000012E0u);   /* L3_SEL=7 UART, IOTYPE=3 LVCMOS33, TRI=0 (output) */
w32(0xF800072Cu, 0x000012E1u);   /* L3_SEL=7 UART, IOTYPE=3, TRI=1 (input) */

/* Deassert UART resets */
w32(0xF8000228u, 0x0u);

/* Enable APER clock for UART0 */
uint32_t aper = r32(0xF800012Cu);
w32(0xF800012Cu, aper | (1u << 20));

/* Enable UART0 ref clock at IOPLL/20 */
w32(0xF8000154u, 0x00001401u);

/* Configure UART0 controller */
w32(0xE0001000u, 0x00000028u);   /* CR: RX_RST + TX_RST */
w32(0xE0001004u, 0x00000020u);   /* MR: 8N1 */
w32(0xE0001018u, 0x0000007Cu);   /* BAUDGEN */
w32(0xE0001034u, 0x00000006u);   /* BAUDDIV (for ~115200 with 50 MHz ref) */
w32(0xE0001000u, 0x00000017u);   /* CR: TX_EN + RX_EN */
```

After SD boot completes and FSBL reaches marker 0xC0, JTAG no-reset probe reads:

```
MIO_PIN_10  @0xF8000728 = 0x000012E0   ✓ our write stuck
MIO_PIN_11  @0xF800072C = 0x000012E1   ✓ our write stuck
UART_RST_CTRL @0xF8000228 = 0x00000000 ✓ deasserted
APER_CLK_CTRL @0xF800012C = 0x015C0C0D ✓ bit 20 (UART0 AMBA clk) set
UART_CLK_CTRL @0xF8000154 = 0x00001401 ✓ UART0 ref clk enabled, DIV=20, src=IOPLL

CR     @0xE0001000 = 0x00000000   ✗ should be 0x17
MR     @0xE0001004 = 0x00000000   ✗ should be 0x20
BAUDGEN@0xE0001018 = 0x00000000   ✗ should be 0x7C
BAUDDIV@0xE0001034 = 0x00000000   ✗ should be 0x06
SR     @0xE000102C = 0x00000000   ✗ should at least show TXEMPTY
```

I then tried writing the same UART0 registers **directly from xsdb via `mwr -force`**:

```
xsdb> mwr -force 0xE0001000 0x00000017
xsdb> mrd -force 0xE0001000 1
0xE0001000:   00000000          ← write silently dropped
```

So this is not an FSBL bug. ARM's DAP can write the registers but they read back as zero. DAP itself does not show error state (`targets` lists APU and CPU0 normally, no DAP node with sticky errors).

## What I've confirmed about the DAP / bus path

- `mwr -force 0xF8000228 0x0F` followed by `mrd 0xF8000228` returns `0x0F` ✓
- `mwr -force 0xF8000228 0x00` followed by `mrd 0xF8000228` returns `0x00` ✓
- `mrd 0xF8000170` returns the FPGA0 clock control value ✓
- `mrd 0xE0001000` always returns 0 regardless of `mwr` to that address ✗

So the ARM AHB-AP can write SLCR (`0xF8000xxx`) but not the IOP register block (`0xE0001000+`). Looking at the Zynq-7000 address map (UG585 §10.1), `0xE0000000-0xE03FFFFF` is the IOP slcr+peripherals range; `0xF8000000-0xF8FFFFFF` is the central peripheral interconnect.

`ps7_init.c` (from this build's LiteX-generated PS config) contains **zero references to addresses `0xE0001xxx`** — so ps7_init never touches the UART0 controller registers. It does set:
- UART0 pinmux on MIO10/11 (already verified, stuck correctly)
- `UART0_CPU_1XCLKACT = 0x1` in APER_CLK_CTRL bit 20
- UART_CLK_CTRL ref clock

But it does not enable the UART0 peripheral controller itself.

The 54-day-old project memory note recorded UART0 working on this board via a bare-metal TACTIVE=1 test. So the hardware path **was** alive at some point. Now writes don't stick.

## My hypothesis

Something between the ARM DAP and the IOP slave bus is gating writes after PCAP completes. Candidates:

1. **A Zynq security/lock register** that locks the IOP block until something explicit unlocks it. The SLCR has lock/unlock keys; maybe the IOP block has its own?
2. **DDR / address-remap window** that overlays 0xE0000000+. Unlikely on Zynq-7000.
3. **The TrustZone/security control** in `slcr.SCL` (`0xF8000000`) is somehow restricting the AHB-AP's access. UG585 §27 talks about debug-domain access permissions.
4. **A side-effect of the LiteX SoC PCAP'd into the PL.** The LiteX SoC exposes a `ps_iop` window at NaxRiscv `0x80000000` mapping to ARM `0xE0000000`. Maybe the PL has taken AXI master role for that window and is *intercepting* writes meant for the IOP block, even from ARM-side DAP.
5. **Power island**. Per UG585 §6.3.1, the Zynq has a `PSPLL_PWRDWN`/peripheral power-down sequence. After PCAP some peripherals may be powered down by ps7_init.

## What I need from you

1. **What gates ARM AHB-AP writes to `0xE0001000+` (UART0) when SLCR writes work normally?** Specifically — is there an unlock or power-up sequence that needs to happen for the IOP peripheral controllers to accept register writes?

2. **Does ps7_init (LiteX-generated, no UART0 init) intentionally power down or hold UART0 in a state that ignores writes?** If yes, what's the wakeup sequence?

3. **Could the LiteX SoC's `ps_iop` window (NaxRiscv 0x80000000-0x80300000 → ARM 0xE0000000-0xE0300000) be capturing ARM-side writes** in a way that bypasses the actual UART0 controller? If yes, how do I confirm and how do I rewire?

4. **Is there a register/CSR that I can read from xsdb that would show whether UART0 is in a "responding" vs "ignoring" state**, before I burn another iteration?

## Repo paths

- `hardware/zynq/fsbl_build/minimal_fsbl/fsbl_main.c` — FSBL with UART0 init
- `hardware/zynq/fsbl_build/ps7_init.c` — PS7 init (LiteX-generated, no UART0 controller init)
- `hardware/zynq/litex-build-nax64-sdboot/csr.csv` — LiteX SoC CSR map (shows `ps_iop` window)
- `hardware/zynq/litex-build-nax64-sdboot/gateware/hamgeek_rk7020f.v` — synthesized RTL

Please tell me the one register to probe, or the one xsdb command to try, that distinguishes the hypotheses above.
