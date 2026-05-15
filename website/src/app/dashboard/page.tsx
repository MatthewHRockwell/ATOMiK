"use client";

import Link from "next/link";
import Nav from "@/components/Nav";
import { CopyCodeBlock } from "@/components/CopyCodeBlock";
import DownloadCenter from "@/components/DownloadCenter";
import { useState, useEffect, useRef, useCallback } from "react";

/* ─────────────────────────── Mock Data ─────────────────────────── */

const MOCK_USER = {
  name: "Matt Rockwell",
  email: "mrockwell@atomik.tech",
  plan: "pro" as const,
  licenseKey: "ATK-PRO-8F3A-29D1-CC47-B0E6",
  renewalDate: "2026-04-18",
  memberSince: "2025-12-01",
};

const MOCK_NOW_MS = new Date("2026-03-20T00:00:00Z").getTime();

const PLAN_META: Record<
  string,
  { color: string; label: string; rank: number; gradient: string }
> = {
  community: {
    color: "#22c55e",
    label: "Community",
    rank: 0,
    gradient: "linear-gradient(135deg, #22c55e, #16a34a)",
  },
  pro: {
    color: "#4f8fff",
    label: "Pro",
    rank: 1,
    gradient: "linear-gradient(135deg, #4f8fff, #3a7aee)",
  },
  team: {
    color: "#22d3ee",
    label: "Team",
    rank: 2,
    gradient: "linear-gradient(135deg, #22d3ee, #06b6d4)",
  },
  enterprise: {
    color: "#8b5cf6",
    label: "Enterprise",
    rank: 3,
    gradient: "linear-gradient(135deg, #8b5cf6, #7c3aed)",
  },
};

const MOCK_METRICS = {
  opsPerSec: 4_280_000,
  bandwidthSavedGB: 127.4,
  activeContexts: 1_842,
  wasteDetectedMB: 3_291,
  uptimePercent: 99.97,
  deltasProcessed: 48_291_044,
};

const MOCK_API_KEYS = [
  {
    id: "key_1",
    name: "Production Pipeline",
    prefix: "atk_live_7f3a",
    created: "2026-01-15",
    lastUsed: "2026-03-17",
    status: "active" as const,
  },
  {
    id: "key_2",
    name: "CI/CD Integration",
    prefix: "atk_live_9b2c",
    created: "2026-02-20",
    lastUsed: "2026-03-18",
    status: "active" as const,
  },
  {
    id: "key_3",
    name: "Staging Environment",
    prefix: "atk_test_d4e1",
    created: "2025-12-10",
    lastUsed: "2026-02-28",
    status: "revoked" as const,
  },
];

const QUICK_START_CARDS = [
  {
    title: "First Delta",
    description: "Create a context, load initial state, accumulate a delta, read the result.",
    icon: "\u25B6",
    color: "#4f8fff",
    code: `from atomik_core import AtomikContext

ctx = AtomikContext()
ctx.load(0xDEADBEEF)
ctx.accum(0x0000FFFF)
state = ctx.read()
print(f"State: 0x{state:08X}")  # 0xDEAD1410`,
  },
  {
    title: "Change Detection",
    description: "Fingerprint a memory region and detect mutations in O(1).",
    icon: "\u{1F50D}",
    color: "#22c55e",
    code: `from atomik_core import Fingerprint

fp = Fingerprint()
fp.load(original_data)
fp.update(possibly_changed)
if fp.changed:
    print(f"Delta: 0x{fp.delta:x}")`,
  },
  {
    title: "Multi-Node Sync",
    description: "Converge state across nodes with order-independent delta streaming.",
    icon: "\u{1F310}",
    color: "#22d3ee",
    code: `from atomik_core import DeltaStream

stream = DeltaStream(nodes=["A", "B", "C"])
stream.emit("A", delta=0xFF00)
stream.emit("B", delta=0x00FF)
# All nodes converge to same state
assert stream.state("A") == stream.state("C")`,
  },
  {
    title: "Benchmark",
    description: "Measure throughput on your hardware and get a shareable results card.",
    icon: "\u26A1",
    color: "#8b5cf6",
    code: `# Run from terminal:
python -m atomik_core benchmark --share

# Programmatic:
from atomik_core import benchmark
results = benchmark.run(iterations=1_000_000)
print(f"{results.ops_per_sec:,.0f} measured iterations/s")`,
  },
];

/* ─────────────────────────── Gauge Component ─────────────────────────── */

