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
  title: "ATOMiK - Cooler, Faster State-Aware Compute",
  description:
    "ATOMiK targets wasted state movement so devices and infrastructure can run cooler, move less data, and do more useful work per watt.",
  icons: {
    icon: [
      { url: "/favicon.ico", sizes: "any" },
      { url: "/favicon-32x32.png", sizes: "32x32", type: "image/png" },
      { url: "/favicon-16x16.png", sizes: "16x16", type: "image/png" },
    ],
    apple: "/apple-touch-icon.png",
  },
  openGraph: {
    title: "ATOMiK - Cooler, Faster State-Aware Compute",
    description:
      "Less heat, less bandwidth, and more useful work per watt through state-aware compute.",
    url: "https://atomik.tech",
    siteName: "ATOMiK",
    images: [{ url: "https://atomik.tech/09-current-live-atomik-desk-v039k.png", width: 1920, height: 1080 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK - Cooler, Faster State-Aware Compute",
    description:
      "Less heat, less bandwidth, and more useful work per watt through state-aware compute.",
    images: ["https://atomik.tech/09-current-live-atomik-desk-v039k.png"],
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
        "State-aware compute for systems constrained by heat, power, bandwidth, latency, or hardware footprint.",
    },
    {
      "@type": "Service",
      name: "ATOMiK Evaluation Access",
      serviceType: "Technical evaluation",
      provider: {
        "@type": "Organization",
        name: "ATOMiK",
      },
      description:
        "Request-based technical evaluation access for workloads constrained by heat, power, bandwidth, latency, or hardware footprint.",
      url: "https://atomik.tech/contact",
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
