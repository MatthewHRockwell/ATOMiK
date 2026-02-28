# Ultra-minimal assembly test for flash execution
# NO CRT, NO C CODE - pure assembly to test if CPU can execute from flash

.section .text
.global _start

_start:
    # Initialize UART
    li t0, 0x83000000      # UART base address
    li t1, 232             # CLKDIV = 27000000/115200 - 2 = 232
    sw t1, 4(t0)           # UART->CLKDIV

    # Wait for UART to settle
    li t2, 2000
wait_loop:
    addi t2, t2, -1
    bnez t2, wait_loop

    # Print 'H' repeatedly
print_loop:
    li a0, 'H'
    sw a0, 0(t0)           # UART->DATA

    # Delay between prints
    li t3, 100000
delay:
    addi t3, t3, -1
    bnez t3, delay

    j print_loop
