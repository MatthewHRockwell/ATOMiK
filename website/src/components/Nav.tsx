import Link from "next/link";

export default function Nav({ active }: { active?: string }) {
  const links = [
    { href: "/", label: "Home" },
    { href: "/docs", label: "Docs" },
    { href: "/solutions", label: "Solutions" },
    { href: "/demo", label: "Demo" },
    { href: "/pricing", label: "Pricing" },
    { href: "/blog", label: "Blog" },
    { href: "/about", label: "About" },
  ];

  return (
    <nav
      className="sticky top-0 z-50 backdrop-blur-md border-b"
      style={{ background: "rgba(10, 10, 15, 0.85)", borderColor: "#1e1e2e" }}
    >
      <div className="max-w-5xl mx-auto px-6 py-4 flex items-center justify-between">
        <Link
          href="/"
          className="text-lg font-bold tracking-tight hover:opacity-80 transition-opacity"
        >
          <span style={{ color: "#8b5cf6" }}>ATOM</span>
          <span style={{ color: "#4f8fff" }}>i</span>
          <span style={{ color: "#8b5cf6" }}>K</span>
        </Link>
        <div className="flex items-center gap-6 text-sm" style={{ color: "#8888a0" }}>
          {links.map((link) =>
            link.label === active ? (
              <span key={link.href} className="text-white font-medium">
                {link.label}
              </span>
            ) : (
              <Link
                key={link.href}
                href={link.href}
                className="hover:text-white transition-colors"
              >
                {link.label}
              </Link>
            )
          )}
        </div>
      </div>
    </nav>
  );
}
