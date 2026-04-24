# Load and run bare-metal UART sender on Cortex-A9
# ps7_init configures clocks/MIO, then ARM core runs uart_sender.elf

set SCRIPT_DIR [file dirname [info script]]

connect
after 500

puts "=== Bare-Metal UART Sender ==="

# Reset
targets -set -filter {name =~ "APU*"}
rst -system
after 1000

# PS7 init (PLLs, clocks, MIO, DDR)
source $SCRIPT_DIR/ps7_init_rk7020f.tcl
ps7_init
ps7_post_config
puts "ps7_init complete"

# Target Cortex-A9 #0
targets -set -filter {name =~ "ARM*#0"}
puts "Target: [targets -filter {name =~ "ARM*#0"}]"

# Stop the core
stop
after 100

# Load ELF to OCM
dow test/uart_sender.elf
puts "ELF loaded to OCM"

# Verify first few words
set w0 [mrd -force -value 0x00000000]
set w1 [mrd -force -value 0x00000004]
puts [format "OCM[0x00]=0x%08X OCM[0x04]=0x%08X" $w0 $w1]

# Check UART0 status before running
set cr [mrd -force -value 0xE0000000]
set sr [mrd -force -value 0xE000002C]
puts [format "Before run: CR=0x%08X SR=0x%08X" $cr $sr]

# GO — run the ARM core
puts "Starting ARM core..."
con

# Wait and check
after 2000

# Read UART0 status while ARM is running
set sr2 [mrd -force -value 0xE000002C]
set cr2 [mrd -force -value 0xE0000000]
puts [format "While running: CR=0x%08X SR=0x%08X (TACTIVE=%d)" $cr2 $sr2 [expr {($sr2 >> 11) & 1}]]

puts ""
puts "ARM core is running. UART0 TX should be blasting on MIO10."
puts "Monitor /dev/ttyUSB0 (or ttyUSB1) at 115200 for data."
puts "Press Ctrl-C or run 'stop' to halt."
puts ""
puts "Leaving xsdb running (ARM core continues)..."

# Keep xsdb alive so ARM core keeps running
# (disconnecting would halt the core)
after 30000
puts "30s elapsed — stopping core"
stop
set sr3 [mrd -force -value 0xE000002C]
puts [format "After stop: SR=0x%08X (TACTIVE=%d)" $sr3 [expr {($sr3 >> 11) & 1}]]
