import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "ATOMiK White Paper — Delta-State Algebra Technical Deep-Dive",
  description:
    "Mathematical foundations, hardware implementation notes, and proof artifacts with evidence labels.",
  openGraph: {
    title: "ATOMiK White Paper — Delta-State Algebra Technical Deep-Dive",
    description:
      "Technical background for state-aware compute. Performance claims require linked measured artifacts.",
    url: "https://atomik.tech/whitepaper",
    images: [{ url: "https://atomik.tech/og-image.jpg", width: 1200, height: 630 }],
    type: "article",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK White Paper — Delta-State Algebra Technical Deep-Dive",
    description:
      "Technical background, proof notes, and evidence-labeled artifact guidance.",
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
