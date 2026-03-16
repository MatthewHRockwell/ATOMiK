"use client";

import Link from "next/link";
import { useState, useEffect, useRef } from "react";

export default function Nav({ active }: { active?: string }) {
  const [menuOpen, setMenuOpen] = useState(false);
  const navRef = useRef<HTMLElement>(null);

  const links = [
    { href: "/", label: "Home" },
    { href: "/docs", label: "Docs" },
    { href: "/solutions", label: "Solutions" },
    { href: "/demo", label: "Demo" },
    { href: "/pricing", label: "Pricing" },
    { href: "/blog", label: "Blog" },
    { href: "/about", label: "About" },
  ];

  // Close menu when clicking outside the nav
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

  // Close menu on route change (resize past md breakpoint)
  useEffect(() => {
    function handleResize() {
      if (window.innerWidth >= 768) {
        setMenuOpen(false);
      }
    }
    window.addEventListener("resize", handleResize);
    return () => window.removeEventListener("resize", handleResize);
  }, []);

  return (
    <nav
      ref={navRef}
      className="sticky top-0 z-50 backdrop-blur-md border-b"
      style={{ background: "rgba(10, 10, 15, 0.85)", borderColor: "#1e1e2e" }}
    >
      <div className="max-w-5xl mx-auto px-6 py-4 flex items-center justify-between">
        {/* Logo */}
        <Link
          href="/"
          className="text-lg font-bold tracking-tight hover:opacity-80 transition-opacity"
        >
          <span style={{ color: "#8b5cf6" }}>ATOM</span>
          <span style={{ color: "#4f8fff" }}>i</span>
          <span style={{ color: "#8b5cf6" }}>K</span>
        </Link>

        {/* Desktop links */}
        <div
          className="hidden md:flex items-center gap-6 text-sm"
          style={{ color: "#8888a0" }}
        >
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

        {/* Hamburger button (mobile only) */}
        <button
          className="md:hidden flex flex-col justify-center items-center w-8 h-8 gap-[5px] group"
          onClick={() => setMenuOpen((prev) => !prev)}
          aria-label={menuOpen ? "Close menu" : "Open menu"}
          aria-expanded={menuOpen}
        >
          <span
            className="block w-5 h-[2px] rounded-full transition-all duration-300 ease-in-out"
            style={{
              background: "#8888a0",
              transform: menuOpen
                ? "translateY(7px) rotate(45deg)"
                : "none",
            }}
          />
          <span
            className="block w-5 h-[2px] rounded-full transition-all duration-300 ease-in-out"
            style={{
              background: "#8888a0",
              opacity: menuOpen ? 0 : 1,
            }}
          />
          <span
            className="block w-5 h-[2px] rounded-full transition-all duration-300 ease-in-out"
            style={{
              background: "#8888a0",
              transform: menuOpen
                ? "translateY(-7px) rotate(-45deg)"
                : "none",
            }}
          />
        </button>
      </div>

      {/* Mobile dropdown panel */}
      <div
        className="md:hidden overflow-hidden transition-all duration-300 ease-in-out"
        style={{
          maxHeight: menuOpen ? `${links.length * 48 + 16}px` : "0px",
          opacity: menuOpen ? 1 : 0,
          borderTop: menuOpen ? "1px solid #1e1e2e" : "none",
        }}
      >
        <div className="max-w-5xl mx-auto px-6 py-2 flex flex-col text-sm">
          {links.map((link) =>
            link.label === active ? (
              <span
                key={link.href}
                className="text-white font-medium py-2.5 px-2"
              >
                {link.label}
              </span>
            ) : (
              <Link
                key={link.href}
                href={link.href}
                className="py-2.5 px-2 hover:text-white transition-colors"
                style={{ color: "#8888a0" }}
                onClick={() => setMenuOpen(false)}
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