function GaugeChart({
  value,
  max,
  label,
  color,
  format,
}: {
  value: number;
  max: number;
  label: string;
  color: string;
  format: (v: number) => string;
}) {
  const ref = useRef<HTMLDivElement>(null);
  const [visible, setVisible] = useState(false);
  const [animValue, setAnimValue] = useState(0);

  useEffect(() => {
    const el = ref.current;
    if (!el) return;
    const observer = new IntersectionObserver(
      ([entry]) => {
        if (entry.isIntersecting) {
          setVisible(true);
          observer.disconnect();
        }
      },
      { threshold: 0.3 }
    );
    observer.observe(el);
    return () => observer.disconnect();
  }, []);

  useEffect(() => {
    if (!visible) return;
    let start: number | null = null;
    let raf: number;
    function step(ts: number) {
      if (!start) start = ts;
      const progress = Math.min((ts - start) / 1600, 1);
      const eased = 1 - Math.pow(1 - progress, 3);
      setAnimValue(eased * value);
      if (progress < 1) raf = requestAnimationFrame(step);
    }
    raf = requestAnimationFrame(step);
    return () => cancelAnimationFrame(raf);
  }, [visible, value]);

  const percent = Math.min((animValue / max) * 100, 100);
  const circumference = 2 * Math.PI * 40;
  const dashLen = (percent / 100) * circumference * 0.75; // 270-degree arc

  return (
    <div ref={ref} className="flex flex-col items-center">
      <div className="relative w-24 h-24">
        <svg viewBox="0 0 100 100" className="w-full h-full -rotate-[135deg]">
          <circle
            cx="50"
            cy="50"
            r="40"
            fill="none"
            stroke="#1e1e2e"
            strokeWidth="6"
            strokeDasharray={`${circumference * 0.75} ${circumference * 0.25}`}
            strokeLinecap="round"
          />
          <circle
            cx="50"
            cy="50"
            r="40"
            fill="none"
            stroke={color}
            strokeWidth="6"
            strokeDasharray={`${dashLen} ${circumference - dashLen}`}
            strokeLinecap="round"
            style={{ transition: "stroke-dasharray 0.3s ease" }}
          />
        </svg>
        <div className="absolute inset-0 flex items-center justify-center">
          <span
            className="text-sm font-bold font-mono tabular-nums"
            style={{ color }}
          >
            {format(animValue)}
          </span>
        </div>
      </div>
      <span className="text-[11px] mt-2 text-center font-medium" style={{ color: "#8888a0" }}>
        {label}
      </span>
    </div>
  );
}

/* ────────────────────── Sparkline Micro-Chart ────────────────────── */

function Sparkline({ data, color }: { data: number[]; color: string }) {
  const max = Math.max(...data);
  const min = Math.min(...data);
  const range = max - min || 1;
  const w = 120;
  const h = 32;
  const points = data
    .map((v, i) => {
      const x = (i / (data.length - 1)) * w;
      const y = h - ((v - min) / range) * (h - 4) - 2;
      return `${x},${y}`;
    })
    .join(" ");

  return (
    <svg width={w} height={h} className="shrink-0">
      <polyline
        points={points}
        fill="none"
        stroke={color}
        strokeWidth="1.5"
        strokeLinecap="round"
        strokeLinejoin="round"
      />
      <defs>
        <linearGradient id={`spark-${color.replace("#", "")}`} x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stopColor={color} stopOpacity="0.15" />
          <stop offset="100%" stopColor={color} stopOpacity="0" />
        </linearGradient>
      </defs>
      <polygon
        points={`0,${h} ${points} ${w},${h}`}
        fill={`url(#spark-${color.replace("#", "")})`}
      />
    </svg>
  );
}

/* ─────────────────────── Metric Card (Pro+) ─────────────────────── */

function MetricCard({
  label,
  value,
  suffix,
  color,
  sparkData,
  change,
}: {
  label: string;
  value: string;
  suffix?: string;
  color: string;
  sparkData: number[];
  change: string;
}) {
  const isPositive = change.startsWith("+");
  return (
    <div
      className="rounded-xl border p-4"
      style={{ background: "#12121a", borderColor: "#1e1e2e" }}
    >
      <div className="flex items-start justify-between mb-3">
        <span className="text-[11px] font-medium uppercase tracking-wider" style={{ color: "#8888a0" }}>
          {label}
        </span>
        <span
          className="text-[11px] font-semibold px-1.5 py-0.5 rounded"
          style={{
            background: isPositive ? "rgba(34,197,94,0.1)" : "rgba(239,68,68,0.1)",
            color: isPositive ? "#22c55e" : "#ef4444",
          }}
        >
          {change}
        </span>
      </div>
      <div className="flex items-end justify-between">
        <div>
          <span className="text-2xl font-bold font-mono tabular-nums" style={{ color }}>
            {value}
          </span>
          {suffix && (
            <span className="text-xs ml-1" style={{ color: "#8888a0" }}>
              {suffix}
            </span>
          )}
        </div>
        <Sparkline data={sparkData} color={color} />
      </div>
    </div>
  );
}

