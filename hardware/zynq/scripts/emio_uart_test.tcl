# Test EMIO UART: PS UART0+1 routed through PL to header pins
# emio_uart.bit: fans uart0_tx & uart1_tx to 32 header pins

set SCRIPT_DIR [file dirname [info script]]

connect
after 500

puts "=== EMIO UART Test ==="

# Step 1: System reset
targets -set -filter {name =~ "APU*"}
rst -system
after 1000

# Step 2: PS7 init
source $SCRIPT_DIR/ps7_init_rk7020f.tcl
ps7_init

# Step 3: Program PL with emio_uart bitstream
puts "Programming emio_uart.bit..."
targets -set -filter {name =~ "xc7z*"}
fpga -file test/emio_uart.bit
after 500

# Step 4: Post-config
targets -set -filter {name =~ "APU*"}
ps7_post_config
after 500

# Verify PL is alive
set lvl [mrd -force -value 0xF8000900]
set rst [mrd -force -value 0xF8000240]
puts [format "LVL_SHFTR=0x%08X FPGA_RST=0x%08X" $lvl $rst]

puts ""
puts "=== Configure UART0 for EMIO TX ==="

# The emio_uart block design set UART0_IO=EMIO.
# But ps7_init configured MIO10/11 for UART0.
# We need to check if PS UART0 TX goes to both MIO and EMIO,
# or only to the one configured in SLCR.

# Let's configure UART0 for 115200 and test
# Reset UART0
mwr -force 0xE0000000 0x00000180
after 10

# Baud: 115200 = 100MHz / (62 * 14)
mwr -force 0xE0000034 62
mwr -force 0xE0000018 13

# Mode: 8N1, LOCAL LOOPBACK to verify UART is functional first
mwr -force 0xE0000004 0x00000220
mwr -force 0xE0000000 0x00000017
after 10

# Quick loopback verify
mwr -force 0xE0000030 0xAA
after 20
set sr [mrd -force -value 0xE000002C]
set rxe [expr {($sr >> 1) & 1}]
if {$rxe == 0} {
    set rxd [mrd -force -value 0xE0000030]
    puts [format "Loopback: sent 0xAA, got 0x%02X — %s" $rxd [expr {$rxd == 0xAA ? "PASS" : "FAIL"}]]
} else {
    puts "Loopback failed — UART0 not working"
}

# Now switch to NORMAL mode for actual TX
mwr -force 0xE0000000 0x00000180
after 10
mwr -force 0xE0000034 62
mwr -force 0xE0000018 13
mwr -force 0xE0000004 0x00000020
mwr -force 0xE0000000 0x00000017
after 10

# Send data
puts "Sending ATOMIK on UART0..."
foreach byte {65 84 79 77 73 75 13 10} {
    mwr -force 0xE0000030 $byte
}
for {set i 0} {$i < 60} {incr i} {
    mwr -force 0xE0000030 0x55
}
after 200
puts "Sent ATOMIK + 60x U-pattern"

puts ""
puts "=== Also configure UART1 ==="
# Enable UART1 clock
mwr -force 0xF8000008 0x0000DF0D
set uart_clk [mrd -force -value 0xF8000154]
mwr -force 0xF8000154 [expr {$uart_clk | 0x02}]
set aper [mrd -force -value 0xF800012C]
mwr -force 0xF800012C [expr {$aper | (1 << 21)}]
mwr -force 0xF8000228 0x00000000

# Configure UART1
mwr -force 0xE0001000 0x00000180
after 10
mwr -force 0xE0001034 62
mwr -force 0xE0001018 13
mwr -force 0xE0001004 0x00000020
mwr -force 0xE0001000 0x00000017
after 10

# Send on UART1
puts "Sending ATOMIK on UART1..."
foreach byte {65 84 79 77 73 75 13 10} {
    mwr -force 0xE0001030 $byte
}
for {set i 0} {$i < 60} {incr i} {
    mwr -force 0xE0001030 0x55
}
after 200
puts "Sent ATOMIK + 60x U-pattern on UART1"

puts ""
puts "Check /dev/ttyUSB1 for data."
puts "=== Done ==="
