# ChatGPT prompt — SD boot reaches FSBL handoff but LiteX BIOS in freshly-PCAP'd PL is silent

Paste everything below this line.

---

I'm doing standalone SD boot on an ALINX AX7020 (Xilinx Zynq-7000 XC7Z020-2CLG484-2). I've gotten past every BootROM and FSBL bug and the Cortex-A9-side bootloader now runs end-to-end, but after PCAP succeeds the LiteX BIOS in the freshly-loaded PL bitstream produces zero UART output. I need help figuring out why.

## Working state (verified via JTAG no-reset probe)

After SD-boot power-on, with the strict-FAT32 / 130k-cluster layout on the card:

- `BOOT_MODE @0xF800025C = 0x5` → SD boot mode confirmed.
- `REBOOT_STATUS @0xF8000258 = 0x00400000` → clean state, no boot ROM error.
- `DEVCFG_STATUS @0xF8007014 = 0x50000F30` → bit 30 (DMA_Q_E), bit 28, bits 8-11, bits 4-5 set. PCFG_INIT high.
- `DEVCFG_INT_STS @0xF800700C = 0xF802300F` → **bit 12 (D_P_DONE), bit 13 (DMA_DONE), bit 2 (PCFG_DONE_INT), bit 17 (WR_FIFO_LVL) all set, no error bits**. PCAP DMA completed AND PCFG_DONE fired.
- `FPGA_RST_CTRL @0xF8000240 = 0x00000000` → all PL fabric resets deasserted.
- `MIO40..47 @0xF80007A0..BC = 0x00001280` → SDIO0 pinmux still configured.

DDR contents at the addresses FSBL wrote to:
- `0x00100000 = 0x1280006F` (Image69 — RISC-V kernel first instruction)
- `0x00FF0000 = 0xEDFE0DD0` (linux69.dtb — DTB magic in LE)
- `0x01000000 = 0x00050433` (fw_jump69.bin — OpenSBI first instruction)
- `0x02100000 = ...` (rootfs69.cpio)

DDR scratch (FSBL → LiteX BIOS protocol):
- `0x10100000 = 0xC0DEC0DE` (S_DONE → DONE_MAGIC)
- `0x10100004 = 0xA70A1CBA` (S_MAGIC — FSBL post-DDR alive marker)
- `0x10100010 = 0x000000C0` (S_MARKER — FSBL "all stages done" marker)
- `0x10100014 = 0x00000000` (S_LAST_ERR — zero)
- `0x10100008 = 0x00004D78` (heartbeat at last probe; not incrementing across subsequent probes — see below)

CPU0 PC was at `0x00000188` (inside the FSBL OCM .text section) when first halted; the FSBL is sitting in its heartbeat loop after writing DONE_MAGIC, exactly as designed.

After PCAP succeeded, **JTAG DAP now consistently shows `AHB AP transaction error, DAP status 0x30000021` / `0xF0000021`** for ARM-side accesses. SDIO and DEVCFG reads still work but most ARM accesses fail. The DAP is sticky-error-latched. This is consistent with "PL bitstream is alive and has taken AHB master state."

## What's broken

After power-cycle into SD boot, I waited 95 seconds with a Python monitor listening on every available `/dev/ttyUSB*` at 115200, 921600, 9600, 19200, 57600, 230400, 460800. **Zero bytes captured on any port at any baud.**

Available ports during SD boot: only `/dev/ttyUSB1` and `/dev/ttyUSB2`. `/dev/ttyUSB0` does not enumerate (it usually does when this PL bitstream is loaded via JTAG path — see "JTAG-path control" below).

## Background — proven JTAG-path comparison

The same NaxRiscv Linux stack boots successfully via the **JTAG path**. In that path:
1. xsdb `rst -system`, then `ps7_init`, then `fpga -file hamgeek_rk7020f.bit` (the LiteX SoC bitstream — full LiteX with UART/console/CSRs, NOT the autoboot variant).
2. JTAG `dow -data` of `Image_nax64`, `linux_nax64.dtb`, `fw_jump_nax64.bin`, `ubuntu_rv64.cpio.gz`, `trampoline.bin` to DDR.
3. Open `/dev/ttyUSB0` or `/dev/ttyUSB2` (varies per session) at 115200 and the LiteX BIOS prompt is there: `litex> ` (ANSI-colored). Send `boot 0x40A00000` and Linux boots, prints copiously, drops to a shell.

So:
- The PL UART path is wired and functional on this board.
- The kernel/dtb/opensbi files I'm loading ARE the same files that work via JTAG (sized identically; sd_verify confirmed byte-perfect read-back from the SD card after sd_writer pushed them via SDHC0).

## What's different between the working JTAG path and the silent SD path

