#!/usr/bin/env python3
"""
ATOMiK LiteX SoC — VexRiscv Linux+CFU on HamGeek RK-ZYNQ7020-F

Control experiment: single-core VexRiscv with Linux capability (MMU, S-mode)
and CFU bus. No ATOMiK CFU adapter wired yet — first prove Linux boots on
this CPU variant.

Usage:
    python3 soc_linux_cfu.py --build
    python3 soc_linux_cfu.py --build --cfu-verilog /path/to/atomik_cfu_adapter.v
"""

import os
import argparse

from migen import *
from litex.gen import *
from litex_boards.platforms import hamgeek_rk7020f
from litex.soc.cores.clock import *
from litex.soc.integration.soc_core import *
from litex.soc.integration.soc import SoCRegion
from litex.soc.integration.builder import *
from litex.soc.cores.cpu import zynq7000
from litex.soc.interconnect import wishbone

from migen.genlib.misc import WaitTimer

# CRG ------------------------------------------------------------------------------------

class _CRG(LiteXModule):
    def __init__(self, platform, sys_clk_freq):
        self.rst    = Signal()
        self.cd_sys = ClockDomain()
        self.comb += ClockSignal("sys").eq(ClockSignal("ps7"))
        self.comb += ResetSignal("sys").eq(ResetSignal("ps7") | self.rst)

# CLINT + PLIC wrappers ------------------------------------------------------------------

class CLINTWrapper(LiteXModule):
    """Wraps litex_clint.v as a Wishbone slave."""
    def __init__(self, platform, clint_v):
        self.bus = bus = wishbone.Interface(data_width=32, address_width=32, addressing="word")
        self.timer_irq = Signal()
        self.soft_irq  = Signal()

        platform.add_source(clint_v)
        self.specials += Instance("litex_clint",
            i_clk               = ClockSignal("sys"),
            i_rst               = ResetSignal("sys"),
            i_adr               = bus.adr[:14],
            i_dat_w             = bus.dat_w,
            o_dat_r             = bus.dat_r,
            i_we                = bus.we,
            i_cyc               = bus.cyc,
            i_stb               = bus.stb,
            o_ack               = bus.ack,
            o_timer_interrupt   = self.timer_irq,
            o_software_interrupt = self.soft_irq,
        )

class PLICWrapper(LiteXModule):
    """Wraps litex_plic.v as a Wishbone slave."""
    def __init__(self, platform, plic_v, num_sources=32):
        self.bus = bus = wishbone.Interface(data_width=32, address_width=32, addressing="word")
        self.irq_sources = Signal(num_sources)
        self.meip = Signal()
        self.seip = Signal()

        platform.add_source(plic_v)
        self.specials += Instance("litex_plic",
            p_NUM_SOURCES        = num_sources,
            i_clk                = ClockSignal("sys"),
            i_rst                = ResetSignal("sys"),
            i_adr                = bus.adr[:22],
            i_dat_w              = bus.dat_w,
            o_dat_r              = bus.dat_r,
            i_we                 = bus.we,
            i_cyc                = bus.cyc,
            i_stb                = bus.stb,
            o_ack                = bus.ack,
            i_irq_sources        = self.irq_sources,
            o_meip               = self.meip,
            o_seip               = self.seip,
        )

# BaseSoC ---------------------------------------------------------------------------------

