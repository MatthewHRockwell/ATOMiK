import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "ATOMiK White Paper — Delta-State Algebra Technical Deep-Dive",
  description:
    "Mathematical foundations, hardware implementation, and production results. 92 Lean4 theorems, 69.7 Gops/s peak throughput, 916,000x memory traffic reduction.",
  openGraph: {
    title: "ATOMiK White Paper — Delta-State Algebra Technical Deep-Dive",
    description:
      "Mathematical foundations, hardware implementation, and production results. 92 Lean4 theorems, 69.7 Gops/s peak throughput.",
    url: "https://atomik.tech/whitepaper",
    images: [{ url: "https://atomik.tech/og-image.jpg", width: 1200, height: 630 }],
    type: "article",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK White Paper — Delta-State Algebra Technical Deep-Dive",
    description:
      "92 Lean4 theorems. 69.7 Gops/s peak. 916,000x memory traffic reduction. Read the technical deep-dive.",
    images: ["https://atomik.tech/og-image.jpg"],
  },
};

export default function WhitepaperLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return <>{children}</>;
}
