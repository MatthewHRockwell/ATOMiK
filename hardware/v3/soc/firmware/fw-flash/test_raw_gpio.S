# Absolute minimal test - write to GPIO immediately at address 0
# No C runtime, no initialization, just raw assembly

.section .text
.global crtStart

#define GPIO_BASE 0x82000000

crtStart:
    # Set all GPIOs as outputs (write 0x3F to OE register at offset 8)
    li t0, GPIO_BASE
    li t1, 0x3F
    sw t1, 8(t0)

    # Set LED pattern 0x15 (binary 010101 - alternating LEDs)
    li t1, 0x15
    sw t1, 0(t0)

    # Wait a bit
    li t2, 1000000
delay1:
    addi t2, t2, -1
    bnez t2, delay1

    # Change to 0x2A (binary 101010 - opposite pattern)
    li t1, 0x2A
    sw t1, 0(t0)

    # Wait
    li t2, 1000000
delay2:
    addi t2, t2, -1
    bnez t2, delay2

    # Blink pattern 0x3F continuously
loop:
    li t1, 0x3F
    sw t1, 0(t0)

    li t2, 500000
delay3:
    addi t2, t2, -1
    bnez t2, delay3

    li t1, 0x00
    sw t1, 0(t0)

    li t2, 500000
delay4:
    addi t2, t2, -1
    bnez t2, delay4

    j loop

    # Padding to match expected entry point structure
    nop
    nop
    nop
