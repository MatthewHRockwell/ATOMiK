import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({
    title: "AI Workload Concept",
    subtitle: "Conceptual evaluation surface. Measured claims require artifacts.",
    accent: "#22d3ee",
    badge: "Concept",
  });
}
