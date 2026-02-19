.section .text

.global crtStart
.global main

crtStart:
  j crtInit
  nop
  nop
  nop

crtInit:
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop
  la sp, _stack_start

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

  call main
infinitLoop:
  j infinitLoop
