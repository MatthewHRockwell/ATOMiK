import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "Technical Proof Brief - ATOMiK",
  description:
    "Artifact-first technical proof map for ATOMiK state-aware compute.",
  openGraph: {
    title: "ATOMiK Technical Proof Brief",
    description:
      "Review ATOMiK by linked evidence artifacts, not headline numbers.",
    url: "https://atomik.tech/whitepaper",
    images: [{ url: "https://atomik.tech/10-current-live-atomik-desk-v040a.png", width: 1920, height: 1080 }],
    type: "article",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK Technical Proof Brief",
    description:
      "Evidence-labeled technical proof map for state-aware compute.",
    images: ["https://atomik.tech/10-current-live-atomik-desk-v040a.png"],
  },
};

export default function WhitepaperLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return <>{children}</>;
}
