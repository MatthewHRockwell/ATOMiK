import Link from "next/link";

/* ── Inline code-block styling ────────────────────────────────────── */

export const codeBlockStyle: React.CSSProperties = {
  background: "#0d0d14",
  border: "1px solid #1e1e2e",
  borderRadius: "0.75rem",
  padding: "1.25rem",
  overflowX: "auto",
  fontSize: "0.8125rem",
  lineHeight: "1.7",
  fontFamily: "var(--font-mono), ui-monospace, monospace",
};

export const kwColor = "#c084fc"; // purple — keywords
export const fnColor = "#4f8fff"; // blue — functions/methods
export const strColor = "#22c55e"; // green — strings
export const numColor = "#d4a843"; // gold — numbers
export const cmtColor = "#555570"; // muted — comments
export const typeColor = "#38bdf8"; // cyan — types
export const varColor = "#e0e0e8"; // default text — variables

/* ── Prev / Next navigation ───────────────────────────────────────── */

interface NavLink {
  href: string;
  label: string;
}

export function DocsNav({ prev, next }: { prev?: NavLink; next?: NavLink }) {
  return (
    <div
      className="mt-16 pt-8 border-t flex items-center justify-between"
      style={{ borderColor: "#1e1e2e" }}
    >
      {prev ? (
        <Link
          href={prev.href}
          className="text-sm no-underline transition-colors"
          style={{ color: "#8888a0" }}
        >
          <span style={{ color: "#555570" }}>&larr;</span> {prev.label}
        </Link>
      ) : (
        <span />
      )}
      {next ? (
        <Link
          href={next.href}
          className="text-sm no-underline transition-colors"
          style={{ color: "#8888a0" }}
        >
          {next.label} <span style={{ color: "#555570" }}>&rarr;</span>
        </Link>
      ) : (
        <span />
      )}
    </div>
  );
}
