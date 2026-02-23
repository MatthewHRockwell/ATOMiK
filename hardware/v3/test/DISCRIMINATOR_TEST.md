# UART Discriminator Test Instructions

## Purpose
This test determines WHERE the UART failure occurs by monitoring internal signals on GPIO pins.

## Hardware Setup
Flash the bitstream:
```bash
cd /home/mattrock/Projects/ATOMiK/hardware/v3/test
openFPGALoader -b tangnano9k impl/pnr/project.fs
```

Wait 5 seconds after loading, then observe the GPIO pins.

## GPIO Pin Mapping

| Pin | Signal | Meaning |
|-----|--------|---------|
| 25 | gpio[0] | State bit 0 (LSB) |
| 26 | gpio[1] | State bit 1 |
| 27 | gpio[2] | State bit 2 |
| 28 | gpio[3] | State bit 3 (MSB) |
| 29 | gpio[4] | (unused, always 0) |
| 31 | heartbeat | Toggles when FSM is active |
| 32 | uart_tap | **Internal uart_tx signal** |

## Expected Behavior

### FSM State Progression (pins 25-28)
The state should progress through this sequence:
- **State 0 (0b0000)**: Initial wait after reset (~1000 cycles = 37 µs)
- **State 1 (0b0001)**: Assert CLKDIV write (1 cycle)
- **State 2 (0b0010)**: Deassert CLKDIV (1 cycle)
- **State 3 (0b0011)**: Wait for dummy bits (~4000 cycles = 148 µs)
- **State 4 (0b0100)**: Assert DATA write
- **State 5 (0b0101)**: Wait for UART ready
- **State 6 (0b0110)**: Wait for acknowledge
- **State 7 (0b0111)**: Inter-byte delay (~5000 cycles = 185 µs)
- Loop back to State 4

**Visual Check:** After ~200 µs, pins 25-28 should show state 4-7 pattern repeating.

### Heartbeat (pin 31)
Should **toggle continuously** (changes every ~200 µs as FSM progresses).

**If heartbeat is STUCK:** FSM/clock/reset issue - the CPU isn't running!

### UART Internal Tap (pin 32)
Should **toggle during transmission** at 115200 baud (~8.7 µs per bit, ~87 µs per byte).

**If heartbeat toggles but uart_tap is STUCK HIGH:** UART logic issue (simpleuart broken)
**If heartbeat toggles and uart_tap toggles:** Check pin 17 (actual uart_tx pad)

### Actual UART TX (pin 17)
Should match pin 32 (uart_tap).

**If pin 32 toggles but pin 17 is stuck:** I/O pad mapping/constraint issue

## Diagnostic Results

### Scenario A: Heartbeat STUCK
```
Pin 31 (heartbeat): Constant (not toggling)
Pin 32 (uart_tap): Constant HIGH
```
**Diagnosis:** FSM not running - clock or reset issue
**Next:** Check clock generation, extend reset hold time

### Scenario B: Heartbeat OK, Internal Tap STUCK
```
Pin 31 (heartbeat): Toggling ✅
Pin 32 (uart_tap): Constant HIGH ❌
```
**Diagnosis:** simpleuart logic broken after synthesis
**Next:** Add (* keep *) attributes or replace UART module

### Scenario C: Heartbeat OK, Internal Tap OK, Pad STUCK
```
Pin 31 (heartbeat): Toggling ✅
Pin 32 (uart_tap): Toggling ✅
Pin 17 (uart_tx): Constant HIGH ❌
```
**Diagnosis:** I/O pad contention or pin constraint issue
**Next:** Check .cst file, verify no other signals driving pin 17

### Scenario D: Everything Toggles!
```
Pin 31 (heartbeat): Toggling ✅
Pin 32 (uart_tap): Toggling ✅
Pin 17 (uart_tx): Toggling ✅
```
**Diagnosis:** UART TX is working! Check /dev/ttyUSB1 again
**Possible:** Wrong baud rate on receive side

## Quick Visual Test

**Easiest check with multimeter:**
1. Measure pin 31 (heartbeat) - should oscillate ~2.7 kHz (toggling every 185 µs)
2. Measure pin 32 (uart_tap) - should show UART activity

**Or use LED:**
- Connect LED + resistor to pin 31 - should flicker rapidly
- Connect LED to pin 32 - should show brief flashes during TX

## Report Results

Please report which scenario matches your observation:
- [ ] Scenario A: Heartbeat stuck
- [ ] Scenario B: Heartbeat OK, tap stuck
- [ ] Scenario C: Heartbeat OK, tap OK, pad stuck
- [ ] Scenario D: Everything works (check serial port!)

This single test will tell us exactly where to focus next.
