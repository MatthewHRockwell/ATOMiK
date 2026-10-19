import { generateOGImage, ogSize } from "@/lib/og";

export const runtime = "edge";
export const alt = "ATOMiK archived demo route redirect";
export const size = ogSize;
export const contentType = "image/png";

export default function OG() {
  return generateOGImage({
    title: "ATOMiK Hardware Proof",
    subtitle: "Archived demo route redirected to the current hardware evidence map.",
    accent: "#22d3ee",
    badge: "Archived",
  });
}
