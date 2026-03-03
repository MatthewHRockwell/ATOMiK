.section .vector,"ax"

.global _start
.global main

_start:
  j crtInit
  nop
  nop
  nop
  nop
  nop
  nop
  nop

crtInit:
  # Set trap vector FIRST — before anything else can fault
  .option push
  .option norelax
  la t0, _trap_handler
  csrw mtvec, t0
  .option pop

  # Set sp from linker script symbol (grows downward from _stack_start)
  .option push
  .option norelax
  la sp, _stack_start
  .option pop

  call main
infinitLoop:
  j infinitLoop

# ==========================================================================
# Trap handler — self-contained, no stack dependency
# Prints: !CC_AAAA\n  (CC=mcause low byte, AAAA=mepc low 16 bits)
# Uses only t0-t3, saves nothing (handler loops forever)
# ==========================================================================
.balign 4
_trap_handler:
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
