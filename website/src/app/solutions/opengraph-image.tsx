import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "ATOMiK Solutions", subtitle: "Use cases for heat, battery, bandwidth, latency, and hardware footprint constraints.", accent: "#22c55e", badge: "Solutions" });
}
