# Minimal flash firmware for ATOMiK v3
# No AUIPC, no CRT, just print 'H' repeatedly
# This tests the complete boot chain: Boot ROM → Flash XIP

.section .text
.global _start

_start:
    # Initialize UART (same as Boot ROM)
    li t0, 0x83000000      # UART base
    li t1, 232             # CLKDIV for 27 MHz / 115200 baud
    sw t1, 4(t0)

    # Wait for UART
    li t2, 2000
1:  addi t2, t2, -1
    bnez t2, 1b

    # Print 'H' repeatedly to prove flash execution
print_loop:
    li t1, 'H'
    sw t1, 0(t0)           # UART->DATA

    # Delay
    li t2, 100000
2:  addi t2, t2, -1
    bnez t2, 2b

    j print_loop
