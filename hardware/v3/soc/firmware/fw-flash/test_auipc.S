# Test 1: AUIPC-only isolation test
# Goal: Prove whether AUIPC+ADDI works from flash XIP
# No RAM access, no stack usage - pure instruction execution test

.section .text
.global _start

_start:
    # Initialize UART (using only li + sw)
    li t0, 0x83000000      # UART base
    li t1, 232             # CLKDIV for 27 MHz
    sw t1, 4(t0)           # UART->CLKDIV

    # Wait for UART to settle
    li t2, 2000
1:  addi t2, t2, -1
    bnez t2, 1b

    # TEST: Execute AUIPC + ADDI sequence (what la does)
    # This is the critical instruction pair used in CRT for address loading
    auipc gp, 0x40001      # Load PC + 0x40001000 into gp
    addi gp, gp, -2036     # Add offset (should result in 0x40000840)

    # If we survived AUIPC+ADDI, print 'A'
    li t1, 'A'
    sw t1, 0(t0)

    # Small delay
    li t2, 100000
2:  addi t2, t2, -1
    bnez t2, 2b

    # Print 'A' again to show we're still alive
    li t1, 'A'
    sw t1, 0(t0)

    # Infinite loop printing 'A'
3:  li t2, 100000
4:  addi t2, t2, -1
    bnez t2, 4b
    li t1, 'A'
    sw t1, 0(t0)
    j 3b
