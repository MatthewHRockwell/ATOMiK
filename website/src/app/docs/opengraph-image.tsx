import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "ATOMiK Documentation", subtitle: "Evidence labels, proof maps, API shape, examples, and hardware validation.", accent: "#4f8fff", badge: "Docs" });
}
