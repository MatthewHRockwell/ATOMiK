"""PSUart0Bridge — LiteX UART CSR proxied to Zynq PS UART0.

This module presents the standard LiteX UART CSR layout
(``rxtx``/``txfull``/``rxempty``/``ev_*``) so that any LiteX-aware
software writing to ``CSR_UART_BASE`` (the BIOS, Linux's ``liteuart``
driver, OpenSBI's litex_console) actually drives **Zynq PS UART0**
on MIO10/11 → FT232 → ``/dev/ttyUSB1`` of the development host,
instead of the LiteX UART's serial pin (V12 on this board, unwired).

Architecture
------------
- The module is a CSR slave at the UART CSR slot, exposing the same
  register interface that the upstream :mod:`litex.soc.cores.uart`
  ``UART``/``UARTPHY`` pair provides.
- It also exposes a wishbone *master* (``self.bus``) which the SoC
  integrator wires into the main bus interconnect with
  ``self.bus.add_master``.  An internal FSM polls PS UART0 status
  and shuttles bytes to/from its TX/RX FIFO over that wishbone master.
- All wishbone accesses target NaxRiscv addresses
  ``0x8000_0000 + offset``, which the SoC's existing ``ps_iop``
  remapper translates to ARM ``0xE000_0000 + offset`` via GP1 slave.
  No new PS-side AXI port is required.

The FSBL initializes UART0 (controller registers + MIO10/11 pinmux +
UART_CLK_CTRL DIV=18) before programming the PL bitstream, so by the
time NaxRiscv runs LiteX BIOS, the UART is alive at 115200 8N1 and
the bridge can write to its TX FIFO immediately.
"""

from migen import (
    Signal, If, Cat, FSM, NextValue, NextState, Constant, ClockSignal,
)
from litex.gen import LiteXModule
from litex.soc.interconnect.csr import CSR, CSRStatus, AutoCSR
from litex.soc.interconnect.csr_eventmanager import EventManager, EventSourceLevel
from litex.soc.interconnect import wishbone


# Zynq PS UART register layout (NaxRiscv view via ps_iop window).
# ps_iop maps NaxRiscv 0x8000_0000 -> ARM 0xE000_0000, so:
#   UART0 controller @ ARM 0xE0000000 == NaxRiscv 0x80000000
# Register offsets (UG585 Table B-50 / Xilinx XSDK xuartps_hw.h):
_PS_UART0_BASE     = 0x8000_0000
_PS_UART_OFF_SR    = 0x2C       # Channel Status Register
_PS_UART_OFF_FIFO  = 0x30       # TX / RX FIFO

# PS UART Channel Status Register bits:
_SR_RX_EMPTY = (1 << 1)         # set when RX FIFO is empty
_SR_RX_FULL  = (1 << 2)         # set when RX FIFO is full
_SR_TX_EMPTY = (1 << 3)         # set when TX FIFO is empty
_SR_TX_FULL  = (1 << 4)         # set when TX FIFO is full


