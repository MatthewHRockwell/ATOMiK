"use client";

import { useState, FormEvent } from "react";
import Link from "next/link";

interface EmailCaptureProps {
  variant?: "default" | "blog";
}

export default function EmailCapture({ variant = "default" }: EmailCaptureProps) {
  const [email, setEmail] = useState("");
  const [status, setStatus] = useState<"idle" | "loading" | "success" | "error">("idle");
  const [errorMsg, setErrorMsg] = useState("");
  const [copied, setCopied] = useState(false);

  const isBlog = variant === "blog";

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
          source: isBlog ? "blog-capture" : undefined,
        }),
      });

      const data = await res.json();

      if (!res.ok) {
        setStatus("error");
        setErrorMsg(data.error || "Something went wrong");
        return;
      }

      setStatus("success");
      setEmail("");
    } catch {
      setStatus("error");
      setErrorMsg("Network error. Please try again.");
    }
  }

  function handleCopy() {
    navigator.clipboard.writeText("pip install atomik-core");
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  }

  if (status === "success") {
    if (isBlog) {
      return (
        <div
          className="rounded-lg p-6 text-center"
          style={{ background: "#12121a", border: "1px solid #1e1e2e" }}
        >
          <p className="text-lg font-semibold text-white mb-3">You&apos;re in!</p>
          <div
            className="inline-flex items-center gap-2 rounded-md px-4 py-2.5 font-mono text-sm"
            style={{ background: "#0a0a0f", border: "1px solid #1e1e2e" }}
          >
            <span style={{ color: "#22d3ee" }}>pip install atomik-core</span>
            <button
              onClick={handleCopy}
              className="ml-2 px-2 py-1 rounded text-xs font-medium transition-colors"
              style={{
                background: copied ? "rgba(34, 197, 94, 0.15)" : "rgba(79, 143, 255, 0.15)",
                color: copied ? "#22c55e" : "#4f8fff",
                border: `1px solid ${copied ? "rgba(34, 197, 94, 0.3)" : "rgba(79, 143, 255, 0.3)"}`,
              }}
            >
              {copied ? "Copied!" : "Copy"}
            </button>
          </div>
          <div className="mt-4">
            <Link
              href="/docs/hardware"
              className="text-sm font-medium hover:underline"
              style={{ color: "#4f8fff" }}
            >
              View hardware proof &rarr;
            </Link>
          </div>
        </div>
      );
    }

    return (
      <div
        className="rounded-lg p-6 text-center"
        style={{ background: "#12121a", border: "1px solid #1e1e2e" }}
      >
        <p className="text-lg font-semibold text-white">You&apos;re in!</p>
        <p className="text-sm mt-1" style={{ color: "#8888a0" }}>
          We&apos;ll keep you posted on releases and updates.
        </p>
      </div>
    );
  }

  return (
    <div
      className="rounded-lg p-6"
      style={{ background: "#12121a", border: "1px solid #1e1e2e" }}
    >
      <h3 className="text-lg font-semibold text-white">
        {isBlog ? "Get the SDK" : "Stay in the loop"}
      </h3>
      <p className="text-sm mt-1 mb-4" style={{ color: "#8888a0" }}>
        {isBlog
          ? "Request updates and evaluation access for delta-state work"
          : "Release notes, technical articles, and hardware updates. No spam."}
      </p>
      <form
        onSubmit={handleSubmit}
        className="flex flex-col sm:flex-row gap-2"
      >
        <input
          type="email"
          placeholder="you@example.com"
          value={email}
          onChange={(e) => {
            setEmail(e.target.value);
            if (status === "error") setStatus("idle");
          }}
          required
          className="flex-1 px-4 py-2 rounded-md text-sm text-white placeholder-gray-500 outline-none focus:ring-2 focus:ring-[#4f8fff]"
          style={{
            background: "#0a0a0f",
            border: "1px solid #1e1e2e",
          }}
        />
        <button
          type="submit"
          disabled={status === "loading"}
          className="px-5 py-2 rounded-md text-sm font-medium text-white transition-opacity hover:opacity-90 disabled:opacity-50 whitespace-nowrap"
          style={{ background: "#4f8fff" }}
        >
          {status === "loading"
            ? isBlog ? "Registering..." : "Subscribing..."
            : isBlog ? "Request Access" : "Subscribe"}
        </button>
      </form>
      {status === "error" && (
        <p className="text-sm mt-2" style={{ color: "#ef4444" }}>
          {errorMsg}
        </p>
      )}
    </div>
  );
}
