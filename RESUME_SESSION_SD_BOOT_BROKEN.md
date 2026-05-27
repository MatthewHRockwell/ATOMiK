# Session resumption — SD card boot broken, L2 cache fix proven

**Paste this file as first message. Critical context.**

---

## State of the board RIGHT NOW

**SD boot is broken.** The board powers on but produces NO serial output. The Zynq
BootROM falls back to JTAG mode (ARM at ~0x984 BootROM JTAG wait loop). FSBL never
runs. This happened after writing `BOOT_l2off.bin` (then restoring `BOOT.bin`) to the
SD card via `sd_boot_image_writer.elf`. The writes all reported DONE_MAGIC (success),
but the SD card is no longer bootable.

**Verified:** FAT32 structure is correct (MBR, BPB, FAT tables, dir entry, BOOT.BIN at
cluster 3 = LBA 10280, cluster_count=130,811 > 65,525). Everything looks valid. The
BootROM silently rejects it anyway.

**openFPGALoader** can see the JTAG chain (ARM Cortex-A9 + xc7z020). The board IS powered.
**Vivado hw_server** shows empty targets (FT2232H enumeration issue, separate problem from SD).

## Key result achieved THIS session (commit fc4ea94)

**L2 cache = confirmed root cause of both bugs:**
- 32KB L2 + 32-bit GP0 burst → DDR corruption → 16/524288 memtest errors → kernel panic
- L2-off (l2_bytes=0) → Memtest OK → Linux boots to root shell, no panic

This was proven by JTAG-reprogramming the PL with the l2off bitstream (while in SD boot
mode, FSBL had run and enabled level shifters). The BIOS printed "Memtest OK" and Linux
booted fully.

## Files on disk

| File | Status |
|---|---|
| `hardware/zynq/fsbl_build/BOOT.bin` | Original working BOOT.bin (sdboot bitstream) |
| `hardware/zynq/fsbl_build/BOOT_l2off.bin` | L2-off BOOT.bin (what we want on SD) |
| `hardware/zynq/litex-build-nax64-l2off/gateware/hamgeek_rk7020f.bit` | L2-off bitstream |
| `hardware/zynq/litex-build-nax64-sdboot/gateware/hamgeek_rk7020f.bit` | Working sdboot bitstream |
| `/tmp/fat_final/strict_partition.bin` | 512MB FAT32 image (fresh, cluster_count=130811) |
| `/tmp/sd_meta_strict_mbr.bin` | MBR (partition at LBA 8192) |
| `/tmp/sd_strict_boot_meta_patched.bin` | FAT metadata (BOOT.BIN at cluster 3 = LBA 10280) |

## What's on the SD card right now

After sd_boot_image_writer (succeeded but broke boot):
- Sector 0: New MBR (partition type 0x0C, LBA 8192, size 1048576)
- Sectors 8192-10279: New FAT metadata (reserved=32, spf=1024, BOOT.BIN at cluster 3)
- Sectors 10280+: BOOT.bin (original, 4,071,764 bytes)

## Diagnostic paths to try

### Path 1: ChatGPT diagnostic
See `hardware/zynq/CHATGPT_SD_BOOT_SILENT.md` for a detailed prompt. Key question: why
does a FAT32-valid SD card fail to boot on Zynq BootROM after being rewritten by a
bare-metal ARM SDHC writer?

### Path 2: Verify actual SD card data via ARM program
Load a small ARM diagnostic to OCM (0xFFFC0000 — OCM is always accessible) via JTAG,
execute it to read sectors 0 and 8192 from SDIO0, store in DDR, read back via JTAG.
This would confirm whether the data on the card actually matches what we wrote.

OCM approach: The ARM BootROM is at 0x984 (JTAG wait). OCM at 0xFFFC0000 is accessible.
If we write a small ARM program to OCM and change the PC to point there (via JTAG register
write), the CPU would execute from OCM. The program could read SD sectors and write results
to DDR for inspection.

BUT: Vivado hw_server can't see targets. openFPGALoader sees the chain but can't write
ARM registers. May need urjtag or openocd.

### Path 3: openFPGALoader + sdboot bitstream + JTAG boot
If we can get the ARM to execute ps7_init properly (enabling level shifters), then:
1. `openFPGALoader --fpga-part xc7z020 litex-build-nax64-sdboot/gateware/hamgeek_rk7020f.bit`
2. Load Linux files to DDR via JTAG
3. Boot via BIOS boot command on serial

For this to work with PSUart0Bridge, the level shifters (0xF8000900) must be enabled.
Without FSBL running, they're not. Need ps7_post_config via Vivado TCL (which doesn't
work because hw_server has no targets).

**The level shifter problem:** SW_RESET bit in FPGA_RST_CTRL (0xF8000240) and LVL_SHFTR_EN
(0xF8000900) both need to be set. After `openFPGALoader` programs the PL, these get reset.
Only FSBL's `ps7_post_config` sets them correctly.

### Path 4: Repair Vivado hw_server JTAG detection
The FT2232H presents as "my_product_desc" (vendor="Xilinx"). Vivado hw_server should 
recognize this but currently shows empty targets. Previously this worked.

Try: rebuild Vivado cable driver recognition, or check if FT2232H EEPROM is corrupted
(compare with factory-default 0403:6010 enumeration).

## JTAG tool situation

- **openFPGALoader**: WORKS (detects ARM + xc7z020, can program FPGA)
- **Vivado hw_server / xsdb**: BROKEN (empty targets, can't access ARM or run TCL)
- **Serial port**: ttyUSB2 = FT2232H interface 1 = UART (silent right now)

## For Friday deadline (ATOMiK demo)

The ATOMiK OS desktop demo runs on the existing Linux with the l2off bitstream. The demo
doesn't REQUIRE the SD boot to work — it can be done with JTAG-initiated boot IF:
1. Level shifters get enabled (FSBL must run at least once)
2. openFPGALoader loads l2off bitstream
3. JTAG loads Linux files
4. Serial sends boot command

The cleanest path: fix the SD card boot (restore original BOOT.bin to working SD state),
then use that to enable level shifters, then reprogram with l2off and boot Linux.

## Commands to reproduce the L2-off Linux boot (working path from previous session)

```bash
# From working SD boot state (original BOOT.bin on SD, board in SD mode):
# 1. Board power cycles, ps_loader runs (reads Image69 etc from SD card)
# 2. JTAG: load l2off bitstream (after FSBL ran, level shifters enabled)
BIT="/home/mattrock/Projects/ATOMiK/hardware/zynq/litex-build-nax64-l2off/gateware/hamgeek_rk7020f.bit"
openFPGALoader --fpga-part xc7z020 "$BIT"
# 3. After bitstream loads: NaxRiscv restarts, BIOS runs memtest (OK), BIOS prompt
# 4. Load kernel files (non-intrusive JTAG while BIOS is at prompt):
# -- reload Image_nax64, linux_nax64.dtb, fw_jump_nax64.bin, ubuntu_rv64.cpio.gz --
# 5. Send to BIOS: boot 0x40f00000
# 6. Linux boots without panic!
```

## Port assignments (current)

- ttyUSB0 = FT232R single channel (not UART)
- ttyUSB1 = FT2232H interface 0 (JTAG — claimed by hw_server or ftdi_sio)  
- ttyUSB2 = FT2232H interface 1 (UART = PS UART0 via PSUart0Bridge)
- When hw_server runs: ttyUSB1 disappears (claimed by hw_server)

Run `sudo modprobe ftdi_sio` to restore ttyUSB1/ttyUSB2 after hw_server exits.
