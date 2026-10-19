# ChatGPT Prompt — Debug AX7020 SD write from RISC-V Linux

Copy everything below this line into ChatGPT.

---

I am stuck on a Zynq-7000 SD-card write problem and need help debugging. I'll give you the architecture, what works, what fails, the two relevant pieces of code (the failing one and the proven-working reference), and my best hypotheses. Please:

1. Identify the most likely root cause of `card_init` hanging.
2. Tell me exactly what to print/read to confirm or rule it out (specific SDHC register offsets and the values you expect).
3. Propose a minimal patch to `sd_rw.c`.

## Architecture

- **Board**: ALINX AX7020, Zynq-7000 XC7Z020-2CLG484-2 (Cortex-A9 + Artix-7 PL in one die).
- **SD card slot**: wired to PS SDIO0 via MIO40–47, L3_SEL=4. Confirmed by schematic and by a working ARM-side driver. The earlier-suspected SDIO1 (0xE0101000) routing was wrong; the correct base is **SDIO0 = 0xE0100000**.
- **Boot chain**:
  1. ARM Cortex-A9 runs a minimal FSBL out of OCM. FSBL calls `ps7_init()` (clocks/MIO/DDR) and then a separate `sdhc_controller_init()` + `sdhc_card_init()` that reads files from FAT32.
  2. FSBL loads our LiteX/NaxRiscv bitstream into the PL via PCAP, runs `ps7_post_config`, fills DDR with Linux images, signals DONE.
  3. LiteX BIOS in PL boots `Image_nax64` (RISC-V Linux 6.9). ARM is parked.
- **NaxRiscv side memory map**: System RAM at `0x40000000-0x5FFFFFFF` (NaxRiscv physical addresses). The ARM PS register window is exposed to NaxRiscv as `ps_iop`: NaxRiscv `0x80000000 + 0x300000` = ARM `0xE0000000 + 0x300000`. So ARM `0xE0100000` (SDHC0) appears at NaxRiscv `0x80100000`. Confirmed by `/proc/iomem` and by being able to mmap and read it.
- **What I want to do**: from RISC-V Linux running on the PL NaxRiscv, write two files (BOOT.bin ~4.0 MB and nax64.bit.bin ~4.0 MB) onto raw SD sectors via a small `mmap(/dev/mem)` tool. I have already proven that:
  - `/dev/mem` mmap of `0x80100000` works (no fault).
  - I can read/write 32-bit registers there from userspace.
  - I have separately staged the file bytes into DDR via JTAG and copied them into `/tmp` via `dd if=/dev/mem`, so reading the controller window is fine.

The thing that hangs is the SDHC controller initialization.

## The failing code (RISC-V Linux userspace, hangs)

`sd_rw.c`, opened with `O_RDWR|O_SYNC`, mmapped at `0x80100000`, 4 KiB. `B` is the mapped base.

