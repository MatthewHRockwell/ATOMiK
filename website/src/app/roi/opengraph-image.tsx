import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "Scenario Calculator", subtitle: "Model where redundant state movement may create avoidable cost.", accent: "#22c55e", badge: "Calculator" });
}
