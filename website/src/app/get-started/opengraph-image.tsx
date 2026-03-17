import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "Get Started with ATOMiK", subtitle: "pip install atomik-core — zero dependencies, 218+ tests, Python 3.9+", accent: "#22c55e", badge: "Quick Start" });
}
