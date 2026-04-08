#!/usr/bin/env python3
"""
ATOMiK SMP SoC + CFU Adapter Test

Extends the proven SMP SoC (which boots Linux with CLINT+PLIC) by adding
the atomik_cfu_adapter.v as a Wishbone-mapped peripheral. This validates
the adapter logic on the known-good platform.

Usage:
    python3 soc_smp_adapter.py --build
"""

import os
import sys

# Add the litex-boards target to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "..", ".."))

from migen import *
from litex.gen import *
from litex_boards.targets.hamgeek_rk7020f import BaseSoC as OrigBaseSoC
from litex.soc.integration.builder import *
from litex.soc.integration.soc import SoCRegion
from litex.soc.interconnect import wishbone

class BaseSoC(OrigBaseSoC):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

        platform = self.platform
        zynq_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

        # Add the CFU adapter Wishbone wrapper
        adapter_v = os.path.join(zynq_dir, "rtl", "atomik_cfu_adapter.v")
        wrapper_v = os.path.join(zynq_dir, "rtl", "atomik_cfu_wishbone.v")
        platform.add_source(adapter_v)
        platform.add_source(wrapper_v)

        # Create Wishbone bus for the adapter
        adapter_bus = wishbone.Interface(data_width=32, address_width=32, addressing="word")

        # Instantiate the Wishbone-wrapped adapter
        self.specials += Instance("atomik_cfu_wishbone",
            i_clk   = ClockSignal("sys"),
            i_rst   = ResetSignal("sys"),
            i_adr   = adapter_bus.adr[:3],
            i_dat_w = adapter_bus.dat_w,
            o_dat_r = adapter_bus.dat_r,
            i_we    = adapter_bus.we,
            i_cyc   = adapter_bus.cyc,
            i_stb   = adapter_bus.stb,
            o_ack   = adapter_bus.ack,
        )

        # Map at 0xf0020000 (past CSR + CLINT regions, in IO space)
        self.bus.add_slave("atomik_adapter", adapter_bus,
            region=SoCRegion(origin=0xf0020000, size=0x20, cached=False))

def main():
    from litex.build.parser import LiteXArgumentParser
    from litex_boards.platforms import hamgeek_rk7020f

    parser = LiteXArgumentParser(platform=hamgeek_rk7020f.Platform,
        description="ATOMiK SMP SoC + CFU Adapter Test")
    parser.add_target_argument("--sys-clk-freq", default=100e6, type=float)
    args = parser.parse_args()

    soc = BaseSoC(sys_clk_freq=args.sys_clk_freq, **parser.soc_argdict)
    builder = Builder(soc, **parser.builder_argdict)
    if args.build:
        builder.build(**parser.toolchain_argdict)

if __name__ == "__main__":
    main()
