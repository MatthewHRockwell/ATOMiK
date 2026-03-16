"use client";

import Link from "next/link";
import Nav from "@/components/Nav";
import { useState } from "react";

const tiers = [
  {
    name: "Community",
    price: "Free",
    period: "",
    description: "For developers exploring delta-state algebra",
    cta: "Get Started Free",
    ctaHref: "/get-started",
    ctaStyle: { background: "transparent", color: "#e0e0e8", border: "1px solid #1e1e2e" },
    features: [
      { text: "Python SDK + C header + examples", included: true },
      { text: "4-operation API (LOAD, ACCUM, READ, SWAP)", included: true },
      { text: "AtomikTable, DeltaStream, Fingerprint", included: true },
      { text: "Apache 2.0 license", included: true },
      { text: "Community support via GitHub", included: true },
      { text: "Linux kernel module", included: false },
      { text: "SDK generation pipeline", included: false },
      { text: "Hardware acceleration", included: false },
    ],
  },
  {
    name: "Pro",
    price: "$99",
    period: "/mo",
    description: "System-level optimization with the Linux kernel module",
    cta: "Start 90-Day Free Trial",
    ctaAction: "professional",
    ctaStyle: { background: "transparent", color: "#4f8fff", border: "1px solid #4f8fff" },
    features: [
      { text: "Everything in Community", included: true },
      { text: "Linux kernel module (COW detection, network dedup, cgroup tracking)", included: true },
      { text: "27 sysfs metrics", included: true },
      { text: "atomik-status dashboard", included: true },
      { text: "atomik-bench performance suite", included: true },
      { text: "Priority email support (48hr SLA)", included: true },
      { text: "90-day free trial", included: true },
      { text: "SDK generation pipeline", included: false },
      { text: "Hardware acceleration", included: false },
    ],
  },
  {
    name: "Team",
    price: "$299",
    period: "/mo",
    description: "Multi-language SDK generation and team collaboration",
    cta: "Start Free Trial",
    ctaAction: "team",
    ctaStyle: { background: "#4f8fff", color: "#fff", border: "none" },
    featured: true,
    features: [
      { text: "Everything in Pro", included: true },
      { text: "SDK generation pipeline (Python, Rust, C, JavaScript, Verilog)", included: true },
      { text: "atomik-report waste analysis tool", included: true },
      { text: "Team license (5 seats)", included: true },
      { text: "JSON/CSV export for CI integration", included: true },
      { text: "Schema-driven code generation from domain specs", included: true },
      { text: "Hardware acceleration", included: false },
    ],
  },
  {
    name: "Enterprise",
    price: "$999",
    period: "/mo",
    description: "FPGA hardware acceleration and dedicated support",
    cta: "Contact Sales",
    ctaHref: "/contact",
    ctaStyle: { background: "transparent", color: "#e0e0e8", border: "1px solid #1e1e2e" },
    features: [
      { text: "Everything in Team", included: true },
      { text: "FPGA hardware acceleration (Zynq AXI4-Lite, up to 512 parallel banks)", included: true },
      { text: "Custom RV64I CPU with ATOMiK ISA", included: true },
      { text: "Hardware context tables", included: true },
      { text: "4-hour response SLA", included: true },
      { text: "Dedicated support channel", included: true },
      { text: "Custom integration consulting", included: true },
      { text: "Formal verification certificate", included: true },
      { text: "ASIC development path", included: true },
      { text: "Volume licensing", included: true },
    ],
  },
];

