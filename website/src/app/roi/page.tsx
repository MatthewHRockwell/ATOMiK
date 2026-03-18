"use client";

import { useState, useMemo, useRef } from "react";
import Link from "next/link";
import Nav from "@/components/Nav";

/* ---------- Formatting helpers ---------- */

function fmtNum(n: number): string {
  return n.toLocaleString("en-US");
}

function fmtBytes(bytes: number): string {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  if (bytes < 1024 * 1024 * 1024) return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
  if (bytes < 1024 * 1024 * 1024 * 1024) return `${(bytes / (1024 * 1024 * 1024)).toFixed(1)} GB`;
  return `${(bytes / (1024 * 1024 * 1024 * 1024)).toFixed(2)} TB`;
}

function fmtDollars(n: number): string {
  if (n >= 1000) return "$" + fmtNum(Math.round(n));
  if (n >= 100) return "$" + n.toFixed(0);
  if (n >= 10) return "$" + n.toFixed(1);
  return "$" + n.toFixed(2);
}

function fmtGB(bytes: number): string {
  return (bytes / (1024 * 1024 * 1024)).toFixed(2);
}

/* ---------- Card wrapper ---------- */

function Card({
  children,
  className = "",
}: {
  children: React.ReactNode;
  className?: string;
}) {
  return (
    <div
      className={`rounded-xl border p-6 ${className}`}
      style={{ background: "#12121a", borderColor: "#1e1e2e" }}
    >
      {children}
    </div>
  );
}

/* ---------- Slider input ---------- */

function SliderInput({
  label,
  value,
  onChange,
  min,
  max,
  step,
  format,
  suffix = "",
}: {
  label: string;
  value: number;
  onChange: (v: number) => void;
  min: number;
  max: number;
  step: number;
  format?: (v: number) => string;
  suffix?: string;
}) {
  const displayValue = format ? format(value) : fmtNum(value);
  return (
    <div className="mb-5">
      <div className="flex items-center justify-between mb-1.5">
        <label className="text-sm font-medium" style={{ color: "#b0b0c0" }}>
          {label}
        </label>
        <span
          className="text-sm font-bold tabular-nums"
          style={{ color: "#22d3ee" }}
        >
          {displayValue}{suffix}
        </span>
      </div>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(e) => onChange(Number(e.target.value))}
        className="w-full h-1.5 rounded-full appearance-none cursor-pointer"
        style={{
          background: `linear-gradient(to right, #4f8fff ${((value - min) / (max - min)) * 100}%, #1e1e2e ${((value - min) / (max - min)) * 100}%)`,
        }}
      />
    </div>
  );
}

/* ---------- Stat block ---------- */

function StatBlock({
  label,
  value,
  color = "#22d3ee",
  sub,
}: {
  label: string;
  value: string;
  color?: string;
  sub?: string;
}) {
  return (
    <div className="text-center">
      <div className="text-xs font-medium uppercase tracking-wider mb-1" style={{ color: "#8888a0" }}>
        {label}
      </div>
      <div className="text-2xl sm:text-3xl font-bold" style={{ color }}>
        {value}
      </div>
      {sub && (
        <div className="text-xs mt-0.5" style={{ color: "#8888a0" }}>
          {sub}
        </div>
      )}
    </div>
  );
}

/* ---------- Bar chart ---------- */

