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
  title: "ATOMiK — Delta-State Computing",
  description: "O(1) state reconstruction, 99% less bandwidth, 333,333x less memory. Formally proven with 92 Lean4 theorems.",
  icons: {
    icon: [
      { url: "/favicon.ico", sizes: "any" },
      { url: "/favicon-32x32.png", sizes: "32x32", type: "image/png" },
      { url: "/favicon-16x16.png", sizes: "16x16", type: "image/png" },
    ],
    apple: "/apple-touch-icon.png",
  },
  openGraph: {
    title: "ATOMiK — Stop moving data. Start evolving it.",
    description: "O(1) state reconstruction. 99% less bandwidth. 333,333x less memory. Formally proven with 92 Lean4 theorems.",
    url: "https://atomik.tech",
    siteName: "ATOMiK",
    images: [{ url: "https://atomik.tech/og-image.jpg", width: 1200, height: 630 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK — Stop moving data. Start evolving it.",
    description: "O(1) state reconstruction. 99% less bandwidth. 333,333x less memory.",
    images: ["https://atomik.tech/og-image.jpg"],
  },
};

const jsonLd = {
  "@context": "https://schema.org",
  "@type": "SoftwareApplication",
  name: "ATOMiK",
  applicationCategory: "DeveloperApplication",
  operatingSystem: "Linux",
  description:
    "Delta-state algebra for O(1) state reconstruction. 99% less bandwidth, 333,333x less memory. Formally proven with 92 Lean4 theorems.",
  url: "https://atomik.tech",
  offers: [
    {
      "@type": "Offer",
      name: "Community",
      price: "0",
      priceCurrency: "USD",
      description: "Python SDK, Apache 2.0",
    },
    {
      "@type": "Offer",
      name: "Professional",
      price: "99",
      priceCurrency: "USD",
      description: "Linux kernel module with COW detection and network monitoring",
    },
    {
      "@type": "Offer",
      name: "Enterprise",
      price: "499",
      priceCurrency: "USD",
      description: "Hardware acceleration path with dedicated support",
    },
  ],
  author: {
    "@type": "Person",
    name: "Matt Rockwell",
  },
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
