/* Linker script — bare-metal Zynq OCM */
ENTRY(_start)

MEMORY {
    OCM (rwx) : ORIGIN = 0x00000000, LENGTH = 192K
}

SECTIONS {
    .text : {
        *(.text.boot)
        *(.text*)
    } > OCM

    .rodata : { *(.rodata*) } > OCM
    .data : { *(.data*) } > OCM
    .bss : { *(.bss*) *(COMMON) } > OCM

    /DISCARD/ : { *(.ARM.*) *(.comment) }
}