```c
#define SDHC0_PHYS  0x80100000UL

#define w32(o,v) (*(volatile uint32_t*)(B+(o))=(v))
#define w16(o,v) (*(volatile uint16_t*)(B+(o))=(v))
#define w8(o,v)  (*(volatile uint8_t *)(B+(o))=(v))
#define r32(o)   (*(volatile uint32_t*)(B+(o)))
#define r16(o)   (*(volatile uint16_t*)(B+(o)))
#define r8(o)    (*(volatile uint8_t *)(B+(o)))

#define INT_CMD_COMPLETE  (1u<<0)
#define INT_XFER_COMPLETE (1u<<1)
#define INT_BUF_WR_READY  (1u<<4)
#define INT_BUF_RD_READY  (1u<<5)
#define INT_ERR           0xFFFF0000u

static int wait_int(uint32_t mask, int ms) {
    for (int i=0; i<ms*100; i++) {
        uint32_t s = r32(0x30);
        if (s & mask) { w32(0x30, s & (mask|INT_ERR)); return 0; }
        if (s & INT_ERR) { w32(0x30,s); fprintf(stderr,"INT_ERR 0x%08x\n",s); return -1; }
        usleep(10);
    }
    fprintf(stderr,"timeout waiting 0x%x\n",mask); return -2;
}

static int send_cmd(uint8_t cmd, uint32_t arg, uint8_t rtype, uint32_t *resp) {
    /* offset 0x24 = PRESENT_STATE; bit 0 = CMD_INHIBIT */
    for (int i=0; i<100000 && (r32(0x24)&0x1); i++) usleep(10);
    w32(0x08, arg);                                    /* ARGUMENT */
    w16(0x0E, (uint16_t)((cmd<<8)|(rtype&0xFF)));      /* CMD register, no xfer mode */
    if (wait_int(INT_CMD_COMPLETE, 300)<0) return -1;
    if (resp) *resp = r32(0x10);                       /* RESPONSE */
    return 0;
}

static int card_init(void) {
    /* 0x2F = SW_RESET, write 0x07 = ALL|CMD|DAT */
    w8(0x2F, 0x07); usleep(50000);
    for (int i=0;i<100&&r8(0x2F);i++) usleep(1000);

    /* 0x34 = INT_STATUS_EN, 0x38 = INT_SIGNAL_EN, 0x30 = INT_STATUS */
    w32(0x34, 0xFFFFFFFF); w32(0x38, 0);
    w32(0x30, 0xFFFFFFFF);

    /* 0x29 = HOST_CTRL (byte). Setting 0x0F: not certain this is right; ARM driver writes 0xC0 to force "card inserted" test bits. */
    w8(0x29, 0x0F);

    /* 0x2C = CLOCK_CTRL (16-bit). 0x8001 = divisor 0x80 (=/256) + INTCLK_EN(bit 0). */
    w16(0x2C, 0x8001); usleep(10000);
    /* 0x8007 = same divisor + INTCLK_EN + INTCLK_STABLE(read-only?) + SDCLK_EN(bit 2). */
    w16(0x2C, 0x8007); usleep(5000);

    w8(0x2E, 0x0E);                                    /* TIMEOUT_CTRL */
    uint32_t r;
    send_cmd(0, 0, 0, NULL); usleep(2000);             /* CMD0 GO_IDLE */
    if (send_cmd(8, 0x1AA, 0x1A, &r)==0 && (r&0xFFF)==0x1AA) is_hc=1;  /* CMD8 */
    /* ACMD41 loop */
    for (int i=0;i<100;i++) {
        send_cmd(55,0,0x1A,NULL);
        if (send_cmd(41, is_hc?0x40FF8000:0x00FF8000, 0x02,&r)<0) break;
        if (r&0x80000000){is_hc=(r>>30)&1;break;}
        usleep(20000);
    }
    /* CMD2, CMD3, CMD7, ACMD6 (bus width 4) ... */
    /* Then bumps clock: w16(0x2C, 0x0101) — divisor 1 (/2) + INTCLK_EN, BUT NO SDCLK_EN BIT */
    return 0;
}
```

Symptom: `sd_rw detect` never returns. We just added `alarm(30)` so the hang is now bounded, but we haven't yet captured what specific step it's stuck in — that's the next experiment. From a previous session the failure looked like `wait_int` timing out on `INT_CMD_COMPLETE` for CMD0 ("timeout waiting 0x1").

## The reference that WORKS from ARM (different code path, same controller)

When the same Cortex-A9 FSBL talks to the same SDHC0 base before handing off to NaxRiscv, this driver successfully runs CMD0→8→55→41→2→3→7 and reads FAT32. So the hardware path works; the question is what's different about the RISC-V path.

