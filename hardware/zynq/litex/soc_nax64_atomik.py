#!/usr/bin/env python3
"""
ATOMiK NaxRiscv RV64 SoC on HamGeek RK-ZYNQ7020-F

64-bit RISC-V soft-core with ATOMiK delta-state adapter.
Uses NaxRiscv (SpinalHDL) with Sv39 MMU for full Linux support.

Usage:
    python3 soc_nax64_atomik.py --build --xlen=64
"""

import os
import sys

from migen import *
from litex.gen import *
from litex_boards.platforms import hamgeek_rk7020f
from litex.soc.cores.clock import *
from litex.soc.integration.soc_core import *
from litex.soc.integration.builder import *
from litex.soc.integration.soc import SoCRegion
from litex.soc.cores.cpu import zynq7000
from litex.soc.interconnect import wishbone
from litex.soc.cores.cpu.naxriscv.core import NaxRiscv
from litex.soc.cores.video import VideoS7HDMIPHY
from litex.soc.cores.clock import S7MMCM

# Configure NaxRiscv for RV64 before SoC instantiation
# NOTE: with_fpu and with_rvc MUST be passed via CLI (--with-fpu --with-rvc)
# because LiteXArgumentParser.parse_args() overrides class-level settings.
NaxRiscv.xlen                 = 64
NaxRiscv.data_width           = 64
NaxRiscv.gcc_triple           = ("riscv64-unknown-elf", "riscv64-linux-gnu", "riscv64-unknown-linux-gnu")
NaxRiscv.linker_output_format = "elf64-littleriscv"
# Reduce L2 cache to save LUTs on the XC7Z020
NaxRiscv.l2_bytes             = 32 * 1024  # 32KB (default 128KB too large)
NaxRiscv.l2_ways              = 4

# CRG ------------------------------------------------------------------------------------

class _CRG(LiteXModule):
    def __init__(self, platform, sys_clk_freq):
        self.rst    = Signal()
        self.cd_sys = ClockDomain()
        self.comb += ClockSignal("sys").eq(ClockSignal("ps7"))
        self.comb += ResetSignal("sys").eq(ResetSignal("ps7") | self.rst)

# BaseSoC ---------------------------------------------------------------------------------

class BaseSoC(SoCCore):
    def __init__(self, sys_clk_freq=100e6, with_atomik_adapter=True,
                 with_video_framebuffer=False, **kwargs):
        platform = hamgeek_rk7020f.Platform()
        self.crg = _CRG(platform, sys_clk_freq)

        # cpu_type comes from kwargs (set via --cpu-type or parser default)
        kwargs.setdefault("cpu_type", "naxriscv")
        SoCCore.__init__(self, platform, sys_clk_freq,
            ident       = "ATOMiK NaxRiscv RV64 SoC on HamGeek RK-ZYNQ7020-F",
            **kwargs,
        )

        # PS7 DDR3 via GP slave
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

        # ATOMiK CFU Adapter (Wishbone-mapped)
        if with_atomik_adapter:
            zynq_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
            adapter_v = os.path.join(zynq_dir, "rtl", "atomik_cfu_adapter.v")
            wrapper_v = os.path.join(zynq_dir, "rtl", "atomik_cfu_wishbone.v")
            platform.add_source(adapter_v)
            platform.add_source(wrapper_v)

            adapter_bus = wishbone.Interface(data_width=32, address_width=32, addressing="word")
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

            self.bus.add_slave("atomik_adapter", adapter_bus,
                region=SoCRegion(origin=0xf0020000, size=0x20, cached=False))

        # HDMI Video Framebuffer
        if with_video_framebuffer:
            # Video clock domains from MMCM (driven by PS7 100 MHz FCLK)
            self.cd_hdmi   = ClockDomain()
            self.cd_hdmi5x = ClockDomain()

            self.video_pll = video_pll = S7MMCM(speedgrade=-2)
            self.comb += video_pll.reset.eq(ResetSignal("sys"))
            video_pll.register_clkin(ClockSignal("sys"), sys_clk_freq)
            # 640x480@60Hz: pixel clock = 25.175 MHz, 5x ≈ 125.875 MHz
            video_pll.create_clkout(self.cd_hdmi,   25.175e6)
            video_pll.create_clkout(self.cd_hdmi5x, 5*25.175e6)

            # HDMI PHY
            self.videophy = VideoS7HDMIPHY(platform.request("hdmi_out"),
                                           clock_domain="hdmi")

            # Video terminal (text console over HDMI — no DMA needed)
            self.add_video_terminal(phy=self.videophy,
                                    timings="640x480@60Hz",
                                    clock_domain="hdmi")

            # False path constraints: sys ↔ hdmi clock domains are independent
            platform.add_false_path_constraints(self.crg.cd_sys.clk, self.cd_hdmi.clk)

def main():
    from litex.build.parser import LiteXArgumentParser

    parser = LiteXArgumentParser(platform=hamgeek_rk7020f.Platform,
        description="ATOMiK NaxRiscv RV64 SoC on HamGeek RK-ZYNQ7020-F")
    parser.add_target_argument("--sys-clk-freq", default=100e6, type=float)
    parser.add_target_argument("--with-video-framebuffer", action="store_true",
                               help="Enable HDMI framebuffer output")
    args = parser.parse_args()

    soc = BaseSoC(
        sys_clk_freq=args.sys_clk_freq,
        with_video_framebuffer=args.with_video_framebuffer,
        **parser.soc_argdict,
    )
    builder = Builder(soc, **parser.builder_argdict)
    if args.build:
        builder.build(**parser.toolchain_argdict)

if __name__ == "__main__":
    main()
