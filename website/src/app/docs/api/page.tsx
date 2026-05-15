import type { Metadata } from "next";
import {
  codeBlockStyle,
  kwColor,
  fnColor,
  strColor,
  numColor,
  cmtColor,
  typeColor,
  varColor,
} from "../shared";
import UpgradeGate from "@/components/UpgradeGate";
import LangTabs from "@/components/LangTabs";

export const metadata: Metadata = {
  title: "API Reference — ATOMiK Docs",
  description:
    "Python, C, and kernel module APIs for ATOMiK delta-state operations. Multi-context tables and full operation reference.",
};

/* ── API reference cards data ─────────────────────────────────────── */

const apiLinks = [
  {
    lang: "Python",
    pkg: "atomik-core",
    classes: ["AtomikContext", "AtomikTable", "DeltaStream", "Fingerprint"],
    install: "pip install atomik-core",
    color: "#4f8fff",
  },
  {
    lang: "C",
    pkg: "atomik_core.h",
    classes: ["atomik_ctx_t", "atomik_table_t", "atomik_fingerprint_t"],
    install: '#include "atomik_core.h"',
    color: "#22c55e",
  },
  {
    lang: "Kernel Module",
    pkg: "/dev/atomik",
    classes: [
      "ATOMIK_IOC_LOAD",
      "ATOMIK_IOC_ACCUM",
      "ATOMIK_IOC_READ",
      "ATOMIK_IOC_SWAP",
      "ATOMIK_IOC_BATCH",
    ],
    install: "sudo ./install.sh",
    color: "#d4a843",
  },
];

