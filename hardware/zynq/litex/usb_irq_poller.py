"""UsbIrqPoller — derive the USB0 interrupt in the PL by polling the controller.

Why
---
On this SoC the PS chipidea USB0 interrupt is meant to reach NaxRiscv's PLIC via
the PS->PL fabric tap ``IRQ_P2F_USB0``.  That tap proved unreliable: the
controller asserts its interrupt (USBSTS & USBINTR nonzero) yet ``IRQ_P2F_USB0``
does not track it out to the PL, so the PLIC source never pends and steady-state
HID input dies (mouse motion never reaches the kernel; ``event*`` reads 0 bytes).
2026-06-19: confirmed by reading registers live — USBSTS=0xc408b with USBINTR
enabling the bits, but PLIC source 3 pending stayed clear.

Crucially the *register access* path (GP1: NaxRiscv 0x8000_0000 -> ARM
0xE000_0000 via the ps_iop remapper) IS reliable — Linux reads/writes the USB
controller registers over it without trouble.  Only the dedicated interrupt tap
is broken.  So we recover a deterministic interrupt by polling the controller's
own status over that working path and replicating its assert condition exactly:

    irq = (USBSTS & USBINTR) != 0

USBSTS is write-1-to-clear, so reads have no side effect — polling is safe and
the kernel still owns the clear via its own W1C writes.  The level we drive is
the standard RISC-V PLIC level model: high while the controller has a pending,
enabled event; the next poll after the handler clears USBSTS drops it.

Integration
-----------
The SoC integrator registers ``self.bus`` as a wishbone master
(``soc.bus.add_master(...)``) so the interconnect routes its accesses to the
ps_iop region, and wires ``self.irq`` to the PLIC line in place of
``ps.get_irq_p2f_usb0()``.  Same pattern as :class:`PSUart0Bridge`; no new PS
AXI port is needed (both share GP1).
"""

from migen import Signal, If, FSM, NextValue, NextState
from litex.gen import LiteXModule
from litex.soc.interconnect import wishbone

# USB0 controller registers, NaxRiscv view via the ps_iop window
# (NaxRiscv 0x8000_0000 == ARM 0xE000_0000, USB0 @ ARM 0xE000_2000):
_USB0_BASE    = 0x8000_2000
_USB_OFF_STS  = 0x144      # USBSTS  (write-1-to-clear status)
_USB_OFF_INTR = 0x148      # USBINTR (interrupt enable mask)


class UsbIrqPoller(LiteXModule):
    """Poll USBSTS/USBINTR over GP1 and drive a level interrupt to the PLIC.

    Parameters
    ----------
    poll_period : int
        ``sys_clk`` cycles to idle between poll cycles.  At 100 MHz a value of
        64 gives a ~1 µs poll cadence (two GP1 reads + idle) — far faster than
        the USB HID interrupt interval (~8 ms), so input latency is negligible,
        and short enough that the level drops quickly after the kernel clears
        USBSTS (only ~1 spurious IRQ per real one, well under the genirq
        "nobody cared" threshold).  Modest GP1 traffic (~2 reads / 1 µs).
    """

    def __init__(self, idle_period: int = 1024,
                 startup_delay_cycles: int = 4_500_000_000):
        self.bus = wishbone.Interface(data_width=32, address_width=32)
        self.irq = Signal()       # level: high while controller has a pending event

        sts   = Signal(32)
        intr  = Signal(32)
        ctr   = Signal(max=max(2, idle_period))
        pend  = Signal()          # latched: last poll saw a pending, enabled interrupt

        sts_addr  = (_USB0_BASE + _USB_OFF_STS)  >> 2
        intr_addr = (_USB0_BASE + _USB_OFF_INTR) >> 2

        # Startup gate.  The poller's GP1 reads disrupt the kernel's
        # timing-sensitive enumeration control transfers (observed: with the
        # poller polling from boot, the device never enumerates — 10/10 boots —
        # while irqpoll-only boots enumerate in 1-2 tries).  So stay COMPLETELY
        # off the bus (no cyc/stb, irq held low) for ~45 s after reset — past the
        # full enumeration window incl. the second-HID-interface probe (~32 s on
        # slow boots).  irqpoll carries IRQ delivery during enumeration; once the
        # device is up the poller takes over for reliable steady-state HID.
        # 45 s @100 MHz = 4.5e9 cycles (needs 33 bits).
        started   = Signal()
        delay_ctr = Signal(33)
        self.sync += If(~started,
            If(delay_ctr >= (startup_delay_cycles - 1),
                started.eq(1),
            ).Else(
                delay_ctr.eq(delay_ctr + 1),
            ),
        )

        fsm = FSM(reset_state="WAIT_START")
        self.submodules += fsm

        # Hold off all bus activity until the enumeration window has passed.
        fsm.act("WAIT_START",
            If(started, NextState("RD_STS")),
        )

        fsm.act("RD_STS",
            self.bus.adr.eq(sts_addr),
            self.bus.sel.eq(0xF),
            self.bus.cyc.eq(1),
            self.bus.stb.eq(1),
            self.bus.we.eq(0),
            If(self.bus.ack,
                NextValue(sts, self.bus.dat_r),
                NextState("RD_INTR"),
            ),
        )
        fsm.act("RD_INTR",
            self.bus.adr.eq(intr_addr),
            self.bus.sel.eq(0xF),
            self.bus.cyc.eq(1),
            self.bus.stb.eq(1),
            self.bus.we.eq(0),
            If(self.bus.ack,
                NextValue(intr, self.bus.dat_r),
                # Adaptive cadence: if a real interrupt is pending, re-poll
                # immediately (catch the kernel's W1C clear fast -> few spurious
                # IRQs); otherwise idle idle_period cycles (~10 us) so the GP1
                # path is mostly free for the kernel — critical during the
                # timing-sensitive control transfers of enumeration.
                # Guard against garbage reads (all-ones) seen if a register is
                # sampled mid controller-reset: never assert on 0xFFFFFFFF.
                If((sts != 0xFFFFFFFF) & (self.bus.dat_r != 0xFFFFFFFF) &
                   ((sts & self.bus.dat_r) != 0),
                    NextValue(pend, 1),
                    NextState("RD_STS"),
                ).Else(
                    NextValue(pend, 0),
                    NextValue(ctr, idle_period - 1),
                    NextState("IDLE"),
                ),
            ),
        )
        fsm.act("IDLE",
            If(ctr == 0,
                NextState("RD_STS"),
            ).Else(
                NextValue(ctr, ctr - 1),
            ),
        )

        # Drive the level from the latched pending state (updated each poll),
        # gated by the startup delay so it never fires during enumeration.
        self.comb += self.irq.eq(started & pend)
