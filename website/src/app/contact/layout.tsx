import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "Contact ATOMiK - Evaluation and Investor Diligence",
  description:
    "Request ATOMiK technical evaluation, investor diligence, design-partner scoping, or licensing review.",
  openGraph: {
    title: "Contact ATOMiK - Evaluation and Investor Diligence",
    description:
      "Request evaluation access around heat, power, bandwidth, latency, footprint, or investor diligence.",
    url: "https://atomik.tech/contact",
    images: [{ url: "https://atomik.tech/10-current-live-atomik-desk-v040a.png", width: 1920, height: 1080 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "Contact ATOMiK - Evaluation and Investor Diligence",
    description:
      "Request evaluation access, investor diligence, or licensing review.",
    images: ["https://atomik.tech/10-current-live-atomik-desk-v040a.png"],
  },
};

export default function ContactLayout({ children }: { children: React.ReactNode }) {
  return children;
}
