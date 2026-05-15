"use client";

import { useState, FormEvent, Suspense } from "react";
import { useSearchParams } from "next/navigation";
import Link from "next/link";
import Nav from "@/components/Nav";
import { CopyCodeBlock } from "@/components/CopyCodeBlock";

const roles = [
  "Developer",
  "Software Architect",
  "Engineering Manager",
  "CTO / VP Engineering",
  "DevOps / SRE",
  "Student / Researcher",
  "Other",
];

const tierSummaries: Record<string, { name: string; price: string; color: string; features: string[] }> = {
  community: {
    name: "Public Repository",
    price: "Public review",
    color: "#22d3ee",
    features: [
      "Public docs and proof notes",
      "Evidence-labeled artifacts",
      "GitHub issue path for repo bugs",
      "No conventional public free tier",
    ],
  },
  professional: {
    name: "Evaluation Access",
    price: "Request-based",
    color: "#4f8fff",
    features: [
      "Proof artifact review",
      "Workload-fit conversation",
      "Technical updates",
      "Early demo availability",
      "Path to scoped benchmark exchange",
    ],
  },
  team: {
    name: "Design Partner",
    price: "Scoped",
    color: "#22d3ee",
    features: [
      "Technical discovery",
      "Success criteria",
      "Prototype mapping",
      "Evaluation deliverables",
      "Continue / refine / stop recommendation",
    ],
  },
};

function CopyButton({ text, label }: { text: string; label?: string }) {
  const [copied, setCopied] = useState(false);

  function handleCopy() {
    navigator.clipboard.writeText(text);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  }

  return (
    <button
      type="button"
      onClick={handleCopy}
      className="shrink-0 px-2.5 py-1 rounded text-xs font-medium transition-all"
      style={{
        background: copied ? "rgba(34,197,94,0.15)" : "rgba(79,143,255,0.1)",
        color: copied ? "#22c55e" : "#4f8fff",
        border: copied
          ? "1px solid rgba(34,197,94,0.3)"
          : "1px solid rgba(79,143,255,0.2)",
      }}
    >
      {copied ? "Copied!" : label || "Copy"}
    </button>
  );
}

