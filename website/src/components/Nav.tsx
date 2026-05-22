"use client";

import Link from "next/link";
import { useState, useEffect, useRef } from "react";

export default function Nav({ active }: { active?: string }) {
  const [menuOpen, setMenuOpen] = useState(false);
  const navRef = useRef<HTMLElement>(null);

  const links = [
    { href: "/", label: "Product" },
    { href: "/solutions", label: "Use Cases" },
    { href: "/benchmarks", label: "Proof" },
    { href: "/investor-brief", label: "Investors" },
    { href: "/pricing", label: "Evaluation" },
    { href: "/about", label: "About" },
    { href: "https://github.com/MatthewHRockwell/ATOMiK", label: "GitHub" },
  ];

  useEffect(() => {
    if (!menuOpen) return;
    function handleClickOutside(e: MouseEvent) {
      if (navRef.current && !navRef.current.contains(e.target as Node)) {
        setMenuOpen(false);
      }
    }
    document.addEventListener("mousedown", handleClickOutside);
    return () => document.removeEventListener("mousedown", handleClickOutside);
  }, [menuOpen]);

  useEffect(() => {
    function handleResize() {
      if (window.innerWidth >= 1024) {
        setMenuOpen(false);
      }
    }
    window.addEventListener("resize", handleResize);
    return () => window.removeEventListener("resize", handleResize);
  }, []);

  return (
    <nav
      ref={navRef}
      className="sticky top-0 z-50 border-b backdrop-blur-md"
      style={{ background: "rgba(7, 11, 18, 0.88)", borderColor: "#1d324a" }}
    >
      <div className="mx-auto flex max-w-7xl items-center justify-between px-6 py-4">
        <Link
          href="/"
          className="text-lg font-bold no-underline transition-opacity hover:opacity-80"
          style={{ color: "#f4f8ff" }}
        >
          ATOM<span style={{ color: "#22d3ee" }}>i</span>K
        </Link>

        <div className="hidden items-center gap-4 text-sm lg:flex" style={{ color: "#9fb1c7" }}>
          {links.map((link) =>
            link.label === active ? (
              <span key={link.href} className="font-medium text-white">
                {link.label}
              </span>
            ) : (
              <Link
                key={link.href}
                href={link.href}
                className="no-underline transition-colors hover:text-white"
              >
                {link.label}
              </Link>
            )
          )}
          <Link
            href="/contact?intent=evaluation"
            className="rounded-lg px-4 py-2 text-sm font-semibold text-white no-underline transition-opacity hover:opacity-90"
            style={{ background: "#4f8fff" }}
          >
            Request Technical Evaluation
          </Link>
        </div>

        <button
          className="flex h-8 w-8 flex-col items-center justify-center gap-[5px] lg:hidden"
          onClick={() => setMenuOpen((prev) => !prev)}
          aria-label={menuOpen ? "Close menu" : "Open menu"}
          aria-expanded={menuOpen}
        >
          <span
            className="block h-[2px] w-5 rounded-full transition-all duration-300"
            style={{
              background: "#9fb1c7",
              transform: menuOpen ? "translateY(7px) rotate(45deg)" : "none",
            }}
          />
          <span
            className="block h-[2px] w-5 rounded-full transition-all duration-300"
            style={{ background: "#9fb1c7", opacity: menuOpen ? 0 : 1 }}
          />
          <span
            className="block h-[2px] w-5 rounded-full transition-all duration-300"
            style={{
              background: "#9fb1c7",
              transform: menuOpen ? "translateY(-7px) rotate(-45deg)" : "none",
            }}
          />
        </button>
      </div>

      <div
        className="overflow-hidden transition-all duration-300 lg:hidden"
        style={{
          maxHeight: menuOpen ? `${(links.length + 1) * 48 + 24}px` : "0px",
          opacity: menuOpen ? 1 : 0,
          borderTop: menuOpen ? "1px solid #1d324a" : "none",
        }}
      >
        <div className="mx-auto flex max-w-7xl flex-col px-6 py-2 text-sm">
          {links.map((link) =>
            link.label === active ? (
              <span key={link.href} className="px-2 py-2.5 font-medium text-white">
                {link.label}
              </span>
            ) : (
              <Link
                key={link.href}
                href={link.href}
                className="px-2 py-2.5 no-underline transition-colors hover:text-white"
                style={{ color: "#9fb1c7" }}
                onClick={() => setMenuOpen(false)}
              >
                {link.label}
              </Link>
            )
          )}
          <Link
            href="/contact?intent=evaluation"
            className="mx-2 mt-2 rounded-lg py-2.5 text-center text-sm font-semibold text-white no-underline transition-opacity hover:opacity-90"
            style={{ background: "#4f8fff" }}
            onClick={() => setMenuOpen(false)}
          >
            Request Technical Evaluation
          </Link>
        </div>
      </div>
    </nav>
  );
}
