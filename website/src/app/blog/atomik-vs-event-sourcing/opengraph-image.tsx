import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK Blog";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "ATOMiK vs Event Sourcing", subtitle: "Conceptual comparison of accumulator models and append-only logs.", accent: "#22c55e", badge: "Comparison" });
}
