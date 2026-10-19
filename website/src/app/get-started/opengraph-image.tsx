import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "Get Started with ATOMiK", subtitle: "Bring one workload, one constraint, and one evidence path.", accent: "#22c55e", badge: "Quick Start" });
}
