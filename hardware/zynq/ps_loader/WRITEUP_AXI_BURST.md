# AXI burst reader on Zynq HP0 — black screen, FSM likely stuck

## Context

v10e (single-beat wishbone → Wishbone2AXI → HP0) showed "blocky" image — correct content but every line repeated 2x because single-beat reads were too slow (640 × ~10 cycles = 6400 cycles > line period). v11 replaces with direct AXI3 burst reads: 40 × 16-beat bursts per line. Screen is now black despite DMA enable=1, base correct, VTG running.

## v11 AXI burst FSM (Migen, sys domain)

```python
fsm.act("WAIT",
    If(enable_sys & (new_line_pending | new_line),
        NextValue(word_idx, 0),
        NextValue(fetch_line, vcount_sys + 1),
        NextState("AR"),
    ),
)
fsm.act("AR",
    axi.ar.valid.eq(1),
    axi.ar.addr.eq(line_addr + (word_idx << 2)),
    axi.ar.burst.eq(1),          # INCR
    axi.ar.len.eq(burst_len - 1), # 15 = 16 beats
    axi.ar.size.eq(2),            # 4 bytes per beat
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
    If(axi.r.valid,
        wr_port.we.eq(1),
        wr_port.adr.eq(Cat(word_idx, wr_page)),
        wr_port.dat_w.eq(axi.r.data),
        NextValue(word_idx, word_idx + 1),
        NextValue(burst_beat, burst_beat + 1),
        If(axi.r.last,
            If(word_idx == (hres - 2),
                NextState("DONE"),
            ).Else(
                NextState("AR"),
            ),
        ),
    ),
)
```

Write channels tied off:
```python
self.comb += [
    axi.aw.valid.eq(0),
    axi.w.valid.eq(0),
    axi.b.ready.eq(1),
]
```

## AXI interface

From LiteX `ps.add_axi_hp_slave(clock_domain="sys", data_width=32)`:
- AXI3, 32-bit data, 32-bit address, 6-bit ID
- HP0 port on Zynq-7000 PS

## Questions

1. Is `axi.ar.size.eq(2)` correct for 32-bit data? (2 = 4 bytes = 2^2)
2. For AXI3 on Zynq HP, does the AR channel need any additional signals I'm not setting? (ARLOCK, ARPROT, ARCACHE, ARQOS — I set them but maybe wrong values?)
3. Could the `axi.ar.len.eq(15)` be wrong? AXI3 supports up to ARLEN=15 (16 beats). Does Zynq HP0 actually accept 16-beat bursts on a 32-bit port?
4. Is `axi.r.data` the correct signal name for the read data in LiteX's AXI interface? Or is it `axi.r.data` vs `axi.r.resp` etc.?
5. Most likely failure mode: AR stuck (ARREADY never asserts) → FSM never leaves AR state → no data → black screen. What would prevent ARREADY from asserting on HP0?
