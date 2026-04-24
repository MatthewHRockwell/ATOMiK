/* ============================================================================
 * ATOMiK PS Loader — Cortex-A9 startup
 *
 * Minimal bare-metal entry for the Zynq-7000 PS. Loaded into PS DDR by xsdb
 * via JTAG after ps7_init has configured PLLs, DDR, MIO. Execution begins at
 * _start (linker sets ENTRY(_start)).
 *
 * This app runs on core 0 only. Core 1 is parked by ps7_init.
 *
 * Phase 9.1.a: smoke test — set up SP, clear BSS, call main(), halt on return.
 * No MMU, no cache enable yet (DDR is uncached which is fine for a small
 * loader that transfers maybe 20 MB total).
 * ========================================================================= */

    .syntax unified
    .arm
    .section .vectors, "ax"
    .global _start
_start:
    b   reset_handler
    b   .               /* Undefined instruction */
    b   .               /* SVC */
    b   .               /* Prefetch abort */
    b   .               /* Data abort */
    b   .               /* Reserved */
    b   .               /* IRQ */
    b   .               /* FIQ */

    .section .text.reset, "ax"
    .global reset_handler
reset_handler:
    /* Stay in whatever mode xsdb drops us in (SVC, typically). Set SP. */
    ldr     sp, =_stack_top

    /* Disable SCTLR.A (strict alignment check). ps7_init can leave this set
     * and any subsequent unaligned 32-bit load then faults. We stay MMU-off
     * and cache-off; unaligned access is safe on Cortex-A9 in that mode. */
    mrc     p15, 0, r0, c1, c0, 0
    bic     r0, r0, #(1 << 1)       /* A: strict alignment */
    mcr     p15, 0, r0, c1, c0, 0
    isb

    /* Zero .bss */
    ldr     r0, =_bss_start
    ldr     r1, =_bss_end
    mov     r2, #0
1:  cmp     r0, r1
    bge     2f
    str     r2, [r0], #4
    b       1b
2:

    bl      main

    /* main() should not return; if it does, spin. */
halt_spin:
    wfe
    b       halt_spin

    .size   reset_handler, . - reset_handler
