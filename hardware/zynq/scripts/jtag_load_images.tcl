# =============================================================================
# JTAG Fast Image Loader for ATOMiK Zynq Linux Boot
#
# Loads kernel, DTB, OpenSBI, and rootfs directly to PS DDR via ARM debug port.
# ~10 seconds vs ~22 minutes for SFL serial upload at 115200 baud.
#
# Usage: xsdb scripts/jtag_load_images.tcl [image-set]
#        image-set: "69" (kernel 6.9, default) or "32" (kernel 4.20)
#
# Prerequisites:
#   - FPGA already programmed (run auto_boot_linux.py first, or make test-jtag)
#   - LiteX BIOS running and ready (serial terminal should show "litex>" prompt)
#
# After this script, type on the serial terminal:
#   boot 0x40f00000
#
# GP0 address mapping: VexRiscv X → PS DDR = X - 0x40000000 + 0x00100000
# =============================================================================

set SCRIPT_DIR [file dirname [info script]]
set ZYNQ_DIR   [file normalize "$SCRIPT_DIR/.."]
set LITEX_BUILD "$ZYNQ_DIR/litex-build"

# Select image set
if {[llength $argv] > 0} {
    set tag [lindex $argv 0]
} else {
    set tag "69"
}

# GP0 address offset: VexRiscv → PS DDR
proc vex_to_ps {vex_addr} {
    return [expr {$vex_addr - 0x40000000 + 0x00100000}]
}

# Image configurations
# Format: {file vex_address description}
switch $tag {
    "69" {
        set images [list \
            [list "Image69"       0x40000000 "Linux 6.9 kernel"] \
            [list "linux69.dtb"   0x40ef0000 "Device tree"]      \
            [list "fw_jump69.bin" 0x40f00000 "OpenSBI 1.3.1"]    \
            [list "rootfs69.cpio" 0x42000000 "Rootfs + atomik_test"] \
        ]
    }
    "32" {
        set images [list \
            [list "Image32"                                                              0x40000000 "Linux 4.20 kernel"] \
            [list "linux32.dtb"                                                          0x40ef0000 "Device tree"]       \
            [list "linux-build/opensbi/build/platform/litex/vexriscv/firmware/fw_jump.bin" 0x40f00000 "OpenSBI 1.3"]      \
            [list "rootfs.cpio"                                                          0x41000000 "BusyBox rootfs"]    \
        ]
    }
    default {
        puts "ERROR: Unknown image set '$tag'. Use '69' or '32'."
        exit 1
    }
}

puts "=============================================="
puts " JTAG Fast Image Loader"
puts " Image set: $tag"
puts "=============================================="

# Verify all files exist
foreach img $images {
    set fname [lindex $img 0]
    set fpath "$LITEX_BUILD/$fname"
    if {![file exists $fpath]} {
        puts "ERROR: Missing: $fpath"
        if {$tag eq "69"} {
            puts "  Run: scripts/prepare_boot_images.sh <buildroot-images-dir>"
        }
        exit 1
    }
}

# Connect to JTAG
puts "\nConnecting to JTAG..."
connect
after 1000

# Target the ARM core for DDR writes
targets -set -filter {name =~ "APU*"}

# Load each image
set total_bytes 0
foreach img $images {
    set fname [lindex $img 0]
    set vex_addr [lindex $img 1]
    set desc [lindex $img 2]
    set fpath "$LITEX_BUILD/$fname"
    set ps_addr [vex_to_ps $vex_addr]
    set fsize [file size $fpath]
    set total_bytes [expr {$total_bytes + $fsize}]

    puts [format "\n  Loading %-24s → PS 0x%08X (VexRiscv 0x%08X)" $desc $ps_addr $vex_addr]
    puts [format "    File: %s (%d bytes, %.1f MB)" $fname $fsize [expr {$fsize / 1048576.0}]]

    dow -data $fpath $ps_addr
}

puts [format "\n  Total: %d bytes (%.1f MB)" $total_bytes [expr {$total_bytes / 1048576.0}]]

# Flush PL310 L2 cache to ensure DDR is coherent with GP0 reads
puts "\n  Flushing PL310 L2 cache..."

# PL310 L2CC registers on Zynq
set PL310_BASE      0xF8F02000
set PL310_CTRL      [expr {$PL310_BASE + 0x100}]
set PL310_CLEAN_INV [expr {$PL310_BASE + 0x7FC}]
set PL310_SYNC      [expr {$PL310_BASE + 0x730}]

# Clean + Invalidate all ways (16-way set-associative → bits [15:0])
mwr -force $PL310_CLEAN_INV 0x0000FFFF
after 100

# Wait for completion (bits clear when done)
for {set i 0} {$i < 10} {incr i} {
    set val [mrd -force -value $PL310_CLEAN_INV]
    if {$val == 0} break
    after 50
}
if {$val != 0} {
    puts "  WARNING: PL310 flush may not have completed (val=0x[format %08X $val])"
}

# Cache sync
mwr -force $PL310_SYNC 0
after 10

puts "  L2 cache flushed."

disconnect

puts "\n=============================================="
puts " Images loaded to DDR via JTAG."
puts ""
puts " Now open a serial terminal and type:"
puts "   boot 0x40f00000"
puts ""
puts " Or use: python3 -m litex.tools.litex_term /dev/ttyUSB0 --speed 115200"
puts "=============================================="