class PSUart0Bridge(LiteXModule, AutoCSR):
    """CSR-compatible LiteX UART that proxies to PS UART0 via wishbone.

    Parameters
    ----------
    poll_period : int
        Number of ``sys_clk`` cycles between idle PS-UART SR polls.
        Higher values reduce SoC-bus traffic at the cost of latency
        observing RX bytes.  At 100 MHz with poll_period=64 the polling
        interval is 0.64 µs which is far faster than a single 115200-baud
        character time (~87 µs), so software never sees stale state.
    """

    def __init__(self, poll_period: int = 10_000):
        # ------------------------------------------------------------------
        # Software-visible CSRs — match upstream LiteUART exactly so the
        # existing BIOS, Linux liteuart driver, and OpenSBI litex_console
        # work without modification.
        # ------------------------------------------------------------------
        self._rxtx     = CSR(8)          # WRITE: TX byte. READ: RX byte.
        self._txfull   = CSRStatus()
        self._rxempty  = CSRStatus()
        self._txempty  = CSRStatus()
        self._rxfull   = CSRStatus()

        # Event manager (LiteUART exposes these for IRQ-driven I/O; we keep
        # the interface for ABI compatibility but events are level-driven
        # off our cached SR / rx_valid state).
        self.submodules.ev = EventManager()
        self.ev.tx = EventSourceLevel()
        self.ev.rx = EventSourceLevel()
        self.ev.finalize()

        # ------------------------------------------------------------------
        # Wishbone master into the SoC bus.  The SoC integrator must
        # register this with ``self.bus.add_master("psuart_bridge", master=
        # bridge.bus)`` so the interconnect routes accesses targeting the
        # ps_iop region (0x8000_0000+) to GP1.
        # ------------------------------------------------------------------
        self.bus = wishbone.Interface(data_width=32, address_width=32)

        # ------------------------------------------------------------------
        # Internal state.
        # ------------------------------------------------------------------
        sr_cache    = Signal(32)    # last value of PS UART0 SR
        sr_valid    = Signal()      # set once we've read SR at least once
        rx_byte     = Signal(8)     # cached RX byte
        rx_valid    = Signal()      # 1 = rx_byte holds an unconsumed byte
        tx_byte     = Signal(8)     # byte to TX, when tx_pending=1
        tx_pending  = Signal()      # 1 = software wrote rxtx; waiting to push
        poll_ctr    = Signal(max=max(2, poll_period))

        # CSR write to rxtx: latch TX byte
        self.sync += If(self._rxtx.re,
            tx_byte.eq(self._rxtx.r),
            tx_pending.eq(1),
        )

        # CSR read of rxtx: return cached RX byte, mark it consumed
        self.comb += self._rxtx.w.eq(rx_byte)
        self.sync += If(self._rxtx.we, rx_valid.eq(0))

        # Status flags — software sees them combinationally
        self.comb += [
            # txfull mirrors PS_UART.SR.TX_FULL OR our local pending byte
            self._txfull.status.eq(sr_cache[4] | tx_pending),
            # rxempty: software sees empty until we cache a byte
            self._rxempty.status.eq(~rx_valid),
            self._txempty.status.eq(sr_cache[3] & ~tx_pending),
            self._rxfull.status.eq(sr_cache[2]),
            # Event sources track the LiteUART semantics: tx event when
            # there's room to write more, rx event when a byte is waiting.
            self.ev.tx.trigger.eq(~self._txfull.status),
            self.ev.rx.trigger.eq(rx_valid),
        ]

        # ------------------------------------------------------------------
        # Bridge FSM.  States:
        #   IDLE     — countdown to next SR poll
        #   POLL_SR  — issue wishbone read at NaxRiscv 0x8000_002C
        #   TX_WRITE — issue wishbone write at NaxRiscv 0x8000_0030
        #   RX_READ  — issue wishbone read at NaxRiscv 0x8000_0030
        # ------------------------------------------------------------------
        fsm = FSM(reset_state="POLL_SR")
        self.submodules += fsm

        sr_addr   = (_PS_UART0_BASE + _PS_UART_OFF_SR)   >> 2
        fifo_addr = (_PS_UART0_BASE + _PS_UART_OFF_FIFO) >> 2

        # Software triggers: a CSR read on _rxtx consumes the cached byte and
        # signals "go check if there's another byte waiting"; a CSR write on
        # _rxtx queues TX. Both should wake the FSM out of IDLE immediately.
        # Otherwise IDLE counts down a slow heartbeat (~100 µs at 100 MHz)
        # so that unsolicited RX bytes are detected eventually without
        # contending with NaxRiscv on every cycle.
        wake = Signal()
        self.comb += wake.eq(tx_pending | self._rxtx.we | self._rxtx.re)

        fsm.act("IDLE",
            If(wake,
                NextValue(poll_ctr, poll_period - 1),
                NextState("POLL_SR"),
            ).Elif(poll_ctr == 0,
                NextValue(poll_ctr, poll_period - 1),
                NextState("POLL_SR"),
            ).Else(
                NextValue(poll_ctr, poll_ctr - 1),
            ),
        )

        fsm.act("POLL_SR",
            self.bus.adr.eq(sr_addr),
            self.bus.sel.eq(0xF),
            self.bus.cyc.eq(1),
            self.bus.stb.eq(1),
            self.bus.we.eq(0),
            If(self.bus.ack,
                NextValue(sr_cache, self.bus.dat_r),
                NextValue(sr_valid, 1),
                # Decide next action.  Priority: TX first (write through),
                # then RX read if a byte is available and we have no cached
                # one; otherwise idle for poll_period cycles.
                If(tx_pending & ~self.bus.dat_r[4],   # TX pending + not full
                    NextState("TX_WRITE"),
                ).Elif(~rx_valid & ~self.bus.dat_r[1],   # RX has data, cache empty
                    NextState("RX_READ"),
                ).Else(
                    NextValue(poll_ctr, poll_period - 1),
                    NextState("IDLE"),
                ),
            ),
        )

        fsm.act("TX_WRITE",
            self.bus.adr.eq(fifo_addr),
            self.bus.sel.eq(0xF),
            self.bus.dat_w.eq(Cat(tx_byte, Constant(0, 24))),
            self.bus.cyc.eq(1),
            self.bus.stb.eq(1),
            self.bus.we.eq(1),
            If(self.bus.ack,
                NextValue(tx_pending, 0),
                # Re-poll SR right away so txfull reflects the updated FIFO
                NextState("POLL_SR"),
            ),
        )

        fsm.act("RX_READ",
            self.bus.adr.eq(fifo_addr),
            self.bus.sel.eq(0xF),
            self.bus.cyc.eq(1),
            self.bus.stb.eq(1),
            self.bus.we.eq(0),
            If(self.bus.ack,
                NextValue(rx_byte, self.bus.dat_r[:8]),
                NextValue(rx_valid, 1),
                NextState("POLL_SR"),
            ),
        )