| | JTAG path (works, prints) | SD path (silent) |
|---|---|---|
| Bitstream | `litex-build-nax64/gateware/hamgeek_rk7020f.bit` (full LiteX, prints `litex> ` prompt) | `litex-build-nax64-sdboot/gateware/hamgeek_rk7020f.bit.bin` ("autoboot" variant that polls DDR scratch for DONE_MAGIC and auto-jumps to trampoline) |
| Linux files | `Image_nax64`, `linux_nax64.dtb`, `fw_jump_nax64.bin`, `ubuntu_rv64.cpio.gz` | `Image69`, `linux69.dtb`, `fw_jump69.bin`, `rootfs69.cpio` (these are the kernel/dtb/sbi we used in older buildroot work) |
| Bitstream programmer | xsdb `fpga -file` over JTAG | Our minimal FSBL pcap_program() |
| Source addresses | NaxRiscv addresses (`0x40000000` for kernel etc., LiteX wishbone remapper translates to PS DDR) | PS DDR addresses (`0x00100000` for kernel — same physical location, just named on the PS side) |
| Boot trigger | User types `boot 0x40A00000` on LiteX UART prompt | FSBL writes `DONE_MAGIC = 0xC0DEC0DE` to PS DDR `0x10100000`; autoboot bitstream's LiteX BIOS should poll this and jump |

The whole point of the `litex-build-nax64-sdboot` variant is to skip the "user types boot at the prompt" step by polling scratch. The repo memory note for it says exactly: *"Autoboot BIOS bitstream — LiteX BIOS polls 0x10100000, auto-boots when ps_loader signals DONE."*

## My hypotheses (ranked)

1. **The autoboot bitstream's UART pin/EMIO routing is different from the regular LiteX BIOS bitstream**, so its output goes to a pin not wired to FT2232H. (Most likely.)
2. **The autoboot bitstream polls a CSR address, not a DDR address.** FSBL writes DONE_MAGIC to DDR `0x10100000`, but the LiteX BIOS in the autoboot variant might be polling an internal LiteX CSR register that ARM was supposed to write to via GP0 (which ARM never did). LiteX BIOS sits forever, no Linux boot, no console.
3. **The autoboot bitstream is a stripped-down LiteX without a UART or with the UART unwired**, since its only job is to poll-and-jump. The kernel that boots after the jump would have its own UART driver, but if Linux's UART driver can't find a usable UART in this bitstream's address map, the kernel boots silently.
4. **NaxRiscv never started.** PCFG_DONE is set, but maybe the NaxRiscv reset signal in this bitstream is gated by something ARM didn't release. Linux files in DDR but no CPU executing them.
5. **The kernel/dtb/initramfs I picked from `hardware/zynq/litex-build/`** were originally built for the **JTAG-path bitstream's memory map**, not the autoboot bitstream's map. If the autoboot bitstream has different DDR base/CSR layout than the regular bitstream, the dtb is wrong and Linux either panics silently or never starts on NaxRiscv.

## What I want from you

Be concrete and specific:

1. Which of (1)..(5) is most likely given that PCFG_DONE_INT is set, DAP errors latched, and zero bytes on any UART? What's the canonical Zynq+LiteX-autoboot-from-SD flow that distinguishes which case we're in?
2. If hypothesis (5), how would I confirm — what bytes/structures should I look for in the autoboot bitstream's `csr.json` / `mem_regions.json` to know if the dtb matches the SoC layout?
3. If hypothesis (1) or (2), where in the LiteX BIOS source (litex/soc/cores/cpu/`naxriscv` / litex/soc/integration) would I look to see what UART pins and what scratch address the autoboot variant uses? Is there a build-time config flag (`with_autoboot=True` or similar) that controls this?
4. Is there a non-invasive way from ARM-side to confirm NaxRiscv is or is not executing — without crashing the DAP further? E.g., can ARM read DDR addresses that NaxRiscv would have modified during early boot (PC counters, page table setup, sbi data structures)?
5. The DAP-locked `0x30000021` state after PCAP — Linux driver comments call this expected after PL takes the AHB master role. Is there a sequence (PS-PL bridge enable / AXI HP enable / AFI write to FPGA_RST_CTRL) that would let me re-establish PS→PL accesses for debug without resetting everything?

The repo paths I'm working in:
- `hardware/zynq/litex-build-nax64-sdboot/` — the autoboot bitstream we're loading
- `hardware/zynq/litex-build/` — the JTAG-path kernels/dtb/initramfs
- `hardware/zynq/fsbl_build/minimal_fsbl/` — our minimal FSBL source
- `hardware/zynq/ps_loader/` — the ARM-side helper utilities

Please prioritize "tell me what to read or what one bit/register to probe next" over rewriting code, because each iteration on this hardware costs a power-cycle plus ~3 min of redeploy.
