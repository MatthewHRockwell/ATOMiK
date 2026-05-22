import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  async redirects() {
    return [
      { source: "/dashboard", destination: "/contact?intent=evaluation", permanent: false },
      { source: "/dashboard/:path*", destination: "/contact?intent=evaluation", permanent: false },
      { source: "/register", destination: "/contact?intent=evaluation", permanent: false },
      { source: "/register/:path*", destination: "/contact?intent=evaluation", permanent: false },
      { source: "/integrations", destination: "/docs/hardware", permanent: false },
      { source: "/integrations/:path*", destination: "/docs/hardware", permanent: false },
      { source: "/community", destination: "/contact?intent=evaluation", permanent: false },
      { source: "/community/:path*", destination: "/contact?intent=evaluation", permanent: false },
      { source: "/success", destination: "/contact?intent=evaluation", permanent: false },
      { source: "/success/:path*", destination: "/contact?intent=evaluation", permanent: false },
      { source: "/roi", destination: "/investor-brief", permanent: false },
      { source: "/roi/:path*", destination: "/investor-brief", permanent: false },
      { source: "/compare", destination: "/whitepaper", permanent: false },
      { source: "/compare/:path*", destination: "/whitepaper", permanent: false },
      { source: "/blog", destination: "/whitepaper", permanent: false },
      { source: "/blog/:path*", destination: "/whitepaper", permanent: false },
      { source: "/demo", destination: "/docs/hardware", permanent: false },
      { source: "/demo/:path*", destination: "/docs/hardware", permanent: false },
      { source: "/ai-demo", destination: "/docs/hardware", permanent: false },
      { source: "/ai-demo/:path*", destination: "/docs/hardware", permanent: false },
      { source: "/solutions/gaming", destination: "/solutions", permanent: false },
      { source: "/solutions/database-sync", destination: "/solutions", permanent: false },
      { source: "/solutions/distributed-systems", destination: "/solutions", permanent: false },
      { source: "/solutions/iot-edge", destination: "/solutions", permanent: false },
      { source: "/solutions/financial", destination: "/solutions", permanent: false },
      { source: "/get-started.html", destination: "/contact?intent=evaluation", permanent: false },
      { source: "/install.html", destination: "/contact?intent=evaluation", permanent: false },
      { source: "/landing.html", destination: "/", permanent: false },
      { source: "/ATOMiK_White_Paper.pdf", destination: "/docs/hardware", permanent: false },
      { source: "/ATOMiK_Benchmarks.pdf", destination: "/benchmarks", permanent: false },
    ];
  },
};

export default nextConfig;
