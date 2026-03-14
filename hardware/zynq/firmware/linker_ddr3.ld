/* ============================================================================
 * ATOMiK Zynq Co-Processor Linker Script
 *
 * Memory map (RV64I view):
 *   0x00000000 - 0x0FFFFFFF  DDR3 via AXI (code + data, translated to
 *                             0x10000000 physical by bus_to_axi bridge)
 *   0x40000000 - 0x40007FFF  Local BRAM (32 KB, fast scratchpad)
 *   0x80000000 - 0x8000003F  Mailbox registers
 *
 * Code and read-only data reside in DDR3 (fetched via AXI HP port).
 * Writable data (.data, .bss, .heap, .stack) in local BRAM for speed.
 *
 * ARM firmware loader copies this binary to DDR3 at physical 0x10000000.
 * The co-processor's RESET_PC is 0x00000000, which maps to that address.
 *
 * Target: ALINX AX7020 (XC7Z020-2CLG400I)
 * ATOMiK Project — March 2026
 * ============================================================================ */

OUTPUT_FORMAT("elf64-littleriscv", "elf64-littleriscv", "elf64-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(crtStart)

MEMORY
{
  DDR3 (rx)  : ORIGIN = 0x00000000, LENGTH = 16M
  BRAM (xrw) : ORIGIN = 0x40000000, LENGTH = 32k
}

_stack_size = DEFINED(_stack_size) ? _stack_size : 4k;
_heap_size  = DEFINED(_heap_size)  ? _heap_size  : 4k;

SECTIONS {
    /* Startup code at address 0x00000000 (first instruction fetched) */
    .vector 0x00000000 : {
        . = ALIGN(4);
        KEEP(*crt_zynq.o(.text .text.*));
    } > DDR3

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
    } > DDR3

    .rodata :
    {
        *(.rdata)
        *(.rodata .rodata.*)
        *(.gnu.linkonce.r.*)
    } > DDR3

    .ctors :
    {
        . = ALIGN(8);
        _ctors_start = .;
        KEEP(*(.init_array*))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
        . = ALIGN(8);
        _ctors_end = .;
    } > DDR3

    /* Writable data in BRAM (copied from DDR3 at startup) */
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
    } > BRAM

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
    } > BRAM

    _end = .;
    PROVIDE (end = .);

    .heap :
    {
        . = ALIGN(8);
        PROVIDE ( _heap_start = .);
        . = . + _heap_size;
        . = ALIGN(8);
        PROVIDE ( _heap_end = .);
    } > BRAM

    .stack :
    {
        . = ALIGN(16);
        PROVIDE(_stack_end = .);
        . = . + _stack_size;
        . = ALIGN(16);
        PROVIDE(_stack_start = .);
    } > BRAM
}