```c
sdhc_err_t sdhc_controller_init(sd_card_t *c, uint32_t base)
{
    /* Full software reset */
    w8(base + SDHC_SW_RESET, SW_RESET_ALL);
    dsb();
    uint32_t tries = 100000;
    while (tries-- && (r8(base + SDHC_SW_RESET) & SW_RESET_ALL)) { }
    if (tries == 0) return SDHC_ETIMEOUT;

    w32(base + SDHC_INT_ENABLE,    0x00FF00FFu | INT_ERR_MASK);
    w32(base + SDHC_INT_SIGNAL_EN, 0);

    /* Force CD: bit6=use-test, bit7=inserted */
    uint8_t hc = r8(base + SDHC_HOST_CTRL);
    w8(base + SDHC_HOST_CTRL, hc | 0xC0);

    w8(base + SDHC_TIMEOUT_CTRL, 0x0E);

    /* Power sequence: INTCLK -> wait STABLE -> voltage -> power -> SDCLK -> >=1ms */
    set_intclk(base, 50_000_000u, 400_000u);     /* divisor is N-of-8 power-of-2 */
    w8 (base + SDHC_POWER_CTRL, PWR_33V);
    busy_us(100);
    w8 (base + SDHC_POWER_CTRL, PWR_33V | PWR_ON);
    busy_us(5000);
    enable_sdclk(base);                          /* sets CLK_SDCLK_EN */
    busy_us(2000);
    return SDHC_OK;
}

/* compute_div: 0->no div, then ONLY-ONE-BIT-SET: 1,2,4,8,...,0x80 → /2,/4,/8,...,/256
 * set_intclk: write divisor + INTCLK_EN, then poll CLK_INTCLK_STABLE before continuing */
```

Important things from the working ARM driver and from earlier notes:
- SDHC 2.0 CLOCK_CTRL[15:8] is **N-of-8 (power-of-two) encoding**: 0x01=/2, 0x02=/4, 0x04=/8, …, 0x80=/256. Writing arbitrary integers gets silently misinterpreted.
- After any command error, you **must** software-reset the CMD line (SW_RESET bit 1) or CMD_INHIBIT_CMD sticks and the next command hangs.
- Power sequence is INTCLK → INTCLK_STABLE poll → voltage select → power on → SDCLK enable → ≥1 ms settle before first CMD. Skipping the STABLE poll can leave SDCLK gated.
- The ARM driver does **not** issue `POWER_CTRL` writes from the RISC-V side, because power and pinmux were configured by `ps7_init`. So when RISC-V re-runs `SW_RESET_ALL`, it kicks the controller back to a state that needs voltage/SDCLK reprogramming — but a partial reprogram may leave SDCLK off.
- The Zynq SDIO ref clock is **IO PLL / 20 = 50 MHz**, configured in the SLCR (ARM address `0xF8000000` range). The SLCR is **not** in the `ps_iop` window (which only covers `0xE0000000–0xE0300000` ARM = `0x80000000–0x80300000` NaxRiscv). So from RISC-V we can talk to the SDHC controller but we **cannot** reconfigure the upstream ref clock. We rely on `ps7_init` having left it at 50 MHz.

## What state the controller is probably in when sd_rw starts

- ARM FSBL ran `ps7_init` (MIO40–47 pinmux for SDIO0, SDIO_CLK_CTRL = 0x1403 → IOPLL/20 = 50 MHz).
- ARM FSBL ran `sdhc_controller_init` + `sdhc_card_init` against the same SDHC0 base. It got to CMD7 (SELECT_CARD) and then read FAT sectors. So the card was previously in "transfer" state with bus-width-4, 25 MHz SDCLK.
- ARM jumped to NaxRiscv. PCAP programmed the PL bitstream. `ps7_post_config` ran.
- Linux booted on NaxRiscv. The PS SDHC controller has been idle since the ARM finished using it.
- Now `sd_rw` re-issues `SW_RESET_ALL` and tries to re-init from scratch.

## My hypotheses (ranked)

