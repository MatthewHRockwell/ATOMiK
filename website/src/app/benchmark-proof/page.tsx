import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import Footer from "@/components/Footer";
import BenchmarkProver from "./BenchmarkProver";

export const metadata: Metadata = {
  title: "Benchmark Proof — Test ATOMiK Against Your Numbers",
  description:
    "Enter your aggregation workload and the throughput-at-correctness bar you gate commitment on. See ATOMiK's silicon-measured verdict — byte-identical, lock-free, on real hardware.",
  openGraph: {
    title: "ATOMiK Benchmark Proof",
    description:
      "Throughput-at-correctness, measured on silicon: 800 M events/s at 8 banks, byte-identical across 1/2/4/8 banks.",
    url: "https://atomik.tech/benchmark-proof",
    type: "website",
  },
};

const c = {
  bg: "#070807",
  text: "#f5f7ee",
  muted: "#b7c4bb",
  faint: "#7f8b81",
  border: "#2d3a34",
  cyan: "#22d3ee",
  green: "#2dd36f",
};

export default function BenchmarkProofPage() {
  return (
    <main style={{ background: c.bg, color: c.text, minHeight: "100vh" }}>
      <Nav active="Workloads" />
      <section style={{ maxWidth: 960, margin: "0 auto", padding: "56px 22px 80px" }}>
        <div
          style={{
            display: "inline-block",
            border: `1px solid ${c.border}`,
            borderRadius: 999,
            padding: "4px 12px",
            fontSize: 12,
            letterSpacing: 0.5,
            color: c.green,
            marginBottom: 18,
          }}
        >
          HARDWARE_VALIDATED · LIVE_MEASURED
        </div>

        <h1 style={{ fontSize: 38, lineHeight: 1.1, margin: "0 0 14px", fontWeight: 800 }}>
          Test ATOMiK against your benchmark.
        </h1>
        <p style={{ color: c.muted, fontSize: 17, lineHeight: 1.55, maxWidth: 720 }}>
          For aggregation and reduction workloads, the bar you gate a contract on
          isn&apos;t &ldquo;is it faster&rdquo; — it&apos;s{" "}
          <strong style={{ color: c.text }}>throughput at correctness</strong>: hitting
          your rate <em>while</em> the result stays exactly correct no matter the
          arrival order or how many cores feed it. A conventional system only
          guarantees that with a lock — which caps its throughput. ATOMiK
          doesn&apos;t need one, and we can prove it on silicon.
        </p>

        <div style={{ margin: "28px 0 12px" }}>
          <BenchmarkProver />
        </div>

        <div
          style={{
            marginTop: 36,
            paddingTop: 24,
            borderTop: `1px solid ${c.border}`,
            color: c.faint,
            fontSize: 13.5,
            lineHeight: 1.6,
            maxWidth: 760,
          }}
        >
          <p>
            <strong style={{ color: c.muted }}>How this is grounded.</strong> The
            verdict is computed from facts measured on an ALINX/HamGeek AX7020
            (XC7Z020) at 100 MHz: the parallel-bank engine accumulates ~1
            delta/bank/cycle (800 M/s at 8 banks), and its merged result is
            byte-identical across 1, 2, 4 and 8 banks — which proves the answer is
            independent of arrival order and parallelism. We only claim
            order-independence for aggregations that form an abelian group; for
            anything else this page returns an honest no.
          </p>
          <p style={{ marginTop: 12 }}>
            Browse{" "}
            <Link href="/workloads" style={{ color: c.cyan }}>
              worked workload scenarios
            </Link>{" "}
            to find yours, see the{" "}
            <Link href="/benchmarks" style={{ color: c.cyan }}>
              proof &amp; benchmarks
            </Link>{" "}
            packet, or{" "}
            <Link href="/contact?topic=live-benchmark" style={{ color: c.cyan }}>
              ask us to run your workload live on the board
            </Link>
            .
          </p>
        </div>
      </section>
      <Footer />
    </main>
  );
}
