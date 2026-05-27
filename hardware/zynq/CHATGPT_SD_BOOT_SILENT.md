# ChatGPT Diagnostic: Zynq-7000 SD card silent boot after FAT32 rewrite

**Context:** ALINX AX7020 (HamGeek RK-ZYNQ7020-F), XC7Z020-2CLG484-2.
PL contains a LiteX/NaxRiscv RV64 SoC. Boot chain: Zynq BootROM → FSBL (ARM) →
PL bitstream via PCAP → ps_loader → NaxRiscv BIOS.

---

## What was working before

The SD card had a strict FAT32 layout (512MB, 4KB clusters, ~130K clusters) with
`BOOT.BIN` in the root directory at **LBA 10280**. The board booted reliably:
- BootROM read BOOT.BIN from SD card
- FSBL ran and printed `[FSBL] booting` on PS UART0 → FT2232H interface 1
- FSBL loaded bitstream via PCAP, ran ps7_post_config, ran ps_loader
- NaxRiscv BIOS printed to the same UART

## What we did

We wrote a new `BOOT.BIN` to the SD card using a bare-metal ARM program
(`sd_boot_image_writer.elf`) that runs via JTAG. The program:
1. Wrote MBR (512 bytes) to sector 0
2. Wrote FAT32 metadata (1,069,056 bytes = 2088 sectors) to sector 8192
3. Wrote new BOOT.BIN (4,071,764 bytes = 7952 sectors) to sector 10280
4. Reported **STATUS=0xC0DEC0DE** (DONE_MAGIC), **PHASE=0xFF**, **ERR=0** = complete success

The ARM program was run on the Cortex-A9 via JTAG while the board had previously
completed ps_loader initialization (the SDIO0 was known-working from ps_loader which
had successfully read ~38MB of files from the card).

We wrote BOOT_l2off.bin first (different PL bitstream, same FSBL, same size), then
restored the original BOOT.bin. Both writes reported success.

## Current state

After power cycling (switches at SD mode, 11):
- **No serial output on any port** — not even `[FSBL] booting`
- FSBL is NOT running (FSBL always prints very early, before PCAP)
- BootROM is silently failing to read the SD card
- openFPGALoader confirms ARM Cortex-A9 + xc7z020 visible in JTAG chain (board IS powered)
- Vivado hw_server shows empty targets (FT2232H enumeration issue, unrelated)

## FAT32 structure verification (confirmed correct)

The FAT metadata written to the SD card was parsed and verified:
```
MBR:      sig=0xAA55, type=0x0C (FAT32 LBA), partition_lba=8192
FAT BPB:  bps=512, spc=8 (4KB clusters), reserved=32, fats=2, spf=1024
          hidden_sectors=8192, cluster_count=130811 (>65525 ✓)
          sig=0xAA55 ✓
Directory: BOOT    BIN | cluster=3 | size=4071764
Absolute BOOT.BIN LBA: 8192 + 32 + 2*1024 + (3-2)*8 = 10280 ✓
```

This is identical to the previous working configuration.

## Key question

**Why does the BootROM fail to read the SD card even though the FAT32 structure
looks correct?** The arm bare-metal writer reported success, the FAT structure
parses correctly, BOOT.BIN is at the right LBA.

## Additional facts

1. The sd_boot_image_writer re-initialized the SDHC controller
   (`sdhc_controller_init_default()`) which includes a power-cycle of the card
   (POWER_CTRL toggle: 100µs off, 5ms on). Then it did `sdhc_card_init()` which
   runs CMD0→CMD8→ACMD41→CMD2→CMD3→CMD7. The test write+read to sector 0x1f000
   passed, then it wrote MBR/FAT/BOOT.BIN.

2. The `sdhc_card_init()` had a problem: CMD8 returns a wrong echo pattern
   (SDHC_ENOSUPPORT) after controller re-initialization. We patched this to treat
   wrong echo as version2=1 and continue. CMD8 response mismatch is handled.

3. The original SD card setup (from a previous session months ago) used a
   different strict_partition.bin (from `/tmp/fat_quietbridge/`) that is no longer
   available. Our new META_PATCHED was generated from a fresh `mkfs.vfat -F 32
   -s 8 -h 8192` image (512MB, 4KB clusters, hidden_sectors=8192). The FAT
   parameters (reserved=32, spf=1024) may differ from the original setup.

## Specific hypothesis to investigate

**Could the original SD card FAT have had different `reserved_sectors` or
`sectors_per_FAT` values than our new metadata?** If the old partition had
`reserved=64` instead of `32`, then:
- Old data area start: 8192 + 64 + 2*1024 = 10304 sectors
- Old BOOT.BIN sector: 10304 + (3-2)*8 = 10312

But we wrote BOOT.BIN to sector 10280 (based on our new FAT with reserved=32).
The BootROM reads the new BPB (reserved=32, data at 10272+8=10280) and looks at
sector 10280 for BOOT.BIN. But if the old BOOT.BIN was originally at 10312 and we
wrote new BOOT.BIN to 10280, the data IS at 10280 and the BPB says it should be
there — so the BootROM should find it correctly.

**Unless** there is a more subtle issue: the BootROM might be failing on the
FAT validation itself, even though our FAT looks valid. Are there any Zynq-7000
BootROM-specific FAT32 requirements beyond cluster_count >= 65525 that we might
be violating?

## What we need

1. Why would a successful sd_boot_image_writer write (DONE_MAGIC returned) result
   in a non-bootable SD card?
2. Is there any known Zynq-7000 BootROM requirement about FAT32 that we might
   be missing (hidden_sectors, specific reserved count, specific FAT size, etc.)?
3. How can we diagnose what exactly the BootROM is reading/rejecting without
   being able to halt the BootROM via JTAG?
4. Is there a way to restore the SD card to a bootable state without another
   host-side mkfs/dd operation (the board has no SD adapter and we can't pull
   the card)?

## Environment

- Host: Ubuntu 24.04, JTAG via FT2232H (openFPGALoader works, Vivado hw_server
  has enumeration issues)
- SD card: standard SDHC card in the AX7020's PS SDIO0 (MIO40-47, L3_SEL=4)
- Board connects via USB only (no USB-SD adapter available)
