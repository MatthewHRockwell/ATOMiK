# Quick TX verification: write data and IMMEDIATELY check SR
# If TEMPTY briefly goes 0, UART is actually transmitting

connect
after 200
targets -set -filter {name =~ "APU*"}

puts "=== TX FIFO Verify ==="

# Read SR before
set sr0 [mrd -force -value 0xE000002C]
puts [format "SR before write: 0x%08X (TEMPTY=%d)" $sr0 [expr {($sr0 >> 3) & 1}]]

# Stuff 64 bytes into TX FIFO (fills it near capacity)
for {set i 0} {$i < 64} {incr i} {
    mwr -force 0xE0000030 0x55
}

# IMMEDIATELY read SR — no delay
set sr1 [mrd -force -value 0xE000002C]
puts [format "SR immediately after 64-byte write: 0x%08X (TEMPTY=%d TFUL=%d)" $sr1 [expr {($sr1 >> 3) & 1}] [expr {($sr1 >> 4) & 1}]]

# Read again
set sr2 [mrd -force -value 0xE000002C]
puts [format "SR second read: 0x%08X (TEMPTY=%d)" $sr2 [expr {($sr2 >> 3) & 1}]]

# Wait 10ms
after 10
set sr3 [mrd -force -value 0xE000002C]
puts [format "SR after 10ms: 0x%08X (TEMPTY=%d)" $sr3 [expr {($sr3 >> 3) & 1}]]

# Wait 100ms
after 100
set sr4 [mrd -force -value 0xE000002C]
puts [format "SR after 100ms: 0x%08X (TEMPTY=%d)" $sr4 [expr {($sr4 >> 3) & 1}]]

puts ""
if {[expr {($sr1 >> 3) & 1}] == 0} {
    puts "CONFIRMED: TX FIFO accepted data and TEMPTY went low"
    puts "UART IS transmitting — FTDI likely not connected to MIO10/11"
} elseif {[expr {($sr1 >> 3) & 1}] == 1 && [expr {($sr1 >> 4) & 1}] == 0} {
    puts "TEMPTY still 1 immediately after write — suspicious"
    puts "Either UART is draining VERY fast or clock/FIFO issue"
    puts ""
    puts "Checking UART clock: reading CD and BDIV"
    set cd [mrd -force -value 0xE0000034]
    set bdiv [mrd -force -value 0xE0000018]
    set cr [mrd -force -value 0xE0000000]
    puts [format "CD=%d BDIV=%d CR=0x%08X" $cd $bdiv $cr]

    puts ""
    puts "Trying with UART reset and 133333 baud (ps7_init default)..."
    # Reset
    mwr -force 0xE0000000 0x00000180
    after 10
    # Restore ps7_init defaults: CD=6, BDIV=124
    mwr -force 0xE0000034 6
    mwr -force 0xE0000018 124
    mwr -force 0xE0000004 0x00000020
    mwr -force 0xE0000000 0x00000017
    after 10

    # Stuff and check
    set sr5 [mrd -force -value 0xE000002C]
    puts [format "SR before (133333): 0x%08X" $sr5]
    for {set i 0} {$i < 64} {incr i} {
        mwr -force 0xE0000030 0x55
    }
    set sr6 [mrd -force -value 0xE000002C]
    puts [format "SR after 64B (133333): 0x%08X (TEMPTY=%d)" $sr6 [expr {($sr6 >> 3) & 1}]]
}

# Also check: is UART0 actually getting its clock?
puts ""
puts "=== Clock Debug ==="
set uart_clk_ctrl [mrd -force -value 0xF8000154]
set io_pll_ctrl [mrd -force -value 0xF8000108]
set io_pll_stat [mrd -force -value 0xF800010C]
set arm_clk_ctrl [mrd -force -value 0xF8000120]
puts [format "UART_CLK_CTRL: 0x%08X" $uart_clk_ctrl]
puts [format "IO_PLL_CTRL:   0x%08X" $io_pll_ctrl]
puts [format "IO_PLL_STATUS: 0x%08X (bit0=ARM_LOCK, bit1=DDR_LOCK, bit2=IO_LOCK)" $io_pll_stat]
puts [format "ARM_CLK_CTRL:  0x%08X" $arm_clk_ctrl]

set io_lock [expr {($io_pll_stat >> 2) & 1}]
puts [format "IO PLL locked: %d" $io_lock]
