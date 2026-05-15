import Link from "next/link";

type EvidenceBannerProps = {
  surface: "blog" | "docs";
};

export default function EvidenceBanner({ surface }: EvidenceBannerProps) {
  const label = surface === "blog" ? "Blog evidence note" : "Docs evidence note";

  return (
    <section className="mx-auto max-w-5xl px-6" aria-label={label}>
      <div
        className="rounded-lg border px-4 py-3 text-xs leading-5"
        style={{
          background: "rgba(79, 143, 255, 0.06)",
          borderColor: "rgba(79, 143, 255, 0.28)",
          color: "#9fb1c7",
        }}
      >
        Historical articles and technical notes may include exploratory examples,
        synthesis figures, or modeled comparisons. Treat performance, power,
        savings, customer, production, and deployment claims as public-safe only
        when they are linked to measured artifacts or explicit evidence labels.{" "}
        <Link href="/docs" style={{ color: "#4f8fff" }}>
          Start with the current docs
        </Link>{" "}
        or{" "}
        <a
          href="https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/evidence-labels.md"
          style={{ color: "#4f8fff" }}
        >
          evidence-label definitions
        </a>
        .
      </div>
    </section>
  );
}
