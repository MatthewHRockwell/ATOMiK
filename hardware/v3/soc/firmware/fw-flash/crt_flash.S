.section .text

.global crtStart
.global main

crtStart:
  j crtInit
  nop
  nop
  nop

crtInit:
  # Diagnostic: Initialize UART first so we can print debug chars
  li t0, 0x83000000    # UART base
  li t1, 232           # CLKDIV for 27 MHz
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
