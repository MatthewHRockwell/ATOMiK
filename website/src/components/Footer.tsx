import Link from "next/link";
import { contactHref } from "@/lib/tracking";

const footerCols: { title: string; links: [string, string][] }[] = [
  {
    title: "Evaluate",
    links: [
      [contactHref("evaluation", "footer", "request-evaluation"), "Request Evaluation"],
      ["/pricing", "Evaluation Process"],
      [contactHref("proof", "footer", "proof-review"), "Proof Review"],
      [contactHref("design-partner", "footer", "design-partnership"), "Design Partnership"],
      [contactHref("licensing", "footer", "licensing-ip"), "Discuss Licensing"],
      ["/investor-brief", "Investor Diligence"],
    ],
  },
  {
    title: "Proof",
    links: [
      ["/benchmarks", "Proof Packet"],
      ["/docs/hardware", "Hardware Proof Map"],
      ["https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/evidence-labels.md", "Evidence Labels"],
      ["https://github.com/MatthewHRockwell/ATOMiK/blob/main/results/claims_registry.yaml", "Claims Registry"],
    ],
  },
  {
    title: "Routes",
    links: [
      ["/solutions", "Use Cases"],
      ["/pitch", "Friday Brief"],
      ["/about", "About"],
      ["/about/roadmap", "Validation Roadmap"],
      ["mailto:matthew.h.rockwell@gmail.com", "Contact"],
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
      style={{ borderTop: "1px solid #2d3a34", background: "#070807", color: "#b7c4bb" }}
    >
      <div className="mx-auto max-w-6xl">
        <div className="mb-10 grid grid-cols-2 gap-10 md:grid-cols-5">
          <div>
            <div className="mb-3 text-lg font-bold text-white">
              ATOM<span style={{ color: "#22d3ee" }}>i</span>K
            </div>
            <p className="text-xs leading-relaxed">
              State-aware compute evaluation for teams constrained by heat, battery, bandwidth, latency, reliability, or hardware footprint.
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
                  const s = { color: "#b7c4bb" as const };
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

        <div className="pt-6" style={{ borderTop: "1px solid #2d3a34" }}>
          <p className="max-w-4xl text-xs leading-6">
            Public claims are evidence-bound. Cooling, battery, speed, bandwidth, power, water, footprint, and production-readiness claims require matching artifacts and context. Concept visuals show direction, not current commercial functionality.
          </p>
          <div className="mt-5 flex flex-wrap justify-between gap-4">
            <p>&copy; 2026 ATOMiK Project. All rights reserved.</p>
            <div className="flex gap-5">
              <Link href="/privacy" className="no-underline transition-colors hover:text-white" style={{ color: "#b7c4bb" }}>
                Privacy
              </Link>
              <Link href="/terms" className="no-underline transition-colors hover:text-white" style={{ color: "#b7c4bb" }}>
                Terms
              </Link>
              <a href="mailto:matthew.h.rockwell@gmail.com?subject=ATOMiK%20Evaluation" className="no-underline transition-colors hover:text-white" style={{ color: "#b7c4bb" }}>
                Contact
              </a>
            </div>
          </div>
        </div>
      </div>
    </footer>
  );
}
