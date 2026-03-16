"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";

const sections = [
  { href: "/docs", label: "Overview" },
  { href: "/docs/quickstart", label: "Quick Start" },
  { href: "/docs/kernel-module", label: "Kernel Module" },
  { href: "/docs/api", label: "API Reference" },
  { href: "/docs/architecture", label: "Architecture" },
  { href: "/docs/examples", label: "Examples" },
  { href: "/docs/hardware", label: "Hardware" },
];

export default function DocsSidebar() {
  const pathname = usePathname();

  return (
    <>
      {/* Desktop sidebar */}
      <aside
        className="hidden lg:block fixed top-[65px] left-0 w-[240px] h-[calc(100vh-65px)] overflow-y-auto border-r"
        style={{ background: "#0a0a0f", borderColor: "#1e1e2e" }}
      >
        <div className="px-5 py-8">
          <p
            className="text-xs font-mono tracking-widest uppercase mb-6"
            style={{ color: "#8b5cf6" }}
          >
            Documentation
          </p>
          <nav className="flex flex-col gap-1">
            {sections.map((s) => {
              const active = pathname === s.href;
              return (
                <Link
                  key={s.href}
                  href={s.href}
                  className="block px-3 py-2 rounded-lg text-sm transition-colors no-underline"
                  style={{
                    background: active ? "#1e1e2e" : "transparent",
                    color: active ? "#e0e0e8" : "#8888a0",
                    fontWeight: active ? 600 : 400,
                  }}
                >
                  {s.label}
                </Link>
              );
            })}
          </nav>
        </div>
      </aside>

      {/* Mobile horizontal scroll menu */}
      <div
        className="lg:hidden sticky top-[65px] z-40 border-b overflow-x-auto"
        style={{ background: "rgba(10, 10, 15, 0.95)", borderColor: "#1e1e2e" }}
      >
        <nav className="flex gap-1 px-4 py-3 min-w-max">
          {sections.map((s) => {
            const active = pathname === s.href;
            return (
              <Link
                key={s.href}
                href={s.href}
                className="px-3 py-1.5 rounded-lg text-sm whitespace-nowrap transition-colors no-underline"
                style={{
                  background: active ? "#1e1e2e" : "transparent",
                  color: active ? "#e0e0e8" : "#8888a0",
                  fontWeight: active ? 600 : 400,
                }}
              >
                {s.label}
              </Link>
            );
          })}
        </nav>
      </div>
    </>
  );
}
