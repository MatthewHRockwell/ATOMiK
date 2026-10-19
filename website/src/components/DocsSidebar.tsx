"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";

const sections = [
  { href: "/docs", label: "Start Here" },
  { href: "/docs/hardware", label: "Hardware Proof" },
  { href: "/benchmarks", label: "Proof Packet" },
  { href: "/docs/architecture", label: "Architecture" },
  { href: "/docs/compatibility", label: "Compatibility" },
  { href: "/docs/quickstart", label: "Quick Start" },
  { href: "/docs/api", label: "API Reference" },
  { href: "/docs/examples", label: "Examples" },
  { href: "/docs/kernel-module", label: "Kernel Module" },
  { href: "/docs/migrate-crdt", label: "From CRDTs" },
  { href: "/docs/migrate-event-sourcing", label: "From Event Sourcing" },
];

export default function DocsSidebar() {
  const pathname = usePathname();
  const current = sections.find((s) => s.href === pathname);

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

      {/* Mobile docs menu: collapsed to avoid crowding the investor proof pages. */}
      <div
        className="lg:hidden sticky top-[57px] z-40 border-b px-4 py-2"
        style={{ background: "rgba(10, 10, 15, 0.96)", borderColor: "#1e1e2e" }}
      >
        <details className="group">
          <summary
            className="flex cursor-pointer list-none items-center justify-between rounded-lg px-3 py-2 text-sm font-semibold"
            style={{ background: "#12121a", color: "#e0e0e8", border: "1px solid #1e1e2e" }}
          >
            <span>Docs / {current?.label ?? "Overview"}</span>
            <span aria-hidden="true" style={{ color: "#8888a0" }}>Menu</span>
          </summary>
          <nav className="mt-2 grid gap-1 rounded-lg p-2" style={{ background: "#0a0a0f", border: "1px solid #1e1e2e" }}>
            {sections.map((s) => {
              const active = pathname === s.href;
              return (
                <Link
                  key={s.href}
                  href={s.href}
                  className="block rounded-md px-3 py-2 text-sm transition-colors no-underline"
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
        </details>
      </div>
    </>
  );
}
