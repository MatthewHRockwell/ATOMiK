# =============================================================================
# ATOMiK v3 Custom Instruction Integration Test
#
# Tests ATOMIK.LOAD, ATOMIK.ACCUM, ATOMIK.READ, ATOMIK.SWAP through the
# full CPU pipeline. Uses tohost mechanism for pass/fail reporting.
#
# Custom-0 encoding (opcode 0x0B):
#   .insn r 0x0B, funct3, 0, rd, rs1, rs2
#
# Macros:
#   ATOMIK_LOAD  rs1(addr), rs2(data) → state_table[rs1] = rs2, clear acc
#   ATOMIK_ACCUM rs1(delta)           → acc ^= rs1
#   ATOMIK_READ  rd                   → rd = initial_state ^ acc
#   ATOMIK_SWAP  rs1(addr)            → active_addr = rs1
# =============================================================================

    .section .text.init
    .globl _start

#define ATOMIK_LOAD(rs1, rs2)   .insn r 0x0B, 0, 0, x0, rs1, rs2
#define ATOMIK_ACCUM(rs1)       .insn r 0x0B, 1, 0, x0, rs1, x0
#define ATOMIK_READ(rd)         .insn r 0x0B, 2, 0, rd, x0, x0
#define ATOMIK_SWAP(rs1)        .insn r 0x0B, 3, 0, x0, rs1, x0

# Test number tracking: on failure, write (test_num << 1) to tohost
#define TEST_NUM  x28
#define TOHOST    x29

_start:
    # Load tohost address
    la      TOHOST, tohost

    # =========================================================================
    # Test 1: LOAD and READ — initial state should read back correctly
    # =========================================================================
    li      TEST_NUM, 1

    li      a0, 0                       # address = 0
    li      a1, 0x1234                  # initial state (small value for easy verify)
    ATOMIK_LOAD(a0, a1)                 # state_table[0] = 0x1234, clear acc
    ATOMIK_READ(a2)                     # a2 = initial_state ^ 0 = 0x1234

    li      a3, 0x1234
    bne     a2, a3, fail

    # =========================================================================
    # Test 2: ACCUM then READ — XOR with delta
    # =========================================================================
    li      TEST_NUM, 2

    li      a0, 0x00FF                  # delta
    ATOMIK_ACCUM(a0)                    # acc ^= 0x00FF
    ATOMIK_READ(a2)                     # a2 = 0x1234 ^ 0x00FF = 0x12CB

    li      a3, 0x12CB                  # 0x1234 ^ 0x00FF = 0x12CB
    bne     a2, a3, fail

    # =========================================================================
    # Test 3: XOR cancellation — same delta twice returns to initial
    # =========================================================================
    li      TEST_NUM, 3

    ATOMIK_ACCUM(a0)                    # acc ^= 0x00FF again (cancels)
    ATOMIK_READ(a2)                     # a2 = 0x1234 ^ 0 = 0x1234

    li      a3, 0x1234
    bne     a2, a3, fail

    # =========================================================================
    # Test 4: Multiple deltas — commutativity
    # =========================================================================
    li      TEST_NUM, 4

    li      a0, 0                       # address = 0
    li      a1, 0                       # initial state = 0
    ATOMIK_LOAD(a0, a1)                 # clear state, clear acc

    li      a0, 0x0F0F                  # d1
    ATOMIK_ACCUM(a0)
    li      a0, 0xF0F0                  # d2
    ATOMIK_ACCUM(a0)
    ATOMIK_READ(a2)                     # a2 = 0 ^ (0x0F0F ^ 0xF0F0) = 0xFFFF

    li      a3, 0xFFFF
    bne     a2, a3, fail

    # =========================================================================
    # Test 5: Commutativity — reverse order gives same result
    # =========================================================================
    li      TEST_NUM, 5

    li      a0, 0
    li      a1, 0
    ATOMIK_LOAD(a0, a1)                 # reset

    li      a0, 0xF0F0                  # d2 first
    ATOMIK_ACCUM(a0)
    li      a0, 0x0F0F                  # d1 second
    ATOMIK_ACCUM(a0)
    ATOMIK_READ(a2)

    li      a3, 0xFFFF
    bne     a2, a3, fail

    # =========================================================================
    # Test 6: SWAP — context switch preserves accumulator
    # =========================================================================
    li      TEST_NUM, 6

    # Setup context 0
    li      a0, 0
    li      a1, 0xAAAA
    ATOMIK_LOAD(a0, a1)                 # ctx0: init=0xAAAA, acc=0

    # Setup context 1
    li      a0, 1
    li      a1, 0x5555
    ATOMIK_LOAD(a0, a1)                 # ctx1: init=0x5555, acc=0

    # Accumulate in context 1
    li      a0, 0x000F
    ATOMIK_ACCUM(a0)                    # acc=0x000F
    ATOMIK_READ(a2)                     # a2 = 0x5555 ^ 0x000F = 0x555A

    li      a3, 0x555A
    bne     a2, a3, fail

    # =========================================================================
    # Test 7: SWAP to context 0 — acc persists
    # =========================================================================
    li      TEST_NUM, 7

    li      a0, 0
    ATOMIK_SWAP(a0)                     # switch to ctx0, acc=0x000F unchanged
    ATOMIK_READ(a2)                     # a2 = 0xAAAA ^ 0x000F = 0xAAA5

    li      a3, 0xAAA5
    bne     a2, a3, fail

    # =========================================================================
    # Test 8: LOAD clears accumulator
    # =========================================================================
    li      TEST_NUM, 8

    li      a0, 2
    li      a1, 0xBEEF
    ATOMIK_LOAD(a0, a1)                 # acc cleared to 0
    ATOMIK_READ(a2)                     # a2 = 0xBEEF ^ 0 = 0xBEEF

    li      a3, 0xBEEF
    bne     a2, a3, fail

    # =========================================================================
    # Test 9: SWAP back — verify context 0 initial state persists
    # =========================================================================
    li      TEST_NUM, 9

    li      a0, 0
    ATOMIK_SWAP(a0)                     # switch to ctx0, acc=0
    ATOMIK_READ(a2)                     # a2 = 0xAAAA ^ 0 = 0xAAAA

    li      a3, 0xAAAA
    bne     a2, a3, fail

    # =========================================================================
    # Test 10: 64-bit values
    # =========================================================================
    li      TEST_NUM, 10

    li      a0, 3                       # address = 3
    # Load 64-bit initial state
    li      a1, 0x0123456789ABCDEF
    ATOMIK_LOAD(a0, a1)
    ATOMIK_READ(a2)

    li      a3, 0x0123456789ABCDEF
    bne     a2, a3, fail

    # =========================================================================
    # Test 11: 64-bit XOR
    # =========================================================================
    li      TEST_NUM, 11

    li      a0, 0xFEDCBA9876543210      # delta
    ATOMIK_ACCUM(a0)
    ATOMIK_READ(a2)
    # expected: 0x0123456789ABCDEF ^ 0xFEDCBA9876543210
    #         = 0xFFFFFFFFFFFFFFFF
    li      a3, -1                      # 0xFFFFFFFFFFFFFFFF
    bne     a2, a3, fail

    # =========================================================================
    # All tests passed
    # =========================================================================
    li      a0, 1
    sw      a0, 0(TOHOST)
    j       done

fail:
    slli    a0, TEST_NUM, 1             # tohost = test_num << 1
    sw      a0, 0(TOHOST)

done:
    j       done                        # spin

    .section .tohost, "aw", @progbits
    .globl tohost
    .globl fromhost
    .align 3
tohost:   .dword 0
fromhost: .dword 0
