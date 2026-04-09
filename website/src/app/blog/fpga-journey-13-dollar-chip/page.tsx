import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import EmailCapture from "@/components/EmailCapture";

export const metadata: Metadata = {
  title: "From Math to Silicon: 69.7 Gops/s on a $13.50 Chip — ATOMiK Blog",
  description:
    "How we went from 108 Lean4 theorems to a custom RISC-V CPU with native delta-state instructions, HD HDMI output, and 69.7 billion operations per second on commodity FPGAs.",
};

function Code({ children }: { children: string }) {
  return (
    <pre
      className="rounded-lg p-4 text-sm overflow-x-auto my-4"
      style={{
        background: "#0a0a0f",
        border: "1px solid #1e1e2e",
        color: "#22d3ee",
        fontFamily: "'SF Mono', 'Fira Code', 'Consolas', monospace",
        lineHeight: 1.6,
      }}
    >
      <code>{children}</code>
    </pre>
  );
}

function Stat({ value, label }: { value: string; label: string }) {
  return (
    <div className="text-center">
      <div className="text-2xl font-bold" style={{ color: "#4f8fff" }}>
        {value}
      </div>
      <div className="text-xs" style={{ color: "#8888a0" }}>
        {label}
      </div>
    </div>
  );
}

export default function FPGABlogPost() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav active="Blog" />

      <article className="max-w-3xl mx-auto px-6 pt-16 pb-24">
        {/* Header */}
        <div className="mb-12">
          <Link
            href="/blog"
            className="text-sm mb-6 inline-block hover:underline"
            style={{ color: "#4f8fff" }}
          >
            &larr; Back to blog
          </Link>
          <time className="block text-sm font-mono mb-3" style={{ color: "#8888a0" }}>
            March 15, 2026
          </time>
          <h1 className="text-3xl font-bold tracking-tight mb-4 leading-tight">
            From Math to Silicon: 69.7 Gops/s on a $13.50 Chip
          </h1>
          <div className="flex gap-2">
            {["hardware", "fpga", "risc-v", "engineering"].map((tag) => (
              <span
                key={tag}
                className="text-xs px-2 py-0.5 rounded-full"
                style={{
                  background: "rgba(139, 92, 246, 0.1)",
                  color: "#8b5cf6",
                  border: "1px solid rgba(139, 92, 246, 0.2)",
                }}
              >
                {tag}
              </span>
            ))}
          </div>
        </div>

        {/* Stats banner */}
        <div
          className="rounded-xl border p-6 mb-10 grid grid-cols-4 gap-4"
          style={{ background: "#12121a", borderColor: "#1e1e2e" }}
        >
          <Stat value="$13.50" label="Board cost" />
          <Stat value="94.5M" label="Ops/sec (single bank)" />
          <Stat value="69.7B" label="Ops/sec (512 banks)" />
          <Stat value="3" label="SoC generations" />
        </div>

        {/* Content */}
        <div className="space-y-6 text-base leading-relaxed" style={{ color: "#c8c8d4" }}>
          <p>
            ATOMiK started as a set of equations on a whiteboard. XOR-based delta-state
            algebra: four operations, provably correct, with some interesting properties.
            But equations on a whiteboard don&apos;t ship products. This is the story of how
            those equations became silicon — three generations of custom hardware, from a
            first blink test to a custom 64-bit RISC-V CPU with native ATOMiK instructions
            and HD video output.
          </p>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            Chapter 1: The Math
          </h2>
          <p>
            The core insight is deceptively simple. Instead of storing state and copying
            it around, you store a reference point and accumulate XOR deltas:
          </p>
          <Code>{`current_state = initial_state XOR accumulator`}</Code>
          <p>
            XOR gives you an Abelian group for free — commutative, associative,
            self-inverse, with identity element zero. We proved all of this formally
            with 108 Lean4 theorems. Not &quot;we tested it&quot; — we <em>proved</em> it.
            The algebra is correct by construction.
          </p>
          <p>
            The practical consequence: deltas can arrive in any order, from any number of
            producers, and the result is identical. No locks. No consensus protocol.
            No ordering constraints. The accumulator is a shared resource by design.
          </p>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            Chapter 2: First Silicon — PicoRV32 + ATOMiK (v1)
          </h2>
          <p>
            The first hardware target was the Tang Nano 9K — a $13.50 FPGA board with
            a Gowin GW1NR-9K chip, 8,640 LUTs, and 26 block RAMs. We paired ATOMiK
            with a PicoRV32 RISC-V soft core.
          </p>
          <p>
            The ATOMiK core lives on the memory bus as an MMIO peripheral. The CPU
            writes to specific addresses to trigger LOAD, ACCUM, READ, and SWAP operations.
            A toggle-handshake CDC bridge crosses between the CPU clock domain (25.2 MHz)
            and the ATOMiK domain (81 MHz).
          </p>
          <Code>{`// ATOMiK v1 — MMIO-mapped operations
#define ATOMIK_BASE     0x20000000
#define ATOMIK_LOAD     (ATOMIK_BASE + 0x00)
#define ATOMIK_ACCUM    (ATOMIK_BASE + 0x04)
#define ATOMIK_READ     (ATOMIK_BASE + 0x08)
#define ATOMIK_SWAP     (ATOMIK_BASE + 0x0C)

*(volatile uint32_t*)ATOMIK_LOAD = 0xDEADBEEF;
*(volatile uint32_t*)ATOMIK_ACCUM = 0x000000FF;
uint32_t state = *(volatile uint32_t*)ATOMIK_READ;
// state == 0xDEADBE10`}</Code>
          <p>
            Result: single-bank ATOMiK at 81 MHz, 94.5 million operations per second.
            The entire SoC fits in 44% of the GW1NR-9K. 11/11 hardware tests pass.
            The core has +23% Fmax margin — it could run faster, but we&apos;re limited by
            the PicoRV32&apos;s bus timing.
          </p>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            Chapter 3: Custom CPU — RV64I + ATOMiK ISA (v2/v3)
          </h2>
          <p>
            MMIO works, but it costs bus cycles. Every ATOMiK operation requires a
            store instruction, a bus transaction, and a load to read the result. What
            if ATOMiK operations were native CPU instructions?
          </p>
          <p>
            We built a custom 64-bit RISC-V CPU from scratch. Not a fork — a
            ground-up implementation with a pipelined FSM (FETCH, DECODE, EXECUTE,
            WRITEBACK), SPI XIP flash boot, UART, and native ATOMiK custom instructions
            using the RISC-V custom-0 opcode space:
          </p>
          <Code>{`// ATOMiK v3 — Native ISA extensions (custom-0 opcode 0x0B)
// funct3 encoding:
//   000 = LOAD   (set reference state)
//   001 = ACCUM  (XOR delta into accumulator)
//   010 = READ   (reconstruct current state)
//   011 = SWAP   (atomic read-and-reset)

// In assembly:
.insn r 0x0b, 0, 0, x0, a0, x0    # LOAD  a0
.insn r 0x0b, 1, 0, x0, a1, x0    # ACCUM a1
.insn r 0x0b, 2, 0, a2, x0, x0    # READ  -> a2
.insn r 0x0b, 3, 0, a3, x0, x0    # SWAP  -> a3`}</Code>
          <p>
            ATOMiK operations now execute in a single EXECUTE stage cycle — the same
            cost as an ADD or XOR instruction. No bus overhead. No MMIO latency.
            Zero extra cycles.
          </p>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            Chapter 4: HD Video — 1280x720@60Hz (v3.1)
          </h2>
          <p>
            To demonstrate delta-driven display, we added an HDMI output pipeline.
            On a $13.50 FPGA. At 1280x720@60Hz.
          </p>
          <p>
            This required 6 pixel pipeline optimizations to hit the 74.25 MHz pixel
            clock: 3-stage TMDS encoding, pre-registered RNG and cursor flags in svo_tcard,
            parallel prefix gray-to-binary conversion, split font pipeline, pre-registered
            BRAM ports, and a register buffer between encoder and TMDS serializer.
          </p>
          <p>
            The delta display module sits in the video pipeline between the overlay
            and the encoder. It maintains a per-scanline buffer and applies LUT-mapped
            delta colors in real time — the display literally shows state changes as
            they happen, driven by the ATOMiK accumulator.
          </p>
          <p>
            Final v3.1.0 resource usage on the GW1NR-9K: 6,287 LUT (73%), 3,783 CLS (88%),
            20/26 BSRAM (77%). Pixel Fmax: 74.384 MHz (+0.18% margin). We hit the
            practical optimization ceiling — CLS at 88% is the binding constraint.
          </p>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            Chapter 5: Scaling Up — Zynq Characterization
          </h2>
          <p>
            The Tang Nano 9K proved the architecture. But 8,640 LUTs limits you to a
            single ATOMiK bank. What happens when you have 53,200 LUTs?
          </p>
          <p>
            We characterized ATOMiK on the Xilinx Zynq XC7Z020, sweeping from 1 to 512
            parallel banks across 4 synthesis strategies (baseline, area, aggressive,
            maximum):
          </p>

          <div
            className="rounded-xl border p-6 my-6 overflow-x-auto"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <table className="w-full text-sm">
              <thead>
                <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                  <th className="text-left p-2" style={{ color: "#8888a0" }}>Banks</th>
                  <th className="text-right p-2" style={{ color: "#8888a0" }}>Fmax (MHz)</th>
                  <th className="text-right p-2" style={{ color: "#8888a0" }}>LUT</th>
                  <th className="text-right p-2" style={{ color: "#8888a0" }}>LUT %</th>
                  <th className="text-right p-2" style={{ color: "#8888a0" }}>Gops/s</th>
                </tr>
              </thead>
              <tbody style={{ color: "#b0b0c0" }}>
                {[
                  ["N=1", "444.4", "302", "0.6%", "0.4"],
                  ["N=4", "347.8", "543", "1.0%", "1.4"],
                  ["N=16", "266.7", "941", "1.8%", "4.4"],
                  ["N=64", "205.1", "3,498", "6.6%", "13.4"],
                  ["N=256", "148.1", "15,197", "28.6%", "38.1"],
                  ["N=512", "135.6", "23,542", "44.3%", "69.7"],
                ].map(([banks, fmax, lut, pct, gops]) => (
                  <tr key={banks} style={{ borderBottom: "1px solid #1e1e2e" }}>
                    <td className="p-2 font-mono">{banks}</td>
                    <td className="text-right p-2 font-mono">{fmax}</td>
                    <td className="text-right p-2 font-mono">{lut}</td>
                    <td className="text-right p-2 font-mono">{pct}</td>
                    <td className="text-right p-2 font-mono font-bold" style={{ color: "#22d3ee" }}>{gops}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>

          <p>
            Sub-linear LUT scaling: 512 banks costs only 44.3% of the fabric. Each
            additional bank adds ~34 LUTs beyond the first — the shared infrastructure
            (BRAM, merge tree, CDC bridge) amortizes across all banks.
          </p>
          <p>
            69.7 billion operations per second. On a $99 development board. With
            56% of the fabric still available for your application logic.
          </p>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            What we learned
          </h2>
          <ul className="list-disc pl-6 space-y-2">
            <li>
              <strong>Start with the math.</strong> The 108 Lean4 proofs caught edge
              cases that testing never would have found. When your algebra is
              provably correct, debugging hardware becomes purely a plumbing exercise.
            </li>
            <li>
              <strong>$13.50 is enough.</strong> You don&apos;t need a $10,000 FPGA board
              to validate a novel architecture. The Tang Nano 9K proved everything
              we needed — the Zynq just showed it scales.
            </li>
            <li>
              <strong>Custom instructions matter.</strong> Going from MMIO (v1) to
              native ISA (v3) eliminated all bus overhead. ATOMiK ops execute at the
              same cost as ALU operations.
            </li>
            <li>
              <strong>The ceiling is the fabric, not the design.</strong> At N=512,
              we&apos;re using 44% of the XC7Z020. N=1024 needs an XC7Z045 (218K LUT) or
              UltraScale+. The ATOMiK core itself has no inherent scaling limit.
            </li>
          </ul>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            What&apos;s next
          </h2>
          <p>
            The ALINX AX7020 Zynq board is on the bench. Next step: PS+PL block
            design, Linux driver integration, and live benchmarks with the kernel module
            talking to hardware-accelerated ATOMiK contexts. After that: ASIC evaluation
            on Sky130.
          </p>
          <p>
            The math works. The software works. The hardware works. Now we scale.
          </p>

          {/* CTA */}
          <div
            className="rounded-xl border p-8 mt-8"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <h3 className="text-xl font-bold mb-4 text-center">Try ATOMiK today</h3>
            <EmailCapture variant="blog" />
          </div>
        </div>
      </article>
    </div>
  );
}
