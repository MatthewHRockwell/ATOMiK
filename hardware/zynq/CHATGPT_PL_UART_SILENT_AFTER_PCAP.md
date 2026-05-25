# ChatGPT prompt — NaxRiscv runs, BIOS executes, uart_init() succeeds, but UART pin produces zero output

Paste everything below this line.

---

I have a Zynq-7000 (XC7Z020-2CLG484-2) on ALINX AX7020 booting from SD card. The SD-boot pipeline now works end-to-end through PCAP, ps7_post_config, and LiteX BIOS handoff. I have **direct proof that NaxRiscv is executing the BIOS** but the UART produces zero bytes on any FT2232H port. The question is: what pin / polarity / routing is wrong, and how do I confirm it from the PS side without re-synthesizing?

## Definitive evidence NaxRiscv runs BIOS

I patched the LiteX BIOS `main()` to write two markers to DDR scratch before and after `uart_init()`:

```c
__attribute__((__used__)) int main(int i, char **c)
{
    /* Nax 0x50000030 = PS 0x10100030 — written before any peripheral init */
    *(volatile unsigned int *)0x50000030UL = 0xA11A11A1UL;
    ...
#ifdef CSR_UART_BASE
    uart_init();
    /* Nax 0x50000034 = PS 0x10100034 — written if uart_init() returned */
    *(volatile unsigned int *)0x50000034UL = 0xB22B22B2UL;
#endif
```

Rebuilt BIOS, regenerated rom.init, re-synthesized bitstream, reflashed SD, power-cycled into SD boot. After 30s, ARM-side JTAG no-reset probe reads:

```
BIOS_MARKER_PRE_UART  @0x10100030 = 0xA11A11A1   PRESENT
BIOS_MARKER_POST_UART @0x10100034 = 0xB22B22B2   PRESENT
```

**Both markers fire.** So:
- NaxRiscv is executing the BIOS `main()`.
- `uart_init()` completes successfully (does not hang).
- Code continues past `uart_init()` into the SD-autoboot poll loop, which calls `printf(...)`.

Yet a 60-second passive UART monitor on `/dev/ttyUSB0..2` at 8 baud rates each (9600, 19200, 57600, 115200, 230400, 460800, 921600, 1000000) catches **zero bytes**.

## PS-side state at the moment of probe (confirmed correct)

```
FPGA0_CLK_CTRL   @0xF8000170 = 0x00300600   (FCLK0 = 100 MHz, IO_PLL src, matches what the LiteX SoC was built for)
APER_CLK_CTRL    @0xF800012C = 0x015C0C0D   (matches LiteX-generated ps7_init expected value)
FPGA_RST_CTRL    @0xF8000240 = 0x00000000   (all PL fabric resets cleared)
LVL_SHFTR_EN     @0xF8000900 = 0x0000000F   (all 4 PS-PL level shifter bits enabled)
DEVCFG_STATUS    @0xF8007014 = 0x50000F30   (PCFG_DONE / DMA queue empty)
SLCR_LOCK_STA    @0xF800000C = 0x00000000   (SLCR unlocked at the moment of probe)
FSBL_MARKER      @0x10100010 = 0x000000C0   (FSBL completed all stages)
FSBL_S_DONE      @0x10100000 = 0xC0DEC0DE   (DONE_MAGIC handed off to LiteX BIOS)
FSBL_HEARTBEAT   @0x10100008 = ~0x2000      (FSBL alive in spin loop after handoff)
```

## What I've confirmed by elimination

- **It's not "NaxRiscv didn't start"** — markers prove it runs main().
- **It's not "uart_init() crashes"** — second marker fires after uart_init.
- **It's not "wrong baud"** — listening at 8 different baud rates, would expect at least garbled bytes; getting *zero*.
- **It's not bitstream-specific** — I tested with both the autoboot variant AND the proven-working `litex-build-nax64` bitstream (the one that prints `litex>` correctly via JTAG path). Same SoC config in both (csr.csv diff is empty). Both silent over SD-boot path.
- **It's not "FCLK frequency mismatch"** — I had it wrong before (was at 50 MHz), now at 100 MHz which is what `BaseSoC(sys_clk_freq=100e6)` was built for.
- **It's not level shifters** — `LVL_SHFTR_EN = 0xF` enables both directions.

## What's different vs the working JTAG path

The JTAG path that prints `litex>` uses **`xsdb fpga -file ...`** to program the same kind of bitstream. The SD path uses our minimal FSBL's PCAP. The xsdb command must do *something extra* in the PS that our minimal FSBL does not, because the same NaxRiscv-side BIOS runs and writes the same UART CSR in both cases, but only the JTAG path gives observable UART output.

Suspect list:
1. **EMIO/MIO routing for UART pin not configured for PL after PCAP**. ps7_init configures MIO pinmux for ps7-side functions. But the LiteX UART comes through EMIO — that needs separate configuration.
2. **UART direction signal not driven from PL**. Even with level shifters on (LVL_SHFTR_EN = 0xF), if the PL doesn't drive the EMIO output enable, the pad floats.
3. **Some Zynq-specific PS-PL bridge enable** that xsdb does as part of `fpga -file` that we miss in PCAP.

## What I need from you

1. **What pin does the LiteX UART end up on for this hamgeek_rk7020f.py target?** Is it EMIO via the Zynq's UART0 / UART1 peripheral pinmuxed to MIO46/47 (which on AX7020 routes to FT2232H channel B)? Or is it on a PL-direct pin that goes through the AX7020's expansion header (and therefore not connected to FT2232H at all)?

2. **What does `xsdb fpga -file` do after PCAP that programs the PS-PL bridge** in a way our minimal FSBL doesn't? I have Xilinx's full `ps7_init.c` (3850 lines) and `ps7_post_config()` is being called. What else?

3. **Concrete next probe** to disambiguate "PL drives UART pin but FT2232H doesn't see it" vs "PL is not driving the pin at all": is there a register/CSR in either PS or PL that I can read from ARM to confirm the UART output is toggling?

4. **If the UART is on a PL-direct pin, what's the cleanest patch?** Modify the LiteX platform file to route UART to a PS-EMIO instead, then re-synthesize? That's a ~16-minute rebuild but eliminates the routing variable.

## Repo paths

- `hardware/zynq/litex-build-nax64-sdboot/csr.csv` — SoC CSR map (autoboot bitstream)
- `hardware/zynq/litex-build-nax64-sdboot/gateware/hamgeek_rk7020f.v` — synthesized Verilog
- `hardware/zynq/litex-build-nax64-sdboot/gateware/hamgeek_rk7020f.xdc` — constraints (pin assignments)
- `/home/mattrock/litex/litex-boards/litex_boards/platforms/hamgeek_rk7020f.py` — platform definition (pin list)
- `/home/mattrock/litex/litex-boards/litex_boards/targets/hamgeek_rk7020f.py` — SoC target
- `/home/mattrock/litex/litex/litex/soc/software/bios/main.c` — patched BIOS

Tell me the one register or one file line to look at next.
