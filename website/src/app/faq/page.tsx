"use client";

import Link from "next/link";
import Nav from "@/components/Nav";
import Footer from "@/components/Footer";
import { useState } from "react";

const faqCategories = [
  {
    title: "General",
    color: "#8b5cf6",
    items: [
      {
        q: "What is ATOMiK?",
        a: "ATOMiK is a delta-state algebra engine that fundamentally departs from Von Neumann architecture. Instead of storing and retrieving state, ATOMiK reconstructs it: current_state = initial_state XOR accumulator. The underlying math forms an Abelian group (commutative, associative, self-inverse, with identity), rigorously proven through 108 Lean4 theorems.",
      },
      {
        q: "What problem does ATOMiK solve?",
        a: "Traditional computing copies full state on every operation, moving enormous amounts of redundant data. ATOMiK eliminates this by tracking only the changes (deltas) between states. This reduces memory traffic by 7,670x to 916,000x across validated workloads, with 22\u201358% execution time improvements.",
      },
      {
        q: "Is ATOMiK open source?",
        a: "The core SDK is open source under the Apache 2.0 license. The kernel module and enterprise features (FPGA IP, hardware acceleration, priority support) are commercially licensed. See our pricing page for details.",
      },
      {
        q: "What languages are supported?",
        a: "ATOMiK provides SDKs for Python, C, JavaScript, Rust, and Verilog, all generated from a single canonical specification via the SDK generator pipeline. Install the Python SDK with pip install atomik-core.",
      },
    ],
  },
  {
    title: "Pricing & Licensing",
    color: "#4f8fff",
    items: [
      {
        q: "Can I use the free tier in production?",
        a: "Yes. The free tier is built on the Apache 2.0 licensed core SDK, which has no restrictions on production use. You can deploy ATOMiK in production applications at no cost.",
      },
      {
        q: "What happens after the 90-day Pro trial?",
        a: "When your Pro trial ends, trial-exclusive features gracefully disable while your data is fully preserved. You can upgrade to a paid Pro plan, downgrade to the free tier, or contact sales for Enterprise options \u2014 all without losing any work.",
      },
      {
        q: "Do I need Enterprise for FPGA development?",
        a: "Yes. FPGA IP cores, hardware acceleration primitives, and the Verilog integration layer require an Enterprise license. This includes the ATOMiK parallel-bank core validated at up to 69.7 Gops/s on Xilinx Zynq.",
      },
      {
        q: "Is there a student or academic discount?",
        a: "We offer academic licensing for qualified institutions and research programs. Contact sales@atomik.tech with your institution details for pricing.",
      },
    ],
  },
  {
    title: "Technical",
    color: "#22d3ee",
    items: [
      {
        q: "Does ATOMiK require a specific operating system?",
        a: "The SDK works everywhere Python, C, or JavaScript runs \u2014 Linux, macOS, and Windows. The kernel module requires Linux 5.15 or later. FPGA integration is supported on Gowin (Tang Nano 9K) and Xilinx (Zynq 7000 series).",
      },
      {
        q: "How does ATOMiK compare to CRDTs?",
        a: "Both ATOMiK and CRDTs enable conflict-free convergence in distributed systems. The key difference is algorithmic complexity: ATOMiK uses O(1) XOR algebra for merges, while CRDTs typically require O(n) merge functions. This makes ATOMiK significantly faster at scale. See our detailed comparison for benchmarks.",
        link: { href: "/compare", label: "View comparison" },
      },
      {
        q: "What's the performance overhead?",
        a: "Near-zero. The Python SDK adds approximately 200 nanoseconds per operation. The C library runs at about 2 nanoseconds per operation. On FPGA hardware, the parallel-bank architecture achieves 69.7 Gops/s. Overhead for tracked memcpy is just 5\u201312% versus plain memcpy.",
        link: { href: "/benchmarks", label: "See benchmarks" },
      },
      {
        q: "Is ATOMiK secure?",
        a: "Security in ATOMiK is architectural, not bolted on. The design provides deterministic latency (eliminating timing side channels), has no speculative execution (eliminating Spectre/Meltdown class attacks), and no cache coherency (eliminating cache-based covert channels). Dynamic reference states provide additional protection.",
        link: { href: "/compliance", label: "View compliance details" },
      },
    ],
  },
  {
    title: "Support",
    color: "#10b981",
    items: [
      {
        q: "How do I get support?",
        a: "Support tiers match your plan: Community users get help via GitHub Discussions. Pro subscribers receive 48-hour email SLA support. Enterprise customers get 4-hour SLA with a dedicated support channel and named account engineer.",
      },
      {
        q: "Where do I report bugs?",
        a: "File bug reports on GitHub Issues. Include your SDK version, language, OS, and a minimal reproduction case for fastest resolution.",
        link: {
          href: "https://github.com/MatthewHRockwell/ATOMiK/issues",
          label: "Open an issue",
          external: true,
        },
      },
      {
        q: "How do I request a feature?",
        a: "Start a thread on GitHub Discussions for community input, or email support@atomik.tech for direct requests. Enterprise customers can request features through their dedicated support channel.",
      },
    ],
  },
];

