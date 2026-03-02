# Step 4.3: ATOMiK full instruction + state reconstruction test
#
# Register allocation:
#   t0 (x5)  = UART base (0x83000000)
#   t1 (x6)  = temp for UART writes
#   t2 (x7)  = temp for delays/comparisons
#   a0 (x10) = ATOMiK address (rs1 for load/swap)
#   a1 (x11) = ATOMiK init_data/delta (rs2 for load, rs1 for accum)
#   a2 (x12) = ATOMiK read result (rd for read/swap)
#   a3 (x13) = expected value
#   a4 (x14) = temp for hex print
#   a5 (x15) = temp for hex print
#
# ATOMiK instruction encodings (custom-0, opcode=0x0B):
#   atomik.load  rs1=a0, rs2=a1 : 0x00B5000B  (funct3=000)
#   atomik.accum rs1=a1         : 0x0005900B  (funct3=001)
#   atomik.read  rd=a2          : 0x0000260B  (funct3=010)
#   atomik.swap  rd=a2, rs1=a0  : 0x0005360B  (funct3=011)
#
# ATOMiK datapath (from atomik_v3_atomik.v):
#   LOAD:  state_table[rs1] = rs2, active_addr = rs1, acc = 0
#   ACCUM: acc ^= rs1
#   READ:  rd = state_table[active_addr] ^ acc
#   SWAP:  rd = state_table[active_addr] ^ acc,
#          state_table[active_addr] = current_state, acc = 0 (1 cycle later)
#
# Expected output: "V3 123456\n" then "PPPP..."
# If any test fails: digit replaced with 'x' + hex dump of actual value

.section .vector,"ax"
.global _start

_start:
    # UART base = 0x83000000
    lui t0, 0x83000

    # Set CLKDIV = 217 for 115200 baud
    li t1, 217
    sw t1, 4(t0)

    # s0 (x8) = fail flag (0 = all pass, 1 = any fail)
    li s0, 0

    # --- Print version stamp "V3 " ---
    li t1, 86        # 'V'
    sw t1, 0(t0)
    li t1, 51        # '3'
    sw t1, 0(t0)
    li t1, 32        # ' '
    sw t1, 0(t0)

    # --- Set mtvec to trap handler ---
    la t1, trap_handler
    csrw mtvec, t1

    # =====================================================================
    # Test 1: LOAD + READ  (acc=0, should return initial state)
    # Load initial_state = 0xAA at addr 0, read back
    # Expected: read == 0xAA
    # =====================================================================
    li a0, 0          # address = 0
    li a1, 0xAA       # init_state = 0xAA
    .word 0x00B5000B  # atomik.load  rs1=a0, rs2=a1
    .word 0x0000260B  # atomik.read  rd=a2

    li a3, 0xAA
    bne a2, a3, test1_fail
    li t1, 49        # '1' = pass
    sw t1, 0(t0)
    j test2
test1_fail:
    li s0, 1         # set fail flag
    li t1, 120       # 'x' = fail
    sw t1, 0(t0)
    # Print actual value (low byte)
    mv a4, a2
    jal x0, print_hex_byte_then_test2

    # =====================================================================
    # Test 2: ACCUM + READ  (acc ^= 0x11, read = 0xAA ^ 0x11 = 0xBB)
    # =====================================================================
test2:
    li a1, 0x11       # delta = 0x11
    .word 0x0005900B  # atomik.accum rs1=a1
    .word 0x0000260B  # atomik.read  rd=a2

    li a3, 0xBB       # 0xAA ^ 0x11 = 0xBB
    bne a2, a3, test2_fail
    li t1, 50        # '2' = pass
    sw t1, 0(t0)
    j test3
test2_fail:
    li s0, 1
    li t1, 120       # 'x'
    sw t1, 0(t0)
    mv a4, a2
    jal x0, print_hex_byte_then_test3

    # =====================================================================
    # Test 3: Self-inverse (accum 0x11 again → acc=0, read = 0xAA)
    # =====================================================================
