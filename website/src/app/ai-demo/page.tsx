import Link from "next/link";
import Nav from "@/components/Nav";

const colors = {
  bg: "#070b12",
  panel: "#0d1420",
  border: "#1d324a",
  text: "#f4f8ff",
  muted: "#9fb1c7",
  cyan: "#22d3ee",
  blue: "#4f8fff",
};

const notes = [
  {
    label: "CONCEPTUAL",
    title: "AI workload surface",
    body: "This page is a discussion surface for where state-aware execution could fit AI-adjacent workloads. It is not a measured inference benchmark.",
  },
  {
    label: "ROADMAP",
    title: "Evaluation path",
    body: "A real AI evaluation would need a workload, baseline, hardware profile, command log, raw output, and interpretation note before any performance or power claim is public-safe.",
  },
  {
    label: "HARDWARE_VALIDATED",
    title: "Current proof boundary",
    body: "ATOMiK Desk and Resource Fabric have live prototype screenshots. AI-specific power, cost, and throughput claims are not current public proof.",
  },
];

export default function AIConceptPage() {
  return (
    <div className="min-h-screen" style={{ background: colors.bg, color: colors.text }}>
      <Nav />

      <main className="mx-auto max-w-5xl px-6 py-16">
        <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>
          Concept surface
        </p>
        <h1 className="mt-4 max-w-3xl text-4xl font-bold md:text-5xl">
          AI workloads need evidence before claims.
        </h1>
        <p className="mt-5 max-w-3xl text-lg leading-8" style={{ color: colors.muted }}>
          ATOMiK may be relevant where AI systems repeatedly track context,
          synchronize state, checkpoint memory, or discard unchanged regions.
          Public AI performance and power claims will only be stated when backed
          by measured artifacts.
        </p>

        <div className="mt-10 grid gap-4 md:grid-cols-3">
          {notes.map((note) => (
            <article
              key={note.title}
              className="rounded-lg p-5"
              style={{ background: colors.panel, border: `1px solid ${colors.border}` }}
            >
              <p className="text-[11px] font-semibold uppercase" style={{ color: colors.cyan }}>
                {note.label}
              </p>
              <h2 className="mt-2 text-lg font-bold">{note.title}</h2>
              <p className="mt-2 text-sm leading-6" style={{ color: colors.muted }}>
                {note.body}
              </p>
            </article>
          ))}
        </div>

        <section className="mt-10 rounded-lg p-6" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
          <h2 className="text-2xl font-bold">What a real AI evaluation would include</h2>
          <ul className="mt-4 space-y-3 text-sm leading-6" style={{ color: colors.muted }}>
            <li>- workload and baseline definition</li>
            <li>- target hardware and software versions</li>
            <li>- commands, raw output, and run date</li>
            <li>- interpretation notes that separate measured results from projections</li>
            <li>- explicit evidence labels for every public claim</li>
          </ul>
          <div className="mt-6 flex flex-wrap gap-3">
            <Link
              href="/contact?intent=evaluation"
              className="rounded-lg px-4 py-2 text-sm font-semibold text-white no-underline"
              style={{ background: colors.blue }}
            >
              Request Evaluation Access
            </Link>
            <Link
              href="https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/technical-proof.md"
              className="rounded-lg px-4 py-2 text-sm font-semibold no-underline"
              style={{ color: colors.cyan, border: `1px solid ${colors.border}` }}
            >
              View Technical Proof
            </Link>
          </div>
        </section>
      </main>
    </div>
  );
}
