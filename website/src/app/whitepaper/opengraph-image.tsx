import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({
    title: "ATOMiK Technical White Paper",
    subtitle: "Foundations, hardware notes, and evidence-labeled proof artifacts.",
    accent: "#22d3ee",
    badge: "White Paper",
  });
}
