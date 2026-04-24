# =============================================================================
# Proper Zynq JTAG Programming Sequence (per Xilinx AR# 52028)
#
# On Zynq, the correct order is:
#   1. System reset
#   2. PS7 initialization (PLLs, DDR, MIO, clocks)
#   3. PL bitstream programming
#   4. PS7 post-config (releases PL I/O from hold)
#
# Usage: xsdb program_zynq_proper.tcl <bitfile>
# =============================================================================

set SCRIPT_DIR [file dirname [info script]]
set bitfile [lindex $argv 0]

if {$bitfile eq ""} {
    puts "Usage: xsdb program_zynq_proper.tcl <bitfile>"
    exit 1
}

puts "============================================"
puts " Zynq Proper JTAG Programming"
puts " Bitfile: $bitfile"
puts "============================================"

connect

puts "\nTargets:"
targets

# Step 1: System reset
puts "\n\[1\] System reset..."
targets -set -filter {name =~ "APU*"}
rst -system
after 1000
puts "    Reset complete."

# Step 2: Initialize PS7
puts "\n\[2\] Initializing PS7..."
targets -set -filter {name =~ "APU*"}
source $SCRIPT_DIR/ps7_init_rk7020f.tcl
ps7_init
puts "    PS7 PLLs, clocks, MIO initialized."

# Step 3: Program PL
puts "\n\[3\] Programming PL bitstream..."
targets -set -filter {name =~ "xc7z*"}
fpga -file $bitfile
after 500
puts "    PL programmed."

# Step 4: Post-config (releases PL I/O hold, enables fabric)
puts "\n\[4\] PS7 post-config..."
targets -set -filter {name =~ "APU*"}
ps7_post_config
puts "    PL I/O released."

puts "\n\[5\] Verifying registers..."
targets -set -filter {name =~ "APU*"}
set lvl [mrd -force -value 0xF8000900]
set rst [mrd -force -value 0xF8000240]
set pll [mrd -force -value 0xF800010C]
set sts [mrd -force -value 0xF8007014]
set ist [mrd -force -value 0xF800700C]
puts "    LVL_SHFTR_EN:   0x[format %08X $lvl] (need 0x0000000F)"
puts "    FPGA_RST_CTRL:  0x[format %08X $rst] (need 0x00000000)"
puts "    PLL_STATUS:      0x[format %08X $pll] (need 0x0000003F)"
puts "    DEVCFG_STATUS:   0x[format %08X $sts] (bits 11,8 = GTS released)"
puts "    DEVCFG_INT_STS:  0x[format %08X $ist] (bit 2 = PCFG_DONE)"
set gts_usr [expr {($sts >> 11) & 1}]
set gts_cfg [expr {($sts >> 8) & 1}]
set pcfg_done [expr {($ist >> 2) & 1}]
puts "    GTS_USR=$gts_usr GTS_CFG=$gts_cfg PCFG_DONE=$pcfg_done"
if {$lvl == 0x0F && $rst == 0 && $pcfg_done == 1} {
    puts "    ALL CHECKS PASSED"
} else {
    puts "    WARNING: Some checks failed!"
}

puts "\n============================================"
puts " Done."
puts "============================================"

after 1000
disconnect
exit
