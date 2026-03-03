# Misaligned access trap test for ATOMiK v3
# Tests:
#   1. Misaligned load (cause 4) — LW from odd address
#   2. Misaligned store (cause 6) — SW to odd address
#
# Expected output:
#   L         (about to do misaligned load)
#   !04_XXXX  (load address misaligned)
#
# Note: trap handler halts, so only one test runs per boot.
# To test store misaligned, comment out the load test and rebuild.

.section .text
.global _start

_start:
    # Set trap vector (%hi/%lo avoids AUIPC)
    lui  t0, %hi(_trap_handler)
    addi t0, t0, %lo(_trap_handler)
    csrw mtvec, t0

    # Initialize UART
    li t0, 0x83000000
    li t1, 185             # 21.6 MHz / 115200
    sw t1, 4(t0)
    li t2, 2000
1:  addi t2, t2, -1
    bnez t2, 1b

    # Print 'L' — about to do misaligned load
    li t1, 'L'
    sw t1, 0(t0)
    li t1, '\n'
    sw t1, 0(t0)

    # === TEST: Misaligned LW (address 0x40000001 — odd, in SRAM) ===
    li t0, 0x40000001
    lw t1, 0(t0)          # Should trap with cause 4

    # Should never reach here
    li t0, 0x83000000
    li t1, 'X'
    sw t1, 0(t0)
2:  j 2b

# ==========================================================================
# Trap handler (same as crt_brom.S)
# ==========================================================================
.balign 4
_trap_handler:
    li t0, 0x83000000
    li t1, '!'
    sw t1, 0(t0)

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

    li t1, '_'
    sw t1, 0(t0)

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

    li t1, '\n'
    sw t1, 0(t0)

15: j 15b