/* ──────────────────────── Section Header ──────────────────────── */

function SectionHeader({
  title,
  badge,
  badgeColor,
}: {
  title: string;
  badge?: string;
  badgeColor?: string;
}) {
  return (
    <div className="flex items-center gap-3 mb-5">
      <h2 className="text-lg font-bold" style={{ color: "#e0e0e8" }}>
        {title}
      </h2>
      {badge && (
        <span
          className="text-[10px] font-semibold uppercase tracking-wider px-2 py-0.5 rounded-full"
          style={{
            background: `${badgeColor}15`,
            color: badgeColor,
            border: `1px solid ${badgeColor}30`,
          }}
        >
          {badge}
        </span>
      )}
    </div>
  );
}

/* ──────────────────────── Dashboard Sidebar Nav ──────────────────────── */

const SECTIONS = [
  { id: "license", label: "License & Plan", icon: "\u229A" },
  { id: "downloads", label: "Downloads", icon: "\u2913" },
  { id: "quickstart", label: "Quick Start", icon: "\u25B7" },
  { id: "metrics", label: "Usage Metrics", icon: "\u2261" },
  { id: "api-keys", label: "API Keys", icon: "\u{1F511}" },
  { id: "support", label: "Support", icon: "\u2709" },
];

function DashboardSidebar({
  activeSection,
  onNavigate,
}: {
  activeSection: string;
  onNavigate: (id: string) => void;
}) {
  return (
    <nav className="hidden lg:block w-52 shrink-0">
      <div
        className="sticky top-24 rounded-xl border p-3 space-y-0.5"
        style={{ background: "#12121a", borderColor: "#1e1e2e" }}
      >
        {SECTIONS.map((s) => (
          <button
            key={s.id}
            onClick={() => onNavigate(s.id)}
            className="w-full text-left px-3 py-2 rounded-lg text-sm flex items-center gap-2.5 transition-colors"
            style={{
              background: activeSection === s.id ? "rgba(79,143,255,0.1)" : "transparent",
              color: activeSection === s.id ? "#4f8fff" : "#8888a0",
            }}
          >
            <span className="text-xs">{s.icon}</span>
            {s.label}
          </button>
        ))}
      </div>
    </nav>
  );
}

/* ══════════════════════════ MAIN PAGE ══════════════════════════ */

