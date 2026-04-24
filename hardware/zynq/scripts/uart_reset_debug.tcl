# Debug UART reset state — check if UART0 is held in reset after ps7_init

connect
after 200
targets -set -filter {name =~ "APU*"}

# Full system reset
rst -system
after 1000

# Source and run ps7_init
source scripts/ps7_init_rk7020f.tcl
ps7_init
ps7_post_config

puts "=== UART Reset and Clock Debug ==="

# UART_RST_CTRL (0xF8000228) — THIS is likely the culprit
set uart_rst [mrd -force -value 0xF8000228]
puts [format "UART_RST_CTRL (0xF8000228): 0x%08X" $uart_rst]
puts [format "  UART0_CPU1X_RST (bit0): %d" [expr {$uart_rst & 1}]]
puts [format "  UART0_REF_RST   (bit1): %d" [expr {($uart_rst >> 1) & 1}]]
puts [format "  UART1_CPU1X_RST (bit2): %d" [expr {($uart_rst >> 2) & 1}]]
puts [format "  UART1_REF_RST   (bit3): %d" [expr {($uart_rst >> 3) & 1}]]

# APER_CLK_CTRL — verify APB clock to UART
set aper_clk [mrd -force -value 0xF800012C]
puts [format "APER_CLK_CTRL (0xF800012C): 0x%08X" $aper_clk]
puts [format "  UART0_CPU_1XCLKACT (bit20): %d" [expr {($aper_clk >> 20) & 1}]]
puts [format "  UART1_CPU_1XCLKACT (bit21): %d" [expr {($aper_clk >> 21) & 1}]]

# UART_CLK_CTRL
set uart_clk [mrd -force -value 0xF8000154]
puts [format "UART_CLK_CTRL (0xF8000154): 0x%08X" $uart_clk]
puts [format "  CLKACT0 (bit0): %d" [expr {$uart_clk & 1}]]
puts [format "  CLKACT1 (bit1): %d" [expr {($uart_clk >> 1) & 1}]]

# Also check PSS_RST_CTRL for broader reset state
set pss_rst [mrd -force -value 0xF8000200]
puts [format "PSS_RST_CTRL (0xF8000200): 0x%08X" $pss_rst]

# A9_CPU_RST_CTRL
set a9_rst [mrd -force -value 0xF8000244]
puts [format "A9_CPU_RST_CTRL (0xF8000244): 0x%08X" $a9_rst]

puts ""
puts "=== Fix: Deassert UART0 reset ==="

# Unlock SLCR
mwr -force 0xF8000008 0x0000DF0D

# Clear UART0 reset bits (write 0 to bits 0 and 1)
mwr -force 0xF8000228 0x00000000
after 10

# Verify
set uart_rst2 [mrd -force -value 0xF8000228]
puts [format "UART_RST_CTRL after fix: 0x%08X" $uart_rst2]

puts ""
puts "=== Configure UART0 from scratch ==="

# Reset UART0 internal state
mwr -force 0xE0000000 0x00000180
after 10

# Baud: 115200 = 100MHz / (62 * 14)
mwr -force 0xE0000034 62
mwr -force 0xE0000018 13

# Mode: 8N1, normal, ref_clk
mwr -force 0xE0000004 0x00000020

# Enable TX+RX, stop break
mwr -force 0xE0000000 0x00000017
after 10

# Verify config
set cr [mrd -force -value 0xE0000000]
set mr [mrd -force -value 0xE0000004]
set cd [mrd -force -value 0xE0000034]
set bdiv [mrd -force -value 0xE0000018]
set sr [mrd -force -value 0xE000002C]
puts [format "CR=0x%08X MR=0x%08X CD=%d BDIV=%d SR=0x%08X" $cr $mr $cd $bdiv $sr]

puts ""
puts "=== TX Test (after reset fix) ==="

# Check SR
set sr0 [mrd -force -value 0xE000002C]
puts [format "SR before TX: 0x%08X (TEMPTY=%d)" $sr0 [expr {($sr0 >> 3) & 1}]]

# Write ONE byte
mwr -force 0xE0000030 0x41

# Immediately read SR
set sr1 [mrd -force -value 0xE000002C]
puts [format "SR after 1 byte: 0x%08X (TEMPTY=%d)" $sr1 [expr {($sr1 >> 3) & 1}]]

# Write 63 more
for {set i 0} {$i < 63} {incr i} {
    mwr -force 0xE0000030 0x55
}

# Check SR again
set sr2 [mrd -force -value 0xE000002C]
puts [format "SR after 64 bytes: 0x%08X (TEMPTY=%d TFUL=%d)" $sr2 [expr {($sr2 >> 3) & 1}] [expr {($sr2 >> 4) & 1}]]

# Wait for drain
after 200

