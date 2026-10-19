# ChatGPT — second opinion on DDR corruption + Linux kernel panic

**Paste this entire file as a single message to ChatGPT (GPT-5 / o-class model preferred — this is dense Zynq+LiteX+NaxRiscv debug).**

---

## Project context (ATOMiK)

I'm building a research SoC on a HamGeek RK-ZYNQ7020-F board (Xilinx Zynq-7000 XC7Z020-2CLG484-2, dual Cortex-A9 PS + 7-series PL). The PL runs a 64-bit NaxRiscv soft-core inside a LiteX SoC. The architectural pattern is:

```
Zynq BootROM → ARM FSBL → PCAP load LiteX bitstream into PL
                       ↘ ARM hands off control by parking in WFE
PL NaxRiscv (RV64IMAFDC) → LiteX BIOS → OpenSBI → Linux 6.1-rc2
```

DDR (PS-side, 512 MB DDR3) is shared with NaxRiscv via the Zynq GP0 master interface (PL master → PS slave). NaxRiscv's `main_ram` window is at `0x40000000` (NaxRiscv view) which the LiteX `axi_full_to_axi` + Zynq central interconnect routes into PS DDR at PS address `0x00100000` and up.

I just landed an architectural cleanup: instead of patching each software layer (BIOS, OpenSBI, Linux liteuart) to know about Zynq PS UART0, I built a migen module `PSUart0Bridge` that **presents a LiteX UART CSR (rxtx/txfull/rxempty/ev_*)** to software but **internally drives PS UART0** (ARM `0xE0000000`, MIO10/11) via wishbone master accesses through GP1. This works end-to-end:

- BIOS banner and prompt: ✅ visible on `/dev/ttyUSB2` @ 115200 (FT2232H ch B)
- BIOS interactive (typed `help`): ✅
- `boot 0x40f00000` → OpenSBI banner: ✅
- Linux kernel boot up to `param_sysfs_init`: ✅ (7942 bytes of clean log captured)

## What's broken

Two issues, almost certainly one root cause:

### Issue 1 — BIOS memtest reports 16 data errors / 524288 reads

Memtest target: `0x40000000` (NaxRiscv main_ram, → PS DDR `0x00100000`).

```
Memtest at 0x40000000 (2.0MiB)...
  bus errors:  0/256
  addr errors: 0/8192
  data errors: 16/524288
Memtest KO
Memspeed:
  Write speed: 7.2MiB/s
   Read speed: 13.1MiB/s
Memory initialization failed
```

**Suspicious pattern:** 524288 reads / 16 errors = 32768 = 2^15 = **one error per 128 KiB**. Not random — this is alignment-related. Cache-line boundary, page boundary, or burst-coalescing boundary.

### Issue 2 — Linux kernel panic at `__kernfs_remove+0x86`

After successful BIOS handoff to OpenSBI and Linux:

```
[    0.300056] NET: Registered PF_NETLINK/PF_ROUTE protocol family
[    0.311751] DMA: preallocated 128 KiB GFP_KERNEL pool for atomic allocations
[    0.318378] DMA: preallocated 128 KiB GFP_KERNEL|GFP_DMA32 pool for atomic allocations
[    0.416528] Oops - illegal instruction [#1]
[    0.418126] CPU: 0 PID: 1 Comm: swapper Not tainted 6.1.0-rc2-g247f34f7b803-dirty #14
[    0.428959] epc : __kernfs_remove.part.0+0x86/0x1b4
[    0.433921]  ra : kernfs_remove_by_name_ns+0x5a/0xbc
[    0.515989] status: 0000000200000120 badaddr: 0000000000000000 cause: 0000000000000002
[    0.523994] Call Trace:
[    0.523994] [<ffffffff80101108>] kernfs_remove_by_name_ns+0x5a/0xbc
[    0.530294] [<ffffffff8010365e>] remove_files+0x24/0x52
[    0.535441] [<ffffffff801039c4>] sysfs_remove_group+0x2e/0x72
[    0.541253] [<ffffffff8041f900>] param_sysfs_init+0x14a/0x302
[    0.546936] [<ffffffff8041a492>] do_one_initcall+0x68/0x118
[    0.552531] [<ffffffff8041a702>] kernel_init_freeable+0x168/0x1bc
[    0.558544] [<ffffffff80414bd2>] kernel_init+0x1a/0xe2
[    0.563629] [<ffffffff80001efc>] ret_from_exception+0x0/0xc
[    0.574403] Kernel panic - not syncing: Attempted to kill init! exitcode=0x0000000b
```

