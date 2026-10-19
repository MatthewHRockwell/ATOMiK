# Phase 1 attempt 4: skip ps7_init entirely.
#
# Theory: the AX7020 boot-mode strap selects QSPI, so the on-chip BootROM
# already configured the LQSPI controller in linear mode to load the FSBL.
# Our ps7_init_rk7020f.tcl is tuned for *LiteX* boot (no QSPI use), so it
# may be tearing QSPI down. If we never run ps7_init, BootROM's config
# should still be live and 0xFC000000 should be readable directly.
#
# Caveat: DDR isn't initialized so big reads will be slow (no caching),
# but for the magic-word probe that's irrelevant.

connect

targets -set -filter {name =~ "*Cortex*A9*0"}
catch {stop} st
puts "stop: $st"

puts ""
puts "-- BootROM-state QSPI Linear @ 0xFC000000 (16 words, -force) --"
for {set i 0} {$i < 16} {incr i} {
    set addr [expr 0xFC000000 + ($i * 4)]
    if {[catch {mrd -force -value $addr} val]} {
        puts [format "  0x%08X = <read failed: %s>" $addr $val]
        if {$i == 0} { break }
        continue
    }
    puts [format "  0x%08X = 0x%08X" $addr $val]
}

puts ""
if {[catch {mrd -force -value 0xFC000020} magic]} {
    puts "FAIL: cannot read 0xFC000020"
    puts "  error: $magic"
} else {
    puts [format "-- Magic check: 0xFC000020 = 0x%08X --" $magic]
    if {$magic == 0xAA995566} {
        puts "OK: Zynq BOOT.BIN magic present. Phase 2 dump is safe."
    } else {
        puts "MISMATCH: expected 0xAA995566. May be empty / non-Zynq / wrong baud."
    }
}

# Also dump the ROM-configured QSPI controller state so we can copy its
# values into our ps7_init if needed.
puts ""
puts "-- BootROM's LQSPI controller state (force-read) --"
catch {mrd -force -value 0xE000D000} cr;          puts [format "  CR        = 0x%08X" $cr]
catch {mrd -force -value 0xE000D008} en;          puts [format "  EN_REG    = 0x%08X" $en]
catch {mrd -force -value 0xE000D03C} lq;          puts [format "  LQSPI_CFG = 0x%08X" $lq]
catch {mrd -force -value 0xE000D040} sts;         puts [format "  LQSPI_STS = 0x%08X" $sts]
catch {mrd -force -value 0xF8000124} clk;         puts [format "  LQSPI_CLK = 0x%08X" $clk]
catch {mrd -force -value 0xF800021C} rst;         puts [format "  LQSPI_RST = 0x%08X" $rst]

disconnect
exit
