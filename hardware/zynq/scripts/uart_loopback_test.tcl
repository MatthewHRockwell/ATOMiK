# Definitive UART test: local loopback mode
# TX data appears in RX FIFO — no external connection needed
# If RX gets data → UART works, issue is routing
# If RX empty → UART itself is broken

connect
after 200
targets -set -filter {name =~ "APU*"}

rst -system
after 1000

source scripts/ps7_init_rk7020f.tcl
ps7_init
ps7_post_config

puts "=== UART0 Loopback Test ==="

# Unlock SLCR
mwr -force 0xF8000008 0x0000DF0D

# Reset UART0
mwr -force 0xE0000000 0x00000180
after 50

# Baud: 115200
mwr -force 0xE0000034 62
mwr -force 0xE0000018 13

# Mode: 8N1, LOCAL LOOPBACK (CHMODE=10, bits[9:8]=10)
# 0x200 = loopback + 0x20 = 8N1 no parity = 0x220
mwr -force 0xE0000004 0x00000220

# Enable TX+RX
mwr -force 0xE0000000 0x00000017
after 50

# Verify mode
set mr [mrd -force -value 0xE0000004]
set cr [mrd -force -value 0xE0000000]
set sr [mrd -force -value 0xE000002C]
puts [format "MR=0x%08X (CHMODE=%d) CR=0x%08X SR=0x%08X" $mr [expr {($mr >> 8) & 3}] $cr $sr]

# Clear any stale RX data
for {set i 0} {$i < 64} {incr i} {
    set check_sr [mrd -force -value 0xE000002C]
    if {[expr {($check_sr >> 1) & 1}] == 1} break
    mrd -force -value 0xE0000030
}

puts ""
puts "--- Sending 8 test bytes ---"

# Send test pattern
set test_bytes {0xDE 0xAD 0xBE 0xEF 0x41 0x54 0x4F 0x4B}
foreach b $test_bytes {
    mwr -force 0xE0000030 $b
}

# Wait for TX to complete (at 115200, 8 bytes ≈ 0.7ms)
after 50

# Check status
set sr_after [mrd -force -value 0xE000002C]
set rxempty [expr {($sr_after >> 1) & 1}]
set tempty [expr {($sr_after >> 3) & 1}]
puts [format "SR after send+wait: 0x%08X (RXEMPTY=%d TEMPTY=%d)" $sr_after $rxempty $tempty]

