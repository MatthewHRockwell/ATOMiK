# ATOMiK Custom Instruction Hardware Validation
# Tests LOAD, ACCUM, READ on real hardware via flash firmware.
#
# Expected output: 1=AB 2=54 3=AB 4=DE 5P OK
# Failure modes:
#   1 then !02_XXXX → custom-0 opcode trapped (illegal instruction)
#   1 then hang     → ATOMiK module stalls FSM
#   1=XX (wrong)    → decode works but data path issue

.section .text
.global _start

# ATOMiK custom-0 instruction macros (opcode 0x0B)
#   .insn r opcode, funct3, funct7, rd, rs1, rs2
#define ATOMIK_LOAD(rs1, rs2)   .insn r 0x0B, 0, 0, x0, rs1, rs2
#define ATOMIK_ACCUM(rs1)       .insn r 0x0B, 1, 0, x0, rs1, x0
#define ATOMIK_READ(rd)         .insn r 0x0B, 2, 0, rd, x0, x0
#define ATOMIK_SWAP(rd, rs1)    .insn r 0x0B, 3, 0, rd, rs1, x0

#define UART_BASE 0x83000000

_start:
    # Set trap vector (%hi/%lo avoids AUIPC)
    lui  t0, %hi(_trap_handler)
    addi t0, t0, %lo(_trap_handler)
    csrw mtvec, t0

    # Initialize UART
    li s0, UART_BASE           # s0 = UART base (preserved across tests)
    li t1, 185                 # CLKDIV for 21.6 MHz / 115200 baud
    sw t1, 4(s0)

    # Wait for UART
    li t2, 2000
1:  addi t2, t2, -1
    bnez t2, 1b

    li s1, 0                   # s1 = fail count

    # =========================================================================
    # Test 1: LOAD + READ round-trip
    # Load initial state 0xAB at address 0, read back
    # Expected: 1=AB
    # =========================================================================
    li t1, '1'
    sw t1, 0(s0)

    li a0, 0                   # address = 0
    li a1, 0xAB                # initial state
    ATOMIK_LOAD(a0, a1)        # state_table[0] = 0xAB, acc = 0
    ATOMIK_READ(a2)            # a2 = 0xAB ^ 0 = 0xAB

    li t1, '='
    sw t1, 0(s0)
    # Print a2[7:0] as 2 hex digits
    srli a3, a2, 4
    andi a3, a3, 0xF
    li t3, 10
    blt a3, t3, 10f
    addi a3, a3, ('A' - 10)
    j 11f
10: addi a3, a3, '0'
11: sw a3, 0(s0)
    andi a3, a2, 0xF
    blt a3, t3, 12f
    addi a3, a3, ('A' - 10)
    j 13f
12: addi a3, a3, '0'
13: sw a3, 0(s0)

    # Verify
    li a3, 0xAB
    beq a2, a3, 14f
    addi s1, s1, 1             # fail
14:
    li t1, ' '
    sw t1, 0(s0)

    # =========================================================================
    # Test 2: ACCUM + READ (XOR delta)
    # Accumulate 0xFF, read: 0xAB ^ 0xFF = 0x54
    # Expected: 2=54
    # =========================================================================
    li t1, '2'
    sw t1, 0(s0)

    li a0, 0xFF                # delta
    ATOMIK_ACCUM(a0)           # acc ^= 0xFF
    ATOMIK_READ(a2)            # a2 = 0xAB ^ 0xFF = 0x54

    li t1, '='
    sw t1, 0(s0)
    srli a3, a2, 4
    andi a3, a3, 0xF
    li t3, 10
    blt a3, t3, 20f
    addi a3, a3, ('A' - 10)
    j 21f
20: addi a3, a3, '0'
21: sw a3, 0(s0)
    andi a3, a2, 0xF
    blt a3, t3, 22f
    addi a3, a3, ('A' - 10)
    j 23f
22: addi a3, a3, '0'
23: sw a3, 0(s0)

    li a3, 0x54
    beq a2, a3, 24f
    addi s1, s1, 1
24:
    li t1, ' '
    sw t1, 0(s0)

    # =========================================================================
    # Test 3: Self-inverse (XOR cancellation)
    # Accumulate 0xFF again: acc = 0xFF ^ 0xFF = 0, read: 0xAB ^ 0 = 0xAB
    # Expected: 3=AB
    # =========================================================================
    li t1, '3'
    sw t1, 0(s0)

    li a0, 0xFF
    ATOMIK_ACCUM(a0)           # acc ^= 0xFF → acc = 0
    ATOMIK_READ(a2)            # a2 = 0xAB ^ 0 = 0xAB

    li t1, '='
    sw t1, 0(s0)
    srli a3, a2, 4
    andi a3, a3, 0xF
    li t3, 10
    blt a3, t3, 30f
    addi a3, a3, ('A' - 10)
    j 31f