export default function DashboardPage() {
  const [licenseRevealed, setLicenseRevealed] = useState(false);
  const [licenseCopied, setLicenseCopied] = useState(false);
  const [activeSection, setActiveSection] = useState("license");
  const [apiKeys, setApiKeys] = useState(MOCK_API_KEYS);
  const [showNewKeyModal, setShowNewKeyModal] = useState(false);
  const [newKeyName, setNewKeyName] = useState("");

  const plan = PLAN_META[MOCK_USER.plan];
  const isPro = plan.rank >= 1;
  const isTeam = plan.rank >= 2;

  /* ── Scroll-spy ── */
  useEffect(() => {
    const observer = new IntersectionObserver(
      (entries) => {
        for (const entry of entries) {
          if (entry.isIntersecting) {
            setActiveSection(entry.target.id);
          }
        }
      },
      { rootMargin: "-20% 0px -60% 0px" }
    );
    SECTIONS.forEach((s) => {
      const el = document.getElementById(s.id);
      if (el) observer.observe(el);
    });
    return () => observer.disconnect();
  }, []);

  const scrollTo = useCallback((id: string) => {
    document.getElementById(id)?.scrollIntoView({ behavior: "smooth", block: "start" });
  }, []);

  function copyLicense() {
    navigator.clipboard.writeText(MOCK_USER.licenseKey);
    setLicenseCopied(true);
    setTimeout(() => setLicenseCopied(false), 2000);
  }

  function revokeKey(id: string) {
    setApiKeys((prev) =>
      prev.map((k) => (k.id === id ? { ...k, status: "revoked" as const } : k))
    );
  }

  function generateKey() {
    if (!newKeyName.trim()) return;
    const hex = Math.random().toString(16).slice(2, 6);
    const newKey = {
      id: `key_${Date.now()}`,
      name: newKeyName.trim(),
      prefix: `atk_live_${hex}`,
      created: new Date().toISOString().split("T")[0],
      lastUsed: "Never",
      status: "active" as const,
    };
    setApiKeys((prev) => [newKey, ...prev]);
    setNewKeyName("");
    setShowNewKeyModal(false);
  }

  const daysUntilRenewal = Math.ceil(
    (new Date(MOCK_USER.renewalDate).getTime() - MOCK_NOW_MS) / (1000 * 60 * 60 * 24)
  );

  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav />

      {/* ── Header ── */}
      <header
        className="border-b px-6 py-6"
        style={{ borderColor: "#1e1e2e", background: "rgba(18,18,26,0.5)" }}
      >
        <div className="max-w-6xl mx-auto flex flex-col sm:flex-row items-start sm:items-center justify-between gap-4">
          <div>
            <div className="flex items-center gap-3 mb-1">
              <h1 className="text-2xl font-bold">Developer Dashboard</h1>
              <span
                className="text-[11px] font-bold uppercase tracking-wider px-2.5 py-1 rounded-full"
                style={{
                  background: plan.gradient,
                  color: "#fff",
                }}
              >
                {plan.label}
              </span>
            </div>
            <p className="text-sm" style={{ color: "#8888a0" }}>
              Welcome back, {MOCK_USER.name}. Your tools, metrics, and downloads in one place.
            </p>
          </div>
          <div className="flex items-center gap-3">
            <Link
              href="/docs"
              className="text-xs font-medium px-4 py-2 rounded-lg transition-colors no-underline"
              style={{
                background: "rgba(255,255,255,0.05)",
                color: "#8888a0",
                border: "1px solid #1e1e2e",
              }}
            >
              Docs
            </Link>
            <Link
              href="/pricing"
              className="text-xs font-medium px-4 py-2 rounded-lg transition-colors no-underline"
              style={{
                background: plan.gradient,
                color: "#fff",
              }}
            >
              Upgrade Plan
            </Link>
          </div>
        </div>
      </header>

      {/* ── Main Layout ── */}
      <div className="max-w-6xl mx-auto px-6 py-8 flex gap-8">
        <DashboardSidebar activeSection={activeSection} onNavigate={scrollTo} />

        <main className="flex-1 min-w-0 space-y-10">
          {/* ════════════ LICENSE & PLAN ════════════ */}
          <section id="license" className="scroll-mt-24">
            <SectionHeader title="License & Plan" />
            <div
              className="rounded-xl border overflow-hidden"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              {/* Gradient bar */}
              <div className="h-1" style={{ background: plan.gradient }} />

              <div className="p-6">
                <div className="grid sm:grid-cols-2 gap-6">
                  {/* Plan Info */}
                  <div>
                    <div className="flex items-center gap-3 mb-4">
                      <span
                        className="text-xs font-bold uppercase tracking-wider px-3 py-1.5 rounded-lg"
                        style={{
                          background: `${plan.color}15`,
                          color: plan.color,
                          border: `1px solid ${plan.color}30`,
                        }}
                      >
                        {plan.label} Plan
                      </span>
                      <span className="text-xs" style={{ color: "#8888a0" }}>
                        Member since{" "}
                        {new Date(MOCK_USER.memberSince).toLocaleDateString("en-US", {
                          month: "short",
                          year: "numeric",
                        })}
                      </span>
                    </div>
                    <div className="space-y-3">
                      <div>
                        <label className="text-[11px] uppercase tracking-wider font-medium block mb-1" style={{ color: "#8888a0" }}>
                          Renewal Date
                        </label>
                        <div className="flex items-center gap-2">
                          <span className="text-sm font-medium">
                            {new Date(MOCK_USER.renewalDate).toLocaleDateString("en-US", {
                              month: "long",
                              day: "numeric",
                              year: "numeric",
                            })}
                          </span>
                          <span
                            className="text-[10px] font-medium px-2 py-0.5 rounded-full"
                            style={{
                              background: daysUntilRenewal > 14 ? "rgba(34,197,94,0.1)" : "rgba(245,158,11,0.1)",
                              color: daysUntilRenewal > 14 ? "#22c55e" : "#f59e0b",
                            }}
                          >
                            {daysUntilRenewal} days
                          </span>
                        </div>
                      </div>
                      <div>
                        <label className="text-[11px] uppercase tracking-wider font-medium block mb-1" style={{ color: "#8888a0" }}>
                          Email
                        </label>
                        <span className="text-sm">{MOCK_USER.email}</span>
                      </div>
                    </div>
                  </div>

                  {/* License Key */}
                  <div>
                    <label className="text-[11px] uppercase tracking-wider font-medium block mb-2" style={{ color: "#8888a0" }}>
                      License Key
                    </label>
                    <div
                      className="flex items-center gap-2 px-4 py-3 rounded-lg font-mono text-sm"
                      style={{
                        background: "#0a0a0f",
                        border: "1px solid #1e1e2e",
                      }}
                    >
                      <span className="flex-1 select-all" style={{ color: licenseRevealed ? "#22d3ee" : "#8888a0" }}>
                        {licenseRevealed
                          ? MOCK_USER.licenseKey
                          : MOCK_USER.licenseKey.replace(/[A-Z0-9]/g, "\u2022")}
                      </span>
                      <button
                        onClick={() => setLicenseRevealed(!licenseRevealed)}
                        className="shrink-0 text-[11px] px-2 py-1 rounded transition-colors"
                        style={{
                          color: "#8888a0",
                          background: "rgba(255,255,255,0.05)",
                        }}
                        title={licenseRevealed ? "Hide" : "Reveal"}
                      >
                        {licenseRevealed ? "Hide" : "Show"}
                      </button>
                      <button
                        onClick={copyLicense}
                        className="shrink-0 text-[11px] px-2 py-1 rounded transition-colors"
                        style={{
                          color: licenseCopied ? "#22c55e" : "#4f8fff",
                          background: licenseCopied ? "rgba(34,197,94,0.1)" : "rgba(79,143,255,0.1)",
                        }}
                      >
                        {licenseCopied ? "Copied!" : "Copy"}
                      </button>
                    </div>
                    <p className="text-[11px] mt-2" style={{ color: "#8888a0" }}>
                      Use this key in your <code className="font-mono" style={{ color: "#22d3ee" }}>atomik.conf</code> or pass via <code className="font-mono" style={{ color: "#22d3ee" }}>ATOMIK_LICENSE_KEY</code> environment variable.
                    </p>

                    {plan.rank < 3 && (
                      <Link
                        href="/pricing"
                        className="inline-flex items-center gap-1.5 mt-4 text-xs font-semibold no-underline px-4 py-2 rounded-lg transition-opacity hover:opacity-90"
                        style={{ background: plan.gradient, color: "#fff" }}
                      >
                        <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5">
                          <path d="M12 19V5M5 12l7-7 7 7" />
                        </svg>
                        Upgrade to {PLAN_META[plan.rank === 1 ? "team" : plan.rank === 2 ? "enterprise" : "pro"].label}
                      </Link>
                    )}
                  </div>
                </div>
              </div>
            </div>
          </section>

          {/* ════════════ DOWNLOADS ════════════ */}
          <section id="downloads" className="scroll-mt-24">
            <SectionHeader title="Downloads & Install" badge="v0.4.0" badgeColor="#8b5cf6" />
            <DownloadCenter currentTier={MOCK_USER.plan} />
          </section>

          {/* ════════════ QUICK START ════════════ */}
          <section id="quickstart" className="scroll-mt-24">
            <SectionHeader title="Quick Start" />
            <div className="grid sm:grid-cols-2 gap-4">
              {QUICK_START_CARDS.map((card) => (
                <div
                  key={card.title}
                  className="rounded-xl border overflow-hidden"
                  style={{ background: "#12121a", borderColor: "#1e1e2e" }}
                >
                  {/* Colored top edge */}
                  <div className="h-0.5" style={{ background: card.color }} />
                  <div className="p-5">
                    <div className="flex items-center gap-2 mb-2">
                      <span className="text-base">{card.icon}</span>
                      <h3 className="text-sm font-bold" style={{ color: "#e0e0e8" }}>
                        {card.title}
                      </h3>
                    </div>
                    <p className="text-xs mb-3 leading-relaxed" style={{ color: "#8888a0" }}>
                      {card.description}
                    </p>
                    <CopyCodeBlock code={card.code} />
                  </div>
                </div>
              ))}
            </div>
          </section>

          {/* ════════════ USAGE METRICS ════════════ */}
          <section id="metrics" className="scroll-mt-24">
            <SectionHeader title="Usage Metrics" badge="Live" badgeColor="#22c55e" />

            {isPro ? (
              <>
                {/* Gauge Row */}
                <div
                  className="rounded-xl border p-6 mb-4"
                  style={{ background: "#12121a", borderColor: "#1e1e2e" }}
                >
                  <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-5 gap-6 justify-items-center">
                    <GaugeChart
                      value={MOCK_METRICS.opsPerSec}
                      max={10_000_000}
                      label="Throughput"
                      color="#4f8fff"
                      format={(v) => `${(v / 1_000_000).toFixed(1)}M`}
                    />
                    <GaugeChart
                      value={MOCK_METRICS.bandwidthSavedGB}
                      max={500}
                      label="BW Saved (GB)"
                      color="#22c55e"
                      format={(v) => `${v.toFixed(0)}`}
                    />
                    <GaugeChart
                      value={MOCK_METRICS.activeContexts}
                      max={5000}
                      label="Active Contexts"
                      color="#22d3ee"
                      format={(v) => `${(v / 1000).toFixed(1)}K`}
                    />
                    <GaugeChart
                      value={MOCK_METRICS.wasteDetectedMB}
                      max={10000}
                      label="Waste (MB)"
                      color="#f59e0b"
                      format={(v) => `${(v / 1000).toFixed(1)}K`}
                    />
                    <GaugeChart
                      value={MOCK_METRICS.uptimePercent}
                      max={100}
                      label="Uptime"
                      color="#8b5cf6"
                      format={(v) => `${v.toFixed(1)}%`}
                    />
                  </div>
                </div>

                {/* Detail Cards */}
                <div className="grid sm:grid-cols-2 lg:grid-cols-3 gap-4">
                  <MetricCard
                    label="Operations / sec"
                    value="4.28M"
                    color="#4f8fff"
                    sparkData={[3.1, 3.4, 3.8, 4.0, 3.9, 4.1, 4.2, 4.0, 4.3, 4.28]}
                    change="+12.3%"
                  />
                  <MetricCard
                    label="Bandwidth Saved"
                    value="127.4"
                    suffix="GB"
                    color="#22c55e"
                    sparkData={[82, 89, 94, 101, 108, 112, 118, 121, 125, 127.4]}
                    change="+8.7%"
                  />
                  <MetricCard
                    label="Active Contexts"
                    value="1,842"
                    color="#22d3ee"
                    sparkData={[1200, 1340, 1410, 1520, 1580, 1640, 1710, 1760, 1800, 1842]}
                    change="+5.1%"
                  />
                  <MetricCard
                    label="Waste Detected"
                    value="3.29"
                    suffix="GB"
                    color="#f59e0b"
                    sparkData={[1.8, 2.1, 2.3, 2.5, 2.7, 2.8, 3.0, 3.1, 3.2, 3.29]}
                    change="+15.4%"
                  />
                  <MetricCard
                    label="Deltas Processed"
                    value="48.3M"
                    color="#8b5cf6"
                    sparkData={[28, 31, 34, 37, 39, 41, 43, 45, 47, 48.3]}
                    change="+22.1%"
                  />
                  <MetricCard
                    label="Uptime"
                    value="99.97"
                    suffix="%"
                    color="#22c55e"
                    sparkData={[99.9, 99.95, 99.92, 99.98, 99.95, 99.96, 99.97, 99.95, 99.98, 99.97]}
                    change="+0.02%"
                  />
                </div>
              </>
            ) : (
              /* Locked state for Community tier */
              <div
                className="rounded-xl border p-8 text-center"
                style={{ background: "#12121a", borderColor: "#1e1e2e" }}
              >
                <div
                  className="w-16 h-16 rounded-2xl flex items-center justify-center mx-auto mb-4"
                  style={{ background: "rgba(79,143,255,0.1)", border: "1px solid rgba(79,143,255,0.2)" }}
                >
                  <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="#4f8fff" strokeWidth="1.5">
                    <rect x="3" y="11" width="18" height="11" rx="2" ry="2" />
                    <path d="M7 11V7a5 5 0 0 1 10 0v4" />
                  </svg>
                </div>
                <h3 className="text-lg font-bold mb-2">Upgrade to Pro to see live metrics</h3>
                <p className="text-sm mb-5 max-w-md mx-auto" style={{ color: "#8888a0" }}>
                  Real-time throughput, bandwidth savings, waste detection, and active context
                  monitoring. Track your delta-state infrastructure performance at a glance.
                </p>
                <Link
                  href="/pricing"
                  className="inline-block text-sm font-semibold no-underline px-6 py-2.5 rounded-lg transition-opacity hover:opacity-90"
                  style={{ background: "#4f8fff", color: "#fff" }}
                >
                  Request Evaluation Access
                </Link>
              </div>
            )}
          </section>

          {/* ════════════ API KEYS ════════════ */}
          <section id="api-keys" className="scroll-mt-24">
            <SectionHeader title="API Keys" badge="Team+" badgeColor="#22d3ee" />

            {isTeam ? (
              <div
                className="rounded-xl border overflow-hidden"
                style={{ background: "#12121a", borderColor: "#1e1e2e" }}
              >
                {/* Header */}
                <div
                  className="flex items-center justify-between px-5 py-4"
                  style={{ borderBottom: "1px solid #1e1e2e" }}
                >
                  <span className="text-sm font-medium" style={{ color: "#8888a0" }}>
                    {apiKeys.filter((k) => k.status === "active").length} active key{apiKeys.filter((k) => k.status === "active").length !== 1 ? "s" : ""}
                  </span>
                  <button
                    onClick={() => setShowNewKeyModal(true)}
                    className="text-xs font-semibold px-3 py-1.5 rounded-lg transition-opacity hover:opacity-90"
                    style={{ background: "#22d3ee", color: "#0a0a0f" }}
                  >
                    + Generate Key
                  </button>
                </div>

                {/* Key List */}
                <div className="divide-y" style={{ borderColor: "#1e1e2e" }}>
                  {apiKeys.map((key) => (
                    <div
                      key={key.id}
                      className="flex items-center justify-between px-5 py-4 gap-4"
                      style={{
                        opacity: key.status === "revoked" ? 0.5 : 1,
                        borderColor: "#1e1e2e",
                      }}
                    >
                      <div className="min-w-0">
                        <div className="flex items-center gap-2 mb-0.5">
                          <span className="text-sm font-medium truncate">{key.name}</span>
                          <span
                            className="text-[10px] font-semibold uppercase tracking-wider px-1.5 py-0.5 rounded"
                            style={{
                              background: key.status === "active" ? "rgba(34,197,94,0.1)" : "rgba(239,68,68,0.1)",
                              color: key.status === "active" ? "#22c55e" : "#ef4444",
                            }}
                          >
                            {key.status}
                          </span>
                        </div>
                        <div className="flex items-center gap-3 text-[11px]" style={{ color: "#8888a0" }}>
                          <span className="font-mono">{key.prefix}...{"\u2022".repeat(8)}</span>
                          <span>Created {key.created}</span>
                          <span>Last used {key.lastUsed}</span>
                        </div>
                      </div>
                      {key.status === "active" && (
                        <button
                          onClick={() => revokeKey(key.id)}
                          className="shrink-0 text-[11px] font-medium px-3 py-1.5 rounded-lg transition-colors"
                          style={{
                            color: "#ef4444",
                            background: "rgba(239,68,68,0.08)",
                            border: "1px solid rgba(239,68,68,0.2)",
                          }}
                        >
                          Revoke
                        </button>
                      )}
                    </div>
                  ))}
                </div>
              </div>
            ) : (
              /* Locked state for non-Team tiers */
              <div
                className="rounded-xl border p-8 text-center"
                style={{ background: "#12121a", borderColor: "#1e1e2e" }}
              >
                <div
                  className="w-16 h-16 rounded-2xl flex items-center justify-center mx-auto mb-4"
                  style={{ background: "rgba(34,211,238,0.1)", border: "1px solid rgba(34,211,238,0.2)" }}
                >
                  <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="#22d3ee" strokeWidth="1.5">
                    <path d="M21 2l-2 2m-7.61 7.61a5.5 5.5 0 1 1-7.778 7.778 5.5 5.5 0 0 1 7.777-7.777zm0 0L15.5 7.5m0 0l3 3L22 7l-3-3m-3.5 3.5L19 4" />
                  </svg>
                </div>
                <h3 className="text-lg font-bold mb-2">API Keys require Team plan</h3>
                <p className="text-sm mb-5 max-w-md mx-auto" style={{ color: "#8888a0" }}>
                  Generate and manage API keys for the SDK generation pipeline, CI/CD integration,
                  and programmatic access to your ATOMiK workspace.
                </p>
                <Link
                  href="/pricing"
                  className="inline-block text-sm font-semibold no-underline px-6 py-2.5 rounded-lg transition-opacity hover:opacity-90"
                  style={{ background: "#22d3ee", color: "#0a0a0f" }}
                >
                  Upgrade to Team
                </Link>
              </div>
            )}
          </section>

          {/* ════════════ SUPPORT & RESOURCES ════════════ */}
          <section id="support" className="scroll-mt-24">
            <SectionHeader title="Support & Resources" />
            <div className="grid sm:grid-cols-2 lg:grid-cols-4 gap-4">
              {[
                {
                  title: "Documentation",
                  description: "API reference, architecture guides, and examples.",
                  href: "/docs",
                  icon: (
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5">
                      <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z" />
                      <polyline points="14 2 14 8 20 8" />
                      <line x1="16" y1="13" x2="8" y2="13" />
                      <line x1="16" y1="17" x2="8" y2="17" />
                      <polyline points="10 9 9 9 8 9" />
                    </svg>
                  ),
                  color: "#4f8fff",
                },
                {
                  title: "GitHub Issues",
                  description: "Report bugs, request features, browse open issues.",
                  href: "https://github.com/MatthewHRockwell/ATOMiK/issues",
                  icon: (
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5">
                      <path d="M9 19c-5 1.5-5-2.5-7-3m14 6v-3.87a3.37 3.37 0 0 0-.94-2.61c3.14-.35 6.44-1.54 6.44-7A5.44 5.44 0 0 0 20 4.77 5.07 5.07 0 0 0 19.91 1S18.73.65 16 2.48a13.38 13.38 0 0 0-7 0C6.27.65 5.09 1 5.09 1A5.07 5.07 0 0 0 5 4.77a5.44 5.44 0 0 0-1.5 3.78c0 5.42 3.3 6.61 6.44 7A3.37 3.37 0 0 0 9 18.13V22" />
                    </svg>
                  ),
                  color: "#e0e0e8",
                },
                {
                  title: "Email Support",
                  description: isPro ? "Priority support (48hr SLA)" : plan.rank >= 3 ? "Dedicated support (4hr SLA)" : "Community support via GitHub",
                  href: "mailto:support@atomik.tech",
                  icon: (
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5">
                      <path d="M4 4h16c1.1 0 2 .9 2 2v12c0 1.1-.9 2-2 2H4c-1.1 0-2-.9-2-2V6c0-1.1.9-2 2-2z" />
                      <polyline points="22,6 12,13 2,6" />
                    </svg>
                  ),
                  color: "#22c55e",
                  badge: isPro
                    ? plan.rank >= 3
                      ? "4hr SLA"
                      : "48hr SLA"
                    : undefined,
                  badgeColor: isPro ? "#22c55e" : undefined,
                },
                {
                  title: "Community",
                  description: "Join the discussion on GitHub Discussions.",
                  href: "https://github.com/MatthewHRockwell/ATOMiK/discussions",
                  icon: (
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5">
                      <path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z" />
                    </svg>
                  ),
                  color: "#8b5cf6",
                },
              ].map((item) => (
                <a
                  key={item.title}
                  href={item.href}
                  target={item.href.startsWith("http") || item.href.startsWith("mailto:") ? "_blank" : undefined}
                  rel={item.href.startsWith("http") ? "noopener noreferrer" : undefined}
                  className="rounded-xl border p-5 transition-all hover:-translate-y-0.5 no-underline block group"
                  style={{ background: "#12121a", borderColor: "#1e1e2e" }}
                >
                  <div className="flex items-start justify-between mb-3">
                    <div
                      className="w-9 h-9 rounded-lg flex items-center justify-center"
                      style={{
                        background: `${item.color}12`,
                        color: item.color,
                      }}
                    >
                      {item.icon}
                    </div>
                    {item.badge && (
                      <span
                        className="text-[10px] font-semibold px-2 py-0.5 rounded-full"
                        style={{
                          background: `${item.badgeColor}15`,
                          color: item.badgeColor,
                          border: `1px solid ${item.badgeColor}30`,
                        }}
                      >
                        {item.badge}
                      </span>
                    )}
                  </div>
                  <h4 className="text-sm font-semibold mb-1 group-hover:text-white transition-colors" style={{ color: "#e0e0e8" }}>
                    {item.title}
                  </h4>
                  <p className="text-xs leading-relaxed" style={{ color: "#8888a0" }}>
                    {item.description}
                  </p>
                </a>
              ))}
            </div>
          </section>

          {/* Spacer */}
          <div className="h-12" />
        </main>
      </div>

      {/* ── New API Key Modal ── */}
      {showNewKeyModal && (
        <div
          className="fixed inset-0 z-50 flex items-center justify-center p-4"
          style={{ background: "rgba(0,0,0,0.7)", backdropFilter: "blur(4px)" }}
        >
          <div
            className="w-full max-w-md rounded-xl border p-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <h3 className="text-lg font-bold mb-1">Generate API Key</h3>
            <p className="text-sm mb-5" style={{ color: "#8888a0" }}>
              Create a new API key for SDK generation pipeline access.
            </p>
            <label className="text-[11px] uppercase tracking-wider font-medium block mb-1.5" style={{ color: "#8888a0" }}>
              Key Name
            </label>
            <input
              type="text"
              value={newKeyName}
              onChange={(e) => setNewKeyName(e.target.value)}
              placeholder="e.g., Production Pipeline"
              className="w-full px-4 py-2.5 rounded-lg text-sm mb-5 outline-none"
              style={{
                background: "#0a0a0f",
                border: "1px solid #1e1e2e",
                color: "#e0e0e8",
              }}
              onKeyDown={(e) => e.key === "Enter" && generateKey()}
              autoFocus
            />
            <div className="flex justify-end gap-3">
              <button
                onClick={() => setShowNewKeyModal(false)}
                className="text-sm px-4 py-2 rounded-lg transition-colors"
                style={{ color: "#8888a0", background: "rgba(255,255,255,0.05)" }}
              >
                Cancel
              </button>
              <button
                onClick={generateKey}
                disabled={!newKeyName.trim()}
                className="text-sm font-semibold px-4 py-2 rounded-lg transition-opacity hover:opacity-90 disabled:opacity-40"
                style={{ background: "#22d3ee", color: "#0a0a0f" }}
              >
                Generate
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
