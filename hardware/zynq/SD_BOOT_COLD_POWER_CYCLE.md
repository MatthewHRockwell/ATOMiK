# SD Boot Cold Power-Cycle Definition

For this RK-ZYNQ7020-F SD BootROM debug loop, a cold power-cycle means the Zynq PS sees a real POR event: `PS_POR_B` asserted, boot-mode straps resampled, SD card power removed, and USB/JTAG/HDMI back-power paths removed.

A cold power-cycle is not:

- `xsdb rst-system`
- the reset pushbutton
- toggling the board power switch while Debug USB-C/JTAG/UART stays connected
- a fast five-second off/on if LEDs or USB devices never fully disappear

Procedure for Claude/operator:

1. Stop serial monitors and xsdb/openFPGALoader sessions.
2. Set BOOT[1:0] to SD mode: `11`.
3. Turn off/remove the board main supply.
4. Unplug Debug USB-C/JTAG/UART, USB OTG/Host, HDMI, and any cable that can source 5 V.
5. Confirm all board LEDs are dark and the FTDI `/dev/ttyUSB*` devices disappear from the host.
6. Wait 30 seconds after LEDs go dark.
7. Reconnect main board power first. Do not press reset.
8. Reconnect Debug USB-C only after main power is up, then wait 10 seconds for USB re-enumeration.
9. Run only the no-reset BootROM probe; do not run `rst-system` before probing.

Why this matters: UG585 says the Zynq boot-mode straps are sampled soon after `PS_POR_B` deasserts. A non-POR reset starts BootROM again but does not recreate the same hardware power-up stage, and this repo has already observed JTAG halt failures after `rst-system`.
