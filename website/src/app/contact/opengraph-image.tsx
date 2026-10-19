import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "Contact ATOMiK", subtitle: "Technical evaluation, investor diligence, design partnership, and licensing review.", accent: "#4f8fff", badge: "Contact" });
}
