# Trap handler test firmware for ATOMiK v3
# Tests illegal instruction trap: should print !02_XXXX
#
# Expected output:
#   1         (test marker: about to trigger illegal instruction)
#   !02_XXXX  (trap: cause 2 = illegal instruction, XXXX = address of .word 0)
#
# The trap handler halts after printing, so only one trap message appears.

.section .text
.global _start

_start:
    # Set trap vector
    .option push
    .option norelax
    la t0, _trap_handler
    csrw mtvec, t0
    .option pop

    # Initialize UART
    li t0, 0x83000000      # UART base
    li t1, 185             # CLKDIV for 21.6 MHz / 115200 baud
    sw t1, 4(t0)

    # Wait for UART
    li t2, 2000
1:  addi t2, t2, -1
    bnez t2, 1b

    # Print '1' — about to trigger illegal instruction
    li t1, '1'
    sw t1, 0(t0)
    li t1, '\n'
    sw t1, 0(t0)

    # === TEST: Illegal instruction ===
    # All-zeros is not a valid RV64I instruction
    .word 0x00000000

    # Should never reach here
    li t1, 'X'
    sw t1, 0(t0)

2:  j 2b

# ==========================================================================
# Trap handler — identical to crt_flash.S version
# Prints: !CC_AAAA\n  (CC=mcause low byte, AAAA=mepc low 16 bits)
# ==========================================================================
.balign 4
_trap_handler:
    li t0, 0x83000000

    # Print '!'
    li t1, '!'
    sw t1, 0(t0)

    # mcause low byte as 2 hex digits
    csrr t2, mcause
    srli t1, t2, 4
    andi t1, t1, 0xF
    li t3, 10
    blt t1, t3, 3f
    addi t1, t1, ('A' - 10)
    j 4f
3:  addi t1, t1, '0'
4:  sw t1, 0(t0)
    andi t1, t2, 0xF
    li t3, 10
    blt t1, t3, 5f
    addi t1, t1, ('A' - 10)
    j 6f
5:  addi t1, t1, '0'
6:  sw t1, 0(t0)

    # Print '_'
    li t1, '_'
    sw t1, 0(t0)

    # mepc low 16 bits as 4 hex digits
    csrr t2, mepc
    srli t1, t2, 12
    andi t1, t1, 0xF
    li t3, 10
    blt t1, t3, 7f
    addi t1, t1, ('A' - 10)
    j 8f
7:  addi t1, t1, '0'
8:  sw t1, 0(t0)
    srli t1, t2, 8
    andi t1, t1, 0xF
    li t3, 10
    blt t1, t3, 9f
    addi t1, t1, ('A' - 10)
    j 10f
9:  addi t1, t1, '0'
10: sw t1, 0(t0)
    srli t1, t2, 4
    andi t1, t1, 0xF
    li t3, 10
    blt t1, t3, 11f
    addi t1, t1, ('A' - 10)
    j 12f
11: addi t1, t1, '0'
12: sw t1, 0(t0)
    andi t1, t2, 0xF
    li t3, 10
    blt t1, t3, 13f
    addi t1, t1, ('A' - 10)
    j 14f
13: addi t1, t1, '0'
14: sw t1, 0(t0)

    # Print newline
    li t1, '\n'
    sw t1, 0(t0)

    # Halt
15: j 15b
