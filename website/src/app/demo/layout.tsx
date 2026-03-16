import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "Interactive Demo — ATOMiK",
  description:
    "Try ATOMiK's delta-state algebra in your browser. LOAD, ACCUM, READ, and SWAP — experience commutativity, self-inverse, and order independence live.",
};

export default function DemoLayout({ children }: { children: React.ReactNode }) {
  return <>{children}</>;
}
