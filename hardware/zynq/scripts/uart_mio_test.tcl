# UART MIO TX test — PS-only, no bitstream required
# Tests UART0 on MIO10(TX)/MIO11(RX) via FTDI FT2232H channel B

puts "=== ATOMiK Zynq UART MIO Debug ==="
puts "Step 1: Connect and reset"

connect
after 500

# Target the APU (Cortex-A9)
targets -set -filter {name =~ "APU*"}
puts "Target: [targets -filter {name =~ "APU*"}]"

# System reset
rst -system
after 1000

puts "Step 2: Run ps7_init"
source scripts/ps7_init_rk7020f.tcl
ps7_init
ps7_post_config
puts "ps7_init + ps7_post_config complete"

puts ""
puts "Step 3: Read MIO and clock registers"
set mio10 [mrd -force -value 0xF8000728]
set mio11 [mrd -force -value 0xF800072C]
set uart_clk [mrd -force -value 0xF8000154]
puts [format "MIO10 (UART0_TX): 0x%08X" $mio10]
puts [format "MIO11 (UART0_RX): 0x%08X" $mio11]
puts [format "UART_CLK_CTRL:    0x%08X" $uart_clk]

# Decode MIO10
set l3_sel [expr {($mio10 >> 5) & 0x7}]
set tri_en [expr {$mio10 & 0x1}]
puts [format "  MIO10: L3_SEL=%d TRI_ENABLE=%d (expect L3_SEL=4=UART0_TX, TRI=1=output)" $l3_sel $tri_en]

# Decode MIO11
set l3_sel11 [expr {($mio11 >> 5) & 0x7}]
set tri_en11 [expr {$mio11 & 0x1}]
puts [format "  MIO11: L3_SEL=%d TRI_ENABLE=%d (expect L3_SEL=4=UART0_RX, TRI=0=input)" $l3_sel11 $tri_en11]

# Decode UART clock
set clkact0 [expr {$uart_clk & 0x1}]
set divisor [expr {($uart_clk >> 8) & 0x3F}]
puts [format "  UART_CLK: CLKACT0=%d DIVISOR=%d (ref_clk = IO_PLL/%d)" $clkact0 $divisor $divisor]

puts ""
puts "Step 4: Read current UART0 config (from ps7_init)"
set cr [mrd -force -value 0xE0000000]
set mr [mrd -force -value 0xE0000004]
set cd_cur [mrd -force -value 0xE0000034]
set bdiv_cur [mrd -force -value 0xE0000018]
set sr_cur [mrd -force -value 0xE000002C]
puts [format "UART0_CR (ctrl):  0x%08X" $cr]
puts [format "UART0_MR (mode):  0x%08X" $mr]
puts [format "UART0_CD (cd):    0x%08X  (CD=%d)" $cd_cur $cd_cur]
puts [format "UART0_BDIV:       0x%08X  (BDIV=%d)" $bdiv_cur $bdiv_cur]
puts [format "UART0_SR (status):0x%08X" $sr_cur]

# Calculate current baud: 100MHz / (CD * (BDIV+1))
set baud_cur [expr {100000000 / ($cd_cur * ($bdiv_cur + 1))}]
puts [format "  Current baud = 100MHz / (%d * %d) = %d" $cd_cur [expr {$bdiv_cur + 1}] $baud_cur]

puts ""
puts "Step 5: Reconfigure UART0 for 115200 baud"

# Reset TX and RX paths
mwr -force 0xE0000000 0x00000180
after 10

# Set baud: 115200 = 100MHz / (62 * 14) = 115207 (0.006% error)
mwr -force 0xE0000034 62
mwr -force 0xE0000018 13
after 10

# Mode: 8N1, normal mode, ref_clk
mwr -force 0xE0000004 0x00000020
after 10

# Enable TX and RX (TXEN=bit4, RXEN=bit6, STPBRK=bit0)
mwr -force 0xE0000000 0x00000051
after 10

# Verify config
set cd_new [mrd -force -value 0xE0000034]
set bdiv_new [mrd -force -value 0xE0000018]
set cr_new [mrd -force -value 0xE0000000]
set sr_new [mrd -force -value 0xE000002C]
puts [format "New CD=%d BDIV=%d CR=0x%08X SR=0x%08X" $cd_new $bdiv_new $cr_new $sr_new]
set baud_new [expr {100000000 / ($cd_new * ($bdiv_new + 1))}]
puts [format "  New baud = 100MHz / (%d * %d) = %d" $cd_new [expr {$bdiv_new + 1}] $baud_new]

puts ""
puts "Step 6: Stuff TX FIFO"

# Check TX empty before starting
set sr [mrd -force -value 0xE000002C]
set tempty [expr {($sr >> 3) & 1}]
puts [format "  SR before TX: 0x%08X (TEMPTY=%d)" $sr $tempty]

# Send "ATOMIK\r\n"
set msg {65 84 79 77 73 75 13 10}
foreach byte $msg {
    mwr -force 0xE0000030 $byte
}
puts "  Sent: ATOMIK\\r\\n (8 bytes)"

# Send 60x 0x55 ('U' = alternating bits pattern, good for scope/baud detection)
for {set i 0} {$i < 60} {incr i} {
    mwr -force 0xE0000030 0x55
}
puts "  Sent: 60x 0x55 ('U' pattern)"

# Send another identifiable string
set msg2 {72 69 65 68 89 10}
foreach byte $msg2 {
    mwr -force 0xE0000030 $byte
}
puts "  Sent: READY\\n"

# Wait for TX to drain
after 100

# Check status after TX
set sr_after [mrd -force -value 0xE000002C]
set tempty_after [expr {($sr_after >> 3) & 1}]
set tful [expr {($sr_after >> 4) & 1}]
puts [format "  SR after TX: 0x%08X (TEMPTY=%d TFUL=%d)" $sr_after $tempty_after $tful]

if {$tempty_after == 1} {
    puts "  TX FIFO drained — data was transmitted (or UART not clocked)"
} else {
    puts "  WARNING: TX FIFO not empty — UART may be stalled"
}

puts ""
puts "Step 7: Check RX (any loopback or echo?)"
set sr_rx [mrd -force -value 0xE000002C]
set rxempty [expr {($sr_rx >> 1) & 1}]
set rxfull [expr {($sr_rx >> 2) & 1}]
puts [format "  SR: 0x%08X (RXEMPTY=%d RXFULL=%d)" $sr_rx $rxempty $rxfull]

if {$rxempty == 0} {
    puts "  RX has data! Reading up to 16 bytes:"
    for {set i 0} {$i < 16} {incr i} {
        set sr_check [mrd -force -value 0xE000002C]
        if {[expr {($sr_check >> 1) & 1}] == 1} {
            puts "  (RX empty after $i bytes)"
            break
        }
        set rxd [mrd -force -value 0xE0000030]
        puts [format "    RX\[%d\] = 0x%02X ('%c')" $i $rxd [expr {($rxd >= 0x20 && $rxd <= 0x7E) ? $rxd : 0x2E}]]
    }
} else {
    puts "  RX empty — no loopback detected"
}

puts ""
puts "=== UART MIO Test Complete ==="
puts "Check /dev/ttyUSB1 capture for received data"
