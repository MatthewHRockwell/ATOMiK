"use client";

import Nav from "@/components/Nav";
import { useState, FormEvent } from "react";

const subjects = [
  "Enterprise licensing inquiry",
  "Hardware acceleration consultation",
  "Custom integration support",
  "Partnership opportunity",
  "Other",
];

export default function ContactPage() {
  const [form, setForm] = useState({ name: "", email: "", company: "", subject: subjects[0], message: "" });
  const [status, setStatus] = useState<"idle" | "sending" | "sent">("idle");

  function update(field: string, value: string) {
    setForm((f) => ({ ...f, [field]: value }));
  }

  async function handleSubmit(e: FormEvent) {
    e.preventDefault();
    setStatus("sending");

    // Send via mailto as a fallback — works without backend
    const body = `Name: ${form.name}\nCompany: ${form.company}\nEmail: ${form.email}\n\nSubject: ${form.subject}\n\n${form.message}`;
    const mailto = `mailto:sales@atomik.tech?subject=${encodeURIComponent(form.subject)}&body=${encodeURIComponent(body)}`;
    window.location.href = mailto;

    // Also store the lead
    try {
      await fetch("/api/subscribe", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ email: form.email }),
      });
    } catch {
      // Non-critical
    }

    setStatus("sent");
  }

  const inputStyle = {
    background: "#0a0a0f",
    border: "1px solid #1e1e2e",
    color: "#e0e0e8",
  };

  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav />

      <div className="max-w-2xl mx-auto px-6 pt-20 pb-24">
        <h1 className="text-3xl font-bold tracking-tight mb-2">Contact Sales</h1>
        <p className="mb-10" style={{ color: "#8888a0" }}>
          Tell us about your use case. Enterprise inquiries typically get a response within 4 hours.
        </p>

        {status === "sent" ? (
          <div
            className="rounded-xl border p-8 text-center"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div className="text-3xl mb-4" style={{ color: "#22c55e" }}>&#10003;</div>
            <h2 className="text-xl font-bold mb-2">Message sent</h2>
            <p style={{ color: "#8888a0" }}>
              Your email client should have opened with a pre-filled message.
              If it didn&apos;t, email us directly at{" "}
              <a href="mailto:sales@atomik.tech" style={{ color: "#4f8fff" }}>
                sales@atomik.tech
              </a>
            </p>
          </div>
        ) : (
          <form onSubmit={handleSubmit} className="space-y-5">
            <div className="grid md:grid-cols-2 gap-5">
              <div>
                <label className="block text-sm font-medium mb-1.5">Name</label>
                <input
                  type="text"
                  required
                  value={form.name}
                  onChange={(e) => update("name", e.target.value)}
                  className="w-full px-4 py-2.5 rounded-lg text-sm outline-none focus:ring-2 focus:ring-[#4f8fff]"
                  style={inputStyle}
                />
              </div>
              <div>
                <label className="block text-sm font-medium mb-1.5">Work email</label>
                <input
                  type="email"
                  required
                  value={form.email}
                  onChange={(e) => update("email", e.target.value)}
                  className="w-full px-4 py-2.5 rounded-lg text-sm outline-none focus:ring-2 focus:ring-[#4f8fff]"
                  style={inputStyle}
                />
              </div>
            </div>

            <div>
              <label className="block text-sm font-medium mb-1.5">Company</label>
              <input
                type="text"
                value={form.company}
                onChange={(e) => update("company", e.target.value)}
                className="w-full px-4 py-2.5 rounded-lg text-sm outline-none focus:ring-2 focus:ring-[#4f8fff]"
                style={inputStyle}
              />
            </div>

            <div>
              <label className="block text-sm font-medium mb-1.5">Subject</label>
              <select
                value={form.subject}
                onChange={(e) => update("subject", e.target.value)}
                className="w-full px-4 py-2.5 rounded-lg text-sm outline-none focus:ring-2 focus:ring-[#4f8fff]"
                style={inputStyle}
              >
                {subjects.map((s) => (
                  <option key={s} value={s}>{s}</option>
                ))}
              </select>
            </div>

            <div>
              <label className="block text-sm font-medium mb-1.5">Message</label>
              <textarea
                required
                rows={5}
                value={form.message}
                onChange={(e) => update("message", e.target.value)}
                placeholder="Tell us about your infrastructure, scale, and what you're trying to achieve..."
                className="w-full px-4 py-2.5 rounded-lg text-sm outline-none focus:ring-2 focus:ring-[#4f8fff] resize-y"
                style={inputStyle}
              />
            </div>

            <button
              type="submit"
              disabled={status === "sending"}
              className="w-full py-3 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90 disabled:opacity-50"
              style={{ background: "#4f8fff", color: "#fff" }}
            >
              {status === "sending" ? "Sending..." : "Send Message"}
            </button>
          </form>
        )}

        <div
          className="mt-10 rounded-xl border p-6 grid md:grid-cols-3 gap-6 text-center text-sm"
          style={{ background: "#12121a", borderColor: "#1e1e2e" }}
        >
          <div>
            <div className="font-semibold mb-1">Sales</div>
            <a href="mailto:sales@atomik.tech" style={{ color: "#4f8fff" }}>sales@atomik.tech</a>
          </div>
          <div>
            <div className="font-semibold mb-1">Support</div>
            <a href="mailto:support@atomik.tech" style={{ color: "#4f8fff" }}>support@atomik.tech</a>
          </div>
          <div>
            <div className="font-semibold mb-1">Founder</div>
            <a href="mailto:mrockwell@atomik.tech" style={{ color: "#4f8fff" }}>mrockwell@atomik.tech</a>
          </div>
        </div>
      </div>
    </div>
  );
}