const faqs = [
  {
    q: "What happens after the 90-day trial?",
    a: "The kernel module gracefully degrades — operations become no-ops and reads return zero. No crashes, no data loss. You can upgrade to Pro at any time to restore full functionality.",
  },
  {
    q: "Do I need the kernel module to use ATOMiK?",
    a: "No. The Python SDK (atomik-core) is free, open source (Apache 2.0), and has zero dependencies. It implements the full delta-state algebra in pure Python. The kernel module adds system-level optimizations like COW detection and network deduplication.",
  },
  {
    q: "What's the difference between Pro and Team?",
    a: "Pro gives you the Linux kernel module for system-level optimization — COW detection, network dedup, cgroup tracking, and 27 sysfs metrics. Team adds the SDK generation pipeline (5 languages), the atomik-report waste analysis tool, a 5-seat team license, and CI-friendly JSON/CSV export. If you're a solo developer or small ops team, Pro is all you need. If your team builds on ATOMiK across multiple languages, Team is the right fit.",
  },
  {
    q: "What Linux kernels are supported?",
    a: "The kernel module supports Linux 5.15+ via DKMS. It builds against your running kernel headers and loads as a standard module. Tested on Ubuntu 22.04/24.04, Debian 12, and RHEL 9.",
  },
  {
    q: "How does hardware acceleration work?",
    a: "Enterprise customers get access to FPGA bitstreams for Xilinx Zynq that implement ATOMiK as an AXI4-Lite peripheral. The kernel module auto-detects hardware via device tree and transparently routes operations to the FPGA. Software fallback is always available.",
  },
  {
    q: "Can I try before I buy?",
    a: "Yes — both Pro and Team tiers include free trials. No credit card required to start. Run sudo ./install.sh and you're up in 60 seconds.",
  },
];

