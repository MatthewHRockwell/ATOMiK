"use client";

import Link from "next/link";
import Nav from "@/components/Nav";
import { useEffect, useState, FormEvent } from "react";

const requestTypes = [
  "Request Evaluation",
  "Request Investor Diligence",
  "Request Proof Review",
  "Discuss Design Partnership",
  "Discuss Licensing / IP Evaluation",
  "Ask Proof / Diligence Question",
];

const roles = [
  "Technical founder",
  "Engineer / architect",
  "Infrastructure team",
  "Researcher",
  "Investor / advisor",
  "Chip partner / strategic",
  "Other",
];

const painCategories = [
  "Heat / cooling / density",
  "Battery life / power budget",
  "Bandwidth / state movement",
  "Latency / local execution",
  "Smaller hardware profile",
  "AI context / agent state",
  "Defense / remote reliability",
  "Technical diligence",
];

const timelines = [
  "Exploring now",
  "Evaluation in 30 days",
  "Evaluation in 90 days",
  "Roadmap research",
];

const requestTypeByIntent: Record<string, string> = {
  evaluation: "Request Evaluation",
  demo: "Request Proof Review",
  "technical-demo": "Request Proof Review",
  "design-partner": "Discuss Design Partnership",
  investor: "Request Investor Diligence",
  diligence: "Request Investor Diligence",
  "investor-diligence": "Request Investor Diligence",
  licensing: "Discuss Licensing / IP Evaluation",
  ip: "Discuss Licensing / IP Evaluation",
  question: "Ask Proof / Diligence Question",
  proof: "Request Proof Review",
};

const intentByRequestType: Record<string, string> = {
  "Request Evaluation": "evaluation",
  "Request Investor Diligence": "investor",
  "Request Proof Review": "proof",
  "Discuss Design Partnership": "design-partner",
  "Discuss Licensing / IP Evaluation": "licensing",
  "Ask Proof / Diligence Question": "question",
};

const roleByIntent: Record<string, string> = {
  investor: "Investor / advisor",
  diligence: "Investor / advisor",
  "investor-diligence": "Investor / advisor",
  licensing: "Chip partner / strategic",
  ip: "Chip partner / strategic",
  "design-partner": "Infrastructure team",
};

const painCategoryByIntent: Record<string, string> = {
  investor: "Technical diligence",
  diligence: "Technical diligence",
  "investor-diligence": "Technical diligence",
  licensing: "Technical diligence",
  ip: "Technical diligence",
  question: "Technical diligence",
  proof: "Technical diligence",
};

function readAttribution() {
  const params = new URLSearchParams(window.location.search);
  return {
    source: params.get("source") || params.get("utm_source") || "public-evaluation-form",
    cta: params.get("cta") || "",
    utm_source: params.get("utm_source") || "",
    utm_medium: params.get("utm_medium") || "",
    utm_campaign: params.get("utm_campaign") || "",
    utm_content: params.get("utm_content") || "",
    utm_term: params.get("utm_term") || "",
  };
}

