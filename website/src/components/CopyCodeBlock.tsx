"use client";

import { useState } from "react";

export function CopyCodeBlock({ code }: { code: string }) {
  const [copied, setCopied] = useState(false);

  function handleCopy() {
    navigator.clipboard.writeText(code);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  }

  return (
    <div className="relative">
      <pre
        className="rounded-lg p-4 text-sm overflow-x-auto pr-20"
        style={{
          background: "#0a0a0f",
          border: "1px solid #1e1e2e",
          color: "#22d3ee",
          fontFamily: "'SF Mono', 'Fira Code', 'Consolas', monospace",
        }}
      >
        <code>{code}</code>
      </pre>
      <button
        onClick={handleCopy}
        className="absolute top-3 right-3 px-2.5 py-1 rounded text-xs font-medium transition-all"
        style={{
          background: copied ? "rgba(34,197,94,0.15)" : "rgba(79,143,255,0.1)",
          color: copied ? "#22c55e" : "#4f8fff",
          border: copied
            ? "1px solid rgba(34,197,94,0.3)"
            : "1px solid rgba(79,143,255,0.2)",
        }}
      >
        {copied ? "Copied!" : "Copy"}
      </button>
    </div>
  );
}

export function CopyInstallButton({ command }: { command: string }) {
  const [copied, setCopied] = useState(false);

  function handleCopy() {
    navigator.clipboard.writeText(command);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  }

  return (
    <button
      onClick={handleCopy}
      className="w-full text-center py-2.5 rounded-lg text-sm font-semibold transition-all"
      style={{
        background: copied ? "rgba(34,197,94,0.15)" : "#22c55e",
        color: copied ? "#22c55e" : "#fff",
        border: copied ? "1px solid rgba(34,197,94,0.3)" : "1px solid transparent",
      }}
    >
      {copied ? "Copied to clipboard!" : "Copy: pip install atomik-core"}
    </button>
  );
}