30: addi a3, a3, '0'
31: sw a3, 0(s0)
    andi a3, a2, 0xF
    blt a3, t3, 32f
    addi a3, a3, ('A' - 10)
    j 33f
32: addi a3, a3, '0'
33: sw a3, 0(s0)

    li a3, 0xAB
    beq a2, a3, 34f
    addi s1, s1, 1
34:
    li t1, ' '
    sw t1, 0(s0)

    # =========================================================================
    # Test 4: LOAD new context at address 1
    # Expected: 4=DE
    # =========================================================================
    li t1, '4'
    sw t1, 0(s0)

    li a0, 1                   # address = 1
    li a1, 0xDE                # initial state
    ATOMIK_LOAD(a0, a1)        # state_table[1] = 0xDE, acc = 0
    ATOMIK_READ(a2)            # a2 = 0xDE

    li t1, '='
    sw t1, 0(s0)
    srli a3, a2, 4
    andi a3, a3, 0xF
    li t3, 10
    blt a3, t3, 40f
    addi a3, a3, ('A' - 10)
    j 41f
40: addi a3, a3, '0'
41: sw a3, 0(s0)
    andi a3, a2, 0xF
    blt a3, t3, 42f
    addi a3, a3, ('A' - 10)
    j 43f
42: addi a3, a3, '0'
43: sw a3, 0(s0)

    li a3, 0xDE
    beq a2, a3, 44f
    addi s1, s1, 1
44:
    li t1, ' '
    sw t1, 0(s0)

    # =========================================================================
    # Test 5: 64-bit value verification
    # Expected: 5P
    # =========================================================================
    li t1, '5'
    sw t1, 0(s0)

    li a0, 2                   # address = 2
    li a1, 0x0123456789ABCDEF  # 64-bit initial state
    ATOMIK_LOAD(a0, a1)
    ATOMIK_READ(a2)

    li a3, 0x0123456789ABCDEF
    beq a2, a3, 50f
    # Fail
    li t1, 'F'
    sw t1, 0(s0)
    addi s1, s1, 1
    j 51f
50: # Pass
    li t1, 'P'
    sw t1, 0(s0)
51:
    li t1, '\n'
    sw t1, 0(s0)

    # =========================================================================
    # Final result
    # =========================================================================
    bnez s1, fail_final

    li t1, 'O'
    sw t1, 0(s0)
    li t1, 'K'
    sw t1, 0(s0)
    li t1, '\n'
    sw t1, 0(s0)
    j done

fail_final:
    li t1, 'F'
    sw t1, 0(s0)
    li t1, 'A'
    sw t1, 0(s0)
    li t1, 'I'
    sw t1, 0(s0)
    li t1, 'L'
    sw t1, 0(s0)
    li t1, '\n'
    sw t1, 0(s0)

done:
    j done

# ==========================================================================
# Trap handler — prints !CC_AAAA\n (same as test_trap.S)
# ==========================================================================
.balign 4
_trap_handler:
    li t0, UART_BASE

    li t1, '!'
    sw t1, 0(t0)

    # mcause low byte as 2 hex digits
    csrr t2, mcause
    srli t1, t2, 4
    andi t1, t1, 0xF
    li t3, 10
    blt t1, t3, 60f
    addi t1, t1, ('A' - 10)
    j 61f
60: addi t1, t1, '0'
61: sw t1, 0(t0)
    andi t1, t2, 0xF
    li t3, 10
    blt t1, t3, 62f
    addi t1, t1, ('A' - 10)
    j 63f
62: addi t1, t1, '0'
63: sw t1, 0(t0)

    li t1, '_'
    sw t1, 0(t0)

    # mepc low 16 bits as 4 hex digits
    csrr t2, mepc
    srli t1, t2, 12
    andi t1, t1, 0xF
    li t3, 10
    blt t1, t3, 64f
    addi t1, t1, ('A' - 10)
    j 65f
64: addi t1, t1, '0'
65: sw t1, 0(t0)
    srli t1, t2, 8
    andi t1, t1, 0xF
    li t3, 10
    blt t1, t3, 66f
    addi t1, t1, ('A' - 10)
    j 67f
66: addi t1, t1, '0'
67: sw t1, 0(t0)
    srli t1, t2, 4
    andi t1, t1, 0xF
    li t3, 10
    blt t1, t3, 68f
    addi t1, t1, ('A' - 10)
    j 69f
68: addi t1, t1, '0'
69: sw t1, 0(t0)
    andi t1, t2, 0xF
    li t3, 10
    blt t1, t3, 70f
    addi t1, t1, ('A' - 10)
    j 71f
70: addi t1, t1, '0'
71: sw t1, 0(t0)

    li t1, '\n'
    sw t1, 0(t0)

72: j 72b
