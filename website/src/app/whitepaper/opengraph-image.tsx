import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "ATOMiK Technical White Paper", subtitle: "Mathematical foundations, hardware implementation, and production results.", accent: "#f59e0b", badge: "White Paper" });
}