set sr3 [mrd -force -value 0xE000002C]
puts [format "SR after 200ms: 0x%08X (TEMPTY=%d)" $sr3 [expr {($sr3 >> 3) & 1}]]

if {[expr {($sr1 >> 3) & 1}] == 0 || [expr {($sr2 >> 3) & 1}] == 0} {
    puts ""
    puts "SUCCESS: TEMPTY went low — UART0 TX is transmitting!"
    puts "Now sending identifiable pattern..."

    # Send "ATOMIK\r\n"
    foreach byte {65 84 79 77 73 75 13 10} {
        mwr -force 0xE0000030 $byte
    }

    # Send 60x 'U'
    for {set i 0} {$i < 60} {incr i} {
        mwr -force 0xE0000030 0x55
    }

    after 200
    puts "Sent ATOMIK + U-pattern. Check serial capture."
} else {
    puts ""
    puts "FAIL: TEMPTY still stuck at 1. UART0 TX is not running."
    puts ""
    puts "=== Trying UART1 on MIO48/49 ==="

    # Enable UART1 clock
    mwr -force 0xF8000008 0x0000DF0D
    # UART_CLK_CTRL: enable CLKACT1 (bit1)
    set uart_clk_cur [mrd -force -value 0xF8000154]
    set uart_clk_new [expr {$uart_clk_cur | 0x02}]
    mwr -force 0xF8000154 $uart_clk_new
    puts [format "UART_CLK_CTRL updated: 0x%08X" [mrd -force -value 0xF8000154]]

    # APER_CLK_CTRL: enable UART1 APB clock (bit21)
    set aper_cur [mrd -force -value 0xF800012C]
    set aper_new [expr {$aper_cur | (1 << 21)}]
    mwr -force 0xF800012C $aper_new

    # Deassert UART1 reset
    mwr -force 0xF8000228 0x00000000

    # MIO48 → UART1_TX (L3_SEL=4, TRI_ENABLE=1)
    # MIO48 config: 0xF80007C0
    # Need L3_SEL=4 (bits[7:5]=100), TRI_ENABLE=1 (bit0), with pull-up and LVCMOS33
    mwr -force 0xF80007C0 0x000016E1
    # MIO49 → UART1_RX (L3_SEL=4, TRI_ENABLE=0)
    mwr -force 0xF80007C4 0x000016E0

    after 10

    # Configure UART1 (base 0xE0001000)
    mwr -force 0xE0001000 0x00000180
    after 10
    mwr -force 0xE0001034 62
    mwr -force 0xE0001018 13
    mwr -force 0xE0001004 0x00000020
    mwr -force 0xE0001000 0x00000017
    after 10

    # TX test on UART1
    set sr_u1_0 [mrd -force -value 0xE000102C]
    puts [format "UART1 SR before: 0x%08X (TEMPTY=%d)" $sr_u1_0 [expr {($sr_u1_0 >> 3) & 1}]]

    mwr -force 0xE0001030 0x41
    set sr_u1_1 [mrd -force -value 0xE000102C]
    puts [format "UART1 SR after 1B: 0x%08X (TEMPTY=%d)" $sr_u1_1 [expr {($sr_u1_1 >> 3) & 1}]]

    for {set i 0} {$i < 63} {incr i} {
        mwr -force 0xE0001030 0x55
    }
    set sr_u1_2 [mrd -force -value 0xE000102C]
    puts [format "UART1 SR after 64B: 0x%08X (TEMPTY=%d TFUL=%d)" $sr_u1_2 [expr {($sr_u1_2 >> 3) & 1}] [expr {($sr_u1_2 >> 4) & 1}]]

    if {[expr {($sr_u1_1 >> 3) & 1}] == 0 || [expr {($sr_u1_2 >> 3) & 1}] == 0} {
        puts "UART1 TX is working! Sending pattern..."
        foreach byte {65 84 79 77 73 75 13 10} {
            mwr -force 0xE0001030 $byte
        }
        for {set i 0} {$i < 60} {incr i} {
            mwr -force 0xE0001030 0x55
        }
        after 200
        puts "Check serial capture."
    } else {
        puts "UART1 TX also stuck. Neither UART is transmitting."
        puts ""
        puts "Dumping all MIO pins for analysis..."
        for {set pin 0} {$pin < 54} {incr pin} {
            set addr [expr {0xF8000700 + $pin * 4}]
            set val [mrd -force -value $addr]
            set l0 [expr {($val >> 1) & 1}]
            set l1 [expr {($val >> 2) & 1}]
            set l2 [expr {($val >> 3) & 3}]
            set l3 [expr {($val >> 5) & 7}]
            set tri [expr {$val & 1}]
            puts [format "MIO%02d: 0x%04X  L0=%d L1=%d L2=%d L3=%d TRI=%d" $pin $val $l0 $l1 $l2 $l3 $tri]
        }
    }
}
