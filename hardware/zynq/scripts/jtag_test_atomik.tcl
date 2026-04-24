# =============================================================================
# ATOMiK JTAG Test Script
#
# Programs the Zynq and runs ATOMiK register-level verification via xsdb.
# No UART or Linux required — pure JTAG access to AXI registers.
#
# Usage: xsdb scripts/jtag_test_atomik.tcl [bitfile]
#        Default bitfile: output/atomik_zynq.bit
#
# Test sequence:
#   1. Program PL via proper Zynq sequence (ps7_init → bitstream → post_config)
#   2. Read STATUS register (version, bank count, acc_zero)
#   3. Read/write CONFIG register
#   4. LOAD initial state → READ → verify
#   5. ACCUM delta → READ → verify XOR result
#   6. ACCUM same delta → READ → verify self-inverse (cancellation)
#   7. SWAP → READ → verify reference update
#   8. Test mailbox connectivity (Phase B)
#
# Register Map (ATOMiK accelerator @ 0x43C00000):
#   0x00  LOAD_ADDR      (W)   Address [7:0]
#   0x04  LOAD_DATA_LO   (W)   Initial state bits [31:0]
#   0x08  LOAD_DATA_HI   (W)   Initial state bits [63:32] -> triggers LOAD
#   0x0C  ACCUM_LO       (W)   Delta bits [31:0]
#   0x10  ACCUM_HI       (W)   Delta bits [63:32] -> triggers ACCUM
#   0x14  STATE_LO       (R)   Current state bits [31:0]
#   0x18  STATE_HI       (R)   Current state bits [63:32]
#   0x1C  STATUS         (R)   {version[15:8], n_banks[7:0], 7'h0, acc_zero}
#   0x20  SWAP_ADDR      (W)   Address [7:0] -> triggers SWAP
#   0x24  CONFIG         (R/W) Bit 0 = core_enable
#
# Date: April 2026
# =============================================================================

set SCRIPT_DIR [file dirname [info script]]
set ZYNQ_DIR   [file normalize "$SCRIPT_DIR/.."]

# Default bitfile
if {[llength $argv] > 0} {
    set bitfile [lindex $argv 0]
} else {
    set bitfile "$ZYNQ_DIR/output/atomik_zynq.bit"
}

# ATOMiK base address (M_AXI_GP0 address map)
set BASE 0x43C00000

# Mailbox base address (Phase B co-processor)
set MBOX 0x43C10000

# Test counters
set pass 0
set fail 0

# =============================================================================
# Helper procedures
# =============================================================================

proc check {desc expected actual} {
    upvar pass pass
    upvar fail fail
    if {$expected == $actual} {
        puts "  PASS: $desc (0x[format %08X $actual])"
        incr pass
    } else {
        puts "  FAIL: $desc (expected 0x[format %08X $expected], got 0x[format %08X $actual])"
        incr fail
    }
}

proc atomik_read_lo {base} {
    # Reading STATE_LO latches the full 64-bit snapshot
    return [mrd -force -value [expr {$base + 0x14}]]
}

proc atomik_read_hi {base} {
    return [mrd -force -value [expr {$base + 0x18}]]
}

proc atomik_status {base} {
    return [mrd -force -value [expr {$base + 0x1C}]]
}

proc atomik_config_read {base} {
    return [mrd -force -value [expr {$base + 0x24}]]
}

proc atomik_config_write {base val} {
    mwr -force [expr {$base + 0x24}] $val
}

proc atomik_load {base addr data_lo data_hi} {
    mwr -force [expr {$base + 0x00}] $addr
    mwr -force [expr {$base + 0x04}] $data_lo
    mwr -force [expr {$base + 0x08}] $data_hi
    # HI write triggers LOAD via CDC bridge
    # Wait for CDC round-trip (~10 AXI cycles = ~100 ns @ 100 MHz)
    after 1
}

proc atomik_accum {base delta_lo delta_hi} {
    mwr -force [expr {$base + 0x0C}] $delta_lo
    mwr -force [expr {$base + 0x10}] $delta_hi
    # HI write triggers ACCUM via CDC bridge
    after 1
}

