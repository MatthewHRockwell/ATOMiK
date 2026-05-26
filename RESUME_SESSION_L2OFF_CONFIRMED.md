# Session resumption — L2-off confirmed, BOOT_l2off.bin write pending

**Paste this file as the first message after reboot.**

---

## Core result (CONFIRMED)

**Test B: l2_bytes=0 definitively fixes both the DDR corruption and the kernel panic.**

| Config | Memtest | Linux |
|---|---|---|
| 32KB L2 (baseline) | 16/524288 errors | Panic at `__kernfs_remove+0x86` at 416ms |
| L2 disabled (l2_bytes=0) | **0 errors (Memtest OK)** | **Full boot to root shell** |

Root cause: NaxRiscv 64-byte L2 cache-line burst refills through the 32-bit GP0 AXI
slave tear DDR on the boundary beats. Single-word accesses (L2 off) eliminate the
tearing. Fix path: move `main_ram` to dedicated 64-bit AXI HP port (HP1).

ChatGPT's Hypothesis B was correct. Hypothesis C (ps7_init DDR timing) is ruled out.

---

## What was done this session

1. Built `litex-build-nax64-l2off/` bitstream with `l2_bytes=0`
2. Built `BOOT_l2off.bin` (FSBL + l2off bitstream + ps_loader.elf)
3. Booted Linux with l2off via JTAG PL reprogram:
   - SD boot → ps_loader loads Image69/fw_jump69/rootfs to DDR → BIOS prompt
   - JTAG `fpga -file litex-build-nax64-l2off/gateware/hamgeek_rk7020f.bit`
   - NaxRiscv restarts with l2off → BIOS memtest → **Memtest OK**
   - JTAG non-intrusive reload of Image69/dtb/opensbi/rootfs to DDR
   - `boot 0x40f00000` → OpenSBI → **Linux boots to root shell**
4. Committed result at `fc4ea94`

## What's NOT done yet

**BOOT_l2off.bin has not been written to the SD card FAT partition.**

The board currently boots via the existing BOOT.bin (32KB L2 bitstream). To make
l2off the default boot, BOOT_l2off.bin must replace BOOT.BIN on the SD card.

## Why SD write is pending

`sd_boot_image_writer.elf` (ARM-side SD writer) has been debugged extensively but
still fails. The issue is `CMD7 CMD_TIMEOUT_ERR` on every attempt — the SD card
doesn't respond to CMD7 after our ELF re-initializes the SDHC controller.

Key facts:
- ps_loader's `sdhc_card_init` works (it reads files from SD successfully)
- Our ELF's `sdhc_card_init` fails at ACMD41 (ETIMEOUT after 10s loop)
- `sdhc_card_reselect` (skip CMD0/CMD8/ACMD41) fails at CMD7 TIMEOUT

Current `sdhc.c` state (from last commit):
- `sdhc_card_init`: no CMD+DAT reset at top; clock restore after cmd_line_reset in CMD8 wrong-echo handler
- `sdhc_card_reselect`: SW_RESET_CMD + clock restore + skip CMD7 + return OK (not committed yet — but last working approach was "skip CMD7, go straight to writes")
- `sd_boot_image_writer.c`: full `sdhc_controller_init + sdhc_card_init` path

## Best next approach to write BOOT_l2off.bin to SD

**Option A (most reliable): Run test again via JTAG PL reprogram → boot Linux → use a RISC-V
sd_write program** that accesses PS SDIO0 via NaxRiscv 0x80100000 (/dev/mem).

**Option B: Fix the sdhc_card_init ETIMEOUT.**
The ETIMEOUT at ACMD41 might be because the card is NOT going to IDLE after CMD0.
After `sdhc_controller_init_default()` toggles POWER_CTRL (which may not actually
power-cycle the card on this board), the card stays in Transfer state. CMD0 is sent
(no response expected), and the card DOES go to IDLE. But then ACMD41's 1000-iteration
loop (10 seconds max) exhausts without bit31=1.

Suspicion: `busy_us` calibration at ARM speed means each 10ms sleep is much shorter,
so ACMD41 only polls for ~1-2 seconds, not 10. Or: after the clock stoppage during
`sdhc_controller_init_default`, the card needs more settle time.

**Simplest fix to try**: Increase ACMD41 iterations from 1000 to 10000, or add
`busy_us(500000)` (500ms) after `sdhc_controller_init_default` before `sdhc_card_init`.

**Option C (brute force)**: Implement minimal SDHC write from the NaxRiscv Linux shell
via /dev/mem at NaxRiscv 0x80100000 (PS SDIO0). The card is in Transfer state (from
ps_loader). A bare-metal-style Python script could write blocks directly.

## File locations

| File | Purpose |
|---|---|
| `hardware/zynq/litex-build-nax64-l2off/gateware/hamgeek_rk7020f.bit` | L2-off PL bitstream |
| `hardware/zynq/fsbl_build/BOOT_l2off.bin` | L2-off BOOT.bin (to write to SD) |
| `hardware/zynq/fsbl_build/boot_l2off.bif` | BIF for bootgen |
| `hardware/zynq/ps_loader/sdhc.c` | Modified SDHC driver |
| `hardware/zynq/ps_loader/sd_boot_image_writer.c` | SD write ELF |

## SD card layout (for direct write if needed)

- MBR: `/tmp/sd_meta_strict_mbr.bin` → sector 0 (512 bytes)
- FAT metadata: `/tmp/sd_strict_boot_meta_patched.bin` → sector 8192 (1,069,056 bytes)
- BOOT.BIN: `BOOT_l2off.bin` → sector 10280 (4,071,764 bytes)

Regenerate these if /tmp is cleared:
```bash
# Rebuild MBR
python3 -c "
import struct
mbr = bytearray(512)
o = 0x1BE
mbr[o+0] = 0x80; mbr[o+4] = 0x0C
struct.pack_into('<I', mbr, o+8, 8192)
struct.pack_into('<I', mbr, o+12, 1048576)
mbr[0x1FE] = 0x55; mbr[0x1FF] = 0xAA
open('/tmp/sd_meta_strict_mbr.bin','wb').write(mbr)
print('MBR written')
"
# Rebuild partition image (for FAT metadata)
mkdir -p /tmp/fat_final
truncate -s 512M /tmp/fat_final/strict_partition.bin
mkfs.vfat -F 32 -s 8 -h 8192 /tmp/fat_final/strict_partition.bin
mcopy -i /tmp/fat_final/strict_partition.bin hardware/zynq/fsbl_build/BOOT.bin ::BOOT.BIN
# Then run swap script dry-run to generate META_PATCHED:
python3 hardware/zynq/fsbl_build/run_sd_strict_boot_swap.py hardware/zynq/fsbl_build/BOOT_l2off.bin --dry-run
```

## Next session recommended path

1. Verify the key result: run the `fpga -file l2off.bit` + JTAG kernel reload + boot 
   sequence to confirm the l2off Linux boot (reference this file for the script).

2. Write BOOT_l2off.bin to SD using Option B (fix ACMD41 loop) or Option C (direct SDIO
   access from Linux shell).

3. After SD write, boot from SD (switches 11) to verify the permanent l2off boot.

4. Then tackle the actual fix: move `main_ram` to AXI HP1 (64-bit) to eliminate the
   burst tearing while keeping L2 enabled for performance.