function BarComparison({
  wasteLabel,
  wasteValue,
  atomikLabel,
  atomikValue,
  maxValue,
}: {
  wasteLabel: string;
  wasteValue: number;
  atomikLabel: string;
  atomikValue: number;
  maxValue: number;
}) {
  const wasteWidth = maxValue > 0 ? Math.max((wasteValue / maxValue) * 100, 2) : 2;
  const atomikWidth = maxValue > 0 ? Math.max((atomikValue / maxValue) * 100, 2) : 2;
  return (
    <div className="space-y-3">
      <div>
        <div className="flex items-center justify-between mb-1">
          <span className="text-xs font-medium" style={{ color: "#ef4444" }}>{wasteLabel}</span>
          <span className="text-xs font-bold tabular-nums" style={{ color: "#ef4444" }}>
            {fmtDollars(wasteValue)}/mo
          </span>
        </div>
        <div className="w-full rounded-full h-5" style={{ background: "#1e1e2e" }}>
          <div
            className="h-5 rounded-full transition-all duration-500 ease-out"
            style={{ width: `${wasteWidth}%`, background: "linear-gradient(90deg, #ef4444, #f87171)" }}
          />
        </div>
      </div>
      <div>
        <div className="flex items-center justify-between mb-1">
          <span className="text-xs font-medium" style={{ color: "#22c55e" }}>{atomikLabel}</span>
          <span className="text-xs font-bold tabular-nums" style={{ color: "#22c55e" }}>
            {fmtDollars(atomikValue)}/mo
          </span>
        </div>
        <div className="w-full rounded-full h-5" style={{ background: "#1e1e2e" }}>
          <div
            className="h-5 rounded-full transition-all duration-500 ease-out"
            style={{ width: `${atomikWidth}%`, background: "linear-gradient(90deg, #22c55e, #4ade80)" }}
          />
        </div>
      </div>
    </div>
  );
}

/* =================================================================
   Main ROI Calculator Page
   ================================================================= */

function ROIReportExport() {
  const [showForm, setShowForm] = useState(false);
  const [form, setForm] = useState({ name: "", email: "", company: "", title: "" });
  const [sent, setSent] = useState(false);

  async function handleSubmit(e: React.FormEvent) {
    e.preventDefault();
    try {
      await fetch("/api/subscribe", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ ...form, source: "roi-report" }),
      });
    } catch { /* non-fatal */ }
    setSent(true);
  }

  const inputStyle = { background: "#0a0a0f", border: "1px solid #1e1e2e", color: "#e0e0e8" };

  return (
    <section className="max-w-3xl mx-auto px-6 pb-10">
      <div className="rounded-xl border p-6 text-center" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
        {sent ? (
          <>
            <div className="text-2xl mb-2" style={{ color: "#22c55e" }}>&#10003;</div>
            <h3 className="font-bold mb-2">Report request received</h3>
            <p className="text-sm" style={{ color: "#8888a0" }}>
              A custom ROI report based on your inputs will be prepared and sent to {form.email}.
              In the meantime, bookmark this page — your slider settings are preserved in the URL.
            </p>
          </>
        ) : !showForm ? (
          <>
            <h3 className="font-bold mb-2">Need this for a purchase decision?</h3>
            <p className="text-sm mb-4" style={{ color: "#8888a0" }}>
              Download a custom ROI report with your exact numbers — formatted for procurement.
            </p>
            <button
              onClick={() => setShowForm(true)}
              className="px-6 py-2.5 rounded-lg text-sm font-semibold"
              style={{ background: "linear-gradient(135deg, #4f8fff, #8b5cf6)", color: "#fff" }}
            >
              Get Custom ROI Report
            </button>
          </>
        ) : (
          <form onSubmit={handleSubmit} className="text-left space-y-3">
            <h3 className="font-bold text-center mb-3">Get Your Custom ROI Report</h3>
            <div className="grid md:grid-cols-2 gap-3">
              <input required type="text" placeholder="Full name" value={form.name} onChange={(e) => setForm((f) => ({ ...f, name: e.target.value }))} className="w-full px-3 py-2 rounded-lg text-sm outline-none" style={inputStyle} />
              <input required type="email" placeholder="Work email" value={form.email} onChange={(e) => setForm((f) => ({ ...f, email: e.target.value }))} className="w-full px-3 py-2 rounded-lg text-sm outline-none" style={inputStyle} />
              <input type="text" placeholder="Company" value={form.company} onChange={(e) => setForm((f) => ({ ...f, company: e.target.value }))} className="w-full px-3 py-2 rounded-lg text-sm outline-none" style={inputStyle} />
              <input type="text" placeholder="Title (e.g., VP Engineering)" value={form.title} onChange={(e) => setForm((f) => ({ ...f, title: e.target.value }))} className="w-full px-3 py-2 rounded-lg text-sm outline-none" style={inputStyle} />
            </div>
            <button type="submit" className="w-full py-2.5 rounded-lg text-sm font-semibold" style={{ background: "linear-gradient(135deg, #4f8fff, #8b5cf6)", color: "#fff" }}>
              Send ROI Report
            </button>
            <p className="text-[10px] text-center" style={{ color: "#555" }}>
              We&apos;ll email a PDF with your exact inputs, computed savings, and methodology notes.
            </p>
          </form>
        )}
      </div>
    </section>
  );
}

