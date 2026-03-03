.section .text

.global crtStart
.global main

crtStart:
  j crtInit
  nop
  nop
  nop

crtInit:
  # Set trap vector FIRST — before anything else can fault
  .option push
  .option norelax
  la t0, _flash_trap_handler
  csrw mtvec, t0
  .option pop

  # Diagnostic: Initialize UART first so we can print debug chars
  li t0, 0x83000000    # UART base
  li t1, 185           # CLKDIV for 21.6 MHz / 115200 baud
  sw t1, 4(t0)         # UART->CLKDIV
  li t2, 2000
1:addi t2, t2, -1
  bnez t2, 1b

  # Print 'A' - CRT started
  li t1, 'A'
  sw t1, 0(t0)

  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  # Print 'B' - GP loaded
  li t0, 0x83000000
  li t1, 'B'
  sw t1, 0(t0)

  la sp, _stack_start

  # Print 'C' - SP loaded
  li t0, 0x83000000
  li t1, 'C'
  sw t1, 0(t0)

# copy data section (flash → RAM)
  la a0, _sidata
  la a1, _sdata
  la a2, _edata
  bge a1, a2, end_init_data
loop_init_data:
  lw a3, 0(a0)
  sw a3, 0(a1)
  addi a0, a0, 4
  addi a1, a1, 4
  blt a1, a2, loop_init_data
end_init_data:

  # Print 'D' - Data copy done
  li t0, 0x83000000
  li t1, 'D'
  sw t1, 0(t0)

bss_init:
  la a0, _bss_start
  la a1, _bss_end
bss_loop:
  beq a0,a1,bss_done
  sd zero,0(a0)
  add a0,a0,8
  j bss_loop
bss_done:

ctors_init:
  la a0, _ctors_start
  addi sp,sp,-8
ctors_loop:
  la a1, _ctors_end
  beq a0,a1,ctors_done
  ld a3,0(a0)
  add a0,a0,8
  sd a0,0(sp)
  jalr a3
  ld a0,0(sp)
  j ctors_loop
ctors_done:
  addi sp,sp,8

  # Print 'E' - About to call main
  li t0, 0x83000000
  li t1, 'E'
  sw t1, 0(t0)

  call main
infinitLoop:
  j infinitLoop

# ==========================================================================
# Trap handler — self-contained, no stack dependency
# Prints: !CC_AAAA\n  (CC=mcause low byte, AAAA=mepc low 16 bits)
# Uses only t0-t3, saves nothing (handler halts forever after print)
# ==========================================================================
.balign 4
_flash_trap_handler:
  # UART base = 0x83000000
  li t0, 0x83000000

  # Print '!'
  li t1, '!'
  sw t1, 0(t0)

  # Read mcause, print low byte as 2 hex digits
  csrr t2, mcause
  # High nibble
  srli t1, t2, 4
  andi t1, t1, 0xF
  li t3, 10
  blt t1, t3, 1f
  addi t1, t1, ('A' - 10)
  j 2f
1:
  addi t1, t1, '0'
2:
  sw t1, 0(t0)
  # Low nibble
  andi t1, t2, 0xF
  li t3, 10
  blt t1, t3, 3f
  addi t1, t1, ('A' - 10)
  j 4f
3:
  addi t1, t1, '0'
4:
  sw t1, 0(t0)

  # Print '_'
  li t1, '_'
  sw t1, 0(t0)

  # Read mepc, print low 16 bits as 4 hex digits
  csrr t2, mepc
  # Digit 3 (bits 15:12)
  srli t1, t2, 12
  andi t1, t1, 0xF
  li t3, 10
  blt t1, t3, 5f
  addi t1, t1, ('A' - 10)
  j 6f
5:
  addi t1, t1, '0'
6:
  sw t1, 0(t0)
  # Digit 2 (bits 11:8)
  srli t1, t2, 8
  andi t1, t1, 0xF
  li t3, 10
  blt t1, t3, 7f
  addi t1, t1, ('A' - 10)
  j 8f
7:
  addi t1, t1, '0'
8:
  sw t1, 0(t0)
  # Digit 1 (bits 7:4)
  srli t1, t2, 4
  andi t1, t1, 0xF
  li t3, 10
  blt t1, t3, 9f
  addi t1, t1, ('A' - 10)
  j 10f
9:
  addi t1, t1, '0'
10:
  sw t1, 0(t0)
  # Digit 0 (bits 3:0)
  andi t1, t2, 0xF
  li t3, 10
  blt t1, t3, 11f
  addi t1, t1, ('A' - 10)
  j 12f
11:
  addi t1, t1, '0'
12:
  sw t1, 0(t0)

  # Print newline
  li t1, '\n'
  sw t1, 0(t0)

  # Halt — loop forever
13:
  j 13b
