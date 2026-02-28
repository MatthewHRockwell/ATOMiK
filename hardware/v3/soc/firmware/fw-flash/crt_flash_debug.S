# Startup code with GPIO debug outputs
# Each stage sets a different LED pattern:
# 0x10 = crtStart reached
# 0x20 = GP/SP initialized
# 0x30 = data copy done
# 0x38 = BSS clear done
# 0x3C = about to call main

.section .text

.global crtStart
.global main

#define GPIO_BASE 0x82000000
#define GPIO_OE_OFFSET 8

crtStart:
  # Signal: reached crtStart (LED pattern 0x10)
  li t0, GPIO_BASE
  li t1, 0x3F
  sw t1, GPIO_OE_OFFSET(t0)  # Set all LEDs as outputs
  li t1, 0x10
  sw t1, 0(t0)               # OUT = 0x10

  j crtInit
  nop
  nop
  nop

crtInit:
  # Signal: starting init (LED pattern 0x20)
  li t0, GPIO_BASE
  li t1, 0x20
  sw t1, 0(t0)

  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop
  la sp, _stack_start

  # Signal: GP/SP initialized (LED pattern 0x28)
  li t0, GPIO_BASE
  li t1, 0x28
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

  # Signal: data copy done (LED pattern 0x30)
  li t0, GPIO_BASE
  li t1, 0x30
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

  # Signal: BSS cleared (LED pattern 0x38)
  li t0, GPIO_BASE
  li t1, 0x38
  sw t1, 0(t0)

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

  # Signal: about to call main (LED pattern 0x3C)
  li t0, GPIO_BASE
  li t1, 0x3C
  sw t1, 0(t0)

  call main
infinitLoop:
  j infinitLoop
