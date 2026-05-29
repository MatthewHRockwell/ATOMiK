# Standalone Boot Plan — QSPI(FSBL) + SD(payload) Hybrid

**Date:** 2026-05-29
**Why:** SD boot fails at BootROM with `0x200A` — BootROM cannot cold-init this SD
card (see `SD_BOOT_0x200A_DIAGNOSIS.md`). Image/FAT are provably correct. QSPI
BootROM init is deterministic (factory QSPI boots). So boot the FSBL from QSPI and
let our (working) FSBL SD driver load the rest from SD.

## Why not pure QSPI

Total payload = bitstream 3.9 MB + kernel 7.4 MB + fw_jump 0.13 MB + dtb 3 KB +
**Ubuntu rootfs 31 MB** ≈ 42 MB > 32 MB QSPI. Ubuntu must stay (feedback_no_compromises),
so the rootfs can't move to QSPI. Only the 15 KB FSBL goes to QSPI.

## Boot chain

```
POR, BOOT[1:0]=10 (QSPI)
  └─ BootROM reads QSPI @0x0 → copies 15 KB FSBL to OCM, jumps   [the part SD couldn't do]
       └─ FSBL (minimal_fsbl/fsbl.elf): ps7_init → UART0 → SD init (OUR driver)
            ├─ read nax64.bit.bin        → PCAP program PL (NaxRiscv + HDMI FB)
            ├─ read fw_jump_nax64.bin    → DDR 0x01000000
            ├─ read linux_nax64.dtb      → DDR 0x00FF0000
            ├─ read ubuntu_rv64.cpio.gz  → DDR 0x02100000   (31 MB)
            ├─ read Image_nax64          → DDR 0x00100000
            └─ write DONE_MAGIC @0x10100000
                 └─ NaxRiscv LiteX BIOS (now live in PL) auto-boots Ubuntu → HDMI
```

## Artifacts (ready)

| Artifact | Path | Notes |
|----------|------|-------|
| QSPI image | `fsbl_build/BOOT_fsbl_only.bin` | 21 KB, image_count=1, FSBL-only, header+cksum verified |
| Driver FSBL | `fsbl_build/minimal_fsbl/fsbl.elf` | our board ps7_init (CLG484) |
| Flash script | `fsbl_build/flash_qspi_fsbl.py` | `program_flash -flash_type qspi_single` wrapper |
| Bitstream (HDMI) | `litex-build-nax64-l2off-fb/gateware/hamgeek_rk7020f.bit.bin` | May 28, L2-off + HDMI FB |

## SD card contents required (FAT root) — NOT BOOT.bin anymore

| File on SD | Source |
|------------|--------|
| `nax64.bit.bin` | copy of `litex-build-nax64-l2off-fb/gateware/hamgeek_rk7020f.bit.bin` (rename!) |
| `fw_jump_nax64.bin` | `litex-build/fw_jump_nax64.bin` |
| `linux_nax64.dtb` | `litex-build/linux_nax64.dtb` |
| `ubuntu_rv64.cpio.gz` | `litex-build/ubuntu_rv64.cpio.gz` (31 MB) |
| `Image_nax64` | `litex-build/Image_nax64` (7.4 MB) |

⚠️ The FSBL hard-codes the name `nax64.bit.bin` (fsbl_main.c:207). Our build tree
only has `hamgeek_rk7020f.bit.bin`, so a renamed copy must be on the SD FAT.
**Verify on-card file list before trusting this** (board step 0).

## Execution (needs board — operator + JTAG)

0. **Verify SD payload.** Boot any working path (JTAG-assisted) or mount on board;
   confirm the 5 files above exist in FAT root with correct names/sizes. Fix names
   if `nax64.bit.bin` is absent.
1. **Start hw_server** (or Vivado Hardware Manager). Straps can stay JTAG (00) for
   flashing — program_flash drives the PS over JTAG.
   ```
   source /opt/Xilinx/2025.2/Vivado/settings64.sh && hw_server &
   ```
2. **Flash QSPI:**
   ```
   python3 fsbl_build/flash_qspi_fsbl.py
   ```
   If program_flash rejects our FSBL (handshake/timeout), build a ps7_init-only
   stub FSBL and pass `--fsbl <stub.elf>`. If QSPI read/verify errors, try an
   explicit 256 Mbit part: `--flash-type mt25ql256-spi-x1_x2_x4` (or s25fl256s).
3. **Set straps BOOT[1:0]=10 (QSPI).** Keep SD inserted.
4. **Cold power-cycle** per `SD_BOOT_COLD_POWER_CYCLE.md`.
5. **Probe:**
   ```
   python3 fsbl_build/probe_sd_boot_pc.py
   ```
   Expect: REBOOT_STATUS POR set, **OCM @0x0 no longer all-zero** (FSBL loaded),
   `[FSBL] ATOMiK minimal FSBL booting` on /dev/ttyUSB1, then NaxRiscv BIOS banner
   on the BIOS UART, then Ubuntu → HDMI.

## Key risk / open question

The hybrid assumes **our FSBL's SD driver can cold-init the card where the BootROM
cannot.** This is the standard Zynq workaround pattern and our `sd_verify`/`sdhc.c`
lineage reads the card reliably — but it has only ever run via JTAG-warm so far. If
our FSBL *also* fails SD cold-init, the next fallback is a busybox initramfs on QSPI
that pivots to the SD Ubuntu rootfs (more work; defer).

## Fallback for demos meanwhile

JTAG-assisted 95 s boot (`fsbl_build/jtag_load_all_then_boot.py`) remains the
validated tethered path. Standalone QSPI is the untethered-demo goal.
