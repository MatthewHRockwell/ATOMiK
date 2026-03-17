import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "ATOMiK Blog", subtitle: "Engineering insights, benchmarks, and the road to delta-state computing everywhere.", accent: "#8b5cf6", badge: "Blog" });
}
