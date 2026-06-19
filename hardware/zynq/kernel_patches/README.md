# Kernel patches — reliable USB HID input on AX7020 (NaxRiscv RV64)

The Linux 6.1 kernel for the NaxRiscv SoC lives in the (gitignored) build tree
`hardware/zynq/litex-build/linux-build/litex-linux`.  These two changes make USB
HID input reliable; they are preserved here because that tree isn't tracked.

## Why
The PS->PL fabric interrupt tap `IRQ_P2F_USB0` does not reliably carry the
chipidea USB controller's interrupt to the NaxRiscv PLIC (register-proven: the
controller asserts `USBSTS & USBINTR != 0` but PLIC source 3 never pends — see
memory `project_usb_irq_root_cause_FINAL`).  Booting with `irqpoll` gets USB
alive but irqpoll is a spurious-IRQ *recovery* mechanism that backs off after
~1-2 s, so HID delivery dies mid-use.  A PL-side gateware poller delivered
reliably but broke USB enumeration as a third bus master.

## The two changes

1. **`CONFIG_HZ=1000` + `CONFIG_HIGH_RES_TIMERS=y`** (`.config`).
   1 ms tick (vs the default 100 = 10 ms, slower than HID's ~8 ms report rate),
   and high-res timers so the poller below can use an hrtimer.

2. **`kernel/usb_irqpoll.c`** (this dir) — a built-in `late_initcall` that starts
   a 1 ms hrtimer which `generic_handle_irq()`s the USB virq (found by name
   `80002000.usb`) **continuously, forever** — the software analogue of the PL
   poller, with no gateware bus master and no irqpoll backoff.  Add to the build:
   ```
   cp usb_irqpoll.c <kerneltree>/kernel/usb_irqpoll.c
   # kernel/Makefile: add a line  ->  obj-y += usb_irqpoll.o
   make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc) Image
   cp arch/riscv/boot/Image  hardware/zynq/litex-build/Image_nax64
   ```
   Boot still passes `irqpoll` in the DTB bootargs as a belt-and-suspenders for
   the early enumeration window.

## Result (board-verified 2026-06-19)
Cursor tracks at the mouse's full 125 Hz (median 8 ms event gaps) and stays alive
indefinitely — the ~1 s delivery stalls are gone.  Enumeration also improved
(first-attempt clean boot).  Pairs with the atomik_os delta-cursor (save-under)
so a mouse move touches ~360 px, not 2,073,600.
