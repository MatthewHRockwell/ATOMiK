import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "Security & Compliance — ATOMiK",
  description:
    "ATOMiK security posture, compliance certifications, data handling practices, GDPR, SOC 2, and formal verification. Enterprise-ready delta-state computing.",
  openGraph: {
    title: "Security & Compliance — ATOMiK",
    description:
      "Enterprise security and compliance for delta-state computing. Local data processing, 108 formally verified theorems, GDPR compliant.",
  },
};

const certifications = [
  {
    name: "Apache 2.0 License",
    status: "Available",
    statusColor: "#22c55e",
    description: "Core SDK is fully open source and auditable",
    icon: "A2",
  },
  {
    name: "Lean4 Formal Proofs",
    status: "108 Theorems Verified",
    statusColor: "#8b5cf6",
    description: "Mathematical correctness proven, not assumed",
    icon: "L4",
  },
  {
    name: "SOC 2 Type II",
    status: "In Progress",
    statusColor: "#d4a843",
    description: "Pursuing SOC 2 Type II certification",
    icon: "S2",
  },
  {
    name: "GDPR Compliant",
    status: "Yes",
    statusColor: "#22c55e",
    description: "Minimal data collection, right to deletion",
    icon: "EU",
  },
  {
    name: "PCI DSS",
    status: "Via Stripe",
    statusColor: "#22c55e",
    description: "Payment data handled by Stripe, never touches our servers",
    icon: "PC",
  },
  {
    name: "ISO 27001",
    status: "Planned",
    statusColor: "#8888a0",
    description: "Information security management certification",
    icon: "IS",
  },
];

const sections = [
  {
    title: "Data Handling",
    color: "#4f8fff",
    items: [
      "ATOMiK processes data locally — no cloud dependency. Your data never leaves your infrastructure.",
      "The kernel module operates at OS level on your machines. The SDK is a local library imported into your application.",
      "No telemetry phones home. No data is transmitted to ATOMiK servers during normal operation.",
      "You maintain full custody of your data at all times.",
    ],
  },
  {
    title: "Open Source Transparency",
    color: "#22c55e",
    items: [
      "The core SDK is licensed under Apache 2.0 — fully auditable by your security team.",
      "108 Lean4 formal proofs verify the mathematical correctness of delta-state algebra. These are machine-checked, not hand-reviewed.",
      "All source code is available on GitHub for independent security review.",
      "No obfuscated binaries. No proprietary black boxes in the critical path.",
    ],
  },
  {
    title: "Encryption & Payment Security",
    color: "#22d3ee",
    items: [
      "All data in transit uses TLS 1.3.",
      "Stripe handles all payment data processing. ATOMiK is PCI DSS compliant via Stripe — we never see, store, or process credit card numbers.",
      "No user data is stored on ATOMiK servers beyond email addresses for licensing.",
      "API keys are scoped, rotatable, and transmitted only over encrypted channels.",
    ],
  },
  {
    title: "GDPR Compliance",
    color: "#8b5cf6",
    items: [
      "Minimal data collection: only email addresses for licensing and communication.",
      "Right to deletion is fully supported — request removal at any time.",
      "Data Processing Agreement (DPA) available on request for enterprise customers.",
      "No personal data is shared with third parties beyond payment processing (Stripe).",
    ],
  },
];

