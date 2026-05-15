"use client";

import { useState, useEffect } from "react";

type Platform = "linux" | "macos" | "windows" | "unknown";

interface PackageInfo {
  id: string;
  name: string;
  description: string;
  installCommand: string;
  installCommandAlt?: Record<string, string>;
  version: string;
  platform: "all" | Platform;
  tierMin: "community" | "pro" | "team" | "enterprise";
  icon: string;
  language: string;
}

const packages: PackageInfo[] = [
  {
    id: "python-sdk",
    name: "Python SDK",
    description: "Core delta-state algebra. Zero dependencies, pure Python.",
    installCommand: "pip install atomik-core",
    version: "v0.4.0",
    platform: "all",
    tierMin: "community",
    icon: "\u{1F40D}",
    language: "Python",
  },
  {
    id: "c-header",
    name: "C Header Library",
    description: "Single-header C implementation. Drop into any project.",
    installCommand: "curl -sL https://get.atomik.tech/atomik.h -o atomik.h",
    version: "v0.4.0",
    platform: "all",
    tierMin: "community",
    icon: "\u2699",
    language: "C",
  },
  {
    id: "js-sdk",
    name: "JavaScript SDK",
    description: "TypeScript-first, works in Node.js and browsers.",
    installCommand: "npm install @atomik/core",
    version: "v0.4.0",
    platform: "all",
    tierMin: "community",
    icon: "\u25B3",
    language: "JavaScript",
  },
  {
    id: "kernel-module",
    name: "Linux Kernel Module",
    description: "COW detection, network dedup, cgroup tracking. DKMS install.",
    installCommand: "sudo apt install atomik-kmod",
    installCommandAlt: {
      deb: "sudo apt install atomik-kmod",
      rpm: "sudo dnf install atomik-kmod",
      dkms: "sudo dkms install atomik/0.4.0",
    },
    version: "v0.4.0",
    platform: "linux",
    tierMin: "pro",
    icon: "\u{1F427}",
    language: "Kernel",
  },
  {
    id: "sdk-generator",
    name: "SDK Generator Pipeline",
    description: "Generate SDKs in 5 languages from domain schemas.",
    installCommand: "pip install atomik-sdk",
    version: "v0.4.0",
    platform: "all",
    tierMin: "team",
    icon: "\u{1F3ED}",
    language: "Multi-lang",
  },
];

function detectPlatform(): Platform {
  if (typeof navigator === "undefined") return "unknown";
  const ua = navigator.userAgent.toLowerCase();
  if (ua.includes("linux")) return "linux";
  if (ua.includes("mac")) return "macos";
  if (ua.includes("win")) return "windows";
  return "unknown";
}

const platformLabels: Record<Platform, string> = {
  linux: "Linux",
  macos: "macOS",
  windows: "Windows",
  unknown: "Your Platform",
};

const tierColors: Record<string, string> = {
  community: "#22c55e",
  pro: "#4f8fff",
  team: "#22d3ee",
  enterprise: "#8b5cf6",
};

const tierLabels: Record<string, string> = {
  community: "Public repo",
  pro: "Evaluation",
  team: "SDK evaluation",
  enterprise: "Enterprise / IP",
};

