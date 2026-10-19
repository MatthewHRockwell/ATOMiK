# Session resumption — ATOMiK SD-boot, DDR corruption + Linux kernel panic

**Paste this entire file as your first message after reboot. I'll pick up exactly where we stopped.**

---

## Where we are right now

End of a multi-day SD-boot bring-up arc on the AX7020 (HamGeek RK-ZYNQ7020-F, Zynq-7000 XC7Z020-2CLG484-2). The architectural milestone is **committed and working**: a custom migen module `PSUart0Bridge` proxies the LiteX UART CSR (`CSR_UART_BASE` at `0xF0001000`) transparently to Zynq PS UART0 (ARM `0xE0000000` via the `ps_iop` GP1 slave), so LiteX BIOS, OpenSBI, and Linux's `liteuart` driver all reach the host UART (`/dev/ttyUSB2` @ 115200) without per-software patches.

**Latest commits (on `main`):**
- `ff9ac12` — `hardware: PSUart0Bridge quiet-idle — full Linux boot trace captured`
- `fcc7589` — `hardware: PSUart0Bridge — LiteX UART CSR proxied to Zynq PS UART0`
- `96ad945` — earlier libbase/uart.c patch (now reverted in favor of the bridge)

Verify with `git log --oneline -10`.

## What's working
- BootROM → FSBL → PCAP → ps7_post_config → bitstream loaded → NaxRiscv runs LiteX BIOS
- BIOS prints full banner: ASCII logo, CPU `NaxRiscv 64-bit @ 100MHz`, memtest, console prompt
- BIOS interactive (typed `help` → got full command list)
- `boot 0x40f00000` → `Liftoff!` → OpenSBI banner → Linux kernel prints copiously
- Captured 7942 bytes of clean Linux kernel boot trace on `/dev/ttyUSB2 @ 115200`
- ALL of this with upstream LiteX BIOS / upstream OpenSBI / upstream Linux liteuart driver — zero per-software UART patches

## What's broken
Two specific issues, almost certainly the same root cause:

### 1. BIOS memtest reports 16 data errors / 524288 reads
At `0x40000000` (NaxRiscv main_ram, routed via GP0 → PS DDR `0x00100000`):
```
Memtest at 0x40000000 (2.0MiB)...
  bus errors:  0/256
  addr errors: 0/8192
  data errors: 16/524288
Memtest KO
Memspeed:
  Write speed: 7.2MiB/s
   Read speed: 13.1MiB/s
Memory initialization failed
```
- Pattern: **524288 / 16 = 32768 = 2^15 = one error per 128 KiB**. This is suspicious, not random — points to alignment / cache-line / arbiter boundary.

### 2. Linux kernel panic at `__kernfs_remove+0x86`
After manual `boot 0x40f00000` from BIOS, OpenSBI hands off to kernel. Kernel runs early init, registers PF_NETLINK, allocates DMA pools, then crashes:
```
[    0.300056] NET: Registered PF_NETLINK/PF_ROUTE protocol family
[    0.311751] DMA: preallocated 128 KiB GFP_KERNEL pool for atomic allocations
[    0.318378] DMA: preallocated 128 KiB GFP_KERNEL|GFP_DMA32 pool for atomic allocations
[    0.416528] Oops - illegal instruction [#1]
[    0.418126] CPU: 0 PID: 1 Comm: swapper Not tainted 6.1.0-rc2-g247f34f7b803-dirty #14
[    0.428959] epc : __kernfs_remove.part.0+0x86/0x1b4
[    0.433921]  ra : kernfs_remove_by_name_ns+0x5a/0xbc
[    0.515989] status: 0000000200000120 badaddr: 0000000000000000 cause: 0000000000000002
[    0.523994] [<ffffffff80101108>] kernfs_remove_by_name_ns+0x5a/0xbc
[    0.530294] [<ffffffff8010365e>] remove_files+0x24/0x52
[    0.535441] [<ffffffff801039c4>] sysfs_remove_group+0x2e/0x72
[    0.541253] [<ffffffff8041f900>] param_sysfs_init+0x14a/0x302
[    0.546936] [<ffffffff8041a492>] do_one_initcall+0x68/0x118
[    0.552531] [<ffffffff8041a702>] kernel_init_freeable+0x168/0x1bc
[    0.558544] [<ffffffff80414bd2>] kernel_init+0x1a/0xe2
[    0.563629] [<ffffffff80001efc>] ret_from_exception+0x0/0xc
[    0.574403] Kernel panic - not syncing: Attempted to kill init! exitcode=0x0000000b
```
- `cause: 0x2` = RISC-V illegal instruction trap.
- `badaddr: 0` — NaxRiscv didn't populate stval (allowed by spec, but unhelpful).
- Kernel ISA `acdfim` (rv64imafdc + Zicsr/Zifencei implicit). DTS declares `rv64imafdc`. Consistent.
- `__kernfs_remove` works on countless other RISC-V Linux boots. Almost certainly the kernel CODE/DATA was corrupted in DDR after load, and dereferencing a flipped pointer landed on a non-instruction.

## Hypotheses, ranked by likelihood

### Hyp B (most likely) — GP0 32-bit AXI vs NaxRiscv 64-bit L2 cache-line tearing
NaxRiscv has a 32 KB L2 cache, 64-byte lines (`NaxRiscv.l2_bytes = 32 * 1024, NaxRiscv.l2_ways = 4`). When it refills a line it issues a burst of 16 × 32-bit beats on GP0 (which is 32-bit AXI). If the central wishbone interconnect inserts a 1-beat GP1 access from our PSUart0Bridge during NaxRiscv's burst, the burst could desync.

