import type { Metadata } from "next";
import { Geist, Geist_Mono } from "next/font/google";
import Footer from "@/components/Footer";
import "./globals.css";

const geistSans = Geist({
  variable: "--font-geist-sans",
  subsets: ["latin"],
});

const geistMono = Geist_Mono({
  variable: "--font-geist-mono",
  subsets: ["latin"],
});

export const metadata: Metadata = {
  metadataBase: new URL("https://atomik.tech"),
  title: "ATOMiK - State-Aware Compute Evaluation",
  description:
    "ATOMiK helps edge and embedded teams evaluate whether state-aware execution can reduce redundant state movement in constrained workloads.",
  icons: {
    icon: [
      { url: "/favicon.ico", sizes: "any" },
      { url: "/favicon-32x32.png", sizes: "32x32", type: "image/png" },
      { url: "/favicon-16x16.png", sizes: "16x16", type: "image/png" },
    ],
    apple: "/apple-touch-icon.png",
  },
  openGraph: {
    title: "ATOMiK - State-Aware Compute Evaluation",
    description:
      "Bring one state-heavy workload, one current baseline, and one painful constraint.",
    url: "https://atomik.tech",
    siteName: "ATOMiK",
    images: [{ url: "https://atomik.tech/10-current-live-atomik-desk-v040a.png", width: 1920, height: 1080 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK - State-Aware Compute Evaluation",
    description:
      "Proof-bound evaluation for edge and embedded teams constrained by state movement.",
    images: ["https://atomik.tech/10-current-live-atomik-desk-v040a.png"],
  },
};

const jsonLd = {
  "@context": "https://schema.org",
  "@graph": [
    {
      "@type": "Organization",
      name: "ATOMiK",
      url: "https://atomik.tech",
      logo: "https://atomik.tech/logo.png",
      sameAs: ["https://github.com/MatthewHRockwell/ATOMiK"],
      description:
        "State-aware compute architecture for teams evaluating constrained state-heavy workloads.",
    },
    {
      "@type": "Service",
      name: "ATOMiK Evaluation",
      serviceType: "Technical evaluation",
      provider: {
        "@type": "Organization",
        name: "ATOMiK",
      },
      description:
        "Proof-bound technical evaluation for one state-heavy workload, one baseline, and one measurable constraint.",
      url: "https://atomik.tech/pricing",
    },
  ],
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en" className="dark">
      <head>
        <script
          type="application/ld+json"
          dangerouslySetInnerHTML={{ __html: JSON.stringify(jsonLd) }}
        />
      </head>
      <body
        className={`${geistSans.variable} ${geistMono.variable} antialiased`}
      >
        {children}
        <Footer />
      </body>
    </html>
  );
}
