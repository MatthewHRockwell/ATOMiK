# 🚀 START HERE - Next Session Quick Start

**Last Updated:** February 24, 2026
**Status:** ISP Works ✅ | Flash Boot Blocked 🔍
**Next Step:** Debug flash execution (estimated 1-2 hours)

---

## TL;DR - Where We Are

✅ **ISP Boot ROM works perfectly** - handshake, timeout, flash read all working
✅ **Flash programming works** - pico-programmer.py successfully writes firmware
❌ **Flash firmware won't execute** - no UART output after boot

**Goal:** Make flash firmware print "HELLO" → then we can run ATOMiK tests!

---

## Quick Test (2 minutes)

Verify ISP still works:

```bash
cd /home/mattrock/Projects/ATOMiK/hardware/v3/deploy

openFPGALoader -b tangnano9k atomik_v3_soc_isp.fs
sleep 0.3

python3 << 'EOF'
import serial, time
ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=0.5)
time.sleep(0.2)
ser.write(bytes([0x55]))
print("✅ ISP WORKS" if 0x56 in ser.read(5) else "❌ BROKEN - read PHASE3D_SESSION2_SUMMARY.md")
ser.close()
EOF
```

**Expected:** `✅ ISP WORKS`

---

## Debugging Plan (Start Here!)

### Step 1: Ultra-Minimal Assembly Test (30 min)

**Goal:** Prove CPU can execute ANYTHING from flash

Create `test_asm_only.S`:
```assembly
.section .text
.global _start

_start:
    # Initialize UART
    li t0, 0x83000000      # UART base
    li t1, 232             # CLKDIV = 27000000/115200 - 2
    sw t1, 4(t0)           # UART->CLKDIV

    # Wait for UART to settle
    li t2, 2000
wait_loop:
    addi t2, t2, -1
    bnez t2, wait_loop

    # Print 'H' repeatedly
print_loop:
    li a0, 'H'
    sw a0, 0(t0)           # UART->DATA

    # Delay
    li t3, 100000
delay:
    addi t3, t3, -1
    bnez t3, delay

    j print_loop
```

Build and flash:
```bash
cd /home/mattrock/Projects/ATOMiK/hardware/v3/soc/firmware/fw-flash

# Compile pure assembly (NO CRT!)
riscv64-unknown-elf-as -march=rv64i -o build/test_asm.o test_asm_only.S
riscv64-unknown-elf-ld -Ttext=0x00000000 -o build/test_asm.elf build/test_asm.o
riscv64-unknown-elf-objcopy -O verilog build/test_asm.elf build/test_asm.v

# Flash it
openFPGALoader -b tangnano9k ../../../deploy/atomik_v3_soc_isp.fs
sleep 0.5
python3 ../scripts/pico-programmer.py build/test_asm.v /dev/ttyUSB1

# Reload bitstream to boot from flash
openFPGALoader -b tangnano9k ../../../deploy/atomik_v3_soc_isp.fs
sleep 2

# Read output
python3 << 'EOF'
import serial, time
ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=2)
data = ser.read(100)
if data:
    print(f"✅ SUCCESS! Got: {data[:20]}")
    print("Flash execution WORKS! Issue is in CRT startup code.")
else:
    print("❌ No output - flash XIP is broken")
    print("Next: Check spimemio_puya.v flash configuration")
ser.close()
EOF
```

**If this works →** Issue is in `crt_flash.S` startup code (go to Step 2)
**If this fails →** Flash XIP is broken (go to Step 3)

---

### Step 2: Debug CRT Startup (if Step 1 works)

Add diagnostics to `crt_flash.S`:

```assembly
crtStart:
  # Print 'A' = started
  li a0, 'A'
  li a1, 0x83000000
  li a2, 232
  sw a2, 4(a1)
  li a3, 2000
1: addi a3, a3, -1
  bnez a3, 1b
  sw a0, 0(a1)

  j crtInit

crtInit:
  # Print 'B' = init
  li a0, 'B'
  li a1, 0x83000000
  sw a0, 0(a1)

  # ... rest of CRT ...
```

Test progressively to find where it crashes.

---

### Step 3: Fix Flash XIP (if Step 1 fails)

Check `/home/mattrock/Projects/ATOMiK/hardware/v3/soc/spimemio_puya.v`:

Look for:
1. **XIP mode enable** - should be default after reset
2. **Fast read command** - should use 0x0B or 0xEB
3. **Boot ROM interference** - does diagnostic read break XIP?

**Key question:** Does Boot ROM leave flash in XIP mode?

Check Boot ROM code:
```c
// isp_flasher.c line 212
volatile uint32_t *fp = (volatile uint32_t *)0x00000004;
uint32_t w = *fp;  // <-- Does this break XIP?
```

**Try:** Comment out lines 211-218 (diagnostic print), rebuild Boot ROM, re-synthesize, test.

---

## Files You'll Need

```
soc/firmware/fw-flash/test_asm_only.S     - Create this (Step 1)
soc/firmware/fw-flash/crt_flash.S         - Debug this (Step 2)
soc/spimemio_puya.v                       - Check this (Step 3)
soc/firmware/fw-brom/isp_flasher.c        - Modify this (Step 3)
```

---

## If You Get Stuck

Read: `PHASE3D_SESSION2_SUMMARY.md` - full debugging context

**Most likely issue:** Flash XIP mode configuration
**Second most likely:** CRT startup code crash
**Least likely:** Jump mechanism broken

---

## Success Criteria

When you see this, you're done:

```
*** FLASH BOOT WORKS! ***
Press X to run ATOMiK tests
*** FLASH BOOT WORKS! ***
Press X to run ATOMiK tests
```

Then flash the full firmware (`fw-flash.v`) and press 'X' to run the 9 ATOMiK tests!

---

## Estimated Time

- **Best case:** 30 min (assembly test works immediately)
- **Typical case:** 1-2 hours (find and fix flash XIP issue)
- **Worst case:** 3 hours (need to dig deep into SPI flash driver)

**You've got this!** 🚀
