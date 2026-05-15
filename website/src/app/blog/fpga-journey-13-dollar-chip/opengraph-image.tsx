import { generateOGImage, ogSize } from "@/lib/og";
export const runtime = "edge";
export const alt = "ATOMiK Blog";
export const size = ogSize;
export const contentType = "image/png";
export default function OG() {
  return generateOGImage({ title: "From Math to Silicon: FPGA Proof Notes", subtitle: "Archived engineering notes on custom RISC-V, HDMI, and synthesis-characterized FPGA scaling.", accent: "#8b5cf6", badge: "Hardware" });
}
