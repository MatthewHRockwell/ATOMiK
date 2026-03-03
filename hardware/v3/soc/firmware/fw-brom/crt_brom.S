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
  # Skip gp initialization - not needed for bringup
  # Set sp from linker script symbol (grows downward from _stack_start)
  .option push
  .option norelax
  la sp, _stack_start
  .option pop

  # Skip BSS clear - not needed for bringup

  call main
infinitLoop:
  j infinitLoop