1. **CLOCK_CTRL is wrong**: writing `0x0101` later as "switch to /2 = 25 MHz" sets divisor=1 + INTCLK_EN but **clears SDCLK_EN**, so SDCLK goes off and all subsequent commands time out. Also, no INTCLK_STABLE polling.
2. **POWER_CTRL was reset to 0** by `w8(0x2F, 0x07)` (SW_RESET_ALL), and `sd_rw` never re-programs `POWER_CTRL` (offset 0x29 in `sd_rw` is being treated as `HOST_CTRL`, not `POWER_CTRL` — the real `POWER_CTRL` is at 0x29 in the 0x100-aligned SD register block; we may be writing to the wrong field). Need to confirm SDHC 2.0 layout: 0x28 = HOST_CTRL_1, 0x29 = POWER_CTRL, 0x2A = BLOCK_GAP_CTRL, 0x2B = WAKEUP_CTRL, 0x2C = CLOCK_CTRL, 0x2E = TIMEOUT_CTRL, 0x2F = SOFTWARE_RESET. So `w8(0x29, 0x0F)` is writing POWER_CTRL = 0x0F = 0b0000_1111 → reserved bits + voltage 0x07 + power-on? On Zynq, valid voltage values are 0x0E (3.3V) or 0x0A/0x0C; 0x07 looks invalid. The code COMMENT says it's setting HOST_CTRL — that's the bug: it's actually setting POWER_CTRL with garbage.
3. **AXI / IOP write ordering** through the NaxRiscv ps_iop bridge may coalesce or reorder 8/16-bit writes to the controller. Some SDHC 2.0 hosts require strict-ordered MMIO and a single 32-bit write of `[TRANSFER_MODE | COMMAND]` (offsets 0x0C/0x0E) — the ARM driver does this as one 32-bit write. `sd_rw` writes only the upper 16 bits at 0x0E, leaving stale TRANSFER_MODE.
4. **Card is in a non-IDLE state**: ARM left the card in transfer mode at 25 MHz with bus-width-4. When RISC-V resets the **controller** with SW_RESET_ALL, the card itself does NOT reset (SD spec — only CMD0 sends GO_IDLE). The card is still in transfer state expecting a clock at 25 MHz, but the controller is now reconfigured to 400 kHz/1-bit. CMD0 at 400 kHz/1-bit should still bring it home, but if SDCLK isn't actually toggling (hypothesis 1), the card never sees CMD0.
5. **Endianness / unaligned access** from NaxRiscv MMIO via ps_iop. Cortex-A9 in MMU-off mode is Strongly Ordered; NaxRiscv is little-endian like A9 so this should be fine, but PIO read of the 32-bit BUFFER_DATA register at offset 0x20 might need a 32-bit access only — `sd_rw` is OK on that.

## What I want from you

1. Which hypothesis is most likely? Are there others I'm missing?
2. What's the minimum 5–10 lines of debug code I should add to print the SDHC register state right after the SW_RESET_ALL completes and right before CMD0, so we can confirm what's actually configured? Specifically: `PRESENT_STATE (0x24)`, `POWER_CTRL (0x29)`, `CLOCK_CTRL (0x2C)`, `INT_STATUS_EN (0x34)`, `HOST_CTRL_1 (0x28)`, `TIMEOUT_CTRL (0x2E)`.
3. What SDHC 2.0 register dump would tell us whether SDCLK is actually toggling vs. just INTCLK?
4. Give me the minimal patch to `sd_rw.c`'s `card_init` that brings it inline with the ARM driver's known-good init sequence (power-of-2 divisor, INTCLK_STABLE poll, voltage select, power on, SDCLK enable, ≥1 ms settle, then CMD0).

Please prioritize **what to print** and **what to test next**, not a full rewrite, because each iteration requires power-cycling the board, JTAG-loading 8MB of files, booting Linux, and copying the new binary — about 5 minutes minimum per try.
