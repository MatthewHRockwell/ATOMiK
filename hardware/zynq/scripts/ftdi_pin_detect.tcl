# FTDI pin detection via GPIO break test
# 1. Read all MIO GPIO pins (idle state - FTDI TX should be HIGH)
# 2. Wait for host to send break on /dev/ttyUSB1 (FTDI TX goes LOW)
# 3. Read again and find which pin changed

connect
after 200
targets -set -filter {name =~ "APU*"}
rst -system
after 1000

source scripts/ps7_init_rk7020f.tcl
ps7_init
ps7_post_config

puts "=== FTDI Pin Detection via GPIO ==="

# Unlock SLCR
mwr -force 0xF8000008 0x0000DF0D

# Configure ALL MIO pins as GPIO inputs (except JTAG/boot pins)
# GPIO DIRM registers: 0 = input
# Bank 0 (MIO[31:0]): 0xE000A204
# Bank 1 (MIO[53:32]): 0xE000A244
mwr -force 0xE000A204 0x00000000
mwr -force 0xE000A244 0x00000000

# Enable GPIO controller output enable registers (OEN) = 0 for inputs
mwr -force 0xE000A208 0x00000000
mwr -force 0xE000A248 0x00000000

puts ""
puts "=== Phase 1: Read GPIO with FTDI idle (expect FTDI TX = HIGH) ==="

# Read DATA_RO (read-only input values)
set bank0_idle [mrd -force -value 0xE000A060]
set bank1_idle [mrd -force -value 0xE000A064]
puts [format "Bank 0 (MIO 31:0):  0x%08X" $bank0_idle]
puts [format "Bank 1 (MIO 53:32): 0x%08X" $bank1_idle]

# Print individual pin states
puts ""
puts "Pin states (1=HIGH, 0=LOW):"
for {set i 0} {$i < 32} {incr i} {
    set val [expr {($bank0_idle >> $i) & 1}]
    if {$val} {
        puts [format "  MIO%02d: HIGH" $i]
    }
}
for {set i 0} {$i < 22} {incr i} {
    set val [expr {($bank1_idle >> $i) & 1}]
    if {$val} {
        puts [format "  MIO%02d: HIGH" [expr {$i + 32}]]
    }
}

puts ""
puts "=== Phase 2: Waiting 8 seconds for break signal ==="
puts "  Run this on host: python3 -c \"import serial,time; s=serial.Serial('/dev/ttyUSB1',115200); s.break_condition=True; time.sleep(10); s.close()\""
puts "  Or: /tmp/ftdi_break.sh"
puts ""

# Signal ready via file
set f [open /tmp/ftdi_gpio_ready "w"]
puts $f "ready"
close $f

# Wait for break to be active
after 8000

puts "=== Phase 3: Read GPIO with FTDI break (expect FTDI TX = LOW) ==="
set bank0_break [mrd -force -value 0xE000A060]
set bank1_break [mrd -force -value 0xE000A064]
puts [format "Bank 0 (MIO 31:0):  0x%08X" $bank0_break]
puts [format "Bank 1 (MIO 53:32): 0x%08X" $bank1_break]

puts ""
puts "=== Phase 4: Diff ==="
set diff0 [expr {$bank0_idle ^ $bank0_break}]
set diff1 [expr {$bank1_idle ^ $bank1_break}]
puts [format "Changed Bank 0: 0x%08X" $diff0]
puts [format "Changed Bank 1: 0x%08X" $diff1]

# Find changed pins (HIGH→LOW = FTDI TX pin)
set found 0
for {set i 0} {$i < 32} {incr i} {
    if {[expr {($diff0 >> $i) & 1}]} {
        set idle_val [expr {($bank0_idle >> $i) & 1}]
        set break_val [expr {($bank0_break >> $i) & 1}]
        puts [format "  *** MIO%02d changed: %d → %d" $i $idle_val $break_val]
        if {$idle_val == 1 && $break_val == 0} {
            puts [format "  >>> MIO%02d is likely FTDI TX (RX from Zynq perspective)" $i]
        }
        set found 1
    }
}
for {set i 0} {$i < 22} {incr i} {
    if {[expr {($diff1 >> $i) & 1}]} {
        set idle_val [expr {($bank1_idle >> $i) & 1}]
        set break_val [expr {($bank1_break >> $i) & 1}]
        puts [format "  *** MIO%02d changed: %d → %d" [expr {$i + 32}] $idle_val $break_val]
        if {$idle_val == 1 && $break_val == 0} {
            puts [format "  >>> MIO%02d is likely FTDI TX (RX from Zynq perspective)" [expr {$i + 32}]]
        }
        set found 1
    }
}

if {!$found} {
    puts "  No MIO pins changed — FTDI TX is NOT on any MIO pin"
    puts "  FTDI TX must be on a PL pin (need bitstream to detect)"
    puts "  OR Channel B is not connected to the Zynq at all"
}

# Also do a third read after break should be released
puts ""
puts "=== Phase 5: Read again (after break should be over) ==="
after 5000
set bank0_post [mrd -force -value 0xE000A060]
set bank1_post [mrd -force -value 0xE000A064]
set diff0_post [expr {$bank0_idle ^ $bank0_post}]
set diff1_post [expr {$bank1_idle ^ $bank1_post}]
puts [format "Bank 0: 0x%08X (diff from idle: 0x%08X)" $bank0_post $diff0_post]
puts [format "Bank 1: 0x%08X (diff from idle: 0x%08X)" $bank1_post $diff1_post]
