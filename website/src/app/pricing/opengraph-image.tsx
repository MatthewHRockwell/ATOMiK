import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "ATOMiK Pricing", subtitle: "Request-based evaluations, design-partner engagements, and commercial licensing scoped around evidence.", accent: "#4f8fff", badge: "Pricing" });
}
