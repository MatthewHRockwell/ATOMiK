import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "Contact ATOMiK", subtitle: "Enterprise inquiries, custom integration, partnership opportunities.", accent: "#4f8fff", badge: "Contact" });
}
