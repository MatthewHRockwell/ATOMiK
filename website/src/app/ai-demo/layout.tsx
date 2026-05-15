import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "AI Workload Concept - ATOMiK",
  description:
    "Conceptual AI workload surface for exploring where state-aware execution could fit. Performance and power claims require measured artifacts.",
  openGraph: {
    title: "AI Workload Concept - ATOMiK",
    description:
      "A concept surface for state-aware AI workload discussion, not a measured power benchmark.",
    url: "https://atomik.tech/ai-demo",
  },
};

export default function AILayout({ children }: { children: React.ReactNode }) {
  return children;
}
