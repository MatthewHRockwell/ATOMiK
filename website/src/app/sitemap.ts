import type { MetadataRoute } from "next";

export default function sitemap(): MetadataRoute.Sitemap {
  const baseUrl = "https://atomik.tech";
  const now = new Date();

  return [
    { url: baseUrl, lastModified: now, changeFrequency: "weekly", priority: 1 },
    { url: `${baseUrl}/pitch`, lastModified: now, changeFrequency: "weekly", priority: 0.98 },
    { url: `${baseUrl}/pricing`, lastModified: now, changeFrequency: "weekly", priority: 0.96 },
    { url: `${baseUrl}/benchmarks`, lastModified: now, changeFrequency: "weekly", priority: 0.94 },
    { url: `${baseUrl}/solutions`, lastModified: now, changeFrequency: "weekly", priority: 0.92 },
    { url: `${baseUrl}/contact`, lastModified: now, changeFrequency: "monthly", priority: 0.9 },
    { url: `${baseUrl}/investor-brief`, lastModified: now, changeFrequency: "weekly", priority: 0.78 },
    { url: `${baseUrl}/docs/hardware`, lastModified: now, changeFrequency: "weekly", priority: 0.74 },
    { url: `${baseUrl}/docs`, lastModified: now, changeFrequency: "weekly", priority: 0.68 },
    { url: `${baseUrl}/about`, lastModified: now, changeFrequency: "monthly", priority: 0.62 },
    { url: `${baseUrl}/about/roadmap`, lastModified: now, changeFrequency: "monthly", priority: 0.58 },
    { url: `${baseUrl}/faq`, lastModified: now, changeFrequency: "monthly", priority: 0.52 },
    { url: `${baseUrl}/compliance`, lastModified: now, changeFrequency: "monthly", priority: 0.5 },
    { url: `${baseUrl}/case-studies`, lastModified: now, changeFrequency: "monthly", priority: 0.48 },
    { url: `${baseUrl}/changelog`, lastModified: now, changeFrequency: "weekly", priority: 0.45 },
    { url: `${baseUrl}/get-started`, lastModified: now, changeFrequency: "monthly", priority: 0.4 },
    { url: `${baseUrl}/docs/quickstart`, lastModified: now, changeFrequency: "monthly", priority: 0.36 },
    { url: `${baseUrl}/docs/api`, lastModified: now, changeFrequency: "monthly", priority: 0.36 },
    { url: `${baseUrl}/docs/architecture`, lastModified: now, changeFrequency: "monthly", priority: 0.36 },
    { url: `${baseUrl}/docs/examples`, lastModified: now, changeFrequency: "monthly", priority: 0.34 },
    { url: `${baseUrl}/docs/kernel-module`, lastModified: now, changeFrequency: "monthly", priority: 0.34 },
    { url: `${baseUrl}/docs/migrate-crdt`, lastModified: now, changeFrequency: "monthly", priority: 0.32 },
    { url: `${baseUrl}/docs/migrate-event-sourcing`, lastModified: now, changeFrequency: "monthly", priority: 0.32 },
    { url: `${baseUrl}/docs/compatibility`, lastModified: now, changeFrequency: "monthly", priority: 0.3 },
    { url: `${baseUrl}/whitepaper`, lastModified: now, changeFrequency: "monthly", priority: 0.3 },
    { url: `${baseUrl}/privacy`, lastModified: now, changeFrequency: "yearly", priority: 0.2 },
    { url: `${baseUrl}/terms`, lastModified: now, changeFrequency: "yearly", priority: 0.2 },
  ];
}