- `cause: 0x2` = RISC-V illegal-instruction trap
- `badaddr: 0` — NaxRiscv didn't populate `stval` (allowed by RISC-V spec, but unhelpful)
- `status: 0x0000000200000120` — SPP=1 (was in supervisor), SPIE=1, SIE=0
- Kernel reports ISA `acdfim` (rv64imafdc + Zicsr/Zifencei implicit). DTS declares the same.
- `__kernfs_remove` runs on every RISC-V Linux boot under the sun. The function isn't broken — almost certainly a memory bit-flip corrupted either the instruction stream OR a pointer that got dereferenced as code.

## Configuration details

**NaxRiscv config (LiteX target):**
```python
NaxRiscv.xlen = 64
NaxRiscv.l2_bytes = 32 * 1024   # 32 KB L2 cache
NaxRiscv.l2_ways  = 4
# CPU clock: 100 MHz from a LiteX MMCM
# Memory model: rv64imafdc with MMU (Sv39)
```

**Bus topology:**
- NaxRiscv issues 64-bit reads/writes on its native bus
- LiteX converts NaxRiscv → wishbone → AXI4 → Zynq GP0 (master)
- **GP0 is 32-bit AXI** (Zynq PS only exposes 32-bit AXI on GP master ports)
- DDR sits behind PS central interconnect → DDR controller

**PSUart0Bridge (the new bit):**
- A wishbone master + CSR slave hybrid
- Polls PS UART0 SR every 10,000 cycles (~100 µs) via a single 32-bit GP1 read
- TX path: software writes CSR `rxtx` → bridge wishbone-master-writes PS UART0 TX FIFO via GP1
- GP1 (PS slave, 32-bit AXI) is a **separate** master port from GP0 (used for DDR)

## My three hypotheses (please critique / rank / suggest alternatives)

### Hypothesis A — Bus-arbiter contention between bridge poll and NaxRiscv DDR access
The bridge polls UART0 SR every 100 µs while idle. Memtest runs ~430 ms → ~4300 polls. 16 errors / 4300 = 0.4% rate per poll. The wishbone arbiter inserts a 1-beat GP1 access into NaxRiscv's stream, somehow causing a beat to be misinterpreted.

**Counter-argument:** GP0 and GP1 are physically separate AXI master ports on the PS — they don't share AXI infrastructure. Wishbone contention shouldn't propagate to DDR accesses going via GP0. *Unless* LiteX's central wishbone interconnect serializes both masters and we're hitting a wishbone-side ordering bug.

### Hypothesis B — Cache-line refill tearing on 32-bit GP0 (most likely)
NaxRiscv L2 = 32 KB, 64-byte lines. A line refill issues a burst of 16 × 32-bit beats on the GP0 AXI master. If a beat is dropped, miscounted, or returned out of order, the line's last 8 bytes (or other suffix) end up zeroed or with stale data. 128 KiB = 2048 cache lines = 1 corrupted line per 2048 (≈ 0.05% line-error rate). Quietly catastrophic for kernel data (where a corrupted pointer becomes an illegal-instruction trap).

**Test:** disable L2 (`l2_bytes = 0`) so NaxRiscv issues single-word DDR accesses. If memtest goes clean, B is confirmed. Fix: move DDR path from GP0 (32-bit) to AXI HP (64-bit) slave, which matches NaxRiscv's native data width.

### Hypothesis C — Pre-existing DDR timing issue from ps7_init
Maybe ps7_init has a marginal DDR3 timing parameter and 16 errors per 2 MiB is the baseline regardless of our bridge / NaxRiscv config. The factory image at the same DDR settings ran cleanly, but it was running on the ARM A9 with different access patterns than NaxRiscv's bursty L2 refills.

## Questions for you

1. **Which hypothesis would you investigate first?** I have ~16-20 min per Vivado iteration, so cost-of-test matters. I'm leaning B (disable L2) because it's a single config change with the highest diagnostic power.