function FAQItem({
  q,
  a,
  link,
  color,
}: {
  q: string;
  a: string;
  link?: { href: string; label: string; external?: boolean };
  color: string;
}) {
  const [open, setOpen] = useState(false);

  return (
    <div
      className="border rounded-lg transition-colors duration-200"
      style={{
        borderColor: open ? color + "40" : "#1e1e2e",
        background: open ? "#12121a" : "transparent",
      }}
    >
      <button
        onClick={() => setOpen(!open)}
        className="w-full text-left px-5 py-4 flex items-center justify-between gap-4 cursor-pointer"
        aria-expanded={open}
      >
        <span
          className="font-medium text-sm md:text-base"
          style={{ color: "#e0e0e8" }}
        >
          {q}
        </span>
        <span
          className="shrink-0 w-5 h-5 flex items-center justify-center rounded-full text-xs transition-transform duration-200"
          style={{
            color: color,
            transform: open ? "rotate(45deg)" : "rotate(0deg)",
          }}
        >
          <svg
            width="14"
            height="14"
            viewBox="0 0 14 14"
            fill="none"
            stroke="currentColor"
            strokeWidth="2"
            strokeLinecap="round"
          >
            <line x1="7" y1="2" x2="7" y2="12" />
            <line x1="2" y1="7" x2="12" y2="7" />
          </svg>
        </span>
      </button>
      <div
        className="overflow-hidden transition-all duration-300 ease-in-out"
        style={{
          maxHeight: open ? "300px" : "0px",
          opacity: open ? 1 : 0,
        }}
      >
        <div
          className="px-5 pb-4 text-sm leading-relaxed"
          style={{ color: "#8888a0" }}
        >
          <p>{a}</p>
          {link && (
            <p className="mt-3">
              {link.external ? (
                <a
                  href={link.href}
                  target="_blank"
                  rel="noopener noreferrer"
                  className="inline-flex items-center gap-1 text-sm font-medium no-underline hover:opacity-80 transition-opacity"
                  style={{ color: "#4f8fff" }}
                >
                  {link.label}
                  <svg
                    width="12"
                    height="12"
                    viewBox="0 0 12 12"
                    fill="none"
                    stroke="currentColor"
                    strokeWidth="1.5"
                    strokeLinecap="round"
                    strokeLinejoin="round"
                  >
                    <path d="M3.5 1.5H10.5V8.5" />
                    <path d="M10.5 1.5L1.5 10.5" />
                  </svg>
                </a>
              ) : (
                <Link
                  href={link.href}
                  className="inline-flex items-center gap-1 text-sm font-medium no-underline hover:opacity-80 transition-opacity"
                  style={{ color: "#4f8fff" }}
                >
                  {link.label} &rarr;
                </Link>
              )}
            </p>
          )}
        </div>
      </div>
    </div>
  );
}

export default function FAQPage() {
  return (
    <>
      <Nav />

      <main
        className="min-h-screen px-6 py-20"
        style={{ background: "#0a0a0f", color: "#e0e0e8" }}
      >
        <div className="max-w-3xl mx-auto">
          {/* Header */}
          <div className="text-center mb-16">
            <h1 className="text-3xl md:text-4xl font-bold mb-4">
              Frequently Asked Questions
            </h1>
            <p
              className="text-base md:text-lg max-w-xl mx-auto"
              style={{ color: "#8888a0" }}
            >
              Everything you need to know about ATOMiK. Can&apos;t find your
              answer?{" "}
              <Link
                href="mailto:support@atomik.tech"
                className="no-underline hover:opacity-80 transition-opacity"
                style={{ color: "#4f8fff" }}
              >
                Contact support
              </Link>
              .
            </p>
          </div>

          {/* FAQ Categories */}
          <div className="flex flex-col gap-12">
            {faqCategories.map((cat) => (
              <section key={cat.title}>
                <h2 className="flex items-center gap-3 text-lg font-semibold mb-4">
                  <span
                    className="w-2 h-2 rounded-full"
                    style={{ background: cat.color }}
                  />
                  {cat.title}
                </h2>
                <div className="flex flex-col gap-2">
                  {cat.items.map((item) => (
                    <FAQItem
                      key={item.q}
                      q={item.q}
                      a={item.a}
                      link={
                        "link" in item
                          ? (item.link as {
                              href: string;
                              label: string;
                              external?: boolean;
                            })
                          : undefined
                      }
                      color={cat.color}
                    />
                  ))}
                </div>
              </section>
            ))}
          </div>

          {/* CTA */}
          <div
            className="mt-16 rounded-xl p-8 text-center border"
            style={{
              background: "#12121a",
              borderColor: "#1e1e2e",
            }}
          >
            <h3 className="text-xl font-semibold mb-2">Still have questions?</h3>
            <p className="text-sm mb-5" style={{ color: "#8888a0" }}>
              Our team is here to help. Reach out and we&apos;ll get back to you
              within 24 hours.
            </p>
            <div className="flex flex-wrap justify-center gap-3">
              <Link
                href="mailto:support@atomik.tech"
                className="px-5 py-2 rounded-full text-sm font-semibold text-white no-underline transition-opacity hover:opacity-85"
                style={{
                  background: "linear-gradient(135deg, #22d3ee, #4f8fff)",
                  boxShadow: "0 2px 12px rgba(79,143,255,0.3)",
                }}
              >
                Contact Support
              </Link>
              <a
                href="https://github.com/MatthewHRockwell/ATOMiK/discussions"
                target="_blank"
                rel="noopener noreferrer"
                className="px-5 py-2 rounded-full text-sm font-semibold no-underline transition-colors hover:text-white border"
                style={{ color: "#8888a0", borderColor: "#1e1e2e" }}
              >
                GitHub Discussions
              </a>
            </div>
          </div>
        </div>
      </main>

      <Footer />
    </>
  );
}
