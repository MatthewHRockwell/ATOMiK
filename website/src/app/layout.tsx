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
  title: "ATOMiK - State-Aware Compute",
  description:
    "State-aware compute for systems that spend too much work rediscovering what changed.",
  icons: {
    icon: [
      { url: "/favicon.ico", sizes: "any" },
      { url: "/favicon-32x32.png", sizes: "32x32", type: "image/png" },
      { url: "/favicon-16x16.png", sizes: "16x16", type: "image/png" },
    ],
    apple: "/apple-touch-icon.png",
  },
  openGraph: {
    title: "ATOMiK - State-Aware Compute",
    description:
      "State-aware compute for systems that cannot afford to recompute everything.",
    url: "https://atomik.tech",
    siteName: "ATOMiK",
    images: [{ url: "https://atomik.tech/01-current-live-atomik-desk.jpg", width: 1200, height: 630 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK - State-Aware Compute",
    description:
      "State-aware compute for systems that cannot afford to recompute everything.",
    images: ["https://atomik.tech/01-current-live-atomik-desk.jpg"],
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
        "State-aware compute for systems that spend too much work rediscovering what changed.",
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
        "Request-based technical evaluation access for state-heavy edge, embedded, and distributed workloads.",
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
