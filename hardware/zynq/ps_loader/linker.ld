/* ============================================================================
 * ATOMiK PS Loader linker script — Cortex-A9, Zynq-7000 PS DDR
 *
 * Layout:
 *   Loader image lives in PS DDR at 0x00100000 (1 MB in).
 *   1 MB stack at top.
 *   Safe: Linux images land at 0x40000000+, so 0x00100000 is conflict-free.
 *
 * ARM boot ROM maps OCM at 0x00000000 by default; we avoid that region to
 * leave OCM / vectors alone. xsdb sets PC to _start after `dow` — no vector
 * table redirection needed.
 * ========================================================================= */

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY
{
    /* Loader lives at 256 MB into PS DDR — well above the kernel/dtb/cpio/
     * opensbi load addresses (0x00100000-0x04200000 in PS view, since the
     * LiteX wishbone remapper maps NaxRiscv 0x40000000 → PS 0x00100000).
     * 256 KB is plenty for the loader code + stack.
     */
    DDR (rwx) : ORIGIN = 0x10000000, LENGTH = 256K
}

_stack_size = DEFINED(_stack_size) ? _stack_size : 0x00008000;   /* 32 KB */

SECTIONS
{
    . = ORIGIN(DDR);

    .vectors : ALIGN(4) {
        KEEP(*(.vectors))
    } > DDR

    .text : ALIGN(4) {
        *(.text.reset)
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(4);
    } > DDR

    .data : ALIGN(8) {
        _data_start = .;
        *(.data .data.*)
        . = ALIGN(8);
        _data_end = .;
    } > DDR

    .bss (NOLOAD) : ALIGN(8) {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(8);
        _bss_end = .;
    } > DDR

    .stack (NOLOAD) : ALIGN(16) {
        _stack_bottom = .;
        . += _stack_size;
        _stack_top = .;
    } > DDR

    _end = .;
}
