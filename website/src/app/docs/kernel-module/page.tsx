import type { Metadata } from "next";
import EvaluationGate from "@/components/EvaluationGate";
import { codeBlockStyle } from "../shared";

export const metadata: Metadata = {
  title: "Kernel Module - ATOMiK Docs",
  description:
    "ATOMiK Linux kernel module notes for request-based technical evaluation and proof review.",
};

const sections = [
  {
    title: "Public review path",
    body:
      "The public repository exposes the current Linux integration materials for technical review. Treat the kernel module as an evaluation surface until a workload-specific proof plan is agreed.",
  },
  {
    title: "Evidence boundary",
    body:
      "Use measured artifacts for performance claims. The website should not infer data-center, Kubernetes, or subscription behavior from illustrative terminal output.",
  },
  {
    title: "Evaluation fit",
    body:
      "Best-fit conversations start with a concrete state-heavy workload, expected heat or bandwidth pressure, and the exact success criteria to validate.",
  },
];

export default function KernelModulePage() {
  return (
    <div className="mx-auto max-w-5xl px-6 py-16">
      <p
        className="mb-4 text-sm font-mono uppercase tracking-widest"
        style={{ color: "#d4a843" }}
      >
        System Integration
      </p>
      <h1 className="mb-4 text-4xl font-bold tracking-tight">Kernel Module</h1>
      <p className="mb-10 max-w-3xl text-lg" style={{ color: "#8888a0" }}>
        The ATOMiK Linux kernel-module path is handled as a request-based
        technical evaluation surface. Public docs should anchor on install
        shape, interface review, and evidence boundaries rather than fake
        subscription, cluster, or savings output.
      </p>

      <div className="mb-12 grid grid-cols-1 gap-6 lg:grid-cols-3">
        {sections.map((section) => (
          <section
            key={section.title}
            className="rounded-xl border p-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <h2 className="mb-3 text-lg font-bold text-white">{section.title}</h2>
            <p className="text-sm leading-6" style={{ color: "#8888a0" }}>
              {section.body}
            </p>
          </section>
        ))}
      </div>

      <div className="mb-10 grid grid-cols-1 gap-6 lg:grid-cols-2">
        <div>
          <p className="mb-2 text-sm font-mono" style={{ color: "#8888a0" }}>
            review source
          </p>
          <div style={codeBlockStyle}>
            <pre style={{ margin: 0 }}>
              <code>{`git clone https://github.com/MatthewHRockwell/ATOMiK
cd ATOMiK/software/atomik_kmod
ls`}</code>
            </pre>
          </div>
        </div>

        <div>
          <p className="mb-2 text-sm font-mono" style={{ color: "#8888a0" }}>
            interface shape
          </p>
          <div style={codeBlockStyle}>
            <pre style={{ margin: 0 }}>
              <code>{`/dev/atomik
/sys/class/atomik/atomik0/backend
/sys/class/atomik/atomik0/version
/sys/class/atomik/atomik0/ops_total`}</code>
            </pre>
          </div>
        </div>
      </div>

      <section
        className="mb-10 rounded-xl border p-6"
        style={{ background: "#12121a", borderColor: "#1e1e2e" }}
      >
        <h2 className="mb-3 text-2xl font-bold tracking-tight">What to validate</h2>
        <ul className="space-y-3 text-sm leading-6" style={{ color: "#8888a0" }}>
          <li>
            <span style={{ color: "#e0e0e8" }}>Workload fit:</span> where repeated
            writes, sync traffic, or hot/cold context retention create measurable cost.
          </li>
          <li>
            <span style={{ color: "#e0e0e8" }}>Producer path:</span> which counters,
            traces, or hardware artifacts will be accepted as evidence.
          </li>
          <li>
            <span style={{ color: "#e0e0e8" }}>Success criteria:</span> heat, power,
            bandwidth, latency, or hardware-footprint thresholds for a real deployment.
          </li>
        </ul>
      </section>

      <EvaluationGate
        title="Ready to evaluate the kernel-module path?"
        description="Request access with a concrete workload and evidence goal. ATOMiK will scope the review around measurable heat, bandwidth, latency, or footprint outcomes."
        ctaText="Request Evaluation Access"
        intent="evaluation"
        accent="#d4a843"
      />
    </div>
  );
}
