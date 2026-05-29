# SD Boot Failure — Definitive Diagnosis (0x200A)

**Date:** 2026-05-29
**Status:** Data path PROVEN CORRECT. Failure is BootROM↔SD-card cold-init, not the image.
**Supersedes the debug direction in:** Codex SD-boot protocol ("full BOOT.bin structural
diff vs ALINX reference"). That path is a **dead end** — the BOOT.bin is correct.

## The smoking gun

After a true cold POR (per `SD_BOOT_COLD_POWER_CYCLE.md`), the no-reset probe reports
`REBOOT_STATUS = 0x0040200A`. Authoritative decode (UG585, slcr 0x258):

| Bits | Field | Value | Meaning |
|------|-------|-------|---------|
| 22 | `POR` | **1** | Last reset WAS a true power-on reset → **cold cycle worked** |
| 15:0 | `BOOTROM_ERROR_CODE` | **0x200A** | BootROM ran, tried SD, recorded this error |

**UG585 Table 6-18, error `0x200A`:**
> "SD card boot mode. The BootROM could not find the boot image at the root of the SD
> card; only a single boot image is supported for this boot mode."
> Solutions: valid Boot Header named BOOT.BIN in root; **SD interface operating reliably**;
> **SD card in 3-byte addressing mode**; check mode pins.
> Note: *"If the SD card was accessed by the FSBL/User code and then a system reset occurs
> without resetting the SD card, then the SD card might be left in 4-byte addressing mode."*

So the BootROM did NOT silently skip SD — it initialized SD, walked the FAT, and could
not obtain a usable boot image. The error is in the **SD read interface**, not the data.

## Data path — exhaustively verified correct (board-free)

Tool: `fsbl_build/boot_struct_diff.py` + direct parse of the on-card 512 MB image
(`/tmp/fat_final/strict_partition.bin`) and MBR builder.

| Layer | Check | Result |
|-------|-------|--------|
| BOOT.bin header | magic 0xAA995566 @0x20, XLNX @0x24, checksum @0x48 | ✓ matches working ALINX SD-boot ref byte-for-byte (only legit FSBL-size fields differ) |
| BOOT.bin partitions | 3 PHs, all checksums, load/exec/offsets | ✓ all PHT checksums OK |
| Register-init table @0xA0 | terminator | ✓ empty (terminator only), same as ref |
| FAT32 geometry | cluster_count | **130811 ≥ 65525** → true FAT32, BootROM-accepted |
| FAT32 BPB | bps=512, spc=8, 2 FATs, sig 0xAA55 | ✓ |
| MBR | type **0x0C** (FAT32 LBA), active 0x80, start LBA 8192, size, sig 0xAA55 | ✓ |
| Root directory | **single** entry, 8.3 = `BOOT    BIN` (424f4f542020202042494e), attr 0x20 (ARC), firstClus 3, size 4071764 | ✓ exactly what BootROM requires |

Every byte the BootROM's FAT parser needs is correct.

## Why "SD boot worked" before but fails now (reconciled)

Memory said SD boot worked (captured `[FSBL] booting`). Commit `a10aa44` body reveals how:
**"After SD-boot reflash + `rst -processor` + con, FSBL UART output [captured]"**.

- `rst -processor` is a **JTAG warm reset**, NOT a cold POR. The SD card stayed powered and
  was already initialized by the ARM-side writer/verify tools (`sd_boot_image_writer.elf`,
  `sd_verify.elf`). BootROM then read it fine.
- A **true cold POR** power-cycles the card; BootROM must cold-initialize it from scratch
  → `0x200A`.

**Conclusion:** prior "standalone SD boot" successes were JTAG-warm-assisted, not true
cold-from-power-on. True standalone cold SD boot has never actually been demonstrated on
this card. (Refines `project_sd_boot_uart_working.md`, `project_sd_boot_2026_05_25_handoff.md`.)

## Ruled out (do NOT re-investigate)

- ❌ BOOT.bin header / partition / checksum construction (correct)
- ❌ FAT32 cluster count < 65525 (it's 130811; that earlier bug is fixed)
- ❌ Filename / 8.3 / LFN / extra-files in root (single clean `BOOT    BIN`)
- ❌ MBR partition type / location (0x0C @ LBA 8192)
- ❌ Bitstream swap (BOOT.bin→BOOT_l2off.bin differ only inside PL partition bytes;
      cannot affect FSBL *find*)
- ❌ Cold cycle "not truly cold" (POR bit 22 confirms a real power-on reset)

## Failure domain (what 0x200A actually points to)

The BootROM cannot reliably cold-initialize / read THIS SD card. Real-world Zynq causes:
1. **Card compatibility** — BootROM SD driver is stricter/slower than Linux/FSBL drivers;
   some SDHC/SDXC cards or brands fail BootROM cold-init while working fine under Linux.
2. **Card left in non-default addressing** by ARM tooling before the cycle (mitigated by
   true POR, but worth a clean re-init).
3. **SD signal integrity / clock** at BootROM's default SD speed on this board's MIO40-47.

## Recommended next steps (require board + operator cold cycle — do post-pitch)

1. **Reproduce + confirm:** true cold cycle, run `probe_sd_boot_pc.py`, confirm
   `REBOOT_STATUS[15:0]==0x200A` is stable.
2. **Strongest standalone fix — pivot to QSPI boot.** Factory QSPI image boots cleanly
   (`project_factory_image_works.md`); BootROM QSPI init is far more deterministic than SD.
   Needs image diet to fit flash (BOOT.bin is 4 MB). This is the most likely route to a
   true no-laptop demo boot.
3. **If staying on SD:** low-level re-format card fresh (mkfs.vfat geometry already proven),
   write BOOT.bin, do NOT touch with ARM tools afterward, then cold POR. If a second
   known-good card is already on hand, A/B it (do NOT buy hardware).
4. **For demo continuity meanwhile:** the JTAG-assisted 95 s boot is the validated path;
   keep it working as fallback (see `jtag_load_all_then_boot.py`, separate issue).

## Reproduction (board-free)

```
python3 fsbl_build/boot_struct_diff.py fsbl_build/BOOT_l2off.bin \
  "alinx_reference/course_s6_linux/1.字符设备/sd_boot/BOOT.BIN"
```