export default function CompliancePage() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav />

      {/* Hero Section */}
      <section className="relative overflow-hidden">
        <div
          className="absolute inset-0 opacity-20"
          style={{
            background:
              "radial-gradient(ellipse 60% 40% at 50% 0%, #4f8fff40, transparent), radial-gradient(ellipse 40% 30% at 80% 20%, #22c55e30, transparent)",
          }}
        />
        <div className="max-w-5xl mx-auto px-6 pt-24 pb-16 relative">
          <p
            className="text-sm font-mono tracking-widest uppercase mb-4"
            style={{ color: "#4f8fff" }}
          >
            Security &amp; Compliance
          </p>
          <h1 className="text-5xl sm:text-6xl font-bold tracking-tight mb-6">
            Enterprise{" "}
            <span
              className="bg-clip-text text-transparent"
              style={{
                backgroundImage: "linear-gradient(135deg, #4f8fff, #22c55e)",
              }}
            >
              Security
            </span>
          </h1>
          <p className="text-xl leading-relaxed max-w-3xl" style={{ color: "#8888a0" }}>
            ATOMiK is built for environments where security is non-negotiable. Local data
            processing, formally verified mathematics, and no cloud dependency.
          </p>
        </div>
      </section>

      {/* Core Principle Banner */}
      <section className="max-w-5xl mx-auto px-6 pb-16">
        <div
          className="rounded-2xl p-8 sm:p-12 border"
          style={{
            background: "linear-gradient(135deg, #12121a, #181824)",
            borderColor: "#1e1e2e",
          }}
        >
          <h2 className="text-2xl font-bold mb-6" style={{ color: "#e0e0e8" }}>
            Security by Architecture
          </h2>
          <div className="space-y-4 text-lg leading-relaxed" style={{ color: "#b0b0c0" }}>
            <p>
              ATOMiK does not bolt security onto an existing architecture.{" "}
              <span className="font-semibold text-white">
                Security is a consequence of the design itself.
              </span>{" "}
              Delta-state algebra uses dynamic reference states — there is no static secret
              to steal. Deterministic latency eliminates timing side channels. No speculative
              execution means no Spectre-class attacks. No cache coherency protocol means no
              Meltdown-class attacks.
            </p>
            <p>
              The mathematical foundation is not assumed — it is{" "}
              <span className="font-semibold text-white">
                proven in 108 Lean4 theorems
              </span>
              , machine-verified to be correct by construction. This is a level of assurance
              that testing alone cannot provide.
            </p>
          </div>
        </div>
      </section>

      {/* Policy Sections */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
          {sections.map((section) => (
            <div
              key={section.title}
              className="rounded-xl p-8 border transition-all duration-300 hover:-translate-y-1"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <div className="flex items-center gap-3 mb-5">
                <div
                  className="w-2 h-2 rounded-full"
                  style={{ background: section.color }}
                />
                <h3 className="text-xl font-bold" style={{ color: section.color }}>
                  {section.title}
                </h3>
              </div>
              <ul className="space-y-3">
                {section.items.map((item, i) => (
                  <li key={i} className="flex gap-3 text-sm leading-relaxed" style={{ color: "#b0b0c0" }}>
                    <span className="mt-1.5 shrink-0 w-1.5 h-1.5 rounded-full" style={{ background: "#333" }} />
                    {item}
                  </li>
                ))}
              </ul>
            </div>
          ))}
        </div>
      </section>

      {/* Formal Verification Highlight */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <div
          className="rounded-2xl p-8 sm:p-12 border"
          style={{
            background: "linear-gradient(135deg, rgba(139,92,246,0.08), rgba(79,143,255,0.08))",
            borderColor: "#8b5cf630",
          }}
        >
          <div className="flex flex-col sm:flex-row gap-8 items-start">
            <div
              className="w-20 h-20 rounded-xl flex items-center justify-center text-2xl font-bold font-mono shrink-0"
              style={{
                background: "linear-gradient(135deg, #8b5cf6, #4f8fff)",
              }}
            >
              92
            </div>
            <div>
              <h3 className="text-2xl font-bold text-white mb-3">
                Formal Verification: A Security Differentiator
              </h3>
              <p className="leading-relaxed mb-4" style={{ color: "#b0b0c0" }}>
                ATOMiK&apos;s delta-state algebra is not just tested — it is formally proven in
                Lean4, a theorem prover used by mathematicians and verified-software researchers.
                Every property of the Abelian group (commutativity, associativity, self-inverse,
                identity) is machine-checked. This means the core algorithm is correct by
                construction — not by convention, not by code review, not by test coverage.
              </p>
              <div className="flex flex-wrap gap-3">
                <Link
                  href="https://github.com/MatthewHRockwell/ATOMiK"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="inline-flex items-center gap-2 px-4 py-2 rounded-lg text-sm font-medium transition-colors hover:bg-white/5"
                  style={{ border: "1px solid #1e1e2e", color: "#8b5cf6" }}
                >
                  View Lean4 Proofs on GitHub &rarr;
                </Link>
                <Link
                  href="/whitepaper"
                  className="inline-flex items-center gap-2 px-4 py-2 rounded-lg text-sm font-medium transition-colors hover:bg-white/5"
                  style={{ border: "1px solid #1e1e2e", color: "#4f8fff" }}
                >
                  Read the Whitepaper &rarr;
                </Link>
              </div>
            </div>
          </div>
        </div>
      </section>

      {/* Certifications Grid */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold mb-4">Certifications &amp; Standards</h2>
        <p className="mb-10 text-lg" style={{ color: "#8888a0" }}>
          Current compliance posture and certification roadmap.
        </p>

        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4">
          {certifications.map((cert) => (
            <div
              key={cert.name}
              className="rounded-xl p-6 border transition-all duration-300 hover:-translate-y-1"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <div className="flex items-start justify-between mb-4">
                <div
                  className="w-12 h-12 rounded-lg flex items-center justify-center text-sm font-bold font-mono"
                  style={{ background: "#1e1e2e", color: cert.statusColor }}
                >
                  {cert.icon}
                </div>
                <span
                  className="text-xs font-mono font-bold px-3 py-1 rounded-full"
                  style={{
                    color: cert.statusColor,
                    background: `${cert.statusColor}15`,
                    border: `1px solid ${cert.statusColor}30`,
                  }}
                >
                  {cert.status}
                </span>
              </div>
              <h3 className="font-bold text-white mb-1">{cert.name}</h3>
              <p className="text-sm" style={{ color: "#8888a0" }}>
                {cert.description}
              </p>
            </div>
          ))}
        </div>
      </section>

      {/* SOC 2 In Progress Banner */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <div
          className="rounded-xl p-8 border flex flex-col sm:flex-row items-start sm:items-center gap-6"
          style={{ background: "#12121a", borderColor: "#d4a84330" }}
        >
          <div
            className="w-14 h-14 rounded-full flex items-center justify-center text-xl font-bold shrink-0"
            style={{ background: "rgba(212, 168, 67, 0.15)", color: "#d4a843" }}
          >
            S2
          </div>
          <div className="flex-1">
            <h3 className="text-lg font-bold text-white mb-1">SOC 2 Type II — In Progress</h3>
            <p className="text-sm leading-relaxed" style={{ color: "#8888a0" }}>
              ATOMiK is actively pursuing SOC 2 Type II certification. This covers security,
              availability, processing integrity, confidentiality, and privacy controls. Contact
              us for current status and expected completion timeline.
            </p>
          </div>
        </div>
      </section>

      {/* Security Contact */}
      <section className="max-w-5xl mx-auto px-6 pb-24">
        <div
          className="rounded-2xl p-8 sm:p-12 border text-center"
          style={{
            background: "linear-gradient(135deg, rgba(79,143,255,0.08), rgba(34,197,94,0.08))",
            borderColor: "#4f8fff30",
          }}
        >
          <h2 className="text-2xl font-bold mb-3">Responsible Disclosure</h2>
          <p className="mb-6 max-w-2xl mx-auto" style={{ color: "#8888a0" }}>
            Found a security issue? We take every report seriously. Reach out to our security
            team directly — we respond within 24 hours.
          </p>
          <a
            href="mailto:security@atomik.tech"
            className="inline-flex items-center gap-2 px-6 py-3 rounded-lg font-semibold text-white transition-all duration-300 hover:scale-105"
            style={{
              background: "linear-gradient(135deg, #4f8fff, #22c55e)",
            }}
          >
            security@atomik.tech
          </a>
          <p className="mt-4 text-sm" style={{ color: "#8888a0" }}>
            For general inquiries, contact{" "}
            <a
              href="mailto:mrockwell@atomik.tech"
              className="hover:underline"
              style={{ color: "#4f8fff" }}
            >
              mrockwell@atomik.tech
            </a>
          </p>
        </div>
      </section>
    </div>
  );
}
