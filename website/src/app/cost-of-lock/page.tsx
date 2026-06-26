import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import Footer from "@/components/Footer";
import CostOfLock from "./CostOfLock";

export const metadata: Metadata = {
  title: "Cost of Lock — What Hitting the Bar Is Worth",
  description:
    "Translate ATOMiK's silicon-measured order-independence into the coordination tax it removes from your stack — computed from your own stated baseline, every line tagged measured vs your-baseline.",
  openGraph: {
    title: "ATOMiK — Cost of Lock",
    description:
      "The lock / shard / CRDT / consensus tax you pay for correctness under concurrency — quantified, and what order-independence removes.",
    url: "https://atomik.tech/cost-of-lock",
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
};

export default function CostOfLockPage() {
  return (
    <main style={{ background: c.bg, color: c.text, minHeight: "100vh" }}>
      <Nav active="Workloads" />
      <section style={{ maxWidth: 960, margin: "0 auto", padding: "56px 22px 80px" }}>
        <p style={{ color: c.cyan, fontSize: 13, fontWeight: 600, textTransform: "uppercase", letterSpacing: 0.5 }}>
          Cost of Lock
        </p>
        <h1 style={{ fontSize: 38, lineHeight: 1.1, margin: "10px 0 14px", fontWeight: 800 }}>
          What is hitting the bar worth?
        </h1>
        <p style={{ color: c.muted, fontSize: 17, lineHeight: 1.55, maxWidth: 740 }}>
          The proof shows ATOMiK hits your throughput-at-correctness bar. This shows what that&apos;s{" "}
          <strong style={{ color: c.text }}>worth</strong>: the coordination tax you pay today —
          locks, shard fan-out, CRDT metadata, a consensus round-trip — exists to guarantee exactly
          the order-independence ATOMiK proves on silicon. So it&apos;s removable. Enter your own
          baseline and see it quantified.
        </p>
        <p style={{ color: c.faint, fontSize: 12.5, marginTop: 10, maxWidth: 740 }}>
          Honesty: ATOMiK&apos;s capability is measured on silicon; the savings are arithmetic on{" "}
          <em>your</em> stated baseline. Every line is tagged. It is the coordination tax that&apos;s
          removed — not a &ldquo;we&apos;re faster&rdquo; multiplier.
        </p>

        <div style={{ margin: "28px 0 12px" }}>
          <CostOfLock />
        </div>

        <div style={{ marginTop: 30, paddingTop: 22, borderTop: `1px solid ${c.border}`, color: c.faint, fontSize: 13, lineHeight: 1.6 }}>
          See the verdict behind the headroom on the{" "}
          <Link href="/benchmark-proof" style={{ color: c.cyan }}>benchmark tool</Link>, browse{" "}
          <Link href="/workloads" style={{ color: c.cyan }}>workload archetypes</Link>, or{" "}
          <Link href="/contact?intent=live-benchmark" style={{ color: c.cyan }}>
            request a live run + conditional LOI
          </Link>
          . The value memo joins the proof report and the LOI as the leave-behind packet.
        </div>
      </section>
      <Footer />
    </main>
  );
}
