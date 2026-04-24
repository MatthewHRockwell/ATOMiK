# Complete Zynq PL programming via OpenOCD
# 1. ps7_init (DDR3, PLLs, clocks)
# 2. Load bitstream to DDR3
# 3. PCAP DMA to program PL
# 4. Enable level shifters

# Load ps7_init procs
source [find scripts/ps7_init_openocd.tcl]

proc program_zynq {bitfile} {
    # Run ps7_init
    puts "Running ps7_init..."
    ps7_init
    ps7_post_config
    puts "ps7_init complete"

    # Load bitstream to DDR3
    puts "Loading bitstream to DDR3..."
    load_image $bitfile 0x04000000 bin
    puts "Bitstream loaded"

    # SLCR unlock
    mww 0xF8000008 0x0000DF0D

    # Enable PCAP clock
    set ctrl [mrw 0xF8007000]
    mww 0xF8007000 [expr {$ctrl | 0x0C006000}]
    sleep 10

    # Clear interrupts
    mww 0xF800700C 0xFFFFFFFF

    # Get file size
    set fsize [file size $bitfile]
    set nwords [expr {$fsize / 4}]
    puts "Bitstream: $fsize bytes ($nwords words)"

    # DMA: source = DDR3, dest = PCAP (0xFFFFFFFF)
    mww 0xF8007018 0x04000000
    mww 0xF800701C 0xFFFFFFFF
    mww 0xF8007020 $nwords
    mww 0xF8007024 $nwords

    puts "PCAP DMA started..."

    # Wait for PCFG_DONE (bit 2 of INT_STS)
    for {set i 0} {$i < 200} {incr i} {
        sleep 50
        set isr [mrw 0xF800700C]
        if {[expr {$isr & 0x04}]} {
            puts "PCFG_DONE!"
            break
        }
    }

    set isr [mrw 0xF800700C]
    set status [mrw 0xF8007014]
    puts [format "DevC ISR=0x%08X STATUS=0x%08X" $isr $status]

    if {[expr {$isr & 0x04}] == 0} {
        puts "ERROR: PCAP programming failed"
        return -1
    }

    # Enable level shifters
    mww 0xF8000900 0x0000000F
    # Clear FPGA resets
    mww 0xF8000240 0x00000000

    puts "PL programmed and enabled"
    return 0
}
