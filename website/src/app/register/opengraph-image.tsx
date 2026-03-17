import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "ATOMiK Developer Program", subtitle: "Join the delta-state computing community. Free to register.", accent: "#4f8fff", badge: "Developer Program" });
}
