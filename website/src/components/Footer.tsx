import Link from "next/link";

const footerCols: { title: string; links: [string, string][] }[] = [
  {
    title: "Evaluate",
    links: [
      ["/contact?intent=evaluation", "Evaluation Access"],
      ["/contact?intent=demo", "Technical Demo"],
      ["/contact?intent=design-partner", "Design Partnership"],
      ["/pricing", "Offer Structure"],
    ],
  },
  {
    title: "Proof",
    links: [
      ["/benchmarks", "Benchmarks"],
      ["/docs", "Docs"],
      ["https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/evidence-labels.md", "Evidence Labels"],
      ["https://github.com/MatthewHRockwell/ATOMiK/blob/main/results/claims_registry.yaml", "Claims Registry"],
    ],
  },
  {
    title: "Company",
    links: [
      ["/about", "About"],
      ["/about/roadmap", "Roadmap"],
      ["/solutions", "Workloads"],
      ["mailto:mrockwell@atomik.tech", "Contact"],
    ],
  },
  {
    title: "Legal",
    links: [
      ["/privacy", "Privacy"],
      ["/terms", "Terms"],
      ["https://github.com/MatthewHRockwell/ATOMiK", "GitHub"],
    ],
  },
];

export default function Footer() {
  return (
    <footer
      className="px-6 pb-10 pt-14 text-sm"
      style={{ borderTop: "1px solid #1d324a", background: "#070b12", color: "#9fb1c7" }}
    >
      <div className="mx-auto max-w-6xl">
        <div className="mb-10 grid grid-cols-2 gap-10 md:grid-cols-5">
          <div>
            <div className="mb-3 text-lg font-bold text-white">
              ATOM<span style={{ color: "#22d3ee" }}>i</span>K
            </div>
            <p className="text-xs leading-relaxed">
              State-aware compute for systems that spend too much work rediscovering what changed.
            </p>
          </div>

          {footerCols.map((col) => (
            <div key={col.title}>
              <h4 className="mb-3 text-xs font-semibold uppercase text-white">
                {col.title}
              </h4>
              <div className="flex flex-col gap-2">
                {col.links.map(([href, label]) => {
                  const isExternal =
                    href.startsWith("http") || href.startsWith("mailto:");
                  const cls =
                    "text-xs no-underline transition-colors hover:text-white";
                  const s = { color: "#9fb1c7" as const };
                  return isExternal ? (
                    <a key={label} href={href} className={cls} style={s}>
                      {label}
                    </a>
                  ) : (
                    <Link key={label} href={href} className={cls} style={s}>
                      {label}
                    </Link>
                  );
                })}
              </div>
            </div>
          ))}
        </div>

        <div className="pt-6" style={{ borderTop: "1px solid #1d324a" }}>
          <p className="max-w-4xl text-xs leading-6">
            Live screenshots show current prototypes. Concept visuals show product direction and are not represented as current shipped functionality. Performance claims are only stated when backed by measured artifacts.
          </p>
          <div className="mt-5 flex flex-wrap justify-between gap-4">
            <p>&copy; 2026 ATOMiK Project. All rights reserved.</p>
            <div className="flex gap-5">
              <Link href="/privacy" className="no-underline transition-colors hover:text-white" style={{ color: "#9fb1c7" }}>
                Privacy
              </Link>
              <Link href="/terms" className="no-underline transition-colors hover:text-white" style={{ color: "#9fb1c7" }}>
                Terms
              </Link>
              <a href="mailto:support@atomik.tech" className="no-underline transition-colors hover:text-white" style={{ color: "#9fb1c7" }}>
                Support
              </a>
            </div>
          </div>
        </div>
      </div>
    </footer>
  );
}