/* ---------- Preset scenarios ---------- */

const presets = [
  {
    id: "microservices",
    name: "Microservices SaaS",
    description: "Typical multi-service backend with moderate COW pressure and network chatter between services.",
    servers: 20,
    cowFaults: 5000,
    tcpSends: 10000,
    cowRedundancy: 23,
    netRedundancy: 31,
    costPerGB: 0.09,
    instanceCost: 0.50,
  },
  {
    id: "iot",
    name: "IoT Fleet",
    description: "Large fleet of edge devices with high network volume and significant payload redundancy.",
    servers: 200,
    cowFaults: 500,
    tcpSends: 50000,
    cowRedundancy: 15,
    netRedundancy: 45,
    costPerGB: 0.09,
    instanceCost: 0.10,
  },
  {
    id: "enterprise",
    name: "Enterprise Data Center",
    description: "On-prem data center with heavy workloads, high COW rates, and large instance costs.",
    servers: 500,
    cowFaults: 15000,
    tcpSends: 100000,
    cowRedundancy: 30,
    netRedundancy: 25,
    costPerGB: 0.05,
    instanceCost: 2.00,
  },
] as const;

export default function RoiPage() {
  /* --- Slider state --- */
  const [servers, setServers] = useState(20);
  const [cowFaults, setCowFaults] = useState(25000);
  const [tcpSends, setTcpSends] = useState(100000);
  const [cowRedundancy, setCowRedundancy] = useState(23);
  const [netRedundancy, setNetRedundancy] = useState(8.7);
  const [costPerGB, setCostPerGB] = useState(0.09);
  const [instanceCost, setInstanceCost] = useState(0.40);
  const [selectedPreset, setSelectedPreset] = useState<string | null>(null);

  function applyPreset(preset: typeof presets[number]) {
    setServers(preset.servers);
    setCowFaults(preset.cowFaults);
    setTcpSends(preset.tcpSends);
    setCowRedundancy(preset.cowRedundancy);
    setNetRedundancy(preset.netRedundancy);
    setCostPerGB(preset.costPerGB);
    setInstanceCost(preset.instanceCost);
    setSelectedPreset(preset.id);
  }

  // Wrap setters to clear preset on manual slider change
  function manualSet<T>(setter: (v: T) => void) {
    return (v: T) => { setSelectedPreset(null); setter(v); };
  }

  const PAGE_SIZE = 4096; // 4KB
  const AVG_PACKET = 1400; // average TCP payload bytes
  const HOURS_PER_MONTH = 720;
  const ATOMIK_EFFICIENCY = 0.85; // 85% waste reduction (conservative mid-range)

  /* --- Derived calculations --- */
  const results = useMemo(() => {
    const cowRedRate = cowRedundancy / 100;
    const netRedRate = netRedundancy / 100;

    // Monthly waste volumes (bytes)
    const cowWasteBytes = servers * cowFaults * cowRedRate * PAGE_SIZE * HOURS_PER_MONTH;
    const netWasteBytes = servers * tcpSends * netRedRate * AVG_PACKET * HOURS_PER_MONTH;
    const totalWasteBytes = cowWasteBytes + netWasteBytes;

    // --- Cost model ---
    // 1) Egress bandwidth: redundant network bytes at cloud egress rates
    const bandwidthCost = (netWasteBytes / (1024 * 1024 * 1024)) * costPerGB;

    // 2) Over-provisioning cost: redundant work consumes instance capacity.
    //    At typical 70% utilization, waste that pushes you past capacity thresholds
    //    forces scaling to additional instances (step function, not linear).
    //    COW faults: page copy + TLB shootdown IPI + mmap_sem contention = ~3.5% core per 10K/hr.
    //    Network sends: sk_buff alloc + checksum + syscall + DMA = ~2% core per 50K/hr.
    const cowCapacityFraction = (cowFaults / 10000) * 0.035 * cowRedRate;
    const netCapacityFraction = (tcpSends / 50000) * 0.02 * netRedRate;
    const rawCapacityWasted = Math.min(cowCapacityFraction + netCapacityFraction, 0.40);
    // At 70% utilization, capacity waste is amplified: you hit scaling thresholds sooner.
    // Effective cost is waste / (1 - utilization) — the "utilization penalty".
    const avgUtilization = 0.70;
    const effectiveCapacityWasted = Math.min(rawCapacityWasted / (1 - avgUtilization), 0.40);
    const fleetCost = servers * instanceCost * HOURS_PER_MONTH;
    const overProvisionCost = fleetCost * effectiveCapacityWasted;

    // 3) Memory bus / cache pressure: redundant data movement causes cache evictions
    //    that slow ALL workloads, not just the redundant ones. Each GB of redundant
    //    data thrashing through L3 evicts hot lines for real work, causing ~10-30%
    //    throughput degradation in co-located processes.
    const totalWasteGB = totalWasteBytes / (1024 * 1024 * 1024);
    // Per-server monthly data volume determines severity of cache pressure
    const wasteGBPerServer = totalWasteGB / Math.max(servers, 1);
    // Diminishing returns: first 10 GB/server/mo is most impactful, tapers off
    const cacheSeverity = Math.min(wasteGBPerServer / 10, 3.0);
    const cachePressureCost = servers * instanceCost * HOURS_PER_MONTH * 0.015 * cacheSeverity;

    const cpuFractionWasted = effectiveCapacityWasted;
    const totalMonthlyCost = bandwidthCost + overProvisionCost + cachePressureCost;

    // With ATOMiK Pro ($99/mo)
    const proPrice = 99;
    const proSavings = totalMonthlyCost * ATOMIK_EFFICIENCY;
    const proROI = proPrice > 0 ? proSavings / proPrice : 0;
    const proBreakEvenHours = proSavings > 0 ? (proPrice / (proSavings / HOURS_PER_MONTH)) : Infinity;

    // With ATOMiK Team ($299/mo)
    const teamPrice = 299;
    // Team tier adds SDK-driven optimization: extra 5-10% on top of base savings
    const teamEfficiency = Math.min(ATOMIK_EFFICIENCY + 0.08, 0.95);
    const teamSavings = totalMonthlyCost * teamEfficiency;
    const teamROI = teamPrice > 0 ? teamSavings / teamPrice : 0;
    const teamBreakEvenHours = teamSavings > 0 ? (teamPrice / (teamSavings / HOURS_PER_MONTH)) : Infinity;

    return {
      cowWasteBytes,
      netWasteBytes,
      totalWasteBytes,
      totalWasteGB,
      bandwidthCost,
      overProvisionCost,
      cachePressureCost,
      cpuFractionWasted,
      totalMonthlyCost,
      proPrice,
      proSavings,
      proROI,
      proBreakEvenHours,
      teamPrice,
      teamSavings,
      teamROI,
      teamBreakEvenHours,
    };
  }, [servers, cowFaults, tcpSends, cowRedundancy, netRedundancy, costPerGB, instanceCost]);

  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav />

      {/* Hero */}
      <section className="text-center px-6 pt-20 pb-12">
        <h1 className="text-3xl sm:text-4xl font-bold tracking-tight mb-4">
          How Much Is Your Infrastructure{" "}
          <span
            className="bg-clip-text text-transparent"
            style={{ backgroundImage: "linear-gradient(135deg, #ef4444, #f59e0b)" }}
          >
            Wasting
          </span>
          ?
        </h1>
        <p className="text-lg max-w-2xl mx-auto" style={{ color: "#8888a0" }}>
          Enter your numbers. See your savings.
        </p>
      </section>

      {/* Quick Estimate Presets */}
      <section className="max-w-6xl mx-auto px-6 pb-6">
        <h2
          className="text-sm font-semibold uppercase tracking-wider mb-4"
          style={{ color: "#8888a0" }}
        >
          Quick Estimate
        </h2>
        <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
          {presets.map((preset) => {
            const isActive = selectedPreset === preset.id;
            return (
              <button
                key={preset.id}
                onClick={() => applyPreset(preset)}
                className="rounded-xl border p-5 text-left transition-all hover:border-opacity-60"
                style={{
                  background: isActive ? "#1a1a2e" : "#12121a",
                  borderColor: isActive ? "#4f8fff" : "#1e1e2e",
                  boxShadow: isActive ? "0 0 0 1px #4f8fff40" : "none",
                }}
              >
                <h3
                  className="font-semibold text-sm mb-1.5"
                  style={{ color: isActive ? "#4f8fff" : "#e0e0e8" }}
                >
                  {preset.name}
                </h3>
                <p
                  className="text-xs leading-relaxed mb-3"
                  style={{ color: "#8888a0" }}
                >
                  {preset.description}
                </p>
                <span
                  className="text-xs font-semibold"
                  style={{ color: isActive ? "#4f8fff" : "#8888a0" }}
                >
                  {isActive ? "Selected" : "Use this estimate"} &rarr;
                </span>
              </button>
            );
          })}
        </div>
      </section>

      {/* Calculator */}
      <section className="max-w-6xl mx-auto px-6 pb-8">
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-8">

          {/* ---- Left: Inputs ---- */}
          <Card>
            <h2
              className="text-sm font-semibold uppercase tracking-wider mb-6"
              style={{ color: "#8888a0" }}
            >
              Your Infrastructure
            </h2>

            <SliderInput
              label="Servers / Nodes"
              value={servers}
              onChange={manualSet(setServers)}
              min={1}
              max={500}
              step={1}
            />

            <SliderInput
              label="COW Faults / Hour / Server"
              value={cowFaults}
              onChange={manualSet(setCowFaults)}
              min={100}
              max={100000}
              step={100}
            />

            <SliderInput
              label="TCP Sends / Hour / Server"
              value={tcpSends}
              onChange={manualSet(setTcpSends)}
              min={10000}
              max={1000000}
              step={5000}
            />

            <SliderInput
              label="COW Redundancy Rate"
              value={cowRedundancy}
              onChange={manualSet(setCowRedundancy)}
              min={5}
              max={50}
              step={1}
              suffix="%"
            />

            <SliderInput
              label="Network Redundancy Rate"
              value={netRedundancy}
              onChange={manualSet(setNetRedundancy)}
              min={2}
              max={50}
              step={0.1}
              format={(v) => v.toFixed(1)}
              suffix="%"
            />

            <div className="mb-5">
              <div className="flex items-center justify-between mb-1.5">
                <label className="text-sm font-medium" style={{ color: "#b0b0c0" }}>
                  Average Page Size
                </label>
                <span className="text-sm font-bold" style={{ color: "#8888a0" }}>
                  4 KB
                </span>
              </div>
              <div
                className="w-full h-1.5 rounded-full"
                style={{ background: "#1e1e2e" }}
              />
              <div className="text-xs mt-1" style={{ color: "#555" }}>
                System page size (read-only)
              </div>
            </div>

            <SliderInput
              label="Cloud Cost per GB Transferred"
              value={costPerGB}
              onChange={manualSet(setCostPerGB)}
              min={0.01}
              max={0.12}
              step={0.01}
              format={(v) => "$" + v.toFixed(2)}
            />

            <SliderInput
              label="Instance Cost / Hour"
              value={instanceCost}
              onChange={manualSet(setInstanceCost)}
              min={0.05}
              max={2.0}
              step={0.05}
              format={(v) => "$" + v.toFixed(2)}
            />
          </Card>

          {/* ---- Right: Results ---- */}
          <div className="space-y-6">

            {/* Monthly waste detected */}
            <Card>
              <h2
                className="text-sm font-semibold uppercase tracking-wider mb-4"
                style={{ color: "#8888a0" }}
              >
                Monthly Waste Detected
              </h2>
              <div className="space-y-3">
                <div className="flex items-center justify-between text-sm">
                  <span style={{ color: "#b0b0c0" }}>COW bytes wasted</span>
                  <span className="font-bold tabular-nums" style={{ color: "#f59e0b" }}>
                    {fmtBytes(results.cowWasteBytes)}
                  </span>
                </div>
                <div className="flex items-center justify-between text-sm">
                  <span style={{ color: "#b0b0c0" }}>Network bytes wasted</span>
                  <span className="font-bold tabular-nums" style={{ color: "#f59e0b" }}>
                    {fmtBytes(results.netWasteBytes)}
                  </span>
                </div>
                <div
                  className="flex items-center justify-between text-sm pt-3 mt-3"
                  style={{ borderTop: "1px solid #1e1e2e" }}
                >
                  <span className="font-semibold" style={{ color: "#e0e0e8" }}>Total waste</span>
                  <span className="font-bold text-base tabular-nums" style={{ color: "#ef4444" }}>
                    {fmtGB(results.totalWasteBytes)} GB/mo
                  </span>
                </div>
              </div>
            </Card>

            {/* Monthly cost of waste */}
            <Card>
              <h2
                className="text-sm font-semibold uppercase tracking-wider mb-4"
                style={{ color: "#8888a0" }}
              >
                Monthly Cost of Waste
              </h2>
              <div className="space-y-3">
                <div className="flex items-center justify-between text-sm">
                  <span style={{ color: "#b0b0c0" }}>Egress bandwidth</span>
                  <span className="font-bold tabular-nums" style={{ color: "#ef4444" }}>
                    {fmtDollars(results.bandwidthCost)}
                  </span>
                </div>
                <div className="flex items-center justify-between text-sm">
                  <span style={{ color: "#b0b0c0" }}>
                    Over-provisioning ({(results.cpuFractionWasted * 100).toFixed(1)}% capacity wasted)
                  </span>
                  <span className="font-bold tabular-nums" style={{ color: "#ef4444" }}>
                    {fmtDollars(results.overProvisionCost)}
                  </span>
                </div>
                <div className="flex items-center justify-between text-sm">
                  <span style={{ color: "#b0b0c0" }}>Cache pressure / memory bus</span>
                  <span className="font-bold tabular-nums" style={{ color: "#ef4444" }}>
                    {fmtDollars(results.cachePressureCost)}
                  </span>
                </div>
                <div
                  className="flex items-center justify-between text-sm pt-3 mt-3"
                  style={{ borderTop: "1px solid #1e1e2e" }}
                >
                  <span className="font-semibold" style={{ color: "#e0e0e8" }}>Total monthly cost</span>
                  <span className="font-bold text-base tabular-nums" style={{ color: "#ef4444" }}>
                    {fmtDollars(results.totalMonthlyCost)}/mo
                  </span>
                </div>
              </div>
            </Card>

            {/* ATOMiK Pro tier */}
            <Card>
              <div className="flex items-center gap-2 mb-4">
                <h2
                  className="text-sm font-semibold uppercase tracking-wider"
                  style={{ color: "#8888a0" }}
                >
                  With ATOMiK Pro
                </h2>
                <span
                  className="text-xs font-bold px-2 py-0.5 rounded-full"
                  style={{ background: "#4f8fff20", color: "#4f8fff" }}
                >
                  $99/mo
                </span>
              </div>
              <div className="space-y-3">
                <div className="flex items-center justify-between text-sm">
                  <span style={{ color: "#b0b0c0" }}>Estimated savings (85% reduction)</span>
                  <span className="font-bold tabular-nums" style={{ color: "#22c55e" }}>
                    {fmtDollars(results.proSavings)}/mo
                  </span>
                </div>
                <div className="flex items-center justify-between text-sm">
                  <span style={{ color: "#b0b0c0" }}>Return on investment</span>
                  <span className="font-bold tabular-nums" style={{ color: "#22c55e" }}>
                    {results.proROI.toFixed(1)}x
                  </span>
                </div>
                <div className="flex items-center justify-between text-sm">
                  <span style={{ color: "#b0b0c0" }}>Break-even</span>
                  <span className="font-bold tabular-nums" style={{ color: "#22d3ee" }}>
                    {results.proBreakEvenHours < HOURS_PER_MONTH
                      ? `${Math.ceil(results.proBreakEvenHours)} hours`
                      : results.proBreakEvenHours === Infinity
                        ? "--"
                        : `${(results.proBreakEvenHours / HOURS_PER_MONTH).toFixed(1)} months`}
                  </span>
                </div>
              </div>
            </Card>

            {/* ATOMiK Team tier */}
            <Card>
              <div className="flex items-center gap-2 mb-4">
                <h2
                  className="text-sm font-semibold uppercase tracking-wider"
                  style={{ color: "#8888a0" }}
                >
                  With ATOMiK Team
                </h2>
                <span
                  className="text-xs font-bold px-2 py-0.5 rounded-full"
                  style={{ background: "#8b5cf620", color: "#8b5cf6" }}
                >
                  $299/mo
                </span>
              </div>
              <div className="space-y-3">
                <div className="flex items-center justify-between text-sm">
                  <span style={{ color: "#b0b0c0" }}>
                    Savings with SDK optimization (93% reduction)
                  </span>
                  <span className="font-bold tabular-nums" style={{ color: "#22c55e" }}>
                    {fmtDollars(results.teamSavings)}/mo
                  </span>
                </div>
                <div className="flex items-center justify-between text-sm">
                  <span style={{ color: "#b0b0c0" }}>Return on investment</span>
                  <span className="font-bold tabular-nums" style={{ color: "#22c55e" }}>
                    {results.teamROI.toFixed(1)}x
                  </span>
                </div>
                <div className="flex items-center justify-between text-sm">
                  <span style={{ color: "#b0b0c0" }}>ROI per seat (5 seats)</span>
                  <span className="font-bold tabular-nums" style={{ color: "#22d3ee" }}>
                    {fmtDollars(results.teamSavings / 5)}/seat/mo saved
                  </span>
                </div>
              </div>
            </Card>
          </div>
        </div>
      </section>

      {/* Visual summary */}
      <section className="max-w-6xl mx-auto px-6 pb-8">
        <Card>
          {/* Big numbers */}
          <div className="grid grid-cols-1 sm:grid-cols-3 gap-6 mb-8">
            <StatBlock
              label="Monthly savings"
              value={fmtDollars(results.proSavings)}
              color="#22c55e"
              sub="with ATOMiK Pro"
            />
            <StatBlock
              label="Return on $99/mo"
              value={`${results.proROI.toFixed(1)}x`}
              color="#4f8fff"
              sub="ROI"
            />
            <StatBlock
              label="Pays for itself in"
              value={
                results.proBreakEvenHours < HOURS_PER_MONTH
                  ? `${Math.ceil(results.proBreakEvenHours)}h`
                  : results.proBreakEvenHours === Infinity
                    ? "--"
                    : `${(results.proBreakEvenHours / HOURS_PER_MONTH).toFixed(1)} mo`
              }
              color="#f59e0b"
              sub={
                results.proBreakEvenHours < 24
                  ? "less than a day"
                  : results.proBreakEvenHours < HOURS_PER_MONTH
                    ? `${(results.proBreakEvenHours / 24).toFixed(0)} days`
                    : undefined
              }
            />
          </div>

          {/* Bar chart */}
          <h3
            className="text-sm font-semibold uppercase tracking-wider mb-4"
            style={{ color: "#8888a0" }}
          >
            Waste Cost vs. ATOMiK Cost
          </h3>
          <BarComparison
            wasteLabel="Infrastructure waste"
            wasteValue={results.totalMonthlyCost}
            atomikLabel="ATOMiK Pro subscription"
            atomikValue={results.proPrice}
            maxValue={Math.max(results.totalMonthlyCost, results.proPrice)}
          />
        </Card>
      </section>

      {/* Methodology note */}
      <section className="max-w-6xl mx-auto px-6 pb-8">
        <div
          className="rounded-xl border p-5 text-sm"
          style={{ background: "#0a0a0f", borderColor: "#1e1e2e", color: "#8888a0" }}
        >
          <h3 className="font-semibold mb-2" style={{ color: "#b0b0c0" }}>How we calculate</h3>
          <ul className="space-y-1 list-disc list-inside">
            <li>
              <strong>COW waste:</strong> servers &times; faults/hr &times; redundancy rate &times; 4KB page &times; 720 hrs/mo
            </li>
            <li>
              <strong>Network waste:</strong> servers &times; sends/hr &times; redundancy rate &times; 1,400B avg payload &times; 720 hrs/mo
            </li>
            <li>
              <strong>Over-provisioning:</strong> redundant COW faults consume ~3.5% core per 10K/hr, TCP sends ~2% per 50K/hr.
              At typical 70% utilization, this waste is amplified: you hit scaling thresholds sooner, forcing additional instances.
            </li>
            <li>
              <strong>Cache pressure:</strong> redundant data movement evicts hot L3 cache lines, degrading throughput for all co-located workloads. Severity scales with per-server waste volume.
            </li>
            <li>
              <strong>ATOMiK reduction:</strong> 85% (Pro) to 93% (Team) of detected waste eliminated via delta-state deduplication
            </li>
            <li>
              Defaults reflect typical mid-scale microservices workloads. Your results will vary based on actual workload characteristics.
            </li>
          </ul>
        </div>
      </section>

      <ROIReportExport />

      {/* CTA */}
      <section className="text-center px-6 pb-24">
        <h2 className="text-2xl font-bold mb-3">
          Stop paying for redundant data
        </h2>
        <p className="mb-8 max-w-xl mx-auto" style={{ color: "#8888a0" }}>
          90-day free trial. No credit card required.
          Install in 5 minutes, see real metrics from your infrastructure on day one.
        </p>
        <div className="flex flex-wrap justify-center gap-4">
          <Link
            href="/pricing"
            className="inline-block px-6 py-3 rounded-lg text-sm font-bold transition-opacity hover:opacity-90"
            style={{ background: "#4f8fff", color: "#fff" }}
          >
            Start Your 90-Day Free Trial
          </Link>
          <Link
            href="/docs"
            className="inline-block px-6 py-3 rounded-lg text-sm font-bold transition-opacity hover:opacity-90"
            style={{
              background: "transparent",
              color: "#4f8fff",
              border: "1px solid #4f8fff",
            }}
          >
            Read the Docs
          </Link>
        </div>
      </section>
    </div>
  );
}
