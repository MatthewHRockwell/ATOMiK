# Test 3: NOP-padded CRT prologue
# Goal: Prove whether tight instruction fetch bursts kill execution
# Same CRT sequence but with NOP padding to slow flash reads

.section .text
.global _start

_start:
    j crtInit
    nop
    nop
    nop

crtInit:
    # Initialize UART
    li t0, 0x83000000
    nop
    nop
    nop
    li t1, 232
    nop
    nop
    nop
    sw t1, 4(t0)
    nop
    nop
    nop

    # Wait for UART
    li t2, 2000
1:  addi t2, t2, -1
    bnez t2, 1b

    # Print 'N' (NOP test started)
    li t1, 'N'
    sw t1, 0(t0)
    nop
    nop
    nop

    # Load global pointer WITH NOP PADDING
    .option push
    .option norelax
    la gp, fake_gp_target
    .option pop
    nop
    nop
    nop

    # Print 'G' (GP loaded)
    li t0, 0x83000000
    li t1, 'G'
    sw t1, 0(t0)
    nop
    nop
    nop

    # Load stack pointer WITH NOP PADDING
    la sp, fake_stack
    nop
    nop
    nop

    # Print 'P' (SP loaded)
    li t0, 0x83000000
    li t1, 'P'
    sw t1, 0(t0)

    # Infinite loop printing 'NGP'
2:  li t2, 100000
3:  addi t2, t2, -1
    bnez t2, 3b
    li t0, 0x83000000
    li t1, 'N'
    sw t1, 0(t0)
    li t1, 'G'
    sw t1, 0(t0)
    li t1, 'P'
    sw t1, 0(t0)
    j 2b

.section .data
fake_gp_target:
    .quad 0x40000840

.section .bss
fake_stack:
    .space 256
