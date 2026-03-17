import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "ROI Calculator", subtitle: "How much is your infrastructure wasting? Calculate your savings.", accent: "#22c55e", badge: "Calculator" });
}
