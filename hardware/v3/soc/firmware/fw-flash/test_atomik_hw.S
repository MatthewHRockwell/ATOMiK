# ATOMiK Custom Instruction Hardware Validation (Extended)
#
# Tests LOAD, ACCUM, READ, SWAP, multi-context on real hardware.
#
# Operand mapping:
#   ATOMIK_LOAD  rs1=address(a0), rs2=initial_state(a1) → state_table[a0] = a1, acc=0
#   ATOMIK_ACCUM rs1=delta(a0)                          → acc ^= a0
#   ATOMIK_READ  rd=result(a2)                          → a2 = state_table[active] ^ acc
#   ATOMIK_SWAP  rd=old_state(a2), rs1=new_addr(a0)     → save current, switch to a0, acc=0
#
# Expected output:
#   1=AB 2=54 3=AB 4=DE 5P 6=5A 7=AA 8P 9P
#   OK
#
# Tests:
#   1: LOAD addr=0 data=0xAB + READ           → 0xAB (round-trip)
#   2: ACCUM delta=0xFF + READ                 → 0x54 (XOR delta)
#   3: ACCUM delta=0xFF + READ                 → 0xAB (self-inverse)
#   4: LOAD addr=1 data=0xDE + READ            → 0xDE (new context)
#   5: LOAD addr=2 data=64-bit + READ          → 64-bit verify (P/F)
#   6: SWAP semantics: accum in ctx1, SWAP to ctx0, read
#   7: Read ctx0 after SWAP (acc should be 0 after swap)
#   8: Multi-context: load 16 contexts, read back in permuted order (P/F)
#   9: SWAP return value: verify rd gets pre-swap state (P/F)

.section .text
.global _start

# ATOMiK custom-0 instruction macros (opcode 0x0B)
#define ATOMIK_LOAD(rs1, rs2)   .insn r 0x0B, 0, 0, x0, rs1, rs2
#define ATOMIK_ACCUM(rs1)       .insn r 0x0B, 1, 0, x0, rs1, x0
#define ATOMIK_READ(rd)         .insn r 0x0B, 2, 0, rd, x0, x0
#define ATOMIK_SWAP(rd, rs1)    .insn r 0x0B, 3, 0, rd, rs1, x0

#define UART_BASE 0x83000000

# ---- Hex byte print macro (inline) ----
# Prints register[7:0] as 2 hex digits via UART at s0
# Uses a3, t3 as scratch. Clobbers a3.
.macro PRINT_HEX_BYTE reg
    srli a3, \reg, 4
    andi a3, a3, 0xF
    li t3, 10
    blt a3, t3, 7778f
    addi a3, a3, ('A' - 10)
    j 7779f
7778: addi a3, a3, '0'
7779: sw a3, 0(s0)
    andi a3, \reg, 0xF
    blt a3, t3, 7780f
    addi a3, a3, ('A' - 10)
    j 7781f
7780: addi a3, a3, '0'
7781: sw a3, 0(s0)
.endm

.macro PRINT_CHAR ch
    li t1, \ch
    sw t1, 0(s0)
.endm

_start:
    # Set trap vector (%hi/%lo avoids AUIPC)
    lui  t0, %hi(_trap_handler)
    addi t0, t0, %lo(_trap_handler)
    csrw mtvec, t0

    # Initialize UART
    li s0, UART_BASE
    li t1, 185                 # CLKDIV for 21.6 MHz / 115200 baud
    sw t1, 4(s0)

    # Wait for UART
    li t2, 2000
1:  addi t2, t2, -1
    bnez t2, 1b

    li s1, 0                   # s1 = fail count

    # =========================================================================
    # Test 1: LOAD + READ round-trip (addr=0, data=0xAB)
    # Expected: 1=AB
    # =========================================================================
    PRINT_CHAR '1'
    li a0, 0
    li a1, 0xAB
    ATOMIK_LOAD(a0, a1)
    ATOMIK_READ(a2)
    PRINT_CHAR '='
    PRINT_HEX_BYTE a2
    li a3, 0xAB
    beq a2, a3, 14f
    addi s1, s1, 1