export default function ContactPage() {
  const [form, setForm] = useState({
    name: "",
    email: "",
    company: "",
    role: roles[0],
    requestType: requestTypes[0],
    intent: "evaluation",
    painCategory: painCategories[0],
    timeline: timelines[0],
    useCase: "",
    currentStack: "",
    message: "",
  });
  const [status, setStatus] = useState<"idle" | "sending" | "sent" | "error">("idle");

  useEffect(() => {
    const intent = new URLSearchParams(window.location.search).get("intent");
    if (!intent) return;

    const requestType = requestTypeByIntent[intent];
    if (!requestType) return;

    const timeout = window.setTimeout(() => {
      setForm((current) =>
        current.requestType === requestType && current.intent === intent
          ? current
          : {
              ...current,
              requestType,
              intent,
              role: roleByIntent[intent] || current.role,
              painCategory: painCategoryByIntent[intent] || current.painCategory,
            }
      );
    }, 0);

    return () => window.clearTimeout(timeout);
  }, []);

  function update(field: string, value: string) {
    setForm((f) => ({ ...f, [field]: value }));
    if (status === "error") setStatus("idle");
  }

  function updateRequestType(value: string) {
    setForm((f) => {
      const intent = intentByRequestType[value] || f.intent;
      return {
        ...f,
        requestType: value,
        intent,
        role: roleByIntent[intent] || f.role,
        painCategory: painCategoryByIntent[intent] || f.painCategory,
      };
    });
    if (status === "error") setStatus("idle");
  }

  async function handleSubmit(e: FormEvent) {
    e.preventDefault();
    setStatus("sending");

    try {
      const attribution = readAttribution();
      const res = await fetch("/api/subscribe", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          email: form.email,
          name: form.name,
          company: form.company,
          role: form.role,
          source: attribution.source,
          cta: attribution.cta,
          intent: form.intent,
          landing_path: window.location.pathname + window.location.search,
          referrer: document.referrer,
          utm_source: attribution.utm_source,
          utm_medium: attribution.utm_medium,
          utm_campaign: attribution.utm_campaign,
          utm_content: attribution.utm_content,
          utm_term: attribution.utm_term,
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
  const fieldClass = "w-full min-h-11 rounded-lg px-4 py-2.5 text-base outline-none focus:ring-2 focus:ring-[#22d3ee] md:text-sm";
  const textareaClass = "w-full resize-y rounded-lg px-4 py-2.5 text-base outline-none focus:ring-2 focus:ring-[#22d3ee] md:text-sm";

  return (
    <div className="min-h-screen" style={{ background: "#070b12", color: "#f4f8ff" }}>
      <Nav />

      <div className="mx-auto max-w-3xl px-6 pb-24 pt-12 md:pt-16">
        <p className="text-sm font-semibold uppercase" style={{ color: "#22d3ee" }}>
          Evaluation request
        </p>
        <h1 className="mt-3 text-3xl font-bold leading-tight md:text-4xl">Tell us the constrained state path you want to evaluate.</h1>
        <p className="mt-4 text-base leading-7" style={{ color: "#9fb1c7" }}>
          Use this form for technical evaluation, proof review, licensing, investor diligence, and design-partner conversations. The strongest request names one workload, one current baseline, one painful constraint, and the decision the evidence needs to support.
        </p>
        <div className="mt-5 rounded-lg p-4 text-sm leading-6" style={{ background: "#101927", border: "1px solid #1d324a", color: "#9fb1c7" }}>
          Share only sanitized or high-level workload context here. Do not submit proprietary traces, source code, customer data, export-controlled material, or confidential implementation details until an NDA and evaluation scope are in place.
        </div>
        {status === "sent" ? (
          <div
            className="mt-10 rounded-lg border p-8"
            style={{ background: "#0d1420", borderColor: "#1d324a" }}
          >
            <h2 className="text-xl font-bold">Request received</h2>
            <p className="mt-3 leading-7" style={{ color: "#9fb1c7" }}>
              Thanks. The request has been captured for follow-up. The next conversation anchors on one workload, current baseline, painful constraint, success metric, and desired path: proof review, technical evaluation, design-partner evaluation, licensing review, investor diligence, or no-fit.
            </p>
            <p className="mt-3 text-sm leading-6" style={{ color: "#9fb1c7" }}>
              To reserve review time now, use the proof-review or technical-evaluation options on the{" "}
              <Link href="/pricing" style={{ color: "#22d3ee" }}>
                evaluation page
              </Link>
              .
            </p>
          </div>
        ) : (
          <form
            onSubmit={handleSubmit}
            className="mt-8 space-y-5 rounded-lg p-4 md:p-6"
            style={{ background: "#0d1420", border: "1px solid #1d324a" }}
          >
            <div className="grid gap-5 md:grid-cols-2">
              <div>
                <label htmlFor="contact-name" className="mb-1.5 block text-sm font-medium">Name</label>
                <input
                  id="contact-name"
                  type="text"
                  autoComplete="name"
                  required
                  value={form.name}
                  onChange={(e) => update("name", e.target.value)}
                  className={fieldClass}
                  style={inputStyle}
                />
              </div>
              <div>
                <label htmlFor="contact-email" className="mb-1.5 block text-sm font-medium">Work email</label>
                <input
                  id="contact-email"
                  type="email"
                  autoComplete="email"
                  inputMode="email"
                  required
                  value={form.email}
                  onChange={(e) => update("email", e.target.value)}
                  className={fieldClass}
                  style={inputStyle}
                />
              </div>
            </div>

            <div className="grid gap-5 md:grid-cols-2">
              <div>
                <label htmlFor="contact-company" className="mb-1.5 block text-sm font-medium">Company</label>
                <input
                  id="contact-company"
                  type="text"
                  autoComplete="organization"
                  value={form.company}
                  onChange={(e) => update("company", e.target.value)}
                  className={fieldClass}
                  style={inputStyle}
                />
              </div>
              <div>
                <label htmlFor="contact-role" className="mb-1.5 block text-sm font-medium">Role</label>
                <select
                  id="contact-role"
                  value={form.role}
                  onChange={(e) => update("role", e.target.value)}
                  className={fieldClass}
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
                <label htmlFor="contact-request" className="mb-1.5 block text-sm font-medium">Request</label>
                <select
                  id="contact-request"
                  value={form.requestType}
                  onChange={(e) => updateRequestType(e.target.value)}
                  className={fieldClass}
                  style={inputStyle}
                >
                  {requestTypes.map((item) => (
                    <option key={item} value={item}>{item}</option>
                  ))}
                </select>
              </div>
              <div>
                <label htmlFor="contact-pain" className="mb-1.5 block text-sm font-medium">Pain category</label>
                <select
                  id="contact-pain"
                  value={form.painCategory}
                  onChange={(e) => update("painCategory", e.target.value)}
                  className={fieldClass}
                  style={inputStyle}
                >
                  {painCategories.map((item) => (
                    <option key={item} value={item}>{item}</option>
                  ))}
                </select>
              </div>
              <div>
                <label htmlFor="contact-timeline" className="mb-1.5 block text-sm font-medium">Timeline</label>
                <select
                  id="contact-timeline"
                  value={form.timeline}
                  onChange={(e) => update("timeline", e.target.value)}
                  className={fieldClass}
                  style={inputStyle}
                >
                  {timelines.map((item) => (
                    <option key={item} value={item}>{item}</option>
                  ))}
                </select>
              </div>
            </div>

            <div>
              <label htmlFor="contact-usecase" className="mb-1.5 block text-sm font-medium">Use case or workload</label>
              <textarea
                id="contact-usecase"
                required
                rows={4}
                value={form.useCase}
                onChange={(e) => update("useCase", e.target.value)}
                placeholder="Example: sanitized battery-limited edge telemetry, AI-at-the-edge context movement, embedded state tracking, robotics or industrial control, remote reliability, or data-center state movement..."
                className={textareaClass}
                style={inputStyle}
              />
            </div>

            <div>
              <label htmlFor="contact-stack" className="mb-1.5 block text-sm font-medium">Current stack or constraints</label>
              <textarea
                id="contact-stack"
                rows={3}
                value={form.currentStack}
                onChange={(e) => update("currentStack", e.target.value)}
                placeholder="Sanitized runtime, target hardware class, approximate state size, update cadence, current baseline, battery, heat, bandwidth, latency, footprint, reliability, cost, or decision threshold..."
                className={textareaClass}
                style={inputStyle}
              />
            </div>

            <div>
              <label htmlFor="contact-message" className="mb-1.5 block text-sm font-medium">Anything else</label>
              <textarea
                id="contact-message"
                rows={3}
                value={form.message}
                onChange={(e) => update("message", e.target.value)}
                className={textareaClass}
                style={inputStyle}
              />
            </div>

            {status === "error" && (
              <p className="text-sm" style={{ color: "#fca5a5" }}>
                Something went wrong. Email matthew.h.rockwell@gmail.com directly if the form does not submit.
              </p>
            )}

            <button
              type="submit"
              disabled={status === "sending"}
              className="min-h-11 w-full rounded-lg py-3 text-sm font-semibold transition-opacity hover:opacity-90 disabled:opacity-50"
              style={{ background: "#2563eb", color: "#fff" }}
            >
              {status === "sending" ? "Sending..." : form.requestType}
            </button>
          </form>
        )}
      </div>
    </div>
  );
}
