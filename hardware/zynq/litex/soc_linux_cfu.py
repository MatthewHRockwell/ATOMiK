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

# CRG ------------------------------------------------------------------------------------

class _CRG(LiteXModule):
    def __init__(self, platform, sys_clk_freq):
        self.rst    = Signal()
        self.cd_sys = ClockDomain()
        self.comb += ClockSignal("sys").eq(ClockSignal("ps7"))
        self.comb += ResetSignal("sys").eq(ResetSignal("ps7") | self.rst)

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
            # Tie off rsp_payload_status (not in LiteX's add_cfu bus layout)
            # Vivado defaults unconnected inputs to 0 (= OK), so this is safe.
            # For explicit tie-off, uncomment:
            # self.cpu.cpu_params["i_CfuPlugin_bus_rsp_payload_status"] = 0

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
