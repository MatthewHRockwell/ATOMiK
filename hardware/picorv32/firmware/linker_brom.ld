OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv", "elf32-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(crtStart)

MEMORY
{
  BROM (rx)      : ORIGIN = 0x80000000, LENGTH = 32k
  RAM  (xrw)     : ORIGIN = 0x40000000, LENGTH = 8k
}

_stack_size = DEFINED(_stack_size) ? _stack_size : 1k;
_heap_size = DEFINED(_heap_size) ? _heap_size : 2k;

SECTIONS {
  .vector : {
    . = ALIGN(4);
    *crt_flash.o(.text);
  } > BROM

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
  } >BROM

  .rodata :
  {
    *(.rdata)
    *(.rodata .rodata.*)
    *(.gnu.linkonce.r.*)
  } > BROM

  .ctors :
  {
    . = ALIGN(4);
    _ctors_start = .;
    KEEP(*(.init_array*))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    . = ALIGN(4);
    _ctors_end = .;
  } > BROM

  .data : AT ( _sidata )
  {
    . = ALIGN(4);
    _sdata = .;
    _ram_start = .;
    . = ALIGN(4);
    *(.data)
    *(.data*)
    *(.gnu.linkonce.d.*)
    . = ALIGN(8);
    PROVIDE( __global_pointer$ = . + 0x800 );
    *(.sdata)
    *(.sdata*)
    *(.gnu.linkonce.s.*)
    . = ALIGN(4);
    _edata = .;
  } >RAM

  .bss :
  {
    . = ALIGN(4);
    _bss_start  = .;
    *(.bss)
    *(.bss*)
    *(.sbss)
    *(.sbss*)
    *(.gnu.linkonce.sb.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN(4);
    _bss_end = .;
  } >RAM

  _end = .;
  PROVIDE (end = .);

  .heap :
  {
    . = ALIGN(8);
    PROVIDE ( _heap_start = .);
    . = . + _heap_size;
    . = ALIGN(8);
    PROVIDE ( _heap_end = .);
  } >RAM

  .stack :
  {
    . = ALIGN(16);
    PROVIDE(_stack_end = .);
    . = _stack_size;
    . = ALIGN(16);
    PROVIDE(_stack_start = .);
  } >RAM
}
