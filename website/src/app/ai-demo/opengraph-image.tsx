import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "AI Inference: GPU vs ATOMiK", subtitle: "Same output. 18x less power. Explore the architectural advantage.", accent: "#f97316", badge: "Interactive Demo" });
}