14: PRINT_CHAR ' '

    # =========================================================================
    # Test 2: ACCUM + READ (delta=0xFF → 0xAB^0xFF=0x54)
    # Expected: 2=54
    # =========================================================================
    PRINT_CHAR '2'
    li a0, 0xFF
    ATOMIK_ACCUM(a0)
    ATOMIK_READ(a2)
    PRINT_CHAR '='
    PRINT_HEX_BYTE a2
    li a3, 0x54
    beq a2, a3, 24f
    addi s1, s1, 1
24: PRINT_CHAR ' '

    # =========================================================================
    # Test 3: Self-inverse (ACCUM 0xFF again → acc cancels → 0xAB)
    # Expected: 3=AB
    # =========================================================================
    PRINT_CHAR '3'
    li a0, 0xFF
    ATOMIK_ACCUM(a0)
    ATOMIK_READ(a2)
    PRINT_CHAR '='
    PRINT_HEX_BYTE a2
    li a3, 0xAB
    beq a2, a3, 34f
    addi s1, s1, 1
34: PRINT_CHAR ' '

    # =========================================================================
    # Test 4: LOAD new context (addr=1, data=0xDE)
    # Expected: 4=DE
    # =========================================================================
    PRINT_CHAR '4'
    li a0, 1
    li a1, 0xDE
    ATOMIK_LOAD(a0, a1)
    ATOMIK_READ(a2)
    PRINT_CHAR '='
    PRINT_HEX_BYTE a2
    li a3, 0xDE
    beq a2, a3, 44f
    addi s1, s1, 1
44: PRINT_CHAR ' '

    # =========================================================================
    # Test 5: 64-bit value (addr=2, data=0x0123456789ABCDEF)
    # Expected: 5P
    # =========================================================================
    PRINT_CHAR '5'
    li a0, 2
    li a1, 0x0123456789ABCDEF
    ATOMIK_LOAD(a0, a1)
    ATOMIK_READ(a2)
    li a3, 0x0123456789ABCDEF
    beq a2, a3, 50f
    PRINT_CHAR 'F'
    addi s1, s1, 1
    j 51f
50: PRINT_CHAR 'P'
51: PRINT_CHAR ' '

    # =========================================================================
    # Test 6: SWAP semantics
    # Setup: LOAD ctx10=0xAA, LOAD ctx11=0x55, ACCUM 0x0F in ctx11
    # Then SWAP to ctx10 — acc clears per RTL (swap_pending), READ ctx10
    # RTL behavior: acc clears after SWAP → READ = 0xAA ^ 0 = 0xAA
    #
    # We print the actual value to show what hardware does.
    # Expected: 6=AA (acc clears on SWAP)
    # =========================================================================
    PRINT_CHAR '6'

    li a0, 10                  # context ID 10
    li a1, 0xAA
    ATOMIK_LOAD(a0, a1)       # state[10]=0xAA, acc=0, active=10

    li a0, 11                  # context ID 11
    li a1, 0x55
    ATOMIK_LOAD(a0, a1)       # state[11]=0x55, acc=0, active=11

    li a0, 0x0F
    ATOMIK_ACCUM(a0)           # acc=0x0F, active=11

    # Verify pre-swap state of ctx11
    ATOMIK_READ(a2)            # should be 0x55 ^ 0x0F = 0x5A
    PRINT_CHAR '='
    PRINT_HEX_BYTE a2
    li a3, 0x5A
    beq a2, a3, 64f
    addi s1, s1, 1
64: PRINT_CHAR ' '

    # =========================================================================
    # Test 7: SWAP to ctx10, then READ
    # SWAP saves ctx11 state (0x5A) to state[11], switches to ctx10, acc clears
    # READ ctx10 = state[10] ^ 0 = 0xAA
    # Expected: 7=AA
    # =========================================================================
    PRINT_CHAR '7'

    li a0, 10
    ATOMIK_SWAP(a2, a0)        # rd=a2 gets pre-swap state, switch to ctx10

    # Now read ctx10 — acc should be 0 after SWAP
    ATOMIK_READ(a2)
    PRINT_CHAR '='
    PRINT_HEX_BYTE a2
    li a3, 0xAA
    beq a2, a3, 74f
    addi s1, s1, 1
