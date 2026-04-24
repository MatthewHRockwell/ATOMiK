/* ============================================================================
 * NaxRiscv L2 cache-flush + jump-to-OpenSBI trampoline
 *
 * Works around the LiteX BIOS bug where flush_l2_cache() is empty for
 * NaxRiscv (CONFIG_L2_SIZE never #defined; only CONFIG_CPU_L2CACHE_SIZE is).
 *
 * On RK-ZYNQ7020-F SoC: NaxRiscv L2 is 128 KB, 8-way, 64-byte lines (256 sets).
 * To force-evict every stale entry left over from BIOS memtest (which
 * touches the first 2 MB of MAIN_RAM_BASE), write 256 KB sequentially to
 * a scratch region. That visits every cache set ≥ 2 times, evicting all
 * old ways with new (write-allocated) lines.
 *
 * After the writes:
 *   fence rw,rw   — order memory ops
 *   fence.i       — invalidate any I-cache speculative fills
 *   jr 0x40f00000 — OpenSBI takes over with a clean L2
 *
 * Place this trampoline at 0x40A00000 (5 MB into main_ram, beyond the 2 MB
 * memtest region — so the trampoline's OWN bytes are NEVER cached stale).
 * ========================================================================= */

    .option norvc           /* uncompressed: predictable size */
    .section .text
    .globl _start
_start:
    li      t0, 0x4FE00000      /* scratch start (far above everything) */
    li      t1, 0x4FE40000      /* + 256 KB                              */
1:
    sw      zero, 0(t0)
    addi    t0, t0, 64
    bne     t0, t1, 1b

    fence   rw, rw
    fence.i

    li      t0, 0x40F00000      /* OpenSBI entry */
    jr      t0
