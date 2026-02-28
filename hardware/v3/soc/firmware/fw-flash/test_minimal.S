.section .text
.global crtStart

crtStart:
    # Set UART baud rate: 15750000 / 115200 - 2 = 134
    li t0, 0x83000000
    li t1, 134
    sw t1, 4(t0)        # UART0->CLKDIV

loop:
    li t1, 72           # 'H' = 0x48
    sw t1, 0(t0)        # UART0->DATA
    # Small delay so UART can send
    li t2, 10000
delay:
    addi t2, t2, -1
    bnez t2, delay
    j loop
