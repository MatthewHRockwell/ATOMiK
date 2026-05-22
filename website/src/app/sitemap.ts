import type { MetadataRoute } from "next";

export default function sitemap(): MetadataRoute.Sitemap {
  const baseUrl = "https://atomik.tech";
  const now = new Date();

  return [
    { url: baseUrl, lastModified: now, changeFrequency: "weekly", priority: 1 },
    { url: `${baseUrl}/investor-brief`, lastModified: now, changeFrequency: "weekly", priority: 0.96 },
    { url: `${baseUrl}/solutions`, lastModified: now, changeFrequency: "weekly", priority: 0.92 },
    { url: `${baseUrl}/pricing`, lastModified: now, changeFrequency: "monthly", priority: 0.9 },
    { url: `${baseUrl}/contact`, lastModified: now, changeFrequency: "monthly", priority: 0.9 },
    { url: `${baseUrl}/benchmarks`, lastModified: now, changeFrequency: "weekly", priority: 0.86 },
    { url: `${baseUrl}/docs/hardware`, lastModified: now, changeFrequency: "weekly", priority: 0.86 },
    { url: `${baseUrl}/docs`, lastModified: now, changeFrequency: "weekly", priority: 0.8 },
    { url: `${baseUrl}/about`, lastModified: now, changeFrequency: "monthly", priority: 0.7 },
    { url: `${baseUrl}/about/roadmap`, lastModified: now, changeFrequency: "monthly", priority: 0.6 },
    { url: `${baseUrl}/changelog`, lastModified: now, changeFrequency: "weekly", priority: 0.55 },
    { url: `${baseUrl}/whitepaper`, lastModified: now, changeFrequency: "monthly", priority: 0.35 },
    { url: `${baseUrl}/privacy`, lastModified: now, changeFrequency: "yearly", priority: 0.2 },
    { url: `${baseUrl}/terms`, lastModified: now, changeFrequency: "yearly", priority: 0.2 },
  ];
}
