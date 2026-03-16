"use client";

import { useState, FormEvent, Suspense } from "react";
import { useRouter, useSearchParams } from "next/navigation";
import Nav from "@/components/Nav";

const roles = [
  "Developer",
  "Software Architect",
  "Engineering Manager",
  "CTO / VP Engineering",
  "DevOps / SRE",
  "Student / Researcher",
  "Other",
];

const interestOptions = [
  "Python SDK",
  "Kernel Module",
  "FPGA Hardware",
  "SDK Generation",
  "Enterprise Support",
];

function RegisterForm() {
  const router = useRouter();
  const searchParams = useSearchParams();
  const plan = searchParams.get("plan");

  const [name, setName] = useState("");
  const [email, setEmail] = useState("");
  const [company, setCompany] = useState("");
  const [role, setRole] = useState("");
  const [interests, setInterests] = useState<string[]>([]);
  const [acceptTerms, setAcceptTerms] = useState(false);
  const [status, setStatus] = useState<"idle" | "loading" | "error">("idle");
  const [errorMsg, setErrorMsg] = useState("");

  function toggleInterest(interest: string) {
    setInterests((prev) =>
      prev.includes(interest)
        ? prev.filter((i) => i !== interest)
        : [...prev, interest]
    );
  }

  async function handleSubmit(e: FormEvent) {
    e.preventDefault();
    if (!name.trim() || !email.trim() || !acceptTerms) return;

    setStatus("loading");
    setErrorMsg("");

    try {
      const res = await fetch("/api/subscribe", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          email: email.trim(),
          name: name.trim(),
          source: "developer-program",
          role,
          company: company.trim(),
          interests,
        }),
      });

      const data = await res.json();

      if (!res.ok) {
        setStatus("error");
        setErrorMsg(data.error || "Something went wrong");
        return;
      }

      // Redirect to get-started, preserving plan param if present
      if (plan) {
        router.push(`/get-started?plan=${plan}`);
      } else {
        router.push("/get-started");
      }
    } catch {
      setStatus("error");
      setErrorMsg("Network error. Please try again.");
    }
  }

  const tierLabel =
    plan === "professional"
      ? "Professional"
      : plan === "team"
        ? "Team"
        : null;

  const inputStyle = {
    background: "#0a0a0f",
    border: "1px solid #1e1e2e",
    color: "#e0e0e8",
  };

  return (
    <>
      {/* Plan banner */}
      {tierLabel && (
        <div className="max-w-lg mx-auto px-6 mb-6">
          <div
            className="rounded-lg px-5 py-3 text-sm text-center"
            style={{
              background: "rgba(79,143,255,0.08)",
              border: "1px solid rgba(79,143,255,0.25)",
              color: "#4f8fff",
            }}
          >
            You&apos;ll be redirected to start your{" "}
            <span className="font-semibold">{tierLabel}</span> trial after
            registration
          </div>
        </div>
      )}

      {/* Registration Form */}
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

          {/* Work Email */}
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
              Company{" "}
              <span className="text-xs" style={{ color: "#8888a0" }}>
                (optional)
              </span>
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

          {/* Interests */}
          <div>
            <p className="text-sm font-medium mb-2.5">
              What are you interested in?
            </p>
            <div className="flex flex-wrap gap-2">
              {interestOptions.map((interest) => {
                const selected = interests.includes(interest);
                return (
                  <button
                    key={interest}
                    type="button"
                    onClick={() => toggleInterest(interest)}
                    className="px-3.5 py-1.5 rounded-lg text-xs font-medium transition-all"
                    style={{
                      background: selected
                        ? "rgba(79,143,255,0.15)"
                        : "rgba(255,255,255,0.03)",
                      border: selected
                        ? "1px solid #4f8fff"
                        : "1px solid #1e1e2e",
                      color: selected ? "#4f8fff" : "#8888a0",
                    }}
                  >
                    {selected ? "\u2713 " : ""}
                    {interest}
                  </button>
                );
              })}
            </div>
          </div>

          {/* Accept Terms */}
          <div className="flex items-start gap-3">
            <input
              id="reg-terms"
              type="checkbox"
              checked={acceptTerms}
              onChange={(e) => setAcceptTerms(e.target.checked)}
              className="mt-1 shrink-0 accent-[#4f8fff]"
              required
            />
            <label
              htmlFor="reg-terms"
              className="text-xs leading-relaxed"
              style={{ color: "#8888a0" }}
            >
              I agree to the{" "}
              <a
                href="/terms"
                className="underline hover:text-white"
                style={{ color: "#4f8fff" }}
              >
                Terms of Service
              </a>{" "}
              and{" "}
              <a
                href="/privacy"
                className="underline hover:text-white"
                style={{ color: "#4f8fff" }}
              >
                Privacy Policy
              </a>
              .
            </label>
          </div>

          {/* Submit */}
          <button
            type="submit"
            disabled={status === "loading" || !acceptTerms}
            className="w-full py-3 rounded-lg text-sm font-semibold text-white transition-opacity hover:opacity-90 disabled:opacity-50"
            style={{
              background: "linear-gradient(135deg, #4f8fff, #3a7aee)",
              boxShadow: "0 4px 24px rgba(79,143,255,0.25)",
            }}
          >
            {status === "loading"
              ? "Creating your account..."
              : "Join the Developer Program"}
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
