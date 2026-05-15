"use client";

import Link from "next/link";
import Nav from "@/components/Nav";
import Footer from "@/components/Footer";
import { useState } from "react";

const faqCategories = [
  {
    title: "General",
    color: "#22d3ee",
    items: [
      {
        q: "What is ATOMiK?",
        a: "ATOMiK is a state-aware compute architecture for systems that spend too much work rediscovering what changed. The public materials cover the live hardware prototype direction, proof artifacts, developer adoption path, and roadmap toward ATOMiK Desk and Resource Fabric.",
      },
      {
        q: "What problem does ATOMiK solve?",
        a: "State-heavy systems repeatedly move, replay, rescan, or reconstruct state even when only a small part changed. ATOMiK makes state change, delta application, and adaptive execution first-class primitives so a workload can be evaluated around the work that actually changed.",
      },
      {
        q: "What is public today?",
        a: "The repository includes website, docs, proof notes, software, SDK, math, hardware, and live prototype implementation areas. Public claims are labeled as measured, hardware-validated, software-validated, synthesis-validated, projected, conceptual, or roadmap.",
        link: { href: "https://github.com/MatthewHRockwell/ATOMiK", label: "View GitHub", external: true },
      },
      {
        q: "Is ATOMiK Desk commercially available today?",
        a: "No. ATOMiK Desk is a live prototype and demonstration surface for state-aware compute. Live screenshots show prototype progress; concept visuals show product direction and are not represented as commercial functionality.",
      },
    ],
  },
  {
    title: "Evaluation",
    color: "#4f8fff",
    items: [
      {
        q: "Is there a public free tier?",
        a: "No conventional public free tier is listed. ATOMiK is currently request-based so evaluation time is focused on real workloads, evidence review, and technical fit.",
        link: { href: "/pricing", label: "View evaluation options" },
      },
      {
        q: "What are the public evaluation paths?",
        a: "There are two public paths: Evaluation Access for technical review and early demo availability, and Design Partner / Paid Technical Evaluation for teams with a real state-heavy workload and defined success criteria.",
        link: { href: "/contact?intent=evaluation", label: "Request evaluation access" },
      },
      {
        q: "Do I need custom hardware to evaluate ATOMiK?",
        a: "Not necessarily. The right first step may be proof review, software exploration, a benchmark exchange, or a hardware-backed demo depending on the workload and available artifacts.",
      },
      {
        q: "Can researchers or technical founders request access?",
        a: "Yes. Evaluation Access is intended for technical founders, engineers, researchers, infrastructure teams, and early evaluators who can review evidence and provide useful workload feedback.",
      },
    ],
  },
  {
    title: "Technical",
    color: "#22d3ee",
    items: [
      {
        q: "Does ATOMiK require a specific operating system?",
        a: "Public software and proof materials can be reviewed in conventional development environments. The live hardware prototype path is currently documented around Zynq / NaxRiscv / Linux artifacts.",
      },
      {
        q: "How does ATOMiK compare to CRDTs?",
        a: "Both address state synchronization problems, but ATOMiK frames state change and delta application as compute primitives. Public comparisons should be read with the evidence labels and artifact links attached to any performance claim.",
        link: { href: "/compare", label: "View comparison" },
      },
      {
        q: "What performance claims are public-safe?",
        a: "Performance claims are public-safe only when linked to measured artifacts or clearly labeled synthesis/modeling notes. The current AX7020 matrix is intentionally caveated: naive hardware access can lose, while batching and workload personality choices determine whether the architectural path helps.",
        link: { href: "/benchmarks", label: "View proof artifacts" },
      },
      {
        q: "Is ATOMiK a certified security product?",
        a: "No public certification claim is made. Some architecture properties may be relevant to security-sensitive systems, but security, compliance, and safety claims require scoped review and artifact-backed validation.",
        link: { href: "/compliance", label: "View compliance details" },
      },
    ],
  },
  {
    title: "Support",
    color: "#22d3ee",
    items: [
      {
        q: "How do I get support?",
        a: "For public evaluation, use the contact form and include the workload, platform, and evidence you want reviewed. GitHub issues are appropriate for repo bugs and documentation problems.",
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
        a: "Feature requests should be tied to a concrete state-heavy workload or evaluation goal. Design partner conversations are the best path when the request affects roadmap, prototype mapping, or evaluation deliverables.",
        link: { href: "/contact?intent=design-partner", label: "Discuss design partnership" },
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
              Public answers about evaluation access, proof labels, and current
              prototype status. Can&apos;t find your answer?{" "}
              <Link
                href="/contact?intent=question"
                className="no-underline hover:opacity-80 transition-opacity"
                style={{ color: "#4f8fff" }}
              >
                Contact ATOMiK
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
              Bring a workload, proof question, or evaluation goal and ATOMiK
              will route it to the right next step.
            </p>
            <div className="flex flex-wrap justify-center gap-3">
              <Link
                href="/contact?intent=evaluation"
                className="px-5 py-2 rounded-full text-sm font-semibold text-white no-underline transition-opacity hover:opacity-85"
                style={{
                  background: "#4f8fff",
                  boxShadow: "0 2px 12px rgba(79,143,255,0.3)",
                }}
              >
                Request Evaluation Access
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
