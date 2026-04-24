# NaxRiscv on Zynq-7000 — JTAG-direct DDR load hangs at OpenSBI → Linux handoff

**Asking for a second opinion.** I've converged on a narrow failure and want to sanity-check whether my remaining hypothesis is right before burning more cycles.

## Goal

Replace a 42-minute LiteX BIOS SFL serial upload with a ~90 s JTAG-direct DDR load, for iteration speed on a LiteX NaxRiscv RV64 Linux SoC running on a Zynq-7000 (XC7Z020-CLG484) board. PS (Cortex-A9) JTAG does `dow -data` into PS DDR; UART then triggers the LiteX BIOS `boot 0x40f00000` which hands off to OpenSBI → Linux.

## Architecture summary

- **PL**: LiteX NaxRiscv RV64GC + LiteX BIOS in BRAM ROM. NaxRiscv L2 = 128 KB, 8-way, 64 B line.
- **DDR**: PS DDR3 (1 GB), LiteX wishbone remapper exposes it at NaxRiscv `0x40000000` ↔ PS `0x00100000` via AXI GP0 slave. NaxRiscv's own main_ram appears in its address space at 0x40000000.
- **Boot images** (NaxRiscv / PS):
  - `Image_nax64` (kernel, 8.2 MB) at 0x40000000 / PS 0x00100000
  - `linux_nax64.dtb` (2.4 KB) at 0x40EF0000 / PS 0x00FF0000
  - `fw_jump_nax64.bin` (OpenSBI, 134 KB) at 0x40F00000 / PS 0x01000000
  - `ubuntu_rv64.cpio.gz` (initramfs, 30 MB) at 0x42000000 / PS 0x02100000
- **Console UART**: LiteX liteuart on FT232R adapter (`/dev/ttyUSB2 @ 921600`). FT2232H ch B is not wired to PL UART on this board.
- **Host**: xsdb v2025.2 + hw_server over JTAG. No NaxRiscv debug stub — xsdb only sees APU (A9) + xc7z020.

## What works

1. **SFL boot (UART upload) with today's bitstream**: End-to-end `root@atomik-rv64:/#` Ubuntu 24.04 login. Took 44 min but boots fully (Linux 6.9.0 + initramfs unpacked + /init run). So today's **bitstream + fw_jump + kernel + dtb + cpio pair is known-good**.

2. **PS A9 baremetal loader**: Cortex-A9 code runs from DDR via xsdb `dow` + `con`. Reads SDIO0 / FAT32 fine. Proves PS boot path exists.

3. **JTAG-direct data load**: Single xsdb session does `rst-system → ps7_init → fpga → ps7_post_config → 4× dow -data` in **91 s**. No DAP errors, no multi-session issues.

4. **`mrd -value` readback** at several offsets after `dow -data` matches the file bytes (e.g. 0x100000 reads 0x0D40006F which is the RISC-V Image format magic, matching the file). So `dow` writes *something* correct.

## What fails

**JTAG-direct boot** — after `dow -data` of all 4 files + trampoline, UART `boot 0x40a00000` (via a 36-byte cache-flush trampoline I wrote) or `boot 0x40f00000` (direct to OpenSBI) produces OpenSBI output through exactly this line:

```
Boot HART MEDELEG         : 0x000000000000b109
```

...then silence forever. SFL path continues on to `Linux version 6.9.0 ...` at this point. Hang is 100% repeatable across multiple bitstream programs / image reloads / reboots.

## Patches attempted so far

### Attempt 1 — Trampoline for L2 flush

Wrote a 36-byte RV64 routine at NaxRiscv 0x40A00000 that reads 256 KB from a scratch region (supposed to evict all 8-way L2 sets) then jumps to OpenSBI. No effect — same hang at MEDELEG.

### Attempt 2 — Fix LiteX `flush_l2_cache` bug (real bug!)

Found `litex/soc/software/libbase/system.c`:

```c
void flush_l2_cache(void) {
#ifdef CONFIG_L2_SIZE
    // ... reads 2*CONFIG_L2_SIZE from MAIN_RAM_BASE ...
#endif
}
```

Generated headers for NaxRiscv builds define `CONFIG_CPU_L2CACHE_SIZE` but never `CONFIG_L2_SIZE`. So `flush_l2_cache()` is a **literal no-op for NaxRiscv**. SFL boot tolerates this because NaxRiscv's own stores go through L2 (naturally coherent). JTAG-direct writes bypass L2 → stale BIOS memtest residue.

Patched the alias + also changed the read-from-region to be past `MEMTEST_DATA_SIZE + 64 KiB` (so reads actually miss L2 rather than returning stale-but-cached data from a region the BIOS memtest had already populated).

Confirmed in `objdump` that the patched BIOS now has a real loop body in `flush_l2_cache`. Rebuilt bitstream. Programmed board. **Same hang at MEDELEG.**

### Attempt 3 — Full DDR hash verification

Dumped 8 MB of PS DDR at 0x100000 to file via `xsdb mrd -bin`. md5 DOESN'T match the kernel file. `cmp` shows all 210k differing bytes are `0xFF` from the DDR side — but `mrd -value 0x100000` after the same dow reports correct bytes. Test currently running to distinguish "`mrd -bin` is broken" vs "dow only writes the first few words".

