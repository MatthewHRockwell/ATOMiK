import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK Blog";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "From Math to Silicon: 69.7 Gops/s on a $13.50 Chip", subtitle: "3 SoC generations, custom RV64I CPU, HD HDMI — on a $13.50 FPGA.", accent: "#8b5cf6", badge: "Hardware" });
}
