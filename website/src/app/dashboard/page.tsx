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

export default function DashboardPage() {
  return (
    <div className="min-h-screen" style={{ background: colors.bg, color: colors.text }}>
      <Nav />

      <main className="mx-auto max-w-4xl px-6 py-16">
        <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>
          Evaluation workspace
        </p>
        <h1 className="mt-4 text-4xl font-bold md:text-5xl">
          Dashboard access is request-based.
        </h1>
        <p className="mt-5 max-w-2xl text-lg leading-8" style={{ color: colors.muted }}>
          ATOMiK does not publish a self-serve product dashboard, deployment
          metrics, usage counts, or bandwidth-reduction claims. Evaluation
          workspaces are created only for scoped technical reviews.
        </p>

        <section className="mt-8 grid gap-4 md:grid-cols-3">
          {[
            ["HARDWARE_VALIDATED", "Live prototype screenshots and hardware validation notes."],
            ["LIVE_MEASURED", "Measured artifacts only when raw outputs are linked."],
            ["ROADMAP", "First-silicon and product-dashboard work are planned paths."],
          ].map(([label, body]) => (
            <article key={label} className="rounded-lg p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
              <p className="text-[11px] font-semibold uppercase" style={{ color: colors.cyan }}>{label}</p>
              <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{body}</p>
            </article>
          ))}
        </section>

        <div className="mt-8 flex flex-wrap gap-3">
          <Link
            href="/contact?intent=evaluation"
            className="rounded-lg px-5 py-3 text-sm font-semibold text-white no-underline"
            style={{ background: colors.blue }}
          >
            Request Technical Evaluation
          </Link>
          <Link
            href="/benchmarks"
            className="rounded-lg px-5 py-3 text-sm font-semibold no-underline"
            style={{ color: colors.text, border: `1px solid ${colors.border}` }}
          >
            View Hardware Proof
          </Link>
        </div>
      </main>
    </div>
  );
}