2. **Is GP0's 32-bit width actually a known problem for high-bandwidth PL → PS DDR access on Zynq-7000?** Xilinx UG585 strongly recommends AXI HP (64-bit) for PL-master-to-DDR traffic. If GP0 cache-line tearing is "well-known," that's a strong tell.

3. **Can NaxRiscv be configured to read DDR with native 64-bit AXI?** LiteX's `AXIInterconnect` should support a 64-bit master, but I need to confirm the path NaxRiscv → LiteX-AXI-to-Zynq-HP would actually preserve 64-bit width end-to-end without LiteX inserting a 64→32 downconverter for GP0 compatibility elsewhere.

4. **Is there a simpler diagnostic test for "bus tearing" beyond just running memtest?** E.g. a deterministic pattern fill + verify that would reveal whether errors cluster at specific byte-positions within a cache line, which would distinguish A vs B definitively.

5. **For the kernel panic specifically:** is there any reason `__kernfs_remove+0x86` would naturally crash on a NaxRiscv that doesn't crash on other RV64GC implementations? (e.g. a missing extension that the kernel uses but NaxRiscv doesn't implement). The instruction at that offset is presumably standard RV64 code emitted by GCC for kernel 6.1-rc2. ISA strings appear consistent (`rv64imafdc`).

6. **What's the cheapest path to validate the bridge isn't the cause** (Hypothesis C testing)? My options: (a) rebuild with bridge removed, (b) somehow assert "bridge never polls" at runtime, (c) add an explicit gate on the bridge wishbone master so it's electrically idle during memtest. Option (c) sounds cleanest if migen supports gating a wishbone master cleanly.

7. **Any sanity checks I'm missing?** For instance, am I sure ps7_init is the right one for *this* board (HamGeek RK-ZYNQ7020-F, not the closely-related ALINX AX7020)? The two boards share the same Zynq part but differ in MIO routing — I already discovered ALINX docs say UART on MIO48/49 but this board uses MIO10/11.

Please be specific and concrete. I'd rather a wrong-but-decisive opinion than a hedged maybe-list. I have hardware in hand, can run any test you suggest, and will report exact results back.

---

## Reference: relevant code snippets

**The migen bridge (~150 lines):**
```python
class PSUart0Bridge(Module, AutoCSR):
    def __init__(self, ps_uart_base=0x80000000, poll_period=10_000):
        # CSR layout — software-visible
        self._rxtx       = CSR(8)        # write→TX, read→RX
        self._txfull     = CSRStatus(1)
        self._rxempty    = CSRStatus(1)
        self._ev_status  = CSRStatus(2)
        self._ev_pending = CSRStorage(2)
        self._ev_enable  = CSRStorage(2)

        # Wishbone master (drives bridge → PS UART0)
        self.bus = wishbone.Interface(data_width=32, adr_width=30)

        # IRQ output (kept low for now)
        self.ev = Signal()

        # Internal FIFOs (1-deep)
        rx_fifo = Signal(8)
        rx_valid = Signal()
        tx_pending = Signal()
        tx_data = Signal(8)

        wake = Signal()
        self.comb += wake.eq(tx_pending | self._rxtx.we | self._rxtx.re)

        poll_ctr = Signal(20)

        # FSM with IDLE, POLL_SR, TX_WRITE, RX_READ
        # IDLE: decrement poll_ctr; if wake or ctr==0, → POLL_SR
        # POLL_SR: wishbone read 0x8000002C → decide TX_WRITE / RX_READ / back to IDLE
        # TX_WRITE: wishbone write 0x80000030 with tx_data
        # RX_READ: wishbone read 0x80000030 → rx_fifo
```

**SoC integration (the diff that introduced this):**
```python
kwargs["uart_name"] = "stub"            # disable LiteX's UART
super().__init__(..., **kwargs)
self.uart = PSUart0Bridge(ps_uart_base=0x80000000, poll_period=10_000)
self.csr.add("uart", use_loc_if_exists=True)
self.bus.add_master("psuart_bridge", master=self.uart.bus)
self.irq.add("uart", use_loc_if_exists=True)
```