test3:
    li a1, 0x11
    .word 0x0005900B  # atomik.accum rs1=a1 (acc = 0x11 ^ 0x11 = 0)
    .word 0x0000260B  # atomik.read  rd=a2

    li a3, 0xAA       # 0xAA ^ 0 = 0xAA (self-inverse!)
    bne a2, a3, test3_fail
    li t1, 51        # '3' = pass
    sw t1, 0(t0)
    j test4
test3_fail:
    li s0, 1
    li t1, 120       # 'x'
    sw t1, 0(t0)
    mv a4, a2
    jal x0, print_hex_byte_then_test4

    # =====================================================================
    # Test 4: Accum different delta (acc ^= 0x55, read = 0xAA ^ 0x55 = 0xFF)
    # =====================================================================
test4:
    li a1, 0x55
    .word 0x0005900B  # atomik.accum rs1=a1
    .word 0x0000260B  # atomik.read  rd=a2

    li a3, 0xFF       # 0xAA ^ 0x55 = 0xFF
    bne a2, a3, test4_fail
    li t1, 52        # '4' = pass
    sw t1, 0(t0)
    j test5
test4_fail:
    li s0, 1
    li t1, 120       # 'x'
    sw t1, 0(t0)
    mv a4, a2
    jal x0, print_hex_byte_then_test5

    # =====================================================================
    # Test 5: SWAP (returns current_state=0xFF, clears acc, updates ref to 0xFF)
    # =====================================================================
test5:
    .word 0x0005360B  # atomik.swap rd=a2, rs1=a0

    li a3, 0xFF       # current_state before swap = 0xAA ^ 0x55 = 0xFF
    bne a2, a3, test5_fail
    li t1, 53        # '5' = pass
    sw t1, 0(t0)
    j test6
test5_fail:
    li s0, 1
    li t1, 120       # 'x'
    sw t1, 0(t0)
    mv a4, a2
    jal x0, print_hex_byte_then_test6

    # =====================================================================
    # Test 6: Read after swap (ref=0xFF, acc=0 → read=0xFF)
    # =====================================================================
test6:
    .word 0x0000260B  # atomik.read  rd=a2

    li a3, 0xFF       # new ref=0xFF, acc=0 → 0xFF ^ 0 = 0xFF
    bne a2, a3, test6_fail
    li t1, 54        # '6' = pass
    sw t1, 0(t0)
    j all_done
test6_fail:
    li s0, 1
    li t1, 120       # 'x'
    sw t1, 0(t0)
    mv a4, a2
    jal x0, print_hex_byte_then_done

    # =====================================================================
    # All tests done — print newline and loop 'P'
    # =====================================================================
all_done:
    li t1, 10        # '\n'
    sw t1, 0(t0)

pass_loop:
    bnez s0, fail_char
    li t1, 80        # 'P' = all pass
    j print_loop_char
fail_char:
    li t1, 70        # 'F' = some failed
print_loop_char:
    sw t1, 0(t0)
    li t1, 10        # '\n'
    sw t1, 0(t0)

    li t2, 100000
1:  addi t2, t2, -1
    bnez t2, 1b
    j pass_loop

    # =====================================================================
    # Hex print helper: prints low byte of a4 as 2 hex digits
    # Uses a5 as temp. Falls through to jump targets.
    # (No JAL/stack — uses inline jump targets)
    # =====================================================================
print_hex_byte_then_test2:
    srli a5, a4, 4
    andi a5, a5, 0xF
    li t1, 48         # '0'
    add t1, t1, a5
    li t2, 58
    blt t1, t2, 1f
    addi t1, t1, 7    # 'A'-'9'-1 = 7
1:  sw t1, 0(t0)
    andi a5, a4, 0xF
    li t1, 48
    add t1, t1, a5
    li t2, 58
    blt t1, t2, 1f
    addi t1, t1, 7
