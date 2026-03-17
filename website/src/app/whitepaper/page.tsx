"use client";

import { useState, FormEvent } from "react";
import Link from "next/link";
import Nav from "@/components/Nav";

const coverageItems = [
  {
    title: "Mathematical Foundations",
    desc: "Abelian group properties (commutativity, associativity, self-inverse, identity) with 92 formally verified Lean4 theorems.",
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
  { value: "92", label: "Lean4 Theorems Proven", color: "#8b5cf6" },
  { value: "69.7 Gops/s", label: "Peak FPGA Throughput", color: "#4f8fff" },
  { value: "500+", label: "Tests Passing", color: "#22c55e" },
  { value: "916,000x", label: "Memory Traffic Reduction", color: "#d4a843" },
];

export default function WhitepaperPage() {
  const [name, setName] = useState("");
  const [email, setEmail] = useState("");
  const [status, setStatus] = useState<
    "idle" | "loading" | "success" | "error"
  >("idle");
  const [errorMsg, setErrorMsg] = useState("");

  async function handleSubmit(e: FormEvent) {
    e.preventDefault();
    if (!email.trim()) return;

    setStatus("loading");
    setErrorMsg("");

    try {
      const res = await fetch("/api/subscribe", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          email: email.trim(),
          name: name.trim(),
          source: "whitepaper",
        }),
      });

      const data = await res.json();

      if (!res.ok) {
        setStatus("error");
        setErrorMsg(data.error || "Something went wrong");
        return;
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
            {status === "success" ? (
              <div
                className="rounded-xl p-8 border"
                style={{
                  background: "linear-gradient(135deg, #12121a, #181824)",
                  borderColor: "#22c55e33",
                }}
              >
                <div className="flex items-center gap-2 mb-4">
                  <span
                    className="text-xl"
                    style={{ color: "#22c55e" }}
                  >
                    &#10003;
                  </span>
                  <h3 className="text-xl font-bold text-white">
                    Access Granted
                  </h3>
                </div>
                <p
                  className="text-sm mb-6"
                  style={{ color: "#8888a0" }}
                >
                  Thank you for your interest. You can read the full
                  technical deep-dive below, or explore the
                  architecture documentation.
                </p>
                <div className="flex flex-col sm:flex-row gap-3">
                  <Link
                    href="/docs/architecture"
                    className="inline-flex items-center justify-center px-6 py-3 rounded-lg text-sm font-semibold text-white transition-opacity hover:opacity-90"
                    style={{ background: "#4f8fff" }}
                  >
                    Read Architecture Docs
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
                </div>

                {/* Condensed technical overview */}
                <div
                  className="mt-8 pt-8"
                  style={{ borderTop: "1px solid #1e1e2e" }}
                >
                  <h4 className="font-bold text-white mb-4">
                    Technical Overview
                  </h4>
                  <div
                    className="space-y-4 text-sm leading-relaxed"
                    style={{ color: "#b0b0c0" }}
                  >
                    <div>
                      <h5
                        className="font-semibold mb-1"
                        style={{ color: "#8b5cf6" }}
                      >
                        The Core Equation
                      </h5>
                      <p>
                        ATOMiK replaces Von Neumann state storage with
                        algebraic reconstruction:{" "}
                        <code
                          className="px-1.5 py-0.5 rounded text-xs font-mono"
                          style={{
                            background: "#0d0d14",
                            color: "#4f8fff",
                          }}
                        >
                          current_state = initial_state XOR
                          accumulator
                        </code>
                        . This single equation, backed by an Abelian
                        group (commutative, associative, self-inverse,
                        identity), enables lock-free parallel
                        accumulation where order never matters.
                      </p>
                    </div>
                    <div>
                      <h5
                        className="font-semibold mb-1"
                        style={{ color: "#4f8fff" }}
                      >
                        Hardware Scaling
                      </h5>
                      <p>
                        Validated across three platforms: Tang Nano 9K
                        ($13.50 FPGA, single-bank at 81 MHz), custom
                        RV64I CPU with ATOMiK ISA extensions at
                        1280x720 HDMI, and Xilinx Zynq XC7Z020 with
                        up to 512 parallel banks achieving 69.7
                        Gops/s. LUT scaling is sub-linear: 3.7x
                        growth for 16x throughput.
                      </p>
                    </div>
                    <div>
                      <h5
                        className="font-semibold mb-1"
                        style={{ color: "#22c55e" }}
                      >
                        Production Results
                      </h5>
                      <p>
                        Memory traffic reduction ranges from 7,670x
                        to 916,000x across validated workloads, with
                        22-58% execution time improvements.
                        Measurements are deterministic (stdev less than 0.5
                        cycles) with zero timing side channels by
                        architecture, not by mitigation.
                      </p>
                    </div>
                    <div>
                      <h5
                        className="font-semibold mb-1"
                        style={{ color: "#d4a843" }}
                      >
                        Kernel Integration
                      </h5>
                      <p>
                        The Linux kernel module attaches via kretprobe
                        to copy-on-write paths, deduplicates TCP
                        sends with CRC32C fingerprinting, and tracks
                        per-cgroup memory waste through 27 real-time
                        sysfs metrics. Graceful degradation on license
                        expiry: operations become no-ops, reads return
                        zero.
                      </p>
                    </div>
                  </div>
                </div>
              </div>
            ) : (
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
                  Download the White Paper
                </h3>
                <p
                  className="text-sm mb-6"
                  style={{ color: "#8888a0" }}
                >
                  Enter your name and email to get instant access to
                  the full technical deep-dive.
                </p>
                <form onSubmit={handleSubmit} className="space-y-3">
                  <input
                    type="text"
                    placeholder="Your name"
                    value={name}
                    onChange={(e) => setName(e.target.value)}
                    className="w-full px-4 py-3 rounded-lg text-sm text-white placeholder-gray-500 outline-none focus:ring-2 focus:ring-[#4f8fff]"
                    style={{
                      background: "#0a0a0f",
                      border: "1px solid #1e1e2e",
                    }}
                  />
                  <input
                    type="email"
                    placeholder="you@example.com"
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
                  <button
                    type="submit"
                    disabled={status === "loading"}
                    className="w-full py-3 rounded-lg text-sm font-semibold text-white transition-opacity hover:opacity-90 disabled:opacity-50"
                    style={{ background: "#4f8fff" }}
                  >
                    {status === "loading"
                      ? "Processing..."
                      : "Download White Paper"}
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
                  value: "92",
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
