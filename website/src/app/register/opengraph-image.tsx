import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "ATOMiK Developer Program", subtitle: "Request evaluation access and technical updates.", accent: "#4f8fff", badge: "Developer Program" });
}
