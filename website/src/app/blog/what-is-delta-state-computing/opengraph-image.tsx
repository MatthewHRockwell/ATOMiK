import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK Blog";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "What is Delta-State Computing?", subtitle: "The definitive guide. Mathematical foundations, comparisons, and code.", accent: "#4f8fff", badge: "Pillar" });
}
