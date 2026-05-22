import type { MetadataRoute } from "next";

export default function robots(): MetadataRoute.Robots {
  return {
    rules: {
      userAgent: "*",
      allow: "/",
      disallow: [
        "/api/",
        "/success",
        "/dashboard",
        "/register",
        "/integrations",
        "/community",
        "/roi",
        "/blog",
        "/demo",
        "/ai-demo",
        "/solutions/gaming",
        "/solutions/database-sync",
        "/solutions/distributed-systems",
        "/solutions/iot-edge",
        "/solutions/financial",
        "/get-started.html",
        "/install.html",
        "/landing.html",
        "/ATOMiK_White_Paper.pdf",
        "/ATOMiK_Benchmarks.pdf",
      ],
    },
    sitemap: "https://atomik.tech/sitemap.xml",
  };
}
