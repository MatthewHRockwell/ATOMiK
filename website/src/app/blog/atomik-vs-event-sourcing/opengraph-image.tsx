import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK Blog";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "ATOMiK vs Event Sourcing", subtitle: "O(1) reconstruction vs O(n) replay. 8 bytes vs unbounded logs.", accent: "#22c55e", badge: "Comparison" });
}
