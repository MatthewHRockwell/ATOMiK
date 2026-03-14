/* ============================================================================
 * ATOMiK Zynq Co-Processor Startup Code (RV64I)
 *
 * Entry point for the RV64I co-processor running in Zynq PL fabric.
 * ARM loads this binary to DDR3 at 0x10000000 (physical), then deasserts
 * co-processor reset via mailbox CTRL[0]. The bus_to_axi bridge translates
 * CPU address 0x00000000 → DDR3 0x10000000.
 *
 * Memory map:
 *   0x00000000  DDR3 (code, via AXI HP port)
 *   0x40000000  Local BRAM (32 KB, stack + data)
 *   0x80000000  Mailbox registers
 *
 * No UART — diagnostics go to mailbox DEBUG_PC (read by ARM) and
 * RESP0/RESP1 registers.
 *
 * ATOMiK Project — March 2026
 * ============================================================================ */

.section .text

.global crtStart
.global main

/* Mailbox register offsets */
.equ MBOX_BASE,    0x80000000
.equ MBOX_CTRL,    0x00
.equ MBOX_STATUS,  0x04
.equ MBOX_CMD,     0x08
.equ MBOX_RESP0,   0x18
.equ MBOX_RESP1,   0x1C

crtStart:
  j crtInit
  nop
  nop
  nop

crtInit:
  /* Set trap vector — traps write cause+PC to mailbox, then halt */
  lui  t0, %hi(_zynq_trap_handler)
  addi t0, t0, %lo(_zynq_trap_handler)
  csrw mtvec, t0

  /* Set global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  /* Set stack pointer (top of BRAM stack region) */
  la sp, _stack_start

  /* Copy .data section from DDR3 to BRAM */
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

  /* Zero BSS */
  la a0, _bss_start
  la a1, _bss_end
bss_loop:
  beq a0, a1, bss_done
  sd zero, 0(a0)
  addi a0, a0, 8
  j bss_loop
bss_done:

  /* Run constructors */
  la a0, _ctors_start
  addi sp, sp, -8
ctors_loop:
  la a1, _ctors_end
  beq a0, a1, ctors_done
  ld a3, 0(a0)
  addi a0, a0, 8
  sd a0, 0(sp)
  jalr a3
  ld a0, 0(sp)
  j ctors_loop
ctors_done:
  addi sp, sp, 8

  /* Call main() */
  call main

  /* If main returns, write return value to RESP0 and halt */
  li t0, MBOX_BASE
  sw a0, MBOX_RESP0(t0)
  /* Fall through to infinite loop */
halt_loop:
  j halt_loop


/* ============================================================================
 * Trap handler — writes mcause and mepc to mailbox, then halts
 *
 * RESP0 = mcause (trap cause)
 * RESP1 = mepc   (faulting PC, low 32 bits)
 *
 * This allows ARM to diagnose co-processor traps via mailbox reads.
 * ============================================================================ */
.balign 4
_zynq_trap_handler:
  li t0, MBOX_BASE

  /* Write mcause to RESP0 */
  csrr t1, mcause
  sw t1, MBOX_RESP0(t0)

  /* Write mepc to RESP1 */
  csrr t1, mepc
  sw t1, MBOX_RESP1(t0)

  /* Halt forever */
trap_halt:
  j trap_halt