## Key facts that narrow the problem

- Same bitstream, same kernel, same fw_jump, same dtb, same cpio pair that boots fully via SFL, hangs via JTAG at MEDELEG.
- Hang is **in OpenSBI or at OpenSBI→Linux boundary** — not before (OpenSBI prints its full platform info including MEDELEG) and not in Linux early boot (no garbled chars, no partial messages).
- xsdb CAN see APU, so any A9-side state manipulation (cache disable/invalidate, MMU settings, etc.) is available.
- xsdb **CAN'T** see NaxRiscv — no way to halt it, inspect CSRs/PC, or single-step through OpenSBI → kernel handoff.

## Remaining hypotheses (roughly in my confidence order)

**H1. PL-side L2 cache still stale, but not the way I tried.** Maybe the flush-from-fresh-region trick doesn't work because the L2 is inclusive of the L1 I-cache or has some subtle allocation policy that makes `read-to-evict` insufficient. Maybe a proper fence-based flush is needed from NaxRiscv's own mode (BIOS already runs `flush_l2_cache` right before `boot` — but maybe that only runs at BIOS command time, not at `boot` time?).

**H2. PS L1/L2 (PL310) cache holding dow writes.** ARM A9 caches might cache the DDR writes on the PS side, and NaxRiscv reads via AXI GP0 slave would miss them. ps7_init docs suggest caches are OFF at post-config, but I haven't verified. This would cause exactly the observed symptom: first words readable via another A9 operation (which goes through the same cache) look correct, but actual DDR is stale. Would also explain why `mrd -bin` may have read from a different path and got 0xFF (uninitialized DRAM) while `mrd -value` went through the A9 DAP and hit cached writes.

**H3. OpenSBI / BIOS handoff preconditions.** Maybe OpenSBI's fw_jump expects specific register/state setup that LiteX BIOS's `boot` path provides only when images are freshly uploaded (SFL mode). E.g. maybe BIOS does something via mmio that's only triggered by the SFL "load complete" transition, not by a bare `boot` command.

**H4. OpenSBI 8 HARTs / 1 hart mismatch.** OpenSBI platform banner says "Platform HART Count: 8" but this SoC has 1 NaxRiscv. OpenSBI HSM might wait for warm harts at MEDELEG printing time. But this would also hit SFL boot, which works — so probably NOT the issue.

## Test I'm running right now

Compare `mrd -value 0x100000` (scalar, after dow) vs `mrd -bin -file ... 0x100000 256` (bulk). If scalar is correct and bulk is 0xFF: **`mrd -bin` uses a different memory path than `mrd -value`**, and the earlier md5 mismatch means nothing about dow integrity. If both are correct: dow is fine and the md5 test was bogus due to some other issue. If both are 0xFF: this specific dow attempt didn't actually write (maybe rst-system + ps7_init before dow in same session leaves DDR in a state I don't understand).

## What I'd like another perspective on

1. **For H2 (PS cache holding dow writes):** Is this the likely explanation? Zynq-7000 A9 + PL310 L2 + AXI GP0 slave — do xsdb dow writes go through A9 caches even when the target is nominally DDR mapped through GP0? Would a cache clean/invalidate operation after dow fix this, and what's the minimal xsdb incantation to do it?

2. **For H3 (OpenSBI handoff state):** Is there known state that LiteX BIOS sets up just before `boot` (beyond writing images to DDR) that JTAG-direct skips? I'm looking at `litex/soc/software/bios/boot.c`'s `boot()` function — it calls `flush_l2_cache()`, prints a banner, then does `boot_helper(r1, r2, r3, entry)` which is assembly that sets up 4 registers and jumps. Nothing else.

3. **Is there a simple way to make NaxRiscv visible to xsdb** so I can halt it at any instruction and inspect state? (RISC-V debug spec 0.13.2 stub in the NaxRiscv gateware would be the proper answer but is a significant rebuild.)

4. **Any known LiteX + NaxRiscv + Zynq + JTAG-direct DDR boot references** that work? I've been assuming this is a supported path but maybe it isn't — maybe the only supported paths are SFL, QSPI, or SD (LiteSDCard).

## Files for context

- `hardware/zynq/ps_loader/jtag_boot.py` — orchestrator (xsdb + UART)
- `hardware/zynq/ps_loader/trampoline.S` — L2 flush attempt
- `hardware/zynq/litex/soc_nax64_atomik.py` — SoC definition
- `hardware/zynq/scripts/ps7_init_rk7020f.tcl` — auto-generated from Vivado block design
- `hardware/zynq/litex-build-nax64/` — original (pre-patch) build output
- `hardware/zynq/litex-build-nax64-patched/` — post-patch build output (patched flush_l2_cache, same hang)
- LiteX patch at `/home/mattrock/litex/litex/litex/soc/software/libbase/system.c`

## What I want to avoid

- Another 40-min rebuild of a subtly-different bitstream that also fails because I'm still wrong about the root cause.
- Punting to "just use SFL" — the whole point of this work is the 27× iteration speedup.

What am I missing?