1:  sw t1, 0(t0)
    j test2

print_hex_byte_then_test3:
    srli a5, a4, 4
    andi a5, a5, 0xF
    li t1, 48
    add t1, t1, a5
    li t2, 58
    blt t1, t2, 1f
    addi t1, t1, 7
1:  sw t1, 0(t0)
    andi a5, a4, 0xF
    li t1, 48
    add t1, t1, a5
    li t2, 58
    blt t1, t2, 1f
    addi t1, t1, 7
1:  sw t1, 0(t0)
    j test3

print_hex_byte_then_test4:
    srli a5, a4, 4
    andi a5, a5, 0xF
    li t1, 48
    add t1, t1, a5
    li t2, 58
    blt t1, t2, 1f
    addi t1, t1, 7
1:  sw t1, 0(t0)
    andi a5, a4, 0xF
    li t1, 48
    add t1, t1, a5
    li t2, 58
    blt t1, t2, 1f
    addi t1, t1, 7
1:  sw t1, 0(t0)
    j test4

print_hex_byte_then_test5:
    srli a5, a4, 4
    andi a5, a5, 0xF
    li t1, 48
    add t1, t1, a5
    li t2, 58
    blt t1, t2, 1f
    addi t1, t1, 7
1:  sw t1, 0(t0)
    andi a5, a4, 0xF
    li t1, 48
    add t1, t1, a5
    li t2, 58
    blt t1, t2, 1f
    addi t1, t1, 7
1:  sw t1, 0(t0)
    j test5

print_hex_byte_then_test6:
    srli a5, a4, 4
    andi a5, a5, 0xF
    li t1, 48
    add t1, t1, a5
    li t2, 58
    blt t1, t2, 1f
    addi t1, t1, 7
1:  sw t1, 0(t0)
    andi a5, a4, 0xF
    li t1, 48
    add t1, t1, a5
    li t2, 58
    blt t1, t2, 1f
    addi t1, t1, 7
1:  sw t1, 0(t0)
    j test6

print_hex_byte_then_done:
    srli a5, a4, 4
    andi a5, a5, 0xF
    li t1, 48
    add t1, t1, a5
    li t2, 58
    blt t1, t2, 1f
    addi t1, t1, 7
1:  sw t1, 0(t0)
    andi a5, a4, 0xF
    li t1, 48
    add t1, t1, a5
    li t2, 58
    blt t1, t2, 1f
    addi t1, t1, 7
1:  sw t1, 0(t0)
    j all_done

    # Padding to separate trap handler
    .align 8
    .space 64

# --- Trap handler ---
trap_handler:
    lui t0, 0x83000

    li t1, 84        # 'T'
    sw t1, 0(t0)
    li t1, 33        # '!'
    sw t1, 0(t0)

    csrr t2, mcause
    andi t2, t2, 0xF
    li t1, 48
    add t1, t1, t2
    li a5, 58
    blt t1, a5, 1f
    addi t1, t1, 7
1:  sw t1, 0(t0)

    # Print mepc low byte for debugging
    li t1, 64        # '@'
    sw t1, 0(t0)
    csrr t2, mepc
    # Print low 2 nibbles of mepc
    srli a5, t2, 4
    andi a5, a5, 0xF
    li t1, 48
    add t1, t1, a5
    li a5, 58
    blt t1, a5, 1f
    addi t1, t1, 7
1:  sw t1, 0(t0)
    andi a5, t2, 0xF
    li t1, 48
    add t1, t1, a5
    li a5, 58
    blt t1, a5, 1f
    addi t1, t1, 7
1:  sw t1, 0(t0)

    li t1, 10        # '\n'
    sw t1, 0(t0)

    li t2, 200000
2:  addi t2, t2, -1
    bnez t2, 2b
    j trap_handler