proc atomik_swap {base addr} {
    mwr -force [expr {$base + 0x20}] $addr
    # Triggers SWAP via CDC bridge (4-stage pipeline on core side)
    after 1
}

# =============================================================================
# Step 0: Connect
# =============================================================================

puts "=============================================="
puts " ATOMiK JTAG Test"
puts " Bitfile: $bitfile"
puts "=============================================="

if {![file exists $bitfile]} {
    puts "\nERROR: Bitfile not found: $bitfile"
    puts "Build first: make blockdesign"
    exit 1
}

connect

# =============================================================================
# Step 1: Program using proper Zynq sequence
# =============================================================================

puts "\n--- Step 1: Programming Zynq ---"

# System reset
targets -set -filter {name =~ "APU*"}
rst -system
after 1000

# PS7 init (board-specific: HamGeek RK-ZYNQ7020-F)
puts "  Running ps7_init_rk7020f..."
source $SCRIPT_DIR/ps7_init_rk7020f.tcl
ps7_init
after 500

# Program PL
puts "  Programming PL bitstream..."
targets -set -filter {name =~ "xc7z*"}
fpga -file $bitfile
after 500

# Post-config (release PL I/O)
puts "  Running ps7_post_config..."
targets -set -filter {name =~ "APU*"}
ps7_post_config
after 500

# Verify PS status
set lvl [mrd -force -value 0xF8000900]
set rst_ctrl [mrd -force -value 0xF8000240]
set pll [mrd -force -value 0xF800010C]
puts "  LVL_SHFTR_EN:  0x[format %08X $lvl]"
puts "  FPGA_RST_CTRL: 0x[format %08X $rst_ctrl]"
puts "  PLL_STATUS:     0x[format %08X $pll]"

if {$lvl != 0x0F || $rst_ctrl != 0} {
    puts "\n  ERROR: PS not configured correctly. Aborting tests."
    disconnect
    exit 1
}
puts "  PS7 OK."

# =============================================================================
# Step 2: Read ATOMiK STATUS register
# =============================================================================

puts "\n--- Step 2: ATOMiK STATUS ---"

set status [atomik_status $BASE]
set version  [expr {($status >> 16) & 0xFF}]
set n_banks  [expr {($status >> 8) & 0xFF}]
set acc_zero [expr {$status & 1}]

puts "  STATUS:   0x[format %08X $status]"
puts "  Version:  $version (expect 2)"
puts "  N_Banks:  $n_banks"
puts "  Acc_Zero: $acc_zero (expect 1 after reset)"

check "STATUS version" 2 $version
check "Acc zero after reset" 1 $acc_zero

# =============================================================================
# Step 3: CONFIG register
# =============================================================================

puts "\n--- Step 3: CONFIG ---"

set cfg [atomik_config_read $BASE]
check "CONFIG default (enabled)" 1 $cfg

# Disable and re-enable
atomik_config_write $BASE 0
after 1
set cfg [atomik_config_read $BASE]
check "CONFIG after disable" 0 $cfg

atomik_config_write $BASE 1
after 1
set cfg [atomik_config_read $BASE]
check "CONFIG after re-enable" 1 $cfg

# =============================================================================
# Step 4: LOAD initial state -> READ
# =============================================================================

puts "\n--- Step 4: LOAD + READ ---"

# Load address 0x42 with initial state 0xDEADBEEF_CAFEBABE
atomik_load $BASE 0x42 0xCAFEBABE 0xDEADBEEF

# Read back — with acc=0, current_state = initial_state XOR 0 = initial_state
set lo [atomik_read_lo $BASE]
set hi [atomik_read_hi $BASE]
check "LOAD/READ LO" 0xCAFEBABE $lo
check "LOAD/READ HI" 0xDEADBEEF $hi

# Verify acc_zero is still set
set status [atomik_status $BASE]
set acc_zero [expr {$status & 1}]
check "Acc zero after LOAD" 1 $acc_zero

# =============================================================================
# Step 5: ACCUM delta -> READ (verify XOR)
# =============================================================================

