# Test 2: Stack/RAM touch isolation test
# Goal: Prove whether first RAM access kills execution
# No AUIPC - set stack pointer with li, then touch RAM

.section .text
.global _start

_start:
    # Initialize UART
    li t0, 0x83000000      # UART base
    li t1, 232             # CLKDIV
    sw t1, 4(t0)

    # Wait for UART
    li t2, 2000
1:  addi t2, t2, -1
    bnez t2, 1b

    # Set stack pointer using li (NO AUIPC)
    li sp, 0x40000BB0      # Stack start address from linker script

    # TEST: Touch RAM via stack
    # This is what CRT does when setting up stack frame
    li t3, 0xDEADBEEF
    sd t3, 0(sp)           # Store to RAM

    # If we survived RAM write, print 'S'
    li t1, 'S'
    sw t1, 0(t0)

    # Small delay
    li t2, 100000
2:  addi t2, t2, -1
    bnez t2, 2b

    # Read back from RAM and verify
    ld t4, 0(sp)
    li t5, 0xDEADBEEF
    bne t4, t5, ram_fail

    # RAM read/write works! Print 'R'
    li t1, 'R'
    sw t1, 0(t0)

    # Infinite loop printing 'SR'
3:  li t2, 100000
4:  addi t2, t2, -1
    bnez t2, 4b
    li t1, 'S'
    sw t1, 0(t0)
    li t1, 'R'
    sw t1, 0(t0)
    j 3b

ram_fail:
    # RAM readback failed - print 'F'
    li t1, 'F'
    sw t1, 0(t0)
5:  j 5b