export default function ApiReferencePage() {
  return (
    <div className="max-w-5xl mx-auto px-6 py-16">
      <p
        className="text-sm font-mono tracking-widest uppercase mb-4"
        style={{ color: "#4f8fff" }}
      >
        Reference
      </p>
      <h1 className="text-4xl font-bold tracking-tight mb-4">API Reference</h1>
      <p className="text-lg mb-10" style={{ color: "#8888a0" }}>
        Three interfaces, one algebra. Every API exposes the same four operations:
        LOAD, ACCUM, READ, SWAP.
      </p>

      {/* API cards */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-6 mb-16">
        {apiLinks.map((api) => (
          <div
            key={api.lang}
            className="rounded-xl p-6 border transition-all duration-300 hover:-translate-y-1"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div className="flex items-center gap-3 mb-4">
              <div
                className="w-2 h-2 rounded-full"
                style={{ background: api.color }}
              />
              <h3 className="text-lg font-bold" style={{ color: api.color }}>
                {api.lang}
              </h3>
            </div>
            <code
              className="block text-xs font-mono px-3 py-2 rounded-lg mb-4"
              style={{ background: "#0d0d14", color: "#8888a0" }}
            >
              {api.install}
            </code>
            <ul className="space-y-1.5">
              {api.classes.map((cls) => (
                <li key={cls} className="text-sm font-mono" style={{ color: "#b0b0c0" }}>
                  <span style={{ color: api.color, opacity: 0.6 }}>&#x25b8;</span>{" "}
                  {cls}
                </li>
              ))}
            </ul>
          </div>
        ))}
      </div>

      <UpgradeGate
        tier="team"
        title="Generate SDKs in 5 Languages"
        description="Team tier includes the SDK generation pipeline: Python, Rust, C, JavaScript, and Verilog from a single schema definition."
        ctaText="Start Team Trial"
      />

      {/* ─────────────── Core Operations Reference ─────────────── */}
      <h2 className="text-3xl font-bold mb-3">Core Operations</h2>
      <p className="mb-8 text-lg" style={{ color: "#8888a0" }}>
        Every ATOMiK SDK exposes the same four operations. Select a language to see
        signatures, descriptions, and examples.
      </p>

      <LangTabs />

      {/* ─────────────── Multi-Context Tables ─────────────── */}
      <div className="mt-16" />
      <h2 className="text-3xl font-bold mb-4">Multi-Context Tables</h2>
      <p className="mb-8 text-lg" style={{ color: "#8888a0" }}>
        Track thousands of independent state channels in a single table.
      </p>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Python */}
        <div>
          <p className="text-sm font-mono mb-2" style={{ color: "#4f8fff" }}>
            Python
          </p>
          <div style={codeBlockStyle}>
            <pre style={{ margin: 0 }}>
              <code>
                <span style={{ color: kwColor }}>from</span>{" "}
                <span style={{ color: varColor }}>atomik_core</span>{" "}
                <span style={{ color: kwColor }}>import</span>{" "}
                <span style={{ color: typeColor }}>AtomikTable</span>
                {"\n\n"}
                <span style={{ color: varColor }}>table</span>{" "}
                <span style={{ color: kwColor }}>=</span>{" "}
                <span style={{ color: typeColor }}>AtomikTable</span>
                <span style={{ color: varColor }}>(num_contexts=</span>
                <span style={{ color: numColor }}>256</span>
                <span style={{ color: varColor }}>)</span>
                {"\n"}
                <span style={{ color: varColor }}>table.</span>
                <span style={{ color: fnColor }}>load</span>
                <span style={{ color: varColor }}>(addr=</span>
                <span style={{ color: numColor }}>0</span>
                <span style={{ color: varColor }}>, initial_state=</span>
                <span style={{ color: numColor }}>0xCAFEBABE</span>
                <span style={{ color: varColor }}>)</span>
                {"\n"}
                <span style={{ color: varColor }}>table.</span>
                <span style={{ color: fnColor }}>accum</span>
                <span style={{ color: varColor }}>(addr=</span>
                <span style={{ color: numColor }}>0</span>
                <span style={{ color: varColor }}>, delta=</span>
                <span style={{ color: numColor }}>0x00000001</span>
                <span style={{ color: varColor }}>)</span>
                {"\n"}
                <span style={{ color: kwColor }}>assert</span>{" "}
                <span style={{ color: varColor }}>table.</span>
                <span style={{ color: fnColor }}>read</span>
                <span style={{ color: varColor }}>(addr=</span>
                <span style={{ color: numColor }}>0</span>
                <span style={{ color: varColor }}>) == </span>
                <span style={{ color: numColor }}>0xCAFEBABF</span>
              </code>
            </pre>
          </div>
        </div>

        {/* C */}
        <div>
          <p className="text-sm font-mono mb-2" style={{ color: "#22c55e" }}>
            C (single-header library)
          </p>
          <div style={codeBlockStyle}>
            <pre style={{ margin: 0 }}>
              <code>
                <span style={{ color: kwColor }}>#define</span>{" "}
                <span style={{ color: varColor }}>ATOMIK_IMPLEMENTATION</span>
                {"\n"}
                <span style={{ color: kwColor }}>#include</span>{" "}
                <span style={{ color: strColor }}>&quot;atomik_core.h&quot;</span>
                {"\n\n"}
                <span style={{ color: typeColor }}>atomik_table_t</span>{" "}
                <span style={{ color: varColor }}>table;</span>
                {"\n"}
                <span style={{ color: fnColor }}>atomik_table_init</span>
                <span style={{ color: varColor }}>(&amp;table, </span>
                <span style={{ color: numColor }}>256</span>
                <span style={{ color: varColor }}>);</span>
                {"\n"}
                <span style={{ color: fnColor }}>atomik_table_load</span>
                <span style={{ color: varColor }}>(&amp;table, </span>
                <span style={{ color: numColor }}>0</span>
                <span style={{ color: varColor }}>, </span>
                <span style={{ color: numColor }}>0xCAFEBABE</span>
                <span style={{ color: varColor }}>);</span>
                {"\n"}
                <span style={{ color: fnColor }}>atomik_table_accum</span>
                <span style={{ color: varColor }}>(&amp;table, </span>
                <span style={{ color: numColor }}>0</span>
                <span style={{ color: varColor }}>, </span>
                <span style={{ color: numColor }}>0x00000001</span>
                <span style={{ color: varColor }}>);</span>
                {"\n"}
                <span style={{ color: typeColor }}>uint64_t</span>{" "}
                <span style={{ color: varColor }}>s = </span>
                <span style={{ color: fnColor }}>atomik_table_read</span>
                <span style={{ color: varColor }}>(&amp;table, </span>
                <span style={{ color: numColor }}>0</span>
                <span style={{ color: varColor }}>);</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"// s == 0xCAFEBABF"}</span>
              </code>
            </pre>
          </div>
        </div>
      </div>

    </div>
  );
}
