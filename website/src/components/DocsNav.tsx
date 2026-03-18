"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";

/* ── Ordered list of docs pages ──────────────────────────────────── */

const docsPages = [
  { href: "/docs", label: "Overview" },
  { href: "/docs/quickstart", label: "Quick Start" },
  { href: "/docs/api", label: "API Reference" },
  { href: "/docs/architecture", label: "Architecture" },
  { href: "/docs/examples", label: "Examples" },
  { href: "/docs/kernel-module", label: "Kernel Module" },
  { href: "/docs/hardware", label: "Hardware" },
  { href: "/docs/migrate-crdt", label: "From CRDTs" },
  { href: "/docs/migrate-event-sourcing", label: "From Event Sourcing" },
  { href: "/docs/compatibility", label: "Compatibility" },
];

/* ── Breadcrumbs (top of page) ───────────────────────────────────── */

export function DocsBreadcrumb() {
  const pathname = usePathname();
  const current = docsPages.find((p) => p.href === pathname);

  return (
    <nav className="flex items-center gap-2 text-sm mb-8" aria-label="Breadcrumb">
      {pathname === "/docs" ? (
        <span style={{ color: "#e0e0e8" }} className="font-medium">
          Docs
        </span>
      ) : (
        <>
          <Link
            href="/docs"
            className="no-underline transition-colors hover:underline"
            style={{ color: "#8888a0" }}
          >
            Docs
          </Link>
          <span style={{ color: "#555570" }}>/</span>
          <span style={{ color: "#e0e0e8" }} className="font-medium">
            {current?.label ?? ""}
          </span>
        </>
      )}
    </nav>
  );
}

/* ── Prev / Next links (bottom of page) ──────────────────────────── */

export function DocsPrevNext() {
  const pathname = usePathname();
  const idx = docsPages.findIndex((p) => p.href === pathname);

  const prev = idx > 0 ? docsPages[idx - 1] : null;
  const next = idx >= 0 && idx < docsPages.length - 1 ? docsPages[idx + 1] : null;

  if (!prev && !next) return null;

  return (
    <div
      className="mt-16 pt-8 border-t flex items-center justify-between"
      style={{ borderColor: "#1e1e2e" }}
    >
      {prev ? (
        <Link
          href={prev.href}
          className="group flex items-center gap-2 text-sm no-underline transition-colors"
          style={{ color: "#4f8fff" }}
        >
          <span
            className="transition-transform group-hover:-translate-x-0.5"
            aria-hidden="true"
          >
            &larr;
          </span>
          {prev.label}
        </Link>
      ) : (
        <span />
      )}
      {next ? (
        <Link
          href={next.href}
          className="group flex items-center gap-2 text-sm no-underline transition-colors"
          style={{ color: "#4f8fff" }}
        >
          {next.label}
          <span
            className="transition-transform group-hover:translate-x-0.5"
            aria-hidden="true"
          >
            &rarr;
          </span>
        </Link>
      ) : (
        <span />
      )}
    </div>
  );
}
