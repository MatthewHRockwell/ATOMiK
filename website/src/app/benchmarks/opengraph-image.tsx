import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "ATOMiK Benchmarks", subtitle: "Verified results from real hardware. Run it yourself.", accent: "#22d3ee", badge: "Benchmarks" });
}