74: PRINT_CHAR ' '

    # =========================================================================
    # Test 8: Multi-context — load 16 contexts with unique patterns, read back
    # Pattern: context N gets value (N * 0x11), for N=0x20..0x2F
    # Read back in reverse order to detect aliasing
    # Expected: 8P
    # =========================================================================
    PRINT_CHAR '8'

    # Load 16 contexts
    li s2, 0x20                # start address
    li s3, 0x30                # end address (exclusive)
    mv a0, s2
80: li a1, 0
    slli a1, a0, 4             # value = addr << 4 (gives 0x200, 0x210, 0x220...)
    ori  a1, a1, 0x0B          # add marker bits → 0x20B, 0x21B, 0x22B...
    ATOMIK_LOAD(a0, a1)
    addi a0, a0, 1
    blt a0, s3, 80b

    # Read back in reverse order
    li s4, 0                   # local fail flag
    addi a0, s3, -1            # start from 0x2F
81: # Need to LOAD to set active_addr (READ uses active_addr, not an operand)
    # But LOAD overwrites state! We need a way to select context for READ.
    # Actually, LOAD sets active_addr AND writes state table. We can't use LOAD
    # just to switch contexts — that would overwrite the state.
    # We need SWAP to switch contexts without losing state.
    # SWAP(rd, rs1): save current_state to state[active], switch to rs1, acc=0
    # So we can chain SWAPs to read each context.

    # For the first context, use LOAD with the same value to set active_addr
    # Actually, we can just SWAP into each context and READ.
    # After SWAP, acc=0, so READ gives raw state_table value.

    ATOMIK_SWAP(a2, a0)        # switch to context a0, acc clears
    ATOMIK_READ(a2)            # read state[a0] ^ 0

    # Expected value: (a0 << 4) | 0x0B
    slli a3, a0, 4
    ori  a3, a3, 0x0B
    beq a2, a3, 82f
    addi s4, s4, 1             # mismatch
82: addi a0, a0, -1
    bge  a0, s2, 81b

    beqz s4, 83f
    PRINT_CHAR 'F'
    addi s1, s1, 1
    j 84f
83: PRINT_CHAR 'P'
84: PRINT_CHAR ' '

    # =========================================================================
    # Test 9: SWAP return value — rd should get pre-swap current_state
    # Setup: LOAD ctx30=0xBE, ACCUM 0x01 → current=0xBF
    # SWAP to ctx31 → rd should be 0xBF (pre-swap state of ctx30)
    # Expected: 9P
    # =========================================================================
    PRINT_CHAR '9'

    li a0, 30
    li a1, 0xBE
    ATOMIK_LOAD(a0, a1)       # ctx30: init=0xBE, acc=0

    li a0, 0x01
    ATOMIK_ACCUM(a0)           # acc=0x01, current=0xBE^0x01=0xBF

    li a0, 31
    ATOMIK_SWAP(a2, a0)        # rd=a2 should be 0xBF (pre-swap current_state)

    li a3, 0xBF
    beq a2, a3, 90f
    # Print what we got for debugging
    PRINT_CHAR '='
    PRINT_HEX_BYTE a2
    addi s1, s1, 1
    j 91f
90: PRINT_CHAR 'P'
91: PRINT_CHAR '\n'

    # =========================================================================
    # Final result
    # =========================================================================
    bnez s1, fail_final

    PRINT_CHAR 'O'
    PRINT_CHAR 'K'
    PRINT_CHAR '\n'
    j done

fail_final:
    PRINT_CHAR 'F'
    PRINT_CHAR 'A'
    PRINT_CHAR 'I'
    PRINT_CHAR 'L'
    PRINT_CHAR '\n'

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
