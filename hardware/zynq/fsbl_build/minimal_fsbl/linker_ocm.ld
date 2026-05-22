/* linker_ocm.ld — Zynq7000 OCM (On-Chip Memory) layout for minimal FSBL.
 * Code + data must fit in 256KB OCM at 0x00000000.
 * Stack at top of OCM.  ps7_init() initializes DDR; after that we use
 * DDR for bitstream staging and Linux image loading.
 */

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY {
    OCM (rwx) : ORIGIN = 0x00000000, LENGTH = 256K
}

_stack_size = 0x00008000;   /* 32 KB stack */

SECTIONS {
    . = ORIGIN(OCM);

    .vectors : ALIGN(4) {
        KEEP(*(.vectors))
    } > OCM

    .text : ALIGN(4) {
        *(.text.reset)
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(4);
    } > OCM

    .data : ALIGN(8) {
        _data_start = .;
        *(.data .data.*)
        . = ALIGN(8);
        _data_end = .;
    } > OCM

    .bss (NOLOAD) : ALIGN(8) {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(8);
        _bss_end = .;
    } > OCM

    .stack (NOLOAD) : ALIGN(16) {
        _stack_bottom = .;
        . += _stack_size;
        _stack_top = .;
    } > OCM

    _end = .;
}
