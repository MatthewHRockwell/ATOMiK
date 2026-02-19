OUTPUT_FORMAT("elf64-littleriscv", "elf64-littleriscv", "elf64-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(crtStart)

MEMORY {
  BROM : ORIGIN = 0x80000000, LENGTH = 8k
}

_stack_size = DEFINED(_stack_size) ? _stack_size : 512;
_heap_size = DEFINED(_heap_size) ? _heap_size : 0;

SECTIONS {

  .vector : {
    *crt_brom.o(.text);
  } > BROM

  .memory : {
    *(.text);
    end = .;
  } > BROM

  .rodata :
  {
    *(.rdata)
    *(.rodata .rodata.*)
    *(.gnu.linkonce.r.*)
  } > BROM

  .ctors :
  {
    . = ALIGN(8);
    _ctors_start = .;
    KEEP(*(.init_array*))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    . = ALIGN(8);
    _ctors_end = .;
  } > BROM

  .data :
  {
    *(.data .data.*)
    *(.gnu.linkonce.d.*)
    . = ALIGN(8);
    PROVIDE( __global_pointer$ = . + 0x800 );
    *(.sdata .sdata.*)
    *(.gnu.linkonce.s.*)
    . = ALIGN(8);
    *(.srodata.cst16)
    *(.srodata.cst8)
    *(.srodata.cst4)
    *(.srodata.cst2)
    *(.srodata .srodata.*)
  } > BROM

  .bss (NOLOAD) : {
    . = ALIGN(8);
    _bss_start = .;
    *(.sbss*)
    *(.gnu.linkonce.sb.*)
    *(.bss .bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN(8);
    _bss_end = .;
  } > BROM

  .noinit (NOLOAD) : {
    . = ALIGN(8);
    *(.noinit .noinit.*)
    . = ALIGN(8);
  } > BROM

  ._user_heap (NOLOAD):
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    PROVIDE ( _heap_start = .);
    . = . + _heap_size;
    . = ALIGN(8);
    PROVIDE ( _heap_end = .);
  } > BROM

  ._stack (NOLOAD):
  {
    . = ALIGN(16);
    PROVIDE (_stack_end = .);
    . = . + _stack_size;
    . = ALIGN(16);
    PROVIDE (_stack_start = .);
  } > BROM

}
