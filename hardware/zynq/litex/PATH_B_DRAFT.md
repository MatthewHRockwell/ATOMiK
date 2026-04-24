# Path B draft — AXI HP dedicated framebuffer master

Sketch; apply after `--with-video-phy-only` test confirms arbiter theory.

## Architectural difference

```
Path A (v5, broken):
    FB → wishbone master → [SoC bus arbiter] → wishbone remapper → AXI GP0 slave → PS DDR
                            ^^^^^^^^^^^^^^^
                            also carries CSR, kernel traffic

Path B (new):
    FB → wishbone master → Wishbone2AXI → AXI HP0 slave → PS DDR  (private path)
    NaxRiscv → [SoC bus, single master] → wishbone remapper → AXI GP0 slave → PS DDR
```

No `bus.add_master()` call. Path B uses a *private* wishbone between the FB
and the AXI bridge — never touches the SoC interconnect.

## Soc integration diff

```python
# Enable HP0 at 32-bit (matches WB width; avoids downsize complexity)
axi_hp0 = ps.add_axi_hp_slave(clock_domain="sys", data_width=32)

# Private wishbone — NOT added to self.bus
fb_wb = wishbone.Interface(data_width=32, address_width=32)

# FB base is now PS DDR physical address (no remapper in this path)
# 0x08100000 = same physical bytes Linux sees at NaxRiscv 0x48000000 via the
# main_ram wishbone remapper (NaxRiscv 0x4xxxxxxx − 0x3FF00000 = PS addr)
FRAMEBUFFER_BASE_PS = 0x08100000

self.video_framebuffer = WishboneVideoFrameBuffer(
    bus            = fb_wb,           # private, not on SoC bus
    hres           = 640, vres = 480,
    base           = FRAMEBUFFER_BASE_PS,
    fifo_depth     = 256,
    clock_domain   = "hdmi",
    default_enable = 0,
)

# Bridge the private WB to the HP AXI3 slave. base_address=0 because our
# WB reader already issues PS-DDR-absolute addresses.
from litex.soc.interconnect import axi
self.fb_wb2axi = axi.Wishbone2AXI(wishbone=fb_wb, axi=axi_hp0, base_address=0)
```

## Userspace implication

`fb_test.c` keeps `FB_BASE = 0x48000000` (NaxRiscv physical address via
/dev/mem) — unchanged. Both paths (Linux writes via GP0 remapper, FB reads
via HP directly) land on the **same bytes in PS DDR**.

## Risks / uncertainties

1. **AXI HP 32-bit mode support**. UG585 says HP ports are 32/64-bit
   configurable. LiteX's `add_axi_hp_slave(data_width=32)` sets the PS7 IP
   config but we should confirm it works on Zynq-7000. Fallback: use 64-bit
   HP + put an AXI downsizer/upconverter between WB2AXI and HP.
2. **Address offset**. If Wishbone2AXI's base_address arg offsets WRITE-side
   addresses, we want 0. If the DMA reader inside WishboneVideoFrameBuffer
   emits absolute bus addresses (which it does via `self._base.storage`),
   base_address=0 is correct.
3. **Clock domain**. HP clocked by sys (100 MHz). Wishbone2AXI is sys
   domain. FB's WB side is sys, pixel stream is hdmi. The same CDC already
   in WishboneVideoFrameBuffer handles sys→hdmi — no change.

## Build invocation

New flag `--with-video-framebuffer-hp`, analogous to existing
`--with-video-framebuffer` but using Path B.

Output dir: `hardware/zynq/litex-build-nax64-fb-hp/`