export default function DownloadCenter({ currentTier = "pro" }: { currentTier?: string }) {
  const [platform, setPlatform] = useState<Platform>("unknown");
  const [showAll, setShowAll] = useState(false);
  const [copiedId, setCopiedId] = useState<string | null>(null);
  const [kmodVariant, setKmodVariant] = useState<string>("deb");

  useEffect(() => {
    const id = window.setTimeout(() => setPlatform(detectPlatform()), 0);
    return () => window.clearTimeout(id);
  }, []);

  function handleCopy(id: string, command: string) {
    navigator.clipboard.writeText(command);
    setCopiedId(id);
    setTimeout(() => setCopiedId(null), 2000);
  }

  const tierRank: Record<string, number> = { community: 0, pro: 1, team: 2, enterprise: 3 };
  const userRank = tierRank[currentTier] ?? 0;

  const relevantPackages = showAll
    ? packages
    : packages.filter(
        (p) => p.platform === "all" || p.platform === platform
      );

  return (
    <div>
      {/* Platform Detection Bar */}
      <div
        className="flex items-center justify-between mb-5 px-4 py-3 rounded-lg"
        style={{ background: "rgba(79,143,255,0.06)", border: "1px solid rgba(79,143,255,0.15)" }}
      >
        <div className="flex items-center gap-3">
          <span className="text-sm font-medium" style={{ color: "#4f8fff" }}>
            Detected: {platformLabels[platform]}
          </span>
          <span className="text-xs" style={{ color: "#8888a0" }}>
            Showing recommended packages
          </span>
        </div>
        <button
          onClick={() => setShowAll(!showAll)}
          className="text-xs font-medium px-3 py-1.5 rounded-md transition-colors"
          style={{
            color: "#4f8fff",
            background: "rgba(79,143,255,0.1)",
            border: "1px solid rgba(79,143,255,0.2)",
          }}
        >
          {showAll ? "Show Recommended" : "All Platforms"}
        </button>
      </div>

      {/* Package Grid */}
      <div className="space-y-3">
        {relevantPackages.map((pkg) => {
          const locked = tierRank[pkg.tierMin] > userRank;
          const activeCommand =
            pkg.id === "kernel-module" && pkg.installCommandAlt
              ? pkg.installCommandAlt[kmodVariant]
              : pkg.installCommand;

          return (
            <div
              key={pkg.id}
              className="rounded-xl border p-5 transition-all"
              style={{
                background: locked ? "rgba(18,18,26,0.5)" : "#12121a",
                borderColor: locked ? "#1a1a28" : "#1e1e2e",
                opacity: locked ? 0.6 : 1,
              }}
            >
              <div className="flex items-start justify-between gap-4 mb-3">
                <div className="flex items-center gap-3">
                  <span className="text-xl">{pkg.icon}</span>
                  <div>
                    <div className="flex items-center gap-2">
                      <h4 className="font-semibold text-sm" style={{ color: "#e0e0e8" }}>
                        {pkg.name}
                      </h4>
                      <span
                        className="text-[10px] font-mono font-semibold px-1.5 py-0.5 rounded"
                        style={{
                          background: "rgba(139,92,246,0.1)",
                          color: "#8b5cf6",
                          border: "1px solid rgba(139,92,246,0.2)",
                        }}
                      >
                        {pkg.version}
                      </span>
                      {pkg.tierMin !== "community" && (
                        <span
                          className="text-[10px] font-semibold uppercase tracking-wider px-1.5 py-0.5 rounded"
                          style={{
                            background: `${tierColors[pkg.tierMin]}12`,
                            color: tierColors[pkg.tierMin],
                            border: `1px solid ${tierColors[pkg.tierMin]}30`,
                          }}
                        >
                          {tierLabels[pkg.tierMin]}+
                        </span>
                      )}
                    </div>
                    <p className="text-xs mt-0.5" style={{ color: "#8888a0" }}>
                      {pkg.description}
                    </p>
                  </div>
                </div>
                <span
                  className="shrink-0 text-[10px] font-mono px-2 py-1 rounded"
                  style={{ background: "rgba(255,255,255,0.04)", color: "#8888a0" }}
                >
                  {pkg.language}
                </span>
              </div>

              {locked ? (
                <div
                  className="flex items-center gap-2 px-3 py-2.5 rounded-lg text-xs"
                  style={{
                    background: `${tierColors[pkg.tierMin]}08`,
                    border: `1px solid ${tierColors[pkg.tierMin]}20`,
                    color: tierColors[pkg.tierMin],
                  }}
                >
                  <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5">
                    <rect x="3" y="11" width="18" height="11" rx="2" ry="2" />
                    <path d="M7 11V7a5 5 0 0 1 10 0v4" />
                  </svg>
                  Upgrade to {tierLabels[pkg.tierMin]} to download
                </div>
              ) : (
                <div className="flex items-center gap-2">
                  {pkg.id === "kernel-module" && pkg.installCommandAlt && (
                    <div className="flex rounded-lg overflow-hidden mr-2" style={{ border: "1px solid #1e1e2e" }}>
                      {Object.keys(pkg.installCommandAlt).map((variant) => (
                        <button
                          key={variant}
                          onClick={() => setKmodVariant(variant)}
                          className="px-2.5 py-1.5 text-[11px] font-mono transition-colors"
                          style={{
                            background: kmodVariant === variant ? "rgba(79,143,255,0.15)" : "transparent",
                            color: kmodVariant === variant ? "#4f8fff" : "#8888a0",
                          }}
                        >
                          {variant}
                        </button>
                      ))}
                    </div>
                  )}
                  <div
                    className="flex-1 flex items-center gap-2 px-3 py-2.5 rounded-lg font-mono text-xs"
                    style={{
                      background: "#0a0a0f",
                      border: "1px solid #1e1e2e",
                      color: "#22d3ee",
                    }}
                  >
                    <span className="mr-1" style={{ color: "#8888a0" }}>$</span>
                    <span className="flex-1 overflow-x-auto whitespace-nowrap">{activeCommand}</span>
                  </div>
                  <button
                    onClick={() => handleCopy(pkg.id, activeCommand)}
                    className="shrink-0 px-3 py-2.5 rounded-lg text-xs font-medium transition-all"
                    style={{
                      background: copiedId === pkg.id ? "rgba(34,197,94,0.15)" : "rgba(79,143,255,0.1)",
                      color: copiedId === pkg.id ? "#22c55e" : "#4f8fff",
                      border: copiedId === pkg.id
                        ? "1px solid rgba(34,197,94,0.3)"
                        : "1px solid rgba(79,143,255,0.2)",
                    }}
                  >
                    {copiedId === pkg.id ? "Copied!" : "Copy"}
                  </button>
                </div>
              )}
            </div>
          );
        })}
      </div>
    </div>
  );
}
