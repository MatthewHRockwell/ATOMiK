import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "ATOMiK Changelog", subtitle: "Every release, every improvement. From Python SDK to FPGA silicon.", accent: "#8b5cf6", badge: "Changelog" });
}