# Read RX FIFO
if {$rxempty == 0} {
    puts ""
    puts "*** UART0 LOOPBACK SUCCESS — RX has data! ***"
    puts "Reading RX bytes:"
    set match 1
    set idx 0
    foreach expected $test_bytes {
        set sr_check [mrd -force -value 0xE000002C]
        if {[expr {($sr_check >> 1) & 1}] == 1} {
            puts "  RX empty after $idx bytes (expected 8)"
            set match 0
            break
        }
        set rxd [mrd -force -value 0xE0000030]
        set exp_val [expr $expected]
        set ok [expr {$rxd == $exp_val ? "OK" : "MISMATCH"}]
        if {$rxd != $exp_val} { set match 0 }
        puts [format "  RX\[%d\] = 0x%02X (expected 0x%02X) %s" $idx $rxd $exp_val $ok]
        incr idx
    }
    if {$match} {
        puts ""
        puts "ALL BYTES MATCH — UART0 is fully functional!"
        puts "Issue is external routing: FTDI not connected to MIO10/11"
    }
} else {
    puts ""
    puts "*** UART0 LOOPBACK FAIL — RX empty ***"
    puts "UART0 TX engine is NOT running."
    puts ""
    puts "Deep clock debug..."

    # Check if IO PLL is really locked and running
    set pll_stat [mrd -force -value 0xF800010C]
    puts [format "PLL_STATUS: 0x%08X (ARM=%d DDR=%d IO=%d)" $pll_stat \
        [expr {$pll_stat & 1}] [expr {($pll_stat >> 1) & 1}] [expr {($pll_stat >> 2) & 1}]]

    # Check UART0 interrupt status for clues
    set isr [mrd -force -value 0xE0000014]
    puts [format "UART0_ISR: 0x%08X" $isr]

    # Try brute force: write CR with explicit enable sequence
    puts ""
    puts "Trying explicit enable sequence..."
    # Disable both
    mwr -force 0xE0000000 0x00000028
    after 10
    # Reset both
    mwr -force 0xE0000000 0x00000180
    after 50
    # Enable both
    mwr -force 0xE0000000 0x00000050
    after 10

    set cr2 [mrd -force -value 0xE0000000]
    set sr2 [mrd -force -value 0xE000002C]
    puts [format "After re-enable: CR=0x%08X SR=0x%08X" $cr2 $sr2]

    # Try sending again
    mwr -force 0xE0000030 0xAA
    after 50
    set sr3 [mrd -force -value 0xE000002C]
    set rxe [expr {($sr3 >> 1) & 1}]
    puts [format "After 1 byte send: SR=0x%08X (RXEMPTY=%d)" $sr3 $rxe]

    if {$rxe == 0} {
        set rxd [mrd -force -value 0xE0000030]
        puts [format "Got RX byte: 0x%02X (expected 0xAA)" $rxd]
    } else {
        puts "Still no RX data."
        puts ""
        puts "Last resort: Check if CR writes actually take effect"
        # Write a known pattern and read back
        mwr -force 0xE0000000 0x00000050
        set cr3 [mrd -force -value 0xE0000000]
        puts [format "Wrote 0x50, read back: 0x%08X" $cr3]
        mwr -force 0xE0000000 0x00000000
        set cr4 [mrd -force -value 0xE0000000]
        puts [format "Wrote 0x00, read back: 0x%08X" $cr4]
        mwr -force 0xE0000000 0x000001FF
        set cr5 [mrd -force -value 0xE0000000]
        puts [format "Wrote 0x1FF, read back: 0x%08X" $cr5]

        puts ""
        puts "Check if MR writes take effect"
        mwr -force 0xE0000004 0x00000000
        set mr1 [mrd -force -value 0xE0000004]
        puts [format "Wrote MR=0x00, read: 0x%08X" $mr1]
        mwr -force 0xE0000004 0x000003FF
        set mr2 [mrd -force -value 0xE0000004]
        puts [format "Wrote MR=0x3FF, read: 0x%08X" $mr2]
    }
}

puts ""
puts "=== Also test UART1 loopback ==="

# Enable UART1 clocks
mwr -force 0xF8000008 0x0000DF0D
set uart_clk [mrd -force -value 0xF8000154]
mwr -force 0xF8000154 [expr {$uart_clk | 0x02}]
set aper [mrd -force -value 0xF800012C]
mwr -force 0xF800012C [expr {$aper | (1 << 21)}]
mwr -force 0xF8000228 0x00000000

# Reset UART1
mwr -force 0xE0001000 0x00000180
after 50
mwr -force 0xE0001034 62
mwr -force 0xE0001018 13
mwr -force 0xE0001004 0x00000220
mwr -force 0xE0001000 0x00000017
after 50

# Send test byte
mwr -force 0xE0001030 0xBB
after 50

set sr_u1 [mrd -force -value 0xE000102C]
set rxe_u1 [expr {($sr_u1 >> 1) & 1}]
puts [format "UART1 SR: 0x%08X (RXEMPTY=%d)" $sr_u1 $rxe_u1]

if {$rxe_u1 == 0} {
    set rxd_u1 [mrd -force -value 0xE0001030]
    puts [format "UART1 loopback: sent 0xBB, got 0x%02X — %s" $rxd_u1 [expr {$rxd_u1 == 0xBB ? "PASS" : "FAIL"}]]
} else {
    puts "UART1 loopback also failed — RX empty"
}
