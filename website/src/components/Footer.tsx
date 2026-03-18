import Link from "next/link";

const footerCols: { title: string; links: [string, string][] }[] = [
  {
    title: "Product",
    links: [
      ["/get-started", "Get Started"],
      ["/dashboard", "Dashboard"],
      ["/pricing", "Pricing"],
      ["/integrations", "Integrations"],
      ["/get-started#downloads", "Downloads"],
    ],
  },
  {
    title: "Developers",
    links: [
      ["/docs", "Docs"],
      ["/blog", "Blog"],
      ["/benchmarks", "Benchmarks"],
      ["/roi", "ROI Calculator"],
      ["/faq", "FAQ"],
    ],
  },
  {
    title: "Company",
    links: [
      ["/about", "About"],
      ["/case-studies", "Case Studies"],
      ["/solutions", "Solutions"],
      ["/compliance", "Compliance"],
      ["mailto:mrockwell@atomik.tech", "Contact"],
    ],
  },
  {
    title: "Legal",
    links: [
      ["/privacy", "Privacy"],
      ["/terms", "Terms"],
      ["/changelog", "Changelog"],
      ["https://github.com/MatthewHRockwell/ATOMiK", "GitHub"],
    ],
  },
];

export default function Footer() {
  return (
    <footer
      className="px-6 pt-14 pb-10 text-sm"
      style={{ borderTop: "1px solid #1e1e2e", background: "#0a0a0f", color: "#8888a0" }}
    >
      <div className="max-w-5xl mx-auto">
        <div className="grid grid-cols-2 md:grid-cols-5 gap-10 mb-10">
          {/* Logo + tagline */}
          <div>
            <div className="text-lg font-bold text-white mb-3">
              <span style={{ color: "#8b5cf6" }}>ATOM</span>
              <span style={{ color: "#4f8fff" }}>i</span>
              <span style={{ color: "#8b5cf6" }}>K</span>
            </div>
            <p className="text-xs leading-relaxed">
              Delta-state computing infrastructure. Stop moving data. Start
              evolving it.
            </p>
          </div>

          {footerCols.map((col) => (
            <div key={col.title}>
              <h4 className="text-xs font-semibold uppercase tracking-wide text-white mb-3">
                {col.title}
              </h4>
              <div className="flex flex-col gap-2">
                {col.links.map(([href, label]) => {
                  const isExternal =
                    href.startsWith("http") || href.startsWith("mailto:");
                  const cls =
                    "hover:text-white transition-colors text-xs no-underline";
                  const s = { color: "#8888a0" as const };
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

        <div
          className="flex flex-wrap justify-between items-center gap-4 pt-6"
          style={{ borderTop: "1px solid #1e1e2e" }}
        >
          <p>&copy; 2026 ATOMiK Project. All rights reserved.</p>
          <div className="flex gap-5">
            <Link
              href="/privacy"
              className="hover:text-white transition-colors no-underline"
              style={{ color: "#8888a0" }}
            >
              Privacy
            </Link>
            <Link
              href="/terms"
              className="hover:text-white transition-colors no-underline"
              style={{ color: "#8888a0" }}
            >
              Terms
            </Link>
            <a
              href="mailto:support@atomik.tech"
              className="hover:text-white transition-colors no-underline"
              style={{ color: "#8888a0" }}
            >
              Support
            </a>
          </div>
        </div>
      </div>
    </footer>
  );
}