128 KiB = 2048 cache lines. 1 error per 128 KiB = ~0.05% per cache-line refill. Plausible for an arbiter-induced glitch.

### Hyp A — bus-arbiter race between bridge poll + NaxRiscv DDR access
Bridge polls PS UART0 SR every `poll_period=10_000` cycles (~100 µs at 100 MHz) when idle. Memtest runs ~430 ms total → ~4300 polls. 16 errors / 4300 = 0.4%. Also plausible.

### Hyp C — pre-existing DDR config issue
Maybe ps7_init has DDR timing slightly off and 16 errors / 2 MiB is the baseline regardless of our bridge. Harder to believe (factory image and earlier ATOMiK builds didn't show this), but possible.

## Four candidate next moves

1. **Test A: silent bridge** — Set `poll_period` effectively infinite (e.g. `0xFFFFFFFF`) so the FSM never polls. Edit `hardware/zynq/litex/ps_uart0_bridge.py`, rebuild bitstream, reflash, rerun memtest. If errors → 0, it's arbiter contention; fix is to keep bridge silent except on software trigger.
2. **Test B: disable L2 cache** — Set `NaxRiscv.l2_bytes = 0` (or remove the L2 entirely) in `hardware/zynq/litex/soc_nax64_atomik.py`. Forces NaxRiscv to issue single-word accesses on GP0 instead of cache-line bursts. Rebuild, reflash, retest. If errors → 0, it's L2 burst tearing on 32-bit GP0; fix is to switch DDR path to AXI HP (64-bit) slave.
3. **Test C: revert bridge** — Build a version that doesn't add the bridge (`uart_name="stub"` removed, no PSUart0Bridge, no bus master added). If memtest STILL shows 16 errors, the bridge isn't the cause and ps7_init / NaxRiscv config is suspect.
4. **Skip DDR for now** — Workaround the kernel panic by trying a smaller kernel config / different initramfs path. Doesn't fix the root cause but may unblock further demo work.

## Files relevant to the next steps

| Path | Purpose |
|---|---|
| `hardware/zynq/litex/ps_uart0_bridge.py` | The migen bridge module (poll_period parameter) |
| `hardware/zynq/litex/soc_nax64_atomik.py` | LiteX SoC target — has `NaxRiscv.l2_bytes`, `NaxRiscv.l2_ways`, and the `PSUart0Bridge` integration |
| `hardware/zynq/fsbl_build/minimal_fsbl/fsbl_main.c` | ARM FSBL — initializes PS UART0 + writes "[FSBL] booting" |
| `hardware/zynq/fsbl_build/run_sd_strict_fat32.py` | Drives JTAG-side SD card flash |
| `hardware/zynq/litex-build-nax64-sdboot/` | Current bitstream build output |
| `hardware/zynq/litex-build/Image_nax64` etc. | Linux kernel + dtb + initramfs used by FSBL boot |

## Build command (reference)
```bash
cd hardware/zynq/litex
source /opt/Xilinx/2025.2/Vivado/settings64.sh
/home/mattrock/Projects/ATOMiK/.venv/bin/python soc_nax64_atomik.py \
    --build --uart-baudrate=115200 \
    --output-dir=../litex-build-nax64-sdboot
```
Takes ~16-20 min for Vivado synth+impl+bitgen. Always use the venv Python so `pythondata-cpu-naxriscv` is importable.

## Reflash + boot procedure (reference)
```bash
# After bitstream rebuild, stage FAT image:
# (use the same staging block from before — mkfs.vfat 512MB, copy
#  BOOT.bin + nax64.bit.bin + Image_nax64 + dtb + cpio.gz, sparse-extract)
# Then with switches at JTAG (00):
python3 hardware/zynq/fsbl_build/run_sd_strict_fat32.py
# Then physically:
#   1. Power off
#   2. Switches to SD mode (11)
#   3. Power on
#   4. Listen on /dev/ttyUSB2 @ 115200
```

ttyUSB layout on this host (after `sudo modprobe -r ftdi_sio usbserial && sudo modprobe ftdi_sio`):
- `ttyUSB0` = FT2232H interface 0 (JTAG when libusb-claimed)
- `ttyUSB1` = FT232 single-channel (NOT the UART path — common misroute)
- `ttyUSB2` = FT2232H interface 1 = PS UART0 on MIO10/11 (THE UART path)

## What I (Claude) should do when this file lands

1. Read this file completely; do not skim.
2. Run `git log --oneline -10` to verify the bridge commits are present.
3. Read `hardware/zynq/litex/ps_uart0_bridge.py` and `hardware/zynq/litex/soc_nax64_atomik.py` to refresh context.
4. Re-present the **four candidate next moves** above to the user via `AskUserQuestion`, but confirm the user wants to pursue DDR debug vs another priority first.
5. Whatever the user picks, implement, rebuild, ask user to power-cycle / flip switches / replug, capture output.

## What NOT to do
- Don't suggest stopping for the day. The user explicitly said "We wrap up when I say so, not when you do."
- Don't propose "pulling the SD card and writing from the laptop" — the laptop has no SD slot.
- Don't switch jumpers without telling the user what physical position to set.
- Don't run repeated short xsdb probes — they destabilize the FT232 USB enumeration on this board. Use long-running listeners and ONE xsdb probe per cycle.

## ChatGPT prompt
There's a companion file `hardware/zynq/CHATGPT_DDR_CORRUPTION_DEBUG.md` with a self-contained prompt for ChatGPT. Drop that into ChatGPT for a second opinion on the four hypotheses before iterating.
