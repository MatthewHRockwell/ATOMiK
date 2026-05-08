# ATOMiK input controller (Pi Pico HID-to-UART bridge)

The "ATOMiK input controller" is a Pi Pico running USB-host firmware
that translates wireless-dongle / wired-keyboard HID reports into a
plain ASCII byte stream over UART, fed into atomik_os via a Linux
serial device on the AX7020.

This is **v0.33** of ATOMiK Desk per the
[ChatGPT-validated v0.33 plan](../../.claude/.../project_v033_plan.md).
It exists because the AX7020's native PS USB host enumerates
unreliably (~50% of cold boots produce `device descriptor read/64
error -110` regression). Native USB hardening is queued as v0.35; the
input controller is the architectural answer that unblocks the demo.

## Why this isn't a hack

The bridge is **a permanent component** of the demo system, framed
the same way every laptop's keyboard MCU is — an embedded input
controller separate from the main compute. ATOMiK OS, Resource
Fabric, personality switching, and the entire compute story all run
on the RV64/NaxRiscv path. The Pico just normalizes input.

## Hardware

| Item | Spec |
|---|---|
| Board | Raspberry Pi Pico (RP2040) |
| Power | Pico's USB-C, supplied from demo enclosure 5V rail |
| USB-host port (for the dongle) | GP2 (D+) / GP3 (D-) — wire to a USB-A breakout |
| UART out (to AX7020) | GP0 TX, 115200 8N1 |
| Reset / BOOTSEL | Pico's onboard buttons |
| Status LED | onboard GP25 |

### Pico ↔ AX7020 wiring

| Pico | AX7020 (PMOD JD or chosen LiteUART) | Notes |
|---|---|---|
| GP0 (TX) | UART RX | data direction: bridge → board |
| GND | GND | common ground required |
| (no Vcc) | — | bridge is self-powered, do NOT cross-power |

The AX7020 side needs a **second LiteUART** in the LiteX SoC routed
to a PMOD pin pair, exposed as `/dev/ttyLXU1`. See
`hardware/zynq/litex/soc_nax64_atomik.py` — TODO patch will add this.

### USB-A breakout for the dongle

The Pico's GP2/GP3 are 3.3V GPIO; USB requires 5V VBUS to the device.
Wire a USB-A receptacle as:

| USB-A pin | wire to |
|---|---|
| 1 (VBUS) | Pico VBUS (5V) |
| 2 (D-)   | GP3 |
| 3 (D+)   | GP2 |
| 4 (GND)  | GND |

Most Pi Pico USB-host hobbyist projects use the [Adafruit USB-host
Featherwing](https://www.adafruit.com/product/5713) or a simple
through-hole USB-A breakout board. Either works.

## Building the firmware

Prerequisites:
- [Pico SDK](https://github.com/raspberrypi/pico-sdk) at `$PICO_SDK_PATH`
- [Pico-PIO-USB](https://github.com/sekigon-gonnoc/Pico-PIO-USB) cloned into `hardware/hid_bridge/Pico-PIO-USB/`
- `arm-none-eabi-gcc` toolchain (Ubuntu: `sudo apt install gcc-arm-none-eabi`)

```bash
cd hardware/hid_bridge
git clone https://github.com/sekigon-gonnoc/Pico-PIO-USB.git
mkdir build && cd build
cmake -DPICO_SDK_PATH=$PICO_SDK_PATH ..
make -j
# → produces hid_bridge.uf2
```

## Flashing

1. Hold the **BOOTSEL** button on the Pico.
2. Plug Pico into the laptop via USB-C.
3. Pico mounts as a USB mass-storage device named `RPI-RP2`.
4. Drag `hid_bridge.uf2` onto the mount.
5. Pico reboots and runs the firmware. LED blinks 3× on boot.

## Protocol

Plain ASCII byte stream over UART at 115200 baud. Each key press
emits one byte. The translation table mirrors
`atomik_os/src/input.c::keycode_to_ascii()` so semantics match.

| Key | Byte |
|---|---|
| `a`..`z`, `A`..`Z` | the literal letter |
| `0`..`9` and shifted symbols | the literal char (US layout) |
| `Enter` | `0x0A` (`\n`) |
| `Esc` | `0x1B` |
| `Tab` | `0x09` |
| `Backspace` | `0x7F` |
| `Ctrl-W` | `0x17` (close window in atomik_os) |
| `Ctrl-C` | `0x03` (quit) |
| Space | `0x20` |

Held keys do **not** auto-repeat from the bridge — atomik_os manages
its own repeat semantics. Edge-detection in `tuh_hid_report_received_cb`
emits one byte on the press transition only.

## Wiring on the AX7020 side

The new LiteUART (added in `soc_nax64_atomik.py`, exposed as
`/dev/ttyLXU1` after Linux DT registration) gets routed into
atomik_os via the boot script:

```sh
# /sbin/atomik_boot.sh additions:
stty -F /dev/ttyLXU1 115200 cs8 -parenb -cstopb raw -echo
cat /dev/ttyLXU1 > /tmp/aos_keys &
```

That makes every byte the Pico sends appear on `/tmp/aos_keys`, which
atomik_os already reads as stdin via the existing FIFO path. No
atomik_os code change required.

## Acceptance test (cold boot, no laptop tethered)

1. Power on the AX7020.
2. Wait for the desktop to appear on HDMI.
3. Plug the wireless dongle into the bridge's USB-A port.
4. Press `R` → Resource Fabric appears.
5. Press `D` → Document appears.
6. Type `hello` in Document → STATE personality activates on Fabric.
7. Press `P` → MANUAL personality badge cycles.
8. Press `Esc` or `Ctrl-W` → closes focused window.
9. Repeat 20× without dropped input.

## Debugging

The Pico's native USB port runs a USB-CDC serial endpoint (separate
from the input UART). Plug the Pico into a laptop while the bridge
is running — `/dev/ttyACM*` on the laptop will show TinyUSB host-stack
messages. Useful for verifying the dongle enumerated, parsing report
descriptors, etc. Disable in production by removing `pico_enable_stdio_usb(hid_bridge 1)` from CMakeLists.txt.

## Limitations

- Boot-protocol keyboards only for v1 (most wireless dongles work).
- No mouse handling yet — wireless dongles that present a separate
  mouse interface have it ignored. Add in v0.34 if needed.
- No multi-modifier handling beyond Shift + Ctrl (Alt and GUI are
  ignored). Add when atomik_os has shortcuts that need them.
- Power: the bridge is self-powered via its own USB-C. The dongle
  draws ~50 mA from VBUS; the Pico can supply that. If the demo
  requires a high-current device (unlikely for HID), add a separate
  5V power input.
