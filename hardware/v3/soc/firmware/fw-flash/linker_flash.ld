OUTPUT_FORMAT("elf64-littleriscv", "elf64-littleriscv", "elf64-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(crtStart)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00000010, LENGTH = 8M - 16
  RAM   (xrw) : ORIGIN = 0x40000000, LENGTH = 8k
}

_stack_size = DEFINED(_stack_size) ? _stack_size : 2k;
_heap_size = DEFINED(_heap_size) ? _heap_size : 1k;

SECTIONS {
    .vector 0x00000010 : {
        . = ALIGN(4);
        KEEP(*crt_flash.o(.text .text.*));
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        *(.srodata)
        *(.srodata*)

        *(.eh_frame_hdr)
        *(.eh_frame)

        . = ALIGN(4);
        _etext = .;
        _sidata = _etext;
    } > FLASH

    .rodata :
    {
        *(.rdata)
        *(.rodata .rodata.*)
        *(.gnu.linkonce.r.*)
    } > FLASH

    .ctors :
    {
        . = ALIGN(8);
        _ctors_start = .;
        KEEP(*(.init_array*))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
        . = ALIGN(8);
        _ctors_end = .;
    } > FLASH

    .data : AT ( _sidata )
    {
        . = ALIGN(8);
        _sdata = .;
        _ram_start = .;
        . = ALIGN(8);
        *(.data)
        *(.data*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(8);
        PROVIDE( __global_pointer$ = . + 0x800 );
        *(.sdata)
        *(.sdata*)
        *(.gnu.linkonce.s.*)
        . = ALIGN(8);
        _edata = .;
    } > RAM

    .bss :
    {
        . = ALIGN(8);
        _bss_start = .;
        *(.bss)
        *(.bss*)
        *(.sbss)
        *(.sbss*)
        *(.gnu.linkonce.sb.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(8);
        _bss_end = .;
    } > RAM

    _end = .;
    PROVIDE (end = .);

    .heap :
    {
        . = ALIGN(8);
        PROVIDE ( _heap_start = .);
        . = . + _heap_size;
        . = ALIGN(8);
        PROVIDE ( _heap_end = .);
    } > RAM

    .stack :
    {
        . = ALIGN(16);
        PROVIDE(_stack_end = .);
        . = _stack_size;
        . = ALIGN(16);
        PROVIDE(_stack_start = .);
    } > RAM

}
