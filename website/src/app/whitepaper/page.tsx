"use client";

import { useState, useEffect, FormEvent } from "react";
import Link from "next/link";
import Nav from "@/components/Nav";

const LOCALSTORAGE_KEY = "atomik_whitepaper_lead";

const coverageItems = [
  {
    title: "Mathematical Foundations",
    desc: "Abelian group properties (commutativity, associativity, self-inverse, identity) with 108 formally verified Lean4 theorems.",
    color: "#8b5cf6",
  },
  {
    title: "Hardware Architecture",
    desc: "From PicoRV32 on a $13.50 FPGA to custom RV64I with ATOMiK ISA extensions to Zynq scaling at 69.7 Gops/s with 512 parallel banks.",
    color: "#4f8fff",
  },
  {
    title: "Production Benchmarks",
    desc: "7,670x to 916,000x memory traffic reduction. Deterministic latency with zero timing side channels.",
    color: "#22c55e",
  },
  {
    title: "Kernel Module Architecture",
    desc: "Copy-on-write detection via kretprobe, TCP send deduplication with CRC32C, per-cgroup waste tracking, 27 real-time sysfs metrics.",
    color: "#d4a843",
  },
  {
    title: "Comparison Analysis",
    desc: "ATOMiK vs CRDTs, event sourcing, and consensus protocols. Where delta-state algebra converges and where it diverges.",
    color: "#22d3ee",
  },
  {
    title: "ASIC Development Roadmap",
    desc: "The path from FPGA validation to custom silicon. Scaling projections, target process nodes, and production timeline.",
    color: "#f472b6",
  },
];

const stats = [
  { value: "108", label: "Lean4 Theorems Proven", color: "#8b5cf6" },
  { value: "69.7 Gops/s", label: "Peak FPGA Throughput", color: "#4f8fff" },
  { value: "500+", label: "Tests Passing", color: "#22c55e" },
  { value: "916,000x", label: "Memory Traffic Reduction", color: "#d4a843" },
];

