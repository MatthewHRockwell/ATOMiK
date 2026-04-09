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
  description: "O(1) state reconstruction, 99% less bandwidth, 330,000x less memory. Formally proven with 108 Lean4 theorems.",
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
    description: "O(1) state reconstruction. 99% less bandwidth. 330,000x less memory. Formally proven with 108 Lean4 theorems.",
    url: "https://atomik.tech",
    siteName: "ATOMiK",
    images: [{ url: "https://atomik.tech/og-image.jpg", width: 1200, height: 630 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK — Stop moving data. Start evolving it.",
    description: "O(1) state reconstruction. 99% less bandwidth. 330,000x less memory.",
    images: ["https://atomik.tech/og-image.jpg"],
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
        "Delta-state algebra for O(1) state reconstruction. 99% less bandwidth, 330,000x less memory. Formally proven with 108 Lean4 theorems.",
    },
    {
      "@type": "SoftwareApplication",
      name: "ATOMiK SDK",
      applicationCategory: "DeveloperApplication",
      operatingSystem: "Linux, macOS, Windows",
      description:
        "Delta-state algebra for O(1) state reconstruction. 99% less bandwidth, 330,000x less memory. Formally proven with 108 Lean4 theorems.",
      url: "https://atomik.tech",
      offers: [
        {
          "@type": "Offer",
          name: "Community",
          price: "0",
          priceCurrency: "USD",
          description: "Python SDK, C99 header, JavaScript SDK, Apache 2.0",
        },
        {
          "@type": "Offer",
          name: "Pro",
          price: "99",
          priceCurrency: "USD",
          description:
            "Linux kernel module (COW detection, network dedup, cgroup tracking), 27 sysfs metrics, atomik-bench, 48hr SLA",
        },
        {
          "@type": "Offer",
          name: "Team",
          price: "299",
          priceCurrency: "USD",
          description:
            "SDK generation pipeline (5 languages), atomik-report waste analysis, 5-seat team license, JSON/CSV CI export",
        },
        {
          "@type": "Offer",
          name: "Enterprise",
          price: "999",
          priceCurrency: "USD",
          description:
            "FPGA hardware acceleration (Zynq, 512 banks), custom RV64I CPU, 4-hour SLA, dedicated support, formal verification",
        },
      ],
      author: {
        "@type": "Person",
        name: "Matt Rockwell",
      },
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
