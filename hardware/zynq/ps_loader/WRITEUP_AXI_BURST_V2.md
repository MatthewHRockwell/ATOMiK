# AXI burst debug — need help getting ARREADY to assert on Zynq HP0

## Context

Single-beat pipelined AXI reads work but are bandwidth-limited (~3 cycles/word, barely keeps up at 800x600). Need 16-beat bursts (1 AR → 16 R beats) to get proper margin. v11 attempted this and got black screen — ARREADY never asserted.

## What works (proven on same HP0 port)

Single-beat reads via Wishbone2AXI → AXILite2AXI → HP0. This path drives AR combinationally:
```python
# From AXILite2AXI (litex source):
self.comb += [
    axi.ar.valid.eq(axi_lite.ar.valid),
    axi.ar.addr.eq(axi_lite.ar.addr),
    axi.ar.burst.eq(0b01),     # INCR
    axi.ar.len.eq(0),          # 1 beat
    axi.ar.size.eq(2),         # 4 bytes
    axi.ar.lock.eq(0),
    axi.ar.prot.eq(0),
    axi.ar.cache.eq(0b0011),
    axi.ar.qos.eq(0),
    axi.ar.id.eq(0),
]
```

Also, my v15 pipelined reader works — drives AR from FSM combinational assignments:
```python
fsm.act("PIPE",
    If(ar_idx < hres,
        axi.ar.valid.eq(1),
        axi.ar.addr.eq(line_addr + (ar_idx << 2)),
        axi.ar.burst.eq(0),      # FIXED
        axi.ar.len.eq(0),        # 1 beat
        axi.ar.size.eq(2),       # 4 bytes
        ...
    ),
    axi.r.ready.eq(1),
    If(axi.r.valid, ...),
)
```

Both of these work. HP0 accepts the AR and returns R data.

## What doesn't work

Same FSM but with `axi.ar.len.eq(15)` (16-beat burst) and `axi.ar.burst.eq(1)` (INCR). Screen goes black — fetch FSM appears stuck in AR state (ARREADY never asserts).

## My v11 burst code (failed)

```python
fsm.act("AR",
    axi.ar.valid.eq(1),
    axi.ar.addr.eq(line_addr + (word_idx << 2)),
    axi.ar.burst.eq(1),          # INCR
    axi.ar.len.eq(burst_len - 1), # 15 = 16 beats
    axi.ar.size.eq(2),            # 4 bytes
    axi.ar.id.eq(0),
    axi.ar.lock.eq(0),
    axi.ar.prot.eq(0),
    axi.ar.cache.eq(0b0011),
    axi.ar.qos.eq(0),
    If(axi.ar.ready,
        NextValue(burst_beat, 0),
        NextState("R"),
    ),
)
fsm.act("R",
    axi.r.ready.eq(1),
    If(axi.r.valid, ...capture data...),
)
```

Write channels tied off: `axi.aw.valid.eq(0), axi.w.valid.eq(0), axi.b.ready.eq(1)`

## HP0 port config

```python
axi_hp0 = ps.add_axi_hp_slave(clock_domain="sys", data_width=32)
```

This sets: `PCW_USE_S_AXI_HP0=1, PCW_S_AXI_HP0_DATA_WIDTH=32, PCW_S_AXI_HP0_ID_WIDTH=6`

## AXI interface from LiteX

AXI3, 32-bit data, 32-bit address, 6-bit ID.

## Generated Verilog for AR (v11, from memory)

AR signals were declared as `reg` in the Verilog and driven with `<=` inside an `always @(posedge)` block, even though the Migen code uses combinational `.eq()` in `fsm.act()`.

## Questions

1. Does the Zynq-7000 HP port support burst reads at 32-bit data width? Or does it require 64-bit for bursts?

2. Does ARLEN=15 work on AXI3 HP ports? The AXI3 spec supports 1-16 beat bursts (ARLEN 0-15). But maybe the Zynq HP implementation has a lower limit?

3. Is the address alignment requirement different for bursts? For INCR burst with SIZE=2 (4 bytes) and LEN=15, the start address must be 4-byte aligned. My addresses are `line_addr + (word_idx << 2)` which should be 4-byte aligned since line_addr = base + line * stride (both multiples of 4).

4. Could the issue be that the v11 AR signals were registered (1 cycle delay) while the working single-beat path drives them combinationally? The AXI spec doesn't require combinational valid, but maybe Zynq HP has a timing sensitivity?

5. Is there a simple test I can do from xsdb or from Linux to verify HP0 accepts burst reads? E.g., is there a PS register that configures HP0 burst support?

6. Should I try ARBURST=0 (FIXED) with ARLEN=15 instead of ARBURST=1 (INCR)? FIXED burst reads the same address 16 times — not useful for framebuffer but would test if the burst mechanism itself works.