export default function WhitepaperPage() {
  const [name, setName] = useState("");
  const [email, setEmail] = useState("");
  const [company, setCompany] = useState("");
  const [role, setRole] = useState("");
  const [status, setStatus] = useState<
    "idle" | "loading" | "success" | "error"
  >("idle");
  const [errorMsg, setErrorMsg] = useState("");

  // Restore access for returning visitors
  useEffect(() => {
    try {
      const stored = localStorage.getItem(LOCALSTORAGE_KEY);
      if (stored) {
        const lead = JSON.parse(stored);
        if (lead?.email) {
          setName(lead.name || "");
          setEmail(lead.email);
          setStatus("success");
        }
      }
    } catch {
      // localStorage unavailable or corrupt — ignore
    }
  }, []);

  async function handleSubmit(e: FormEvent) {
    e.preventDefault();
    if (!email.trim() || !name.trim()) return;

    setStatus("loading");
    setErrorMsg("");

    try {
      const res = await fetch("/api/subscribe", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          email: email.trim(),
          name: name.trim(),
          company: company.trim(),
          role: role || undefined,
          source: "whitepaper",
        }),
      });

      const data = await res.json();

      if (!res.ok) {
        setStatus("error");
        setErrorMsg(data.error || "Something went wrong");
        return;
      }

      // Store lead in localStorage so returning visitors skip the form
      try {
        localStorage.setItem(
          LOCALSTORAGE_KEY,
          JSON.stringify({
            name: name.trim(),
            email: email.trim(),
            company: company.trim(),
            role,
            ts: Date.now(),
          })
        );
      } catch {
        // localStorage unavailable — non-fatal
      }

      setStatus("success");
    } catch {
      setStatus("error");
      setErrorMsg("Network error. Please try again.");
    }
  }

  return (
    <div
      className="min-h-screen"
      style={{ background: "#0a0a0f", color: "#e0e0e8" }}
    >
      <Nav />

      {/* Hero */}
      <section className="text-center px-6 pt-20 pb-12">
        <p
          className="text-sm font-mono tracking-widest uppercase mb-4"
          style={{ color: "#4f8fff" }}
        >
          Technical White Paper
        </p>
        <h1 className="text-4xl sm:text-5xl font-bold tracking-tight mb-5">
          ATOMiK Technical White Paper
        </h1>
        <p
          className="text-lg sm:text-xl max-w-3xl mx-auto leading-relaxed"
          style={{ color: "#8888a0" }}
        >
          Delta-State Algebra: Mathematical Foundations, Hardware
          Implementation, and Production Results
        </p>
      </section>

      {/* Stats bar */}
      <section className="max-w-4xl mx-auto px-6 pb-12">
        <div
          className="grid grid-cols-2 sm:grid-cols-4 gap-4 rounded-xl border p-6"
          style={{ background: "#12121a", borderColor: "#1e1e2e" }}
        >
          {stats.map((s) => (
            <div key={s.label} className="text-center">
              <div
                className="text-2xl sm:text-3xl font-bold font-mono"
                style={{ color: s.color }}
              >
                {s.value}
              </div>
              <div
                className="text-xs mt-1 leading-tight"
                style={{ color: "#8888a0" }}
              >
                {s.label}
              </div>
            </div>
          ))}
        </div>
      </section>

      {/* Main content: two-column */}
      <section className="max-w-5xl mx-auto px-6 pb-16">
        <div className="grid lg:grid-cols-5 gap-10">
          {/* Left column: what's inside + form */}
          <div className="lg:col-span-3">
            {/* What's inside */}
            <h2 className="text-2xl font-bold mb-6">
              What&apos;s inside
            </h2>
            <div className="space-y-4 mb-10">
              {coverageItems.map((item) => (
                <div
                  key={item.title}
                  className="rounded-xl p-5 border flex gap-4"
                  style={{ background: "#12121a", borderColor: "#1e1e2e" }}
                >
                  <div
                    className="mt-1 w-2 h-2 rounded-full shrink-0"
                    style={{ background: item.color }}
                  />
                  <div>
                    <h3
                      className="font-semibold text-sm mb-1"
                      style={{ color: item.color }}
                    >
                      {item.title}
                    </h3>
                    <p className="text-sm" style={{ color: "#8888a0" }}>
                      {item.desc}
                    </p>
                  </div>
                </div>
              ))}
            </div>

            {/* Email gate form */}
            {status !== "success" && (
              <div
                id="download"
                className="rounded-xl p-8 border"
                style={{
                  background: "linear-gradient(135deg, #12121a, #181824)",
                  borderColor: "#4f8fff33",
                  boxShadow: "0 0 40px rgba(79,143,255,0.06)",
                }}
              >
                <h3 className="text-xl font-bold text-white mb-2">
                  Read the White Paper
                </h3>
                <p
                  className="text-sm mb-6"
                  style={{ color: "#8888a0" }}
                >
                  Fill in the short form below for instant access to
                  the full technical deep-dive. No waiting, no approval.
                </p>
                <form onSubmit={handleSubmit} className="space-y-3">
                  <input
                    type="text"
                    placeholder="Your name *"
                    value={name}
                    onChange={(e) => setName(e.target.value)}
                    required
                    className="w-full px-4 py-3 rounded-lg text-sm text-white placeholder-gray-500 outline-none focus:ring-2 focus:ring-[#4f8fff]"
                    style={{
                      background: "#0a0a0f",
                      border: "1px solid #1e1e2e",
                    }}
                  />
                  <input
                    type="email"
                    placeholder="you@example.com *"
                    value={email}
                    onChange={(e) => {
                      setEmail(e.target.value);
                      if (status === "error") setStatus("idle");
                    }}
                    required
                    className="w-full px-4 py-3 rounded-lg text-sm text-white placeholder-gray-500 outline-none focus:ring-2 focus:ring-[#4f8fff]"
                    style={{
                      background: "#0a0a0f",
                      border: "1px solid #1e1e2e",
                    }}
                  />
                  <input
                    type="text"
                    placeholder="Company (optional)"
                    value={company}
                    onChange={(e) => setCompany(e.target.value)}
                    className="w-full px-4 py-3 rounded-lg text-sm text-white placeholder-gray-500 outline-none focus:ring-2 focus:ring-[#4f8fff]"
                    style={{
                      background: "#0a0a0f",
                      border: "1px solid #1e1e2e",
                    }}
                  />
                  <select
                    value={role}
                    onChange={(e) => setRole(e.target.value)}
                    className="w-full px-4 py-3 rounded-lg text-sm outline-none focus:ring-2 focus:ring-[#4f8fff] appearance-none"
                    style={{
                      background: "#0a0a0f",
                      border: "1px solid #1e1e2e",
                      color: role ? "#ffffff" : "#6b7280",
                    }}
                  >
                    <option value="" disabled>Role (optional)</option>
                    <option value="Engineer">Engineer</option>
                    <option value="CTO/VP">CTO / VP Engineering</option>
                    <option value="Researcher">Researcher</option>
                    <option value="Student">Student</option>
                    <option value="Other">Other</option>
                  </select>
                  <button
                    type="submit"
                    disabled={status === "loading"}
                    className="w-full py-3 rounded-lg text-sm font-semibold text-white transition-opacity hover:opacity-90 disabled:opacity-50"
                    style={{ background: "#4f8fff" }}
                  >
                    {status === "loading"
                      ? "Processing..."
                      : "Get Instant Access"}
                  </button>
                </form>
                {status === "error" && (
                  <p
                    className="text-sm mt-3"
                    style={{ color: "#ef4444" }}
                  >
                    {errorMsg}
                  </p>
                )}
                <p
                  className="text-xs mt-4 text-center"
                  style={{ color: "#555566" }}
                >
                  No spam. Unsubscribe anytime. Read our{" "}
                  <Link
                    href="/privacy"
                    className="underline hover:text-white transition-colors"
                  >
                    privacy policy
                  </Link>
                  .
                </p>
                <p
                  className="text-xs mt-3 text-center"
                  style={{ color: "#555566" }}
                >
                  Having trouble? Email{" "}
                  <a
                    href="mailto:mrockwell@atomik.tech?subject=White Paper Request"
                    className="underline hover:text-white transition-colors"
                    style={{ color: "#4f8fff" }}
                  >
                    mrockwell@atomik.tech
                  </a>
                  {" "}and we&apos;ll send it directly.
                </p>
              </div>
            )}

            {/* Unlocked whitepaper content */}
            {status === "success" && (
              <div className="space-y-8">
                {/* Access banner */}
                <div
                  className="rounded-xl p-6 border flex flex-col sm:flex-row items-start sm:items-center justify-between gap-4"
                  style={{
                    background: "linear-gradient(135deg, #12121a, #181824)",
                    borderColor: "#22c55e33",
                  }}
                >
                  <div className="flex items-center gap-3">
                    <span
                      className="text-xl"
                      style={{ color: "#22c55e" }}
                    >
                      &#10003;
                    </span>
                    <div>
                      <h3 className="text-lg font-bold text-white">
                        White Paper Unlocked
                      </h3>
                      <p className="text-xs" style={{ color: "#8888a0" }}>
                        Full content below. Download the PDF or bookmark this page.
                      </p>
                    </div>
                  </div>
                  <div className="flex gap-2 shrink-0">
                    <a
                      href="/ATOMiK_White_Paper.pdf"
                      download
                      className="inline-flex items-center gap-2 px-5 py-2.5 rounded-lg text-sm font-semibold text-white transition-opacity hover:opacity-90"
                      style={{ background: "#4f8fff" }}
                    >
                      Download PDF
                    </a>
                    <a
                      href="/ATOMiK_Benchmarks.pdf"
                      download
                      className="inline-flex items-center gap-2 px-4 py-2.5 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
                      style={{ background: "transparent", color: "#4f8fff", border: "1px solid #4f8fff33" }}
                    >
                      Benchmarks PDF
                    </a>
                  </div>
                </div>

                {/* Print-friendly whitepaper wrapper */}
                <div className="whitepaper-content">
                  {/* Section 1: Abstract */}
                  <div
                    className="rounded-xl p-8 border mb-6"
                    style={{ background: "#12121a", borderColor: "#1e1e2e" }}
                  >
                    <h2
                      className="text-xl font-bold mb-4"
                      style={{ color: "#8b5cf6" }}
                    >
                      Abstract
                    </h2>
                    <div
                      className="space-y-3 text-sm leading-relaxed"
                      style={{ color: "#b0b0c0" }}
                    >
                      <p>
                        ATOMiK introduces a fundamentally new approach to computer state management.
                        Rather than storing state in memory and mutating it in place (the Von Neumann model
                        that has dominated computing since 1945), ATOMiK reconstructs state algebraically:{" "}
                        <code
                          className="px-1.5 py-0.5 rounded text-xs font-mono"
                          style={{ background: "#0d0d14", color: "#4f8fff" }}
                        >
                          current_state = initial_state XOR accumulator
                        </code>.
                      </p>
                      <p>
                        This paper presents the mathematical foundations (108 formally verified Lean4 theorems),
                        hardware implementation across three FPGA platforms, production benchmarks showing
                        7,670x to 916,000x memory traffic reduction, and the Linux kernel module architecture
                        for transparent integration with existing systems.
                      </p>
                    </div>
                  </div>

                  {/* Section 2: Mathematical Foundations */}
                  <div
                    className="rounded-xl p-8 border mb-6"
                    style={{ background: "#12121a", borderColor: "#1e1e2e" }}
                  >
                    <h2
                      className="text-xl font-bold mb-4"
                      style={{ color: "#8b5cf6" }}
                    >
                      1. Mathematical Foundations
                    </h2>
                    <div
                      className="space-y-4 text-sm leading-relaxed"
                      style={{ color: "#b0b0c0" }}
                    >
                      <p>
                        ATOMiK&apos;s delta-state algebra forms an <strong className="text-white">Abelian group</strong> under
                        the XOR operation. This provides four properties that are not merely convenient but
                        architecturally load-bearing:
                      </p>
                      <div
                        className="rounded-lg p-5 space-y-3"
                        style={{ background: "#0d0d14" }}
                      >
                        <div>
                          <span className="font-mono text-xs" style={{ color: "#4f8fff" }}>Commutativity</span>
                          <span className="text-xs" style={{ color: "#8888a0" }}> &mdash; </span>
                          <code className="text-xs font-mono" style={{ color: "#22c55e" }}>a XOR b = b XOR a</code>
                          <p className="text-xs mt-1" style={{ color: "#8888a0" }}>
                            Order of delta accumulation does not affect the result. Multiple producers can
                            feed deltas in any order.
                          </p>
                        </div>
                        <div>
                          <span className="font-mono text-xs" style={{ color: "#4f8fff" }}>Associativity</span>
                          <span className="text-xs" style={{ color: "#8888a0" }}> &mdash; </span>
                          <code className="text-xs font-mono" style={{ color: "#22c55e" }}>(a XOR b) XOR c = a XOR (b XOR c)</code>
                          <p className="text-xs mt-1" style={{ color: "#8888a0" }}>
                            Grouping does not matter. Enables arbitrary parallelism in accumulation.
                          </p>
                        </div>
                        <div>
                          <span className="font-mono text-xs" style={{ color: "#4f8fff" }}>Self-Inverse</span>
                          <span className="text-xs" style={{ color: "#8888a0" }}> &mdash; </span>
                          <code className="text-xs font-mono" style={{ color: "#22c55e" }}>a XOR a = 0</code>
                          <p className="text-xs mt-1" style={{ color: "#8888a0" }}>
                            Every element is its own inverse. No separate &quot;undo&quot; operation needed.
                          </p>
                        </div>
                        <div>
                          <span className="font-mono text-xs" style={{ color: "#4f8fff" }}>Identity</span>
                          <span className="text-xs" style={{ color: "#8888a0" }}> &mdash; </span>
                          <code className="text-xs font-mono" style={{ color: "#22c55e" }}>a XOR 0 = a</code>
                          <p className="text-xs mt-1" style={{ color: "#8888a0" }}>
                            Zero accumulator means &quot;no changes observed.&quot; State equals initial state.
                          </p>
                        </div>
                      </div>
                      <p>
                        All 108 theorems covering these properties and their composition have been formally
                        verified in the Lean4 proof assistant. This is not testing &mdash; it is mathematical
                        proof that the algebra is correct for all possible inputs.
                      </p>
                    </div>
                  </div>

                  {/* Section 3: Core Architecture */}
                  <div
                    className="rounded-xl p-8 border mb-6"
                    style={{ background: "#12121a", borderColor: "#1e1e2e" }}
                  >
                    <h2
                      className="text-xl font-bold mb-4"
                      style={{ color: "#4f8fff" }}
                    >
                      2. Hardware Architecture
                    </h2>
                    <div
                      className="space-y-4 text-sm leading-relaxed"
                      style={{ color: "#b0b0c0" }}
                    >
                      <p>
                        The ATOMiK hardware core implements exactly <strong className="text-white">four operations</strong>:
                      </p>
                      <div className="grid grid-cols-2 gap-3">
                        {[
                          { op: "LOAD", desc: "Set initial reference state", color: "#8b5cf6" },
                          { op: "ACCUM", desc: "XOR delta into accumulator", color: "#4f8fff" },
                          { op: "READ", desc: "Reconstruct current state", color: "#22c55e" },
                          { op: "SWAP", desc: "Atomic checkpoint and reset", color: "#d4a843" },
                        ].map((o) => (
                          <div
                            key={o.op}
                            className="rounded-lg p-3"
                            style={{ background: "#0d0d14" }}
                          >
                            <span className="font-mono text-xs font-bold" style={{ color: o.color }}>
                              {o.op}
                            </span>
                            <p className="text-xs mt-1" style={{ color: "#8888a0" }}>{o.desc}</p>
                          </div>
                        ))}
                      </div>
                      <p>
                        This minimalist ISA maps to three RTL modules: <code className="text-xs font-mono" style={{ color: "#4f8fff" }}>atomik_delta_acc</code> (accumulator),{" "}
                        <code className="text-xs font-mono" style={{ color: "#4f8fff" }}>atomik_state_rec</code> (state reconstructor), and{" "}
                        <code className="text-xs font-mono" style={{ color: "#4f8fff" }}>atomik_core_v2</code> (top-level wrapper with bank parameterization).
                      </p>

                      <h3 className="font-semibold text-white pt-2">Validated Platforms</h3>
                      <div className="overflow-x-auto">
                        <table className="w-full text-xs" style={{ color: "#b0b0c0" }}>
                          <thead>
                            <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                              <th className="text-left py-2 pr-4 font-semibold text-white">Platform</th>
                              <th className="text-left py-2 pr-4 font-semibold text-white">Banks</th>
                              <th className="text-left py-2 pr-4 font-semibold text-white">Fmax</th>
                              <th className="text-left py-2 font-semibold text-white">Throughput</th>
                            </tr>
                          </thead>
                          <tbody>
                            <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                              <td className="py-2 pr-4">Tang Nano 9K (GW1NR-9K)</td>
                              <td className="py-2 pr-4" style={{ color: "#8b5cf6" }}>1</td>
                              <td className="py-2 pr-4">100.2 MHz</td>
                              <td className="py-2">94.5 Mops/s</td>
                            </tr>
                            <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                              <td className="py-2 pr-4">Tang Nano 9K (16-bank)</td>
                              <td className="py-2 pr-4" style={{ color: "#8b5cf6" }}>16</td>
                              <td className="py-2 pr-4">66 MHz</td>
                              <td className="py-2">1,056 Mops/s</td>
                            </tr>
                            <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                              <td className="py-2 pr-4">Zynq XC7Z020 (N=1)</td>
                              <td className="py-2 pr-4" style={{ color: "#4f8fff" }}>1</td>
                              <td className="py-2 pr-4">444.4 MHz</td>
                              <td className="py-2">446 Mops/s</td>
                            </tr>
                            <tr>
                              <td className="py-2 pr-4">Zynq XC7Z020 (N=512)</td>
                              <td className="py-2 pr-4" style={{ color: "#4f8fff" }}>512</td>
                              <td className="py-2 pr-4">135.6 MHz</td>
                              <td className="py-2" style={{ color: "#22c55e" }}>69.7 Gops/s</td>
                            </tr>
                          </tbody>
                        </table>
                      </div>
                      <p>
                        LUT scaling is sub-linear: 3.7x resource growth for 16x throughput on Gowin,
                        and approximately 34 marginal LUTs per additional bank on Zynq. All configurations
                        share a single BRAM instance.
                      </p>
                    </div>
                  </div>

                  {/* Section 4: Production Results */}
                  <div
                    className="rounded-xl p-8 border mb-6"
                    style={{ background: "#12121a", borderColor: "#1e1e2e" }}
                  >
                    <h2
                      className="text-xl font-bold mb-4"
                      style={{ color: "#22c55e" }}
                    >
                      3. Production Benchmarks
                    </h2>
                    <div
                      className="space-y-4 text-sm leading-relaxed"
                      style={{ color: "#b0b0c0" }}
                    >
                      <p>
                        All benchmarks are measured on the Tang Nano 9K production SoC (PicoRV32 at 25.2 MHz,
                        ATOMiK core at 81 MHz, single-bank) using cycle-accurate hardware counters.
                      </p>
                      <div className="grid sm:grid-cols-2 gap-4">
                        <div className="rounded-lg p-4" style={{ background: "#0d0d14" }}>
                          <div className="font-mono text-2xl font-bold" style={{ color: "#22c55e" }}>
                            7,670x &ndash; 916,000x
                          </div>
                          <div className="text-xs mt-1" style={{ color: "#8888a0" }}>
                            Memory traffic reduction across validated workloads
                          </div>
                        </div>
                        <div className="rounded-lg p-4" style={{ background: "#0d0d14" }}>
                          <div className="font-mono text-2xl font-bold" style={{ color: "#22c55e" }}>
                            22 &ndash; 58%
                          </div>
                          <div className="text-xs mt-1" style={{ color: "#8888a0" }}>
                            Execution time improvement
                          </div>
                        </div>
                      </div>

                      <h3 className="font-semibold text-white pt-2">Core Operation Latency</h3>
                      <div
                        className="rounded-lg p-4 font-mono text-xs space-y-1"
                        style={{ background: "#0d0d14" }}
                      >
                        <div className="flex justify-between">
                          <span>LOAD (set reference)</span>
                          <span style={{ color: "#4f8fff" }}>64 cycles</span>
                        </div>
                        <div className="flex justify-between">
                          <span>ACCUM (XOR delta)</span>
                          <span style={{ color: "#4f8fff" }}>70 cycles</span>
                        </div>
                        <div className="flex justify-between">
                          <span>READ (reconstruct)</span>
                          <span style={{ color: "#4f8fff" }}>99 cycles</span>
                        </div>
                        <div className="flex justify-between">
                          <span>Full roundtrip</span>
                          <span style={{ color: "#22c55e" }}>285 cycles</span>
                        </div>
                        <div className="flex justify-between" style={{ color: "#8888a0" }}>
                          <span>Std deviation</span>
                          <span>&le; 0.5 cycles</span>
                        </div>
                      </div>

                      <p>
                        Measurements are <strong className="text-white">deterministic</strong> with standard
                        deviation at or below 0.5 cycles. This is not coincidence &mdash; it is an architectural
                        property. There is no speculative execution, no cache hierarchy, no branch prediction.
                        Timing side channels are eliminated by construction, not by mitigation.
                      </p>
                      <p>
                        Change detection via hardware ATOMiK fingerprinting is{" "}
                        <strong className="text-white">76&ndash;80% faster</strong> than software memcmp,
                        while tracked memcpy adds only 5&ndash;12% overhead versus plain memcpy.
                      </p>
                    </div>
                  </div>

                  {/* Section 5: Kernel Module */}
                  <div
                    className="rounded-xl p-8 border mb-6"
                    style={{ background: "#12121a", borderColor: "#1e1e2e" }}
                  >
                    <h2
                      className="text-xl font-bold mb-4"
                      style={{ color: "#d4a843" }}
                    >
                      4. Kernel Module Architecture
                    </h2>
                    <div
                      className="space-y-4 text-sm leading-relaxed"
                      style={{ color: "#b0b0c0" }}
                    >
                      <p>
                        The ATOMiK Linux kernel module provides transparent integration with existing
                        applications. It attaches to three kernel subsystems without requiring
                        application modifications:
                      </p>
                      <ul className="space-y-2 ml-4">
                        <li className="flex gap-2">
                          <span style={{ color: "#d4a843" }}>&bull;</span>
                          <span>
                            <strong className="text-white">Copy-on-Write Detection</strong> &mdash;
                            Attaches via kretprobe to COW fault paths. Tracks which pages are actually
                            modified versus merely duplicated.
                          </span>
                        </li>
                        <li className="flex gap-2">
                          <span style={{ color: "#d4a843" }}>&bull;</span>
                          <span>
                            <strong className="text-white">TCP Send Deduplication</strong> &mdash;
                            CRC32C fingerprinting of outbound TCP segments. Duplicate content is detected
                            and suppressed before hitting the network stack.
                          </span>
                        </li>
                        <li className="flex gap-2">
                          <span style={{ color: "#d4a843" }}>&bull;</span>
                          <span>
                            <strong className="text-white">Per-Cgroup Waste Tracking</strong> &mdash;
                            27 real-time sysfs metrics per cgroup, enabling container-level visibility
                            into memory waste without per-process instrumentation.
                          </span>
                        </li>
                      </ul>
                      <p>
                        The module implements graceful degradation: on license expiry, all operations
                        become no-ops and reads return zero. No crash, no data corruption, no service
                        interruption.
                      </p>
                    </div>
                  </div>

                  {/* Section 6: Comparison */}
                  <div
                    className="rounded-xl p-8 border mb-6"
                    style={{ background: "#12121a", borderColor: "#1e1e2e" }}
                  >
                    <h2
                      className="text-xl font-bold mb-4"
                      style={{ color: "#22d3ee" }}
                    >
                      5. Comparison Analysis
                    </h2>
                    <div
                      className="space-y-4 text-sm leading-relaxed"
                      style={{ color: "#b0b0c0" }}
                    >
                      <p>
                        ATOMiK&apos;s delta-state algebra occupies a distinct point in the design space
                        compared to existing approaches:
                      </p>
                      <div className="overflow-x-auto">
                        <table className="w-full text-xs" style={{ color: "#b0b0c0" }}>
                          <thead>
                            <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                              <th className="text-left py-2 pr-4 font-semibold text-white">Approach</th>
                              <th className="text-left py-2 pr-4 font-semibold text-white">Ordering</th>
                              <th className="text-left py-2 pr-4 font-semibold text-white">State Size</th>
                              <th className="text-left py-2 font-semibold text-white">Hardware Accel</th>
                            </tr>
                          </thead>
                          <tbody>
                            <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                              <td className="py-2 pr-4">CRDTs</td>
                              <td className="py-2 pr-4">Order-free</td>
                              <td className="py-2 pr-4" style={{ color: "#ef4444" }}>Grows monotonically</td>
                              <td className="py-2">None</td>
                            </tr>
                            <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                              <td className="py-2 pr-4">Event Sourcing</td>
                              <td className="py-2 pr-4" style={{ color: "#ef4444" }}>Ordered log</td>
                              <td className="py-2 pr-4" style={{ color: "#ef4444" }}>Log grows forever</td>
                              <td className="py-2">None</td>
                            </tr>
                            <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                              <td className="py-2 pr-4">Consensus (Raft/Paxos)</td>
                              <td className="py-2 pr-4" style={{ color: "#ef4444" }}>Total order</td>
                              <td className="py-2 pr-4">Fixed</td>
                              <td className="py-2">None</td>
                            </tr>
                            <tr>
                              <td className="py-2 pr-4" style={{ color: "#22c55e" }}>ATOMiK</td>
                              <td className="py-2 pr-4" style={{ color: "#22c55e" }}>Order-free</td>
                              <td className="py-2 pr-4" style={{ color: "#22c55e" }}>Fixed (constant)</td>
                              <td className="py-2" style={{ color: "#22c55e" }}>Native FPGA/ASIC</td>
                            </tr>
                          </tbody>
                        </table>
                      </div>
                      <p>
                        The key differentiator is that ATOMiK achieves order-independence (like CRDTs)
                        with constant state size (unlike CRDTs) and hardware acceleration (unlike all
                        software-only approaches). The accumulator is fixed-width regardless of how many
                        deltas have been applied.
                      </p>
                    </div>
                  </div>

                  {/* Section 7: Roadmap */}
                  <div
                    className="rounded-xl p-8 border mb-6"
                    style={{ background: "#12121a", borderColor: "#1e1e2e" }}
                  >
                    <h2
                      className="text-xl font-bold mb-4"
                      style={{ color: "#f472b6" }}
                    >
                      6. ASIC Development Roadmap
                    </h2>
                    <div
                      className="space-y-4 text-sm leading-relaxed"
                      style={{ color: "#b0b0c0" }}
                    >
                      <p>
                        The path from FPGA validation to custom silicon follows three phases:
                      </p>
                      <div className="space-y-3">
                        {[
                          {
                            phase: "Phase 1: FPGA Validation",
                            status: "Complete",
                            detail: "Tang Nano 9K ($13.50) and Zynq XC7Z020 ($40). Full test suite passing across all configurations.",
                            statusColor: "#22c55e",
                          },
                          {
                            phase: "Phase 2: FPGA Production",
                            status: "In Progress",
                            detail: "Zynq-based PCIe accelerator card with Linux driver. Target: 69.7 Gops/s sustained in real workloads.",
                            statusColor: "#d4a843",
                          },
                          {
                            phase: "Phase 3: ASIC Tape-Out",
                            status: "Planned",
                            detail: "Target 28nm or 16nm process. Projected 100+ Gops/s per core with sub-watt power envelope.",
                            statusColor: "#8888a0",
                          },
                        ].map((p) => (
                          <div
                            key={p.phase}
                            className="rounded-lg p-4 flex gap-4"
                            style={{ background: "#0d0d14" }}
                          >
                            <div
                              className="mt-1.5 w-2 h-2 rounded-full shrink-0"
                              style={{ background: p.statusColor }}
                            />
                            <div>
                              <div className="flex items-center gap-2 mb-1">
                                <span className="text-xs font-semibold text-white">{p.phase}</span>
                                <span
                                  className="text-xs font-mono px-2 py-0.5 rounded"
                                  style={{ background: "#1e1e2e", color: p.statusColor }}
                                >
                                  {p.status}
                                </span>
                              </div>
                              <p className="text-xs" style={{ color: "#8888a0" }}>{p.detail}</p>
                            </div>
                          </div>
                        ))}
                      </div>
                    </div>
                  </div>

                  {/* Next steps CTA */}
                  <div
                    className="rounded-xl p-8 border"
                    style={{
                      background: "linear-gradient(135deg, #12121a, #181824)",
                      borderColor: "#4f8fff33",
                    }}
                  >
                    <h3 className="text-lg font-bold text-white mb-2">
                      Continue exploring
                    </h3>
                    <p className="text-sm mb-5" style={{ color: "#8888a0" }}>
                      Dive deeper into the architecture documentation, or start building with the SDK.
                    </p>
                    <div className="flex flex-col sm:flex-row gap-3">
                      <Link
                        href="/docs/architecture"
                        className="inline-flex items-center justify-center px-6 py-3 rounded-lg text-sm font-semibold text-white transition-opacity hover:opacity-90"
                        style={{ background: "#4f8fff" }}
                      >
                        Architecture Docs
                      </Link>
                      <Link
                        href="/docs/hardware"
                        className="inline-flex items-center justify-center px-6 py-3 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
                        style={{
                          background: "transparent",
                          color: "#e0e0e8",
                          border: "1px solid #1e1e2e",
                        }}
                      >
                        Hardware Deep-Dive
                      </Link>
                      <Link
                        href="/get-started"
                        className="inline-flex items-center justify-center px-6 py-3 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
                        style={{
                          background: "transparent",
                          color: "#e0e0e8",
                          border: "1px solid #1e1e2e",
                        }}
                      >
                        Get Started Free
                      </Link>
                    </div>
                  </div>
                </div>
              </div>
            )}
          </div>

          {/* Right column: social proof sidebar */}
          <div className="lg:col-span-2">
            <div className="sticky top-24 space-y-6">
              <h3
                className="text-sm font-mono tracking-widest uppercase"
                style={{ color: "#8888a0" }}
              >
                By the numbers
              </h3>

              {[
                {
                  value: "108",
                  unit: "theorems",
                  desc: "Formally verified in Lean4 proof assistant. Every algebraic property machine-checked.",
                  color: "#8b5cf6",
                },
                {
                  value: "69.7",
                  unit: "Gops/s",
                  desc: "Peak throughput on Xilinx Zynq XC7Z020 with 512 parallel banks at 135.6 MHz.",
                  color: "#4f8fff",
                },
                {
                  value: "500+",
                  unit: "tests",
                  desc: "Across atomik-core (218), SDK pipeline (353), hardware (80), CDC (37). All passing.",
                  color: "#22c55e",
                },
                {
                  value: "916,000x",
                  unit: "reduction",
                  desc: "Peak memory traffic reduction. 7,670x at minimum across all validated workloads.",
                  color: "#d4a843",
                },
                {
                  value: "4",
                  unit: "operations",
                  desc: "LOAD, ACCUM, READ, SWAP. The complete algebra. Nothing else needed.",
                  color: "#22d3ee",
                },
              ].map((item) => (
                <div
                  key={item.value + item.unit}
                  className="rounded-xl p-5 border"
                  style={{
                    background: "#12121a",
                    borderColor: "#1e1e2e",
                  }}
                >
                  <div className="flex items-baseline gap-2 mb-2">
                    <span
                      className="text-3xl font-bold font-mono"
                      style={{ color: item.color }}
                    >
                      {item.value}
                    </span>
                    <span
                      className="text-sm font-mono"
                      style={{ color: "#8888a0" }}
                    >
                      {item.unit}
                    </span>
                  </div>
                  <p className="text-sm" style={{ color: "#8888a0" }}>
                    {item.desc}
                  </p>
                </div>
              ))}

              {/* Compact CTA to architecture docs */}
              <div
                className="rounded-xl p-5 border"
                style={{
                  background: "#0d0d14",
                  borderColor: "#1e1e2e",
                }}
              >
                <p
                  className="text-xs font-mono uppercase tracking-widest mb-2"
                  style={{ color: "#8888a0" }}
                >
                  Want to dive in now?
                </p>
                <Link
                  href="/docs/architecture"
                  className="text-sm font-semibold hover:underline transition-colors"
                  style={{ color: "#4f8fff" }}
                >
                  Read the architecture docs &rarr;
                </Link>
              </div>
            </div>
          </div>
        </div>
      </section>

      {/* Bottom CTA */}
      <section
        className="border-t"
        style={{ borderColor: "#1e1e2e" }}
      >
        <div className="max-w-3xl mx-auto px-6 py-16 text-center">
          <h2 className="text-2xl font-bold mb-3">
            Ready to eliminate redundant data?
          </h2>
          <p className="text-sm mb-8" style={{ color: "#8888a0" }}>
            Start free with the Python SDK. Scale to kernel-level
            optimization and hardware acceleration when you need it.
          </p>
          <div className="flex flex-col sm:flex-row gap-3 justify-center">
            <Link
              href="/pricing"
              className="inline-flex items-center justify-center px-8 py-3 rounded-lg text-sm font-semibold text-white transition-opacity hover:opacity-90"
              style={{ background: "#4f8fff" }}
            >
              View Pricing
            </Link>
            <Link
              href="/get-started"
              className="inline-flex items-center justify-center px-8 py-3 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
              style={{
                background: "transparent",
                color: "#e0e0e8",
                border: "1px solid #1e1e2e",
              }}
            >
              Get Started Free
            </Link>
          </div>
        </div>
      </section>
    </div>
  );
}
