/* Minimal startup for Cortex-A9 bare-metal
 * Loaded at 0x00000000 (OCM) by xsdb "dow"
 * Sets up stack and jumps to main()
 */
    .global _start
    .section .text.boot

_start:
    /* Set up stack at top of OCM (256KB = 0x40000) */
    ldr sp, =0x00030000

    /* Jump to C main */
    bl main

    /* If main returns, spin */
hang:
    wfe
    b hang
