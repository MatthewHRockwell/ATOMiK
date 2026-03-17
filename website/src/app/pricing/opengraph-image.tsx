import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "ATOMiK Pricing", subtitle: "Free to start. Pro at $99/mo. Team at $299/mo. Enterprise at $999/mo.", accent: "#4f8fff", badge: "Pricing" });
}