class BaseSoC(SoCCore):
    def __init__(self, sys_clk_freq=100e6, cfu_verilog=None, **kwargs):
        platform = hamgeek_rk7020f.Platform()
        self.crg = _CRG(platform, sys_clk_freq)

        # Resolve CFU Verilog: use passthrough if none provided
        zynq_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        if cfu_verilog is None:
            cfu_verilog = os.path.join(zynq_dir, "rtl", "cfu_passthrough.v")

        # Use single-core VexRiscv with Linux+CFU variant
        SoCCore.__init__(self, platform, sys_clk_freq,
            cpu_type           = "vexriscv",
            cpu_variant        = "linux+cfu",
            cpu_cfu            = cfu_verilog,
            integrated_rom_size = 0x10000,   # 64KB BIOS ROM at reset vector
            ident       = "ATOMiK LiteX SoC (VexRiscv Linux+CFU) on HamGeek RK-ZYNQ7020-F",
            **kwargs,
        )

        # PS7 DDR3 via GP slave (same as the SMP SoC)
        self.ps = ps = zynq7000.Zynq7000(platform=platform, variant="standard")
        ps.set_ps7(name="Zynq", config=platform.ps7_config)
        axi_gp0 = ps.add_axi_gp_slave(clock_domain="sys")

        axi_gp0_region  = SoCRegion(origin=0x100000, size=0x2000_0000)
        main_ram_region = SoCRegion(
            origin=self.cpu.mem_map["main_ram"],
            size=axi_gp0_region.size,
            mode="rwx",
        )

        wb = self.bus.add_adapter("axi_gp0", axi_gp0, direction="s2m")
        remap_module = wishbone.Interface(data_width=32, address_width=32)
        self.submodules += wishbone.Remapper(
            master=remap_module, slave=wb,
            src_regions=[main_ram_region], dst_regions=[axi_gp0_region],
        )
        self.bus.add_slave("main_ram", remap_module, main_ram_region)

        # Wire CFU adapter if provided
        if cfu_verilog:
            self.cpu.add_cfu(cfu_verilog)

        # CLINT (timer + software interrupts) -----------------------------------------
        clint_v = os.path.join(zynq_dir, "rtl", "litex_clint.v")
        self.clint = CLINTWrapper(platform, clint_v)
        self.bus.add_slave("clint", self.clint.bus,
            region=SoCRegion(origin=0xf0010000, size=0x10000, cached=False))

        # PLIC (external interrupts) --------------------------------------------------
        plic_v = os.path.join(zynq_dir, "rtl", "litex_plic.v")
        self.plic = PLICWrapper(platform, plic_v, num_sources=32)
        self.bus.add_slave("plic", self.plic.bus,
            region=SoCRegion(origin=0xf0c00000, size=0x400000, cached=False))

        # Wire UART interrupt to PLIC source 1
        self.comb += self.plic.irq_sources[1].eq(self.uart.ev.rx.trigger)

        # Wire CLINT outputs to CPU timer/software interrupt inputs
        # (overrides the defaults set by LiteX VexRiscv core.py)
        self.cpu.cpu_params["i_timerInterrupt"]    = self.clint.timer_irq
        self.cpu.cpu_params["i_softwareInterrupt"] = self.clint.soft_irq

        # Wire PLIC output to CPU external interrupt array
        # VexRiscv linux+cfu uses externalInterruptArray[31:0], not separate M/S lines
        self.cpu.interrupt.eq(0)  # clear LiteX default
        self.comb += self.cpu.interrupt[0].eq(self.plic.meip)

def main():
    parser = argparse.ArgumentParser(description="ATOMiK LiteX SoC (VexRiscv Linux+CFU)")
    parser.add_argument("--build",        action="store_true", help="Build bitstream")
    parser.add_argument("--sys-clk-freq", default=100e6, type=float)
    parser.add_argument("--cfu-verilog",  default=None,  type=str,
                        help="Path to CFU adapter Verilog (omit for control build)")

    # LiteX builder args
    builder_group = parser.add_argument_group("builder")
    builder_group.add_argument("--output-dir", default=None)

    args = parser.parse_args()

    soc = BaseSoC(
        sys_clk_freq = args.sys_clk_freq,
        cfu_verilog  = args.cfu_verilog,
    )

    builder_kwargs = {}
    if args.output_dir:
        builder_kwargs["output_dir"] = args.output_dir

    builder = Builder(soc, **builder_kwargs)
    if args.build:
        builder.build()

if __name__ == "__main__":
    main()
