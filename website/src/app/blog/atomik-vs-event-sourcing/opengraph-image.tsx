import { generateOGImage, ogSize } from "@/lib/og";

export const runtime = "edge";
export const alt = "ATOMiK archived route redirect";
export const size = ogSize;
export const contentType = "image/png";

export default function OG() {
  return generateOGImage({
    title: "ATOMiK Technical Proof",
    subtitle: "Archived route redirected to current evidence-bounded proof material.",
    accent: "#22d3ee",
    badge: "Archived",
  });
}
