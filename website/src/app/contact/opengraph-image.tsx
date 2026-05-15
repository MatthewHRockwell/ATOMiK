import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "Contact ATOMiK", subtitle: "Request evaluation access, a technical demo, or a design-partner discussion.", accent: "#4f8fff", badge: "Contact" });
}