function RegisterForm() {
  const searchParams = useSearchParams();
  const plan = searchParams.get("plan");
  const isPaidTier = plan === "professional" || plan === "team";

  const [name, setName] = useState("");
  const [email, setEmail] = useState("");
  const [company, setCompany] = useState("");
  const [role, setRole] = useState("");
  const [status, setStatus] = useState<
    "idle" | "loading" | "success" | "error"
  >("idle");
  const [errorMsg, setErrorMsg] = useState("");

  const inputStyle = {
    background: "#0a0a0f",
    border: "1px solid #1e1e2e",
    color: "#e0e0e8",
  };

  const tierLabel =
    plan === "professional"
      ? "Evaluation Access"
      : plan === "team"
        ? "Design Partner"
        : null;

  const tierKey = plan || "community";
  const tierInfo = tierSummaries[tierKey] || tierSummaries.community;

  async function handleSubmit(e: FormEvent) {
    e.preventDefault();
    if (!email.trim()) return;
    if (isPaidTier && !name.trim()) return;

    setStatus("loading");
    setErrorMsg("");

    try {
      const body: Record<string, string> = {
        email: email.trim(),
        source: "developer-program",
      };
      if (name.trim()) body.name = name.trim();
      if (company.trim()) body.company = company.trim();
      if (role) body.role = role;
      if (plan) body.plan = plan;

      const res = await fetch("/api/subscribe", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(body),
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

  // --- Success state: First 60 Seconds experience ---
  if (status === "success") {
    const tryThisScript = `from atomik_core import AtomikContext

ctx = AtomikContext()

# 1. Load and accumulate
ctx.load(1000)
ctx.accum(50)
ctx.accum(100)
print(f"State reconstructed: {ctx.read()}")

# 2. Self-inverse (undo by re-applying)
ctx.accum(100)  # applying 100 again undoes it
print(f"After undo: {ctx.read()}")

# 3. Order doesn't matter
a, b = AtomikContext(), AtomikContext()
a.load(500); a.accum(10); a.accum(20)
b.load(500); b.accum(20); b.accum(10)  # reverse order
print(f"Same result: {a.read() == b.read()}")`;

    return (
      <section className="max-w-2xl mx-auto px-6 pb-24">
        <div
          className="rounded-xl p-8 space-y-8"
          style={{ background: "#12121a", border: "1px solid #1e1e2e" }}
        >
          {/* Checkmark + heading */}
          <div className="text-center">
            <div
              className="w-14 h-14 mx-auto rounded-full flex items-center justify-center mb-4"
              style={{
                background: "rgba(34,197,94,0.1)",
                border: "2px solid rgba(34,197,94,0.3)",
              }}
            >
              <svg
                className="w-7 h-7"
                fill="none"
                stroke="#22c55e"
                strokeWidth={2.5}
                viewBox="0 0 24 24"
              >
                <path
                  strokeLinecap="round"
                  strokeLinejoin="round"
                  d="M5 13l4 4L19 7"
                />
              </svg>
            </div>
            <h2 className="text-2xl font-bold text-white mb-1">
              You&apos;re registered!
            </h2>
            <p className="text-sm" style={{ color: "#8888a0" }}>
              Your first 60 seconds with ATOMiK.
            </p>
          </div>

          {/* Step 1: Install */}
          <div>
            <div className="flex items-center gap-2 mb-3">
              <span
                className="inline-flex items-center justify-center w-6 h-6 rounded-full text-xs font-bold"
                style={{ background: "rgba(79,143,255,0.15)", color: "#4f8fff" }}
              >
                1
              </span>
              <p className="text-sm font-semibold text-white">Install</p>
            </div>
            <div
              className="flex items-center justify-between gap-3 rounded-lg px-4 py-3"
              style={{
                background: "#0a0a0f",
                border: "1px solid #1e1e2e",
              }}
            >
              <code
                className="text-sm"
                style={{
                  color: "#22d3ee",
                  fontFamily:
                    "'SF Mono', 'Fira Code', 'Consolas', monospace",
                }}
              >
                pip install atomik-core
              </code>
              <CopyButton text="pip install atomik-core" />
            </div>
          </div>

          {/* Step 2: Try this */}
          <div>
            <div className="flex items-center gap-2 mb-2">
              <span
                className="inline-flex items-center justify-center w-6 h-6 rounded-full text-xs font-bold"
                style={{ background: "rgba(34,211,238,0.15)", color: "#22d3ee" }}
              >
                2
              </span>
              <p className="text-sm font-semibold text-white">
                Try this &mdash; three killer properties in one script
              </p>
            </div>
            <p className="text-xs mb-3 ml-8" style={{ color: "#8888a0" }}>
              Save as <code style={{ color: "#22d3ee" }}>demo.py</code> and run it.
            </p>
            <CopyCodeBlock code={tryThisScript} />

            {/* Expected output */}
            <div className="mt-3">
              <p className="text-xs font-medium mb-1.5 ml-1" style={{ color: "#555566" }}>
                Expected output
              </p>
              <pre
                className="rounded-lg px-4 py-3 text-xs leading-relaxed"
                style={{
                  background: "#0a0a0f",
                  border: "1px solid #1a1a24",
                  color: "#555566",
                  fontFamily: "'SF Mono', 'Fira Code', 'Consolas', monospace",
                }}
              >
{`State reconstructed: 1118
After undo: 1018
Same result: True`}
              </pre>
            </div>
          </div>

          {/* Step 3: Benchmark */}
          <div>
            <div className="flex items-center gap-2 mb-3">
              <span
                className="inline-flex items-center justify-center w-6 h-6 rounded-full text-xs font-bold"
                style={{ background: "rgba(34,197,94,0.15)", color: "#22c55e" }}
              >
                3
              </span>
              <p className="text-sm font-semibold text-white">
                Benchmark your machine
              </p>
            </div>
            <div
              className="flex items-center justify-between gap-3 rounded-lg px-4 py-3"
              style={{
                background: "#0a0a0f",
                border: "1px solid #1e1e2e",
              }}
            >
              <code
                className="text-sm"
                style={{
                  color: "#22d3ee",
                  fontFamily:
                    "'SF Mono', 'Fira Code', 'Consolas', monospace",
                }}
              >
                python -m atomik_core benchmark --share
              </code>
              <CopyButton text="python -m atomik_core benchmark --share" />
            </div>
          </div>

          {/* Next steps */}
          <div className="flex flex-col gap-3 pt-2">
            <Link
              href="/demo"
              className="block text-center py-3 rounded-lg text-sm font-semibold text-white transition-opacity hover:opacity-90"
              style={{
                background: "linear-gradient(135deg, #4f8fff, #3a7aee)",
                boxShadow: "0 4px 24px rgba(79,143,255,0.25)",
              }}
            >
              Next: Try the interactive demo
            </Link>
            <Link
              href="/get-started"
              className="block text-center py-2.5 rounded-lg text-sm font-medium transition-opacity hover:opacity-90"
              style={{
                color: "#4f8fff",
                background: "rgba(79,143,255,0.08)",
                border: "1px solid rgba(79,143,255,0.2)",
              }}
            >
              Full getting started guide
            </Link>
          </div>
        </div>
      </section>
    );
  }

  // --- Paid tier: full form ---
  if (isPaidTier) {
    return (
      <>
        <div className="max-w-lg mx-auto px-6 mb-6">
          <div
            className="rounded-xl p-5"
            style={{
              background: "#12121a",
              border: `1px solid ${tierInfo.color}33`,
            }}
          >
            <div className="flex items-center justify-between mb-3">
              <h3 className="text-lg font-bold" style={{ color: tierInfo.color }}>
                {tierInfo.name}
              </h3>
              <span className="text-sm font-semibold" style={{ color: tierInfo.color }}>
                {tierInfo.price}
              </span>
            </div>
            <ul className="space-y-1.5">
              {tierInfo.features.map((f) => (
                <li key={f} className="flex items-start gap-2 text-xs" style={{ color: "#b0b0c0" }}>
                  <span className="mt-0.5 shrink-0" style={{ color: "#22c55e" }}>{"\u2713"}</span>
                  {f}
                </li>
              ))}
            </ul>
          </div>
        </div>

        <section className="max-w-lg mx-auto px-6 pb-24">
          <form
            onSubmit={handleSubmit}
            className="rounded-xl p-8 space-y-5"
            style={{ background: "#12121a", border: "1px solid #1e1e2e" }}
          >
            {/* Name */}
            <div>
              <label
                htmlFor="reg-name"
                className="block text-sm font-medium mb-1.5"
              >
                Name <span style={{ color: "#ef4444" }}>*</span>
              </label>
              <input
                id="reg-name"
                type="text"
                required
                value={name}
                onChange={(e) => setName(e.target.value)}
                placeholder="Jane Smith"
                className="w-full px-4 py-2.5 rounded-lg text-sm outline-none focus:ring-2 focus:ring-[#4f8fff] placeholder-gray-600"
                style={inputStyle}
              />
            </div>

            {/* Email */}
            <div>
              <label
                htmlFor="reg-email"
                className="block text-sm font-medium mb-1.5"
              >
                Work email <span style={{ color: "#ef4444" }}>*</span>
              </label>
              <input
                id="reg-email"
                type="email"
                required
                value={email}
                onChange={(e) => {
                  setEmail(e.target.value);
                  if (status === "error") setStatus("idle");
                }}
                placeholder="jane@company.com"
                className="w-full px-4 py-2.5 rounded-lg text-sm outline-none focus:ring-2 focus:ring-[#4f8fff] placeholder-gray-600"
                style={inputStyle}
              />
            </div>

            {/* Company */}
            <div>
              <label
                htmlFor="reg-company"
                className="block text-sm font-medium mb-1.5"
              >
                Company
              </label>
              <input
                id="reg-company"
                type="text"
                value={company}
                onChange={(e) => setCompany(e.target.value)}
                placeholder="Acme Inc."
                className="w-full px-4 py-2.5 rounded-lg text-sm outline-none focus:ring-2 focus:ring-[#4f8fff] placeholder-gray-600"
                style={inputStyle}
              />
            </div>

            {/* Role */}
            <div>
              <label
                htmlFor="reg-role"
                className="block text-sm font-medium mb-1.5"
              >
                Role
              </label>
              <select
                id="reg-role"
                value={role}
                onChange={(e) => setRole(e.target.value)}
                className="w-full px-4 py-2.5 rounded-lg text-sm outline-none focus:ring-2 focus:ring-[#4f8fff] appearance-none"
                style={{
                  ...inputStyle,
                  backgroundImage:
                    'url("data:image/svg+xml,%3csvg xmlns=%27http://www.w3.org/2000/svg%27 fill=%27none%27 viewBox=%270 0 20 20%27%3e%3cpath stroke=%27%238888a0%27 stroke-linecap=%27round%27 stroke-linejoin=%27round%27 stroke-width=%271.5%27 d=%27M6 8l4 4 4-4%27/%3e%3c/svg%3e")',
                  backgroundPosition: "right 0.75rem center",
                  backgroundRepeat: "no-repeat",
                  backgroundSize: "1.25em 1.25em",
                }}
              >
                <option value="" style={{ background: "#0a0a0f" }}>
                  Select your role...
                </option>
                {roles.map((r) => (
                  <option key={r} value={r} style={{ background: "#0a0a0f" }}>
                    {r}
                  </option>
                ))}
              </select>
            </div>

            {/* Submit */}
            <button
              type="submit"
              disabled={status === "loading"}
              className="w-full py-3 rounded-lg text-sm font-semibold text-white transition-opacity hover:opacity-90 disabled:opacity-50"
              style={{
                background: "linear-gradient(135deg, #4f8fff, #3a7aee)",
                boxShadow: "0 4px 24px rgba(79,143,255,0.25)",
              }}
            >
              {status === "loading"
                ? "Submitting request..."
                : tierLabel === "Evaluation Access"
                  ? "Request Evaluation Access"
                  : "Discuss Design Partnership"}
            </button>

            {/* Error */}
            {status === "error" && (
              <p className="text-sm text-center" style={{ color: "#ef4444" }}>
                {errorMsg}
              </p>
            )}
          </form>
        </section>
      </>
    );
  }

  // --- Free tier: email-only ---
  return (
    <>
      <div className="max-w-lg mx-auto px-6 mb-6">
        <div
          className="rounded-xl p-5"
          style={{
            background: "#12121a",
            border: `1px solid ${tierInfo.color}33`,
          }}
        >
          <div className="flex items-center justify-between mb-3">
            <h3 className="text-lg font-bold" style={{ color: tierInfo.color }}>
              {tierInfo.name}
            </h3>
            <span className="text-sm font-semibold" style={{ color: tierInfo.color }}>
              {tierInfo.price}
            </span>
          </div>
          <ul className="space-y-1.5">
            {tierInfo.features.map((f) => (
              <li key={f} className="flex items-start gap-2 text-xs" style={{ color: "#b0b0c0" }}>
                <span className="mt-0.5 shrink-0" style={{ color: "#22c55e" }}>{"\u2713"}</span>
                {f}
              </li>
            ))}
          </ul>
        </div>
      </div>

      <section className="max-w-lg mx-auto px-6 pb-24">
        <form
          onSubmit={handleSubmit}
          className="rounded-xl p-8 space-y-5"
          style={{ background: "#12121a", border: "1px solid #1e1e2e" }}
        >
        {/* Email */}
        <div>
          <label
            htmlFor="reg-email"
            className="block text-sm font-medium mb-1.5"
          >
            Email address
          </label>
          <input
            id="reg-email"
            type="email"
            required
            value={email}
            onChange={(e) => {
              setEmail(e.target.value);
              if (status === "error") setStatus("idle");
            }}
            placeholder="you@example.com"
            className="w-full px-4 py-2.5 rounded-lg text-sm outline-none focus:ring-2 focus:ring-[#4f8fff] placeholder-gray-600"
            style={inputStyle}
          />
        </div>

        {/* Submit */}
        <button
          type="submit"
          disabled={status === "loading"}
          className="w-full py-3 rounded-lg text-sm font-semibold text-white transition-opacity hover:opacity-90 disabled:opacity-50"
          style={{
            background: "linear-gradient(135deg, #4f8fff, #3a7aee)",
            boxShadow: "0 4px 24px rgba(79,143,255,0.25)",
          }}
        >
          {status === "loading" ? "Registering..." : "Get Started Free"}
        </button>

        {/* Error */}
        {status === "error" && (
          <p className="text-sm text-center" style={{ color: "#ef4444" }}>
            {errorMsg}
          </p>
        )}

        <p className="text-xs text-center" style={{ color: "#555566" }}>
          Free forever. No credit card required.
        </p>
      </form>
    </section>
    </>
  );
}

export default function RegisterPage() {
  return (
    <div
      className="min-h-screen"
      style={{ background: "#0a0a0f", color: "#e0e0e8" }}
    >
      <Nav />

      {/* Hero */}
      <section className="text-center px-6 pt-20 pb-8">
        <h1 className="text-4xl md:text-5xl font-bold tracking-tight mb-4">
          <span className="bg-gradient-to-r from-[#4f8fff] to-[#22d3ee] bg-clip-text text-transparent">
            ATOMiK
          </span>{" "}
          Developer Program
        </h1>
        <p className="text-lg max-w-xl mx-auto" style={{ color: "#8888a0" }}>
          Join the delta-state computing community. Free to register.
        </p>
      </section>

      <Suspense fallback={null}>
        <RegisterForm />
      </Suspense>
    </div>
  );
}
