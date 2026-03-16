import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "AI Inference Demo — ATOMiK vs Traditional GPU Architecture",
  description:
    "Interactive side-by-side comparison: same AI output, fraction of the energy. See how delta-state architecture reduces inference power from 220W to 12W.",
  openGraph: {
    title: "AI Inference: Conventional Architecture vs ATOMiK",
    description: "Same AI output. 18x less power. See why in 10 seconds.",
    url: "https://atomik.tech/ai-demo",
  },
};

export default function AILayout({ children }: { children: React.ReactNode }) {
  return children;
}
