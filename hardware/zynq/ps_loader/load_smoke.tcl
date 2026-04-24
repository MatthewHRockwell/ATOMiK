# =============================================================================
# ATOMiK PS Loader — load + check status (xsdb)
#
# Loads ps_loader.elf onto Cortex-A9 core 0, runs it, then reads the
# scratch protocol area at 0x10100000 to report progress and per-file
# load results.
#
# Run: /opt/Xilinx/2025.2/Vivado/bin/xsdb load_smoke.tcl
# =============================================================================

set PS7_INIT "/home/mattrock/Projects/ATOMiK/hardware/zynq/scripts/ps7_init_rk7020f.tcl"
set ELF      "/home/mattrock/Projects/ATOMiK/hardware/zynq/ps_loader/build/ps_loader.elf"

# Scratch addresses (must match SCRATCH_BASE in main.c)
set SB           0x10100000
set S_DONE       [expr {$SB + 0x00}]
set S_MAGIC      [expr {$SB + 0x04}]
set S_HEARTBEAT  [expr {$SB + 0x08}]
set S_MARKER     [expr {$SB + 0x10}]
set S_LAST_ERR   [expr {$SB + 0x14}]
set S_OCR        [expr {$SB + 0x18}]
set S_RCA        [expr {$SB + 0x1C}]
set S_CID        [expr {$SB + 0x20}]
set S_FAT_PART   [expr {$SB + 0x40}]
set S_FAT_DATA   [expr {$SB + 0x44}]
set S_FAT_SPC    [expr {$SB + 0x48}]
set S_FILE_RES   [expr {$SB + 0x80}]

puts "== ps_loader smoke / load test =="

connect
after 500

targets -set -filter {name =~ "APU*"}
rst -system
after 500

puts "-- ps7_init --"
source $PS7_INIT
ps7_init
ps7_post_config

targets -set -filter {name =~ "ARM*Cortex-A9 MPCore #0"}
stop

# Pre-clear DONE flag so we can poll for the loader to set it
mwr -force $S_DONE 0x00000000

puts "-- dow $ELF --"
dow $ELF

puts "-- con --"
con

# Poll for completion or up to 30 s
set tries 0
while {$tries < 120} {
    set d [mrd -value -force $S_DONE]
    if {$d == 0xc0dec0de} break
    after 250
    incr tries
}

stop
puts ""
puts [format "magic       = 0x%08x  (expect 0xa70a1cba)"  [mrd -value -force $S_MAGIC]]
puts [format "heartbeat   = 0x%08x"                       [mrd -value -force $S_HEARTBEAT]]
puts [format "marker      = 0x%02x"                       [mrd -value -force $S_MARKER]]
puts [format "last_err    = %d"                           [mrd -value -force $S_LAST_ERR]]
puts [format "OCR         = 0x%08x"                       [mrd -value -force $S_OCR]]
puts [format "RCA         = 0x%08x"                       [mrd -value -force $S_RCA]]
puts [format "FAT part_lba= %d"                           [mrd -value -force $S_FAT_PART]]
puts [format "FAT data_lba= %d"                           [mrd -value -force $S_FAT_DATA]]
puts [format "FAT spc     = %d"                           [mrd -value -force $S_FAT_SPC]]
puts [format "DONE flag   = 0x%08x"                       [mrd -value -force $S_DONE]]

puts "\n-- File load results --"
for {set i 0} {$i < 4} {incr i} {
    set base [expr {$S_FILE_RES + $i * 16}]
    set ddr  [mrd -value -force $base]
    set sz   [mrd -value -force [expr {$base + 4}]]
    set err  [mrd -value -force [expr {$base + 8}]]
    set nm0  [mrd -value -force [expr {$base + 12}]]
    set name ""
    for {set b 0} {$b < 4} {incr b} {
        set by [expr {($nm0 >> ($b * 8)) & 0xff}]
        if {$by >= 0x20 && $by < 0x7F} { append name [format %c $by] } else { append name "." }
    }
    set serr [expr {$err > 0x7FFFFFFF ? $err - 0x100000000 : $err}]
    puts [format "  slot %d: name=%s  ddr=0x%08x  size=%d  err=%d" \
          $i $name $ddr $sz $serr]
}

if {[mrd -value -force $S_DONE] == 0xc0dec0de} {
    puts "\nLOADER: ALL FILES LOADED — ready to boot via UART (`boot 0x40f00000`)"
} else {
    puts "\nLOADER: not done (see marker / err / file slots above)"
}

disconnect
exit