puts "\n--- Step 5: ACCUM + READ ---"

# Accumulate delta 0x00000000_FF00FF00
atomik_accum $BASE 0xFF00FF00 0x00000000

# current_state = 0xDEADBEEF_CAFEBABE ^ 0x00000000_FF00FF00
# Expected:       0xDEADBEEF_35FE45BE
set lo [atomik_read_lo $BASE]
set hi [atomik_read_hi $BASE]
check "ACCUM/READ LO" 0x35FE45BE $lo
check "ACCUM/READ HI" 0xDEADBEEF $hi

# acc_zero should be 0 (accumulator has delta in it)
set status [atomik_status $BASE]
set acc_zero [expr {$status & 1}]
check "Acc non-zero after ACCUM" 0 $acc_zero

# =============================================================================
# Step 6: Self-inverse (XOR cancellation)
# =============================================================================

puts "\n--- Step 6: Self-Inverse ---"

# Accumulate the same delta again — should cancel (XOR self-inverse)
atomik_accum $BASE 0xFF00FF00 0x00000000

# current_state should be back to initial: 0xDEADBEEF_CAFEBABE
set lo [atomik_read_lo $BASE]
set hi [atomik_read_hi $BASE]
check "Self-inverse LO" 0xCAFEBABE $lo
check "Self-inverse HI" 0xDEADBEEF $hi

# acc_zero should be 1 again
set status [atomik_status $BASE]
set acc_zero [expr {$status & 1}]
check "Acc zero after self-inverse" 1 $acc_zero

# =============================================================================
# Step 7: SWAP (update reference, clear accumulator)
# =============================================================================

puts "\n--- Step 7: SWAP ---"

# First accumulate a delta
atomik_accum $BASE 0x11111111 0x22222222

# Verify pre-swap state: 0xDEADBEEF^22222222 _ CAFEBABE^11111111
# = 0xFC8F9CCD_DBEFABAF
set lo [atomik_read_lo $BASE]
set hi [atomik_read_hi $BASE]
check "Pre-swap LO" 0xDBEFABAF $lo
check "Pre-swap HI" 0xFC8F9CCD $hi

# SWAP: sets new reference = current_state, clears accumulator
atomik_swap $BASE 0x42

# After SWAP, accumulator is zero, so READ returns the new reference
# (which equals what current_state was before swap)
set lo [atomik_read_lo $BASE]
set hi [atomik_read_hi $BASE]
check "Post-swap LO" 0xDBEFABAF $lo
check "Post-swap HI" 0xFC8F9CCD $hi

set status [atomik_status $BASE]
set acc_zero [expr {$status & 1}]
check "Acc zero after SWAP" 1 $acc_zero

# =============================================================================
# Step 8: Mailbox connectivity (Phase B co-processor)
# =============================================================================

puts "\n--- Step 8: Mailbox Probe ---"

# Read mailbox STATUS register (0x43C10004)
# If the co-processor is in the design, this should return a valid value
# (not 0xDEADBEEF or bus error)
set mbox_status [mrd -force -value [expr {$MBOX + 0x04}]]
puts "  Mailbox STATUS: 0x[format %08X $mbox_status]"

# Read mailbox CTRL (should be 0 after reset — RV64I held in reset)
set mbox_ctrl [mrd -force -value [expr {$MBOX + 0x00}]]
puts "  Mailbox CTRL:   0x[format %08X $mbox_ctrl]"

# Write and readback a command argument to verify bus connectivity
mwr -force [expr {$MBOX + 0x0C}] 0xA5A5A5A5
after 1
set arg0 [mrd -force -value [expr {$MBOX + 0x0C}]]
check "Mailbox ARG0 readback" 0xA5A5A5A5 $arg0

# =============================================================================
# Summary
# =============================================================================

puts "\n=============================================="
puts " ATOMiK JTAG Test Results"
puts "  PASS: $pass"
puts "  FAIL: $fail"
if {$fail == 0} {
    puts "  ALL TESTS PASSED"
} else {
    puts "  SOME TESTS FAILED"
}
puts "=============================================="

disconnect
exit [expr {$fail > 0}]