export default function PricingPage() {
  const [loading, setLoading] = useState<string | null>(null);
  const [openFaq, setOpenFaq] = useState<number | null>(null);

  async function checkout(tier: string) {
    setLoading(tier);
    try {
      const res = await fetch("/api/checkout", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ tier }),
      });
      const data = await res.json();
      if (data.url) {
        window.location.href = data.url;
      }
    } catch {
      setLoading(null);
    }
  }

  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav />

      {/* Hero */}
      <section className="text-center px-6 pt-20 pb-4">
        <h1 className="text-4xl font-bold tracking-tight mb-4">
          Simple, transparent pricing
        </h1>
        <p className="text-lg max-w-2xl mx-auto" style={{ color: "#8888a0" }}>
          Start free with the Python SDK. Add system-level optimization when you need it.
          Scale to hardware when you&apos;re ready.
        </p>
      </section>

      {/* Pricing Cards */}
      <section className="max-w-6xl mx-auto px-6 py-12">
        <div className="grid md:grid-cols-2 lg:grid-cols-4 gap-6">
          {tiers.map((tier) => (
            <div
              key={tier.name}
              className="rounded-xl border p-6 flex flex-col relative"
              style={{
                background: tier.featured ? "#14142a" : "#12121a",
                borderColor: tier.featured ? "#4f8fff33" : "#1e1e2e",
                boxShadow: tier.featured ? "0 0 40px rgba(79,143,255,0.08)" : "none",
              }}
            >
              {tier.featured && (
                <div
                  className="absolute -top-3 left-1/2 -translate-x-1/2 text-xs font-bold px-3 py-1 rounded-full"
                  style={{ background: "#4f8fff", color: "#fff" }}
                >
                  Most Popular
                </div>
              )}

              <h3 className="text-xl font-bold mb-1">{tier.name}</h3>
              <div className="mb-2">
                <span className="text-3xl font-bold">{tier.price}</span>
                {tier.period && (
                  <span className="text-sm" style={{ color: "#8888a0" }}>
                    {tier.period}
                  </span>
                )}
              </div>
              <p className="text-sm mb-6" style={{ color: "#8888a0" }}>
                {tier.description}
              </p>

              <ul className="space-y-2.5 mb-8 flex-1">
                {tier.features.map((f) => (
                  <li key={f.text} className="flex items-start gap-2 text-sm">
                    <span
                      className="mt-0.5 shrink-0"
                      style={{ color: f.included ? "#22c55e" : "#333344" }}
                    >
                      {f.included ? "\u2713" : "\u2717"}
                    </span>
                    <span style={{ color: f.included ? "#b0b0c0" : "#444455" }}>
                      {f.text}
                    </span>
                  </li>
                ))}
              </ul>

              {tier.ctaHref ? (
                <Link
                  href={tier.ctaHref}
                  className="block text-center py-3 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
                  style={tier.ctaStyle}
                >
                  {tier.cta}
                </Link>
              ) : (
                <button
                  onClick={() => tier.ctaAction && checkout(tier.ctaAction)}
                  disabled={loading === tier.ctaAction}
                  className="block w-full text-center py-3 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90 disabled:opacity-50"
                  style={tier.ctaStyle}
                >
                  {loading === tier.ctaAction ? "Redirecting..." : tier.cta}
                </button>
              )}
            </div>
          ))}
        </div>
      </section>

      {/* Comparison Table */}
      <section className="max-w-5xl mx-auto px-6 py-12">
        <h2 className="text-2xl font-bold text-center mb-8">Feature comparison</h2>
        <div
          className="rounded-xl border overflow-x-auto"
          style={{ background: "#12121a", borderColor: "#1e1e2e" }}
        >
          <table className="w-full text-sm" style={{ minWidth: "640px" }}>
            <thead>
              <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                <th className="text-left p-4 font-semibold" style={{ color: "#8888a0" }}>
                  Capability
                </th>
                <th className="text-center p-4 font-semibold" style={{ color: "#8888a0" }}>
                  Community
                </th>
                <th className="text-center p-4 font-semibold" style={{ color: "#8888a0" }}>
                  Pro
                </th>
                <th
                  className="text-center p-4 font-semibold"
                  style={{ color: "#4f8fff" }}
                >
                  Team
                </th>
                <th className="text-center p-4 font-semibold" style={{ color: "#8888a0" }}>
                  Enterprise
                </th>
              </tr>
            </thead>
            <tbody>
              {[
                ["Python SDK + C header", true, true, true, true],
                ["Delta-state algebra (4 ops)", true, true, true, true],
                ["Fingerprint change detection", true, true, true, true],
                ["Examples + benchmark suite", true, true, true, true],
                ["Linux kernel module", false, true, true, true],
                ["COW / network dedup / cgroup tracking", false, true, true, true],
                ["27 sysfs metrics + dashboard", false, true, true, true],
                ["SDK generation (5 languages)", false, false, true, true],
                ["atomik-report waste analysis", false, false, true, true],
                ["JSON/CSV CI export", false, false, true, true],
                ["Team license (5 seats)", false, false, true, true],
                ["FPGA hardware acceleration", false, false, false, true],
                ["Custom CPU + ATOMiK ISA", false, false, false, true],
                ["Formal verification certificate", false, false, false, true],
                ["Support SLA", "GitHub", "48hr", "48hr", "4hr"],
                ["License", "Apache 2.0", "Commercial", "Commercial", "Commercial"],
              ].map(([feature, ...vals], i) => (
                <tr
                  key={i}
                  style={{
                    borderBottom: "1px solid #1e1e2e",
                    background: i % 2 === 0 ? "transparent" : "rgba(255,255,255,0.01)",
                  }}
                >
                  <td className="p-4" style={{ color: "#b0b0c0" }}>
                    {feature as string}
                  </td>
                  {vals.map((v, j) => (
                    <td key={j} className="text-center p-4">
                      {v === true ? (
                        <span style={{ color: "#22c55e" }}>{"\u2713"}</span>
                      ) : v === false ? (
                        <span style={{ color: "#333344" }}>{"\u2014"}</span>
                      ) : (
                        <span style={{ color: "#b0b0c0" }}>{v as string}</span>
                      )}
                    </td>
                  ))}
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </section>

      {/* FAQ */}
      <section className="max-w-3xl mx-auto px-6 py-12 pb-24">
        <h2 className="text-2xl font-bold text-center mb-8">Frequently asked questions</h2>
        <div className="space-y-3">
          {faqs.map((faq, i) => (
            <div
              key={i}
              className="rounded-xl border overflow-hidden"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <button
                onClick={() => setOpenFaq(openFaq === i ? null : i)}
                className="w-full text-left p-5 flex items-center justify-between"
              >
                <span className="font-medium text-sm">{faq.q}</span>
                <span
                  className="shrink-0 ml-4 transition-transform"
                  style={{
                    color: "#8888a0",
                    transform: openFaq === i ? "rotate(180deg)" : "rotate(0deg)",
                  }}
                >
                  &#9662;
                </span>
              </button>
              {openFaq === i && (
                <div
                  className="px-5 pb-5 text-sm leading-relaxed"
                  style={{ color: "#8888a0" }}
                >
                  {faq.a}
                </div>
              )}
            </div>
          ))}
        </div>
      </section>
    </div>
  );
}
