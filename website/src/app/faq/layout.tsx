import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "FAQ - ATOMiK Evaluation and Proof Boundaries",
  description:
    "ATOMiK public answers about evaluation access, proof labels, current prototype status, and diligence paths.",
  openGraph: {
    title: "FAQ - ATOMiK Evaluation and Proof Boundaries",
    description:
      "Public answers about ATOMiK evaluation access, evidence labels, and current prototype status.",
    url: "https://atomik.tech/faq",
    images: [{ url: "https://atomik.tech/10-current-live-atomik-desk-v040a.png", width: 1920, height: 1080 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "FAQ - ATOMiK Evaluation and Proof Boundaries",
    description:
      "Evaluation access, evidence labels, and current prototype status.",
    images: ["https://atomik.tech/10-current-live-atomik-desk-v040a.png"],
  },
};

export default function FAQLayout({ children }: { children: React.ReactNode }) {
  return children;
}
