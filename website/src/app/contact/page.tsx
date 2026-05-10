"use client";

import Nav from "@/components/Nav";
import { useState, FormEvent } from "react";

const requestTypes = [
  "Request Evaluation Access",
  "Request Technical Demo",
  "Discuss Design Partnership",
];

const roles = [
  "Technical founder",
  "Engineer / architect",
  "Infrastructure team",
  "Researcher",
  "Investor / advisor",
  "Other",
];

const painCategories = [
  "State sync / replication",
  "Edge or embedded workload",
  "Rollback / replay overhead",
  "Change detection",
  "Adaptive execution surface",
  "Technical diligence",
];

const timelines = [
  "Exploring now",
  "Evaluation in 30 days",
  "Evaluation in 90 days",
  "Roadmap research",
];

export default function ContactPage() {
  const [form, setForm] = useState({
    name: "",
    email: "",
    company: "",
    role: roles[0],
    requestType: requestTypes[0],
    painCategory: painCategories[0],
    timeline: timelines[0],
    useCase: "",
    currentStack: "",
    message: "",
  });
  const [status, setStatus] = useState<"idle" | "sending" | "sent" | "error">("idle");

  function update(field: string, value: string) {
    setForm((f) => ({ ...f, [field]: value }));
  }

  async function handleSubmit(e: FormEvent) {
    e.preventDefault();
    setStatus("sending");

    try {
      const res = await fetch("/api/subscribe", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          email: form.email,
          name: form.name,
          company: form.company,
          role: form.role,
          source: "public-evaluation-form",
          requested_path: form.requestType,
          pain_category: form.painCategory,
          timeline: form.timeline,
          use_case: form.useCase,
          current_stack: form.currentStack,
          message: form.message,
          interests: [form.requestType, form.painCategory, form.timeline],
        }),
      });

      if (!res.ok) {
        setStatus("error");
        return;
      }

      setStatus("sent");
    } catch {
      setStatus("error");
    }
  }

  const inputStyle = {
    background: "#070b12",
    border: "1px solid #1d324a",
    color: "#f4f8ff",
  };

  return (
    <div className="min-h-screen" style={{ background: "#070b12", color: "#f4f8ff" }}>
      <Nav />

      <div className="mx-auto max-w-3xl px-6 pb-24 pt-16">
        <p className="text-sm font-semibold uppercase" style={{ color: "#22d3ee" }}>
          Lead request
        </p>
        <h1 className="mt-3 text-4xl font-bold">Tell us what you want to evaluate.</h1>
        <p className="mt-4 leading-7" style={{ color: "#9fb1c7" }}>
          Use this form for evaluation access, technical demo requests, and design partner conversations. The most useful request includes one real workload or state-heavy path.
        </p>

        {status === "sent" ? (
          <div
            className="mt-10 rounded-lg border p-8"
            style={{ background: "#0d1420", borderColor: "#1d324a" }}
          >
            <h2 className="text-xl font-bold">Request received</h2>
            <p className="mt-3 leading-7" style={{ color: "#9fb1c7" }}>
              Thanks. The next step is to anchor on the workload, current stack, and desired path: proof review, technical demo, or scoped evaluation conversation.
            </p>
            <p className="mt-3 text-sm" style={{ color: "#9fb1c7" }}>
              You can also reach ATOMiK at{" "}
              <a href="mailto:mrockwell@atomik.tech" style={{ color: "#22d3ee" }}>
                mrockwell@atomik.tech
              </a>
              .
            </p>
          </div>
        ) : (
          <form onSubmit={handleSubmit} className="mt-10 space-y-5">
            <div className="grid gap-5 md:grid-cols-2">
              <div>
                <label className="mb-1.5 block text-sm font-medium">Name</label>
                <input
                  type="text"
                  required
                  value={form.name}
                  onChange={(e) => update("name", e.target.value)}
                  className="w-full rounded-lg px-4 py-2.5 text-sm outline-none focus:ring-2 focus:ring-[#22d3ee]"
                  style={inputStyle}
                />
              </div>
              <div>
                <label className="mb-1.5 block text-sm font-medium">Work email</label>
                <input
                  type="email"
                  required
                  value={form.email}
                  onChange={(e) => update("email", e.target.value)}
                  className="w-full rounded-lg px-4 py-2.5 text-sm outline-none focus:ring-2 focus:ring-[#22d3ee]"
                  style={inputStyle}
                />
              </div>
            </div>

            <div className="grid gap-5 md:grid-cols-2">
              <div>
                <label className="mb-1.5 block text-sm font-medium">Company</label>
                <input
                  type="text"
                  value={form.company}
                  onChange={(e) => update("company", e.target.value)}
                  className="w-full rounded-lg px-4 py-2.5 text-sm outline-none focus:ring-2 focus:ring-[#22d3ee]"
                  style={inputStyle}
                />
              </div>
              <div>
                <label className="mb-1.5 block text-sm font-medium">Role</label>
                <select
                  value={form.role}
                  onChange={(e) => update("role", e.target.value)}
                  className="w-full rounded-lg px-4 py-2.5 text-sm outline-none focus:ring-2 focus:ring-[#22d3ee]"
                  style={inputStyle}
                >
                  {roles.map((role) => (
                    <option key={role} value={role}>{role}</option>
                  ))}
                </select>
              </div>
            </div>

            <div className="grid gap-5 md:grid-cols-3">
              <div>
                <label className="mb-1.5 block text-sm font-medium">Request</label>
                <select
                  value={form.requestType}
                  onChange={(e) => update("requestType", e.target.value)}
                  className="w-full rounded-lg px-4 py-2.5 text-sm outline-none focus:ring-2 focus:ring-[#22d3ee]"
                  style={inputStyle}
                >
                  {requestTypes.map((item) => (
                    <option key={item} value={item}>{item}</option>
                  ))}
                </select>
              </div>
              <div>
                <label className="mb-1.5 block text-sm font-medium">Pain category</label>
                <select
                  value={form.painCategory}
                  onChange={(e) => update("painCategory", e.target.value)}
                  className="w-full rounded-lg px-4 py-2.5 text-sm outline-none focus:ring-2 focus:ring-[#22d3ee]"
                  style={inputStyle}
                >
                  {painCategories.map((item) => (
                    <option key={item} value={item}>{item}</option>
                  ))}
                </select>
              </div>
              <div>
                <label className="mb-1.5 block text-sm font-medium">Timeline</label>
                <select
                  value={form.timeline}
                  onChange={(e) => update("timeline", e.target.value)}
                  className="w-full rounded-lg px-4 py-2.5 text-sm outline-none focus:ring-2 focus:ring-[#22d3ee]"
                  style={inputStyle}
                >
                  {timelines.map((item) => (
                    <option key={item} value={item}>{item}</option>
                  ))}
                </select>
              </div>
            </div>

            <div>
              <label className="mb-1.5 block text-sm font-medium">Use case or workload</label>
              <textarea
                required
                rows={4}
                value={form.useCase}
                onChange={(e) => update("useCase", e.target.value)}
                placeholder="Example: sync-heavy edge telemetry, rollback-sensitive state path, distributed cache replay, embedded state tracking..."
                className="w-full resize-y rounded-lg px-4 py-2.5 text-sm outline-none focus:ring-2 focus:ring-[#22d3ee]"
                style={inputStyle}
              />
            </div>

            <div>
              <label className="mb-1.5 block text-sm font-medium">Current stack or constraints</label>
              <textarea
                rows={3}
                value={form.currentStack}
                onChange={(e) => update("currentStack", e.target.value)}
                placeholder="Runtime, hardware, state size, update cadence, bandwidth/power/latency constraints..."
                className="w-full resize-y rounded-lg px-4 py-2.5 text-sm outline-none focus:ring-2 focus:ring-[#22d3ee]"
                style={inputStyle}
              />
            </div>

            <div>
              <label className="mb-1.5 block text-sm font-medium">Anything else</label>
              <textarea
                rows={3}
                value={form.message}
                onChange={(e) => update("message", e.target.value)}
                className="w-full resize-y rounded-lg px-4 py-2.5 text-sm outline-none focus:ring-2 focus:ring-[#22d3ee]"
                style={inputStyle}
              />
            </div>

            {status === "error" && (
              <p className="text-sm" style={{ color: "#fca5a5" }}>
                Something went wrong. Email mrockwell@atomik.tech directly if the form does not submit.
              </p>
            )}

            <button
              type="submit"
              disabled={status === "sending"}
              className="w-full rounded-lg py-3 text-sm font-semibold transition-opacity hover:opacity-90 disabled:opacity-50"
              style={{ background: "#4f8fff", color: "#fff" }}
            >
              {status === "sending" ? "Sending..." : form.requestType}
            </button>
          </form>
        )}
      </div>
    </div>
  );
}
