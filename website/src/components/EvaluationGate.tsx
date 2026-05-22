import Link from "next/link";

export default function EvaluationGate({
  title,
  description,
  ctaText = "Request Evaluation Access",
  intent = "evaluation",
  accent = "#4f8fff",
}: {
  title: string;
  description: string;
  ctaText?: string;
  intent?: "evaluation" | "demo" | "design-partner" | "licensing";
  accent?: string;
}) {
  return (
    <div
      className="my-10 overflow-hidden rounded-xl"
      style={{ background: "#12121a", border: "1px solid #1e1e2e" }}
    >
      <div
        className="h-1"
        style={{ background: `linear-gradient(90deg, ${accent}, ${accent}88)` }}
      />

      <div className="px-6 py-6">
        <span
          className="mb-4 inline-block rounded-md px-2.5 py-1 text-xs font-semibold uppercase tracking-wider"
          style={{
            background: `${accent}18`,
            color: accent,
            border: `1px solid ${accent}40`,
          }}
        >
          Evaluation
        </span>

        <h3 className="mb-2 text-lg font-bold" style={{ color: "#e0e0e8" }}>
          {title}
        </h3>

        <p className="mb-5 text-sm leading-relaxed" style={{ color: "#8888a0" }}>
          {description}
        </p>

        <Link
          href={`/contact?intent=${intent}`}
          className="inline-block rounded-lg px-5 py-2.5 text-sm font-semibold text-white no-underline transition-opacity hover:opacity-90"
          style={{ background: accent }}
        >
          {ctaText}
        </Link>
      </div>
    </div>
  );
}
