import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "ATOMiK Changelog", subtitle: "Evidence-labeled updates from software proof to FPGA hardware artifacts.", accent: "#8b5cf6", badge: "Changelog" });
}
